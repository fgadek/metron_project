#include <Arduino.h>
#include <W5500lwIP.h>
#include <WebSocketsServer.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <FS.h>
#include <string.h>
#include "command.h"

#define UDP_PORT 8888
#define WEBSOCKET_PORT 1337
#define HTTP_PORT 80

extern "C" int display(int count, char command[COMMAND_MAX_WORD_COUNT][COMMAND_MAX_WORD_SIZE]);

int udp_server();
int extract_command_data();

void onWebSocketEvent(uint8_t client_num, WStype_t type, uint8_t * payload, size_t length);
void send_current_network_config(uint8_t client_num);
int check_config_format(uint8_t *config);
int write_to_config_file(uint8_t *config);

void handleRoot();
void handleNotFound();

int parse_network_config_file();
void print_network_config();
void reboot_device();

Wiznet5500lwIP eth(17);

typedef struct Network_config
{
	IPAddress ip;
	IPAddress subnet;
	IPAddress gateway;
	IPAddress dns1;
	IPAddress dns2;
	int dhcp = 0;
} Network_config;

Network_config network_config; // parse_network_config_file() writes here parsed config
char network_config_file_content[64]; // contents of config.txt file, parse_network_config_file() writes here while reading the file.

// #==== UDP ====#
char udp_packet_buffer[UDP_TX_PACKET_MAX_SIZE];
WiFiUDP udp;
// #=============#

// #= WebSocket =#
WebSocketsServer webSocket = WebSocketsServer(WEBSOCKET_PORT);
// #=============#

// #=== HTTP ====#
WebServer server(HTTP_PORT);
// #=============#

char command_buffer[COMMAND_MAX_WORD_COUNT][COMMAND_MAX_WORD_SIZE];

int state = 0;

void setup()
{
	Serial.begin(9600);
	//while(!Serial);
	
	if (!LittleFS.begin())
	{
    	Serial.println("*an error has occurred while mounting LittleFS filesystem*");
		while(true);
  	}
	
	if (parse_network_config_file())
	{
		while(true);
	}
	
	if (!network_config.dhcp)
	{
		eth.config(network_config.ip, network_config.gateway, network_config.subnet, network_config.dns1, network_config.dns2);
	}
	else
	{
		Serial.println("*DHCP enabled*");
	}

	eth.begin();
	
	int p = 0;
	while (!eth.isLinked())
	{
		if (!eth.isLinked() && !p)
		{
			Serial.println("*disconnected*");
			p = 1;
		}
		delay(100);
	}

	if (network_config.dhcp)
	{
		Serial.println("Waiting for DHCP server response...");
	}
	
	while (!eth.connected())
	{
		delay(100);
	}
	
	Serial.println("*connected to network*");
	state = 1;

	print_network_config();
	
	udp.begin(UDP_PORT);
	
	server.on("/", HTTP_GET, handleRoot);
	server.onNotFound(handleNotFound);
	server.begin();

	webSocket.begin();
  	webSocket.onEvent(onWebSocketEvent);
}

void loop()
{
	static int command_word_count;
	static int lost_connection_state = 0;

	switch (state)
	{
	case 0:
	{
		if (!eth.isLinked())
		{
			if (!lost_connection_state)
			{
				Serial.println("*connection lost*");
				lost_connection_state = 1;
			}
		}
		else
		{
			Serial.println("*connected to nerwork*");
			lost_connection_state = 0;
			state = 1;
		}

		break;
	}
	case 1:
	{
		if (udp_server())
		{
			command_word_count = extract_command_data();
			
			if (display(command_word_count, command_buffer))
			{
				Serial.println("*display error*");
			}
		}

		server.handleClient();
		webSocket.loop();

		if (!eth.isLinked())
		{
			state = 0;
		}
	
		break;
	}
	default:
		break;
	}
		
	delay(100);
}

int parse_network_config_file()
{
	File file;
	size_t bytesRead;
	char buffor[64];

	file = LittleFS.open("/network_config.txt", "r");

  	if (!file)
	{
		Serial.println("*failed to open /network_config.txt for reading*");
    	return -1;
	}
	
	bytesRead = file.readBytesUntil(EOF, buffor, sizeof(buffor) - 1);
	buffor[bytesRead] = '\0';
	
	strcpy(network_config_file_content, buffor);
	
	Serial.printf("\nNetwork configuration file:\n%s\n", buffor);
	
	IPAddress *network_config_ptr = (IPAddress*)&network_config;
	const char *delimeter = "|";
	char *buff;
	char *working_ptr = buffor;

	buff = strsep(&working_ptr, delimeter);

	if (buff != NULL && (!strcmp(buff, "0") || !strcmp(buff, "1")))
	{
		network_config.dhcp = atoi(buff);
	}

	int i = 0;

	do
	{
		buff = strsep(&working_ptr, delimeter);

		// IPAddress class method fromString() return 1 when string meets ipv4 address format
		// and manages to assign value to given IPAddress object.
		/*
		if ((network_config_ptr + i)->fromString(buff)) 
		{
			Serial.printf("%d | %s\n", i, buff);
		}
		*/

		(network_config_ptr + i)->fromString(buff);
		
		i++;

	} while (working_ptr != NULL);
	
	// Serial.println();
	
	file.close();

	return 0;
}

int udp_server()
{
	// if there's data available, read a packet
	int packet_size = udp.parsePacket();

	if (packet_size) {
		// read the packet into udp_packet_buffer
		udp.read(udp_packet_buffer, UDP_TX_PACKET_MAX_SIZE);
		if (packet_size >= UDP_TX_PACKET_MAX_SIZE)
		{
			udp_packet_buffer[UDP_TX_PACKET_MAX_SIZE - 1] = '\0';
		}
		else if (udp_packet_buffer[packet_size - 1] == '\n')
		{
			udp_packet_buffer[packet_size - 1] = '\0';
		}
		else
		{
			udp_packet_buffer[packet_size] = '\0';
		}

		IPAddress remote = udp.remoteIP();
		Serial.printf("UDP received packet from %s %s \n", remote.toString().c_str(), udp_packet_buffer);
	}
	
	return packet_size;
}

int extract_command_data()
{
	int count = 0;
	int i = 0, j = 0;
	
	while (count < COMMAND_MAX_WORD_COUNT)
	{
		if (udp_packet_buffer[i] == '\0')
		{
			if (j > 0)
			{
				command_buffer[count][j] = '\0';
				count++;
			}
			
			break;
		}

		if (udp_packet_buffer[i] == ';')
		{
			command_buffer[count][j] = '\0';
			j = 0;
			count++;
		}
		else if (j < COMMAND_MAX_WORD_SIZE - 1)
		{
			command_buffer[count][j] = udp_packet_buffer[i];
			j++;
		}
		
		i++;
	}

	return count;
}

void handleRoot()
{
	Serial.printf("HTTP request for / from %s\n", server.client().remoteIP().toString().c_str());
	File file = LittleFS.open("/index.html", "r");
	if (file)
	{
		server.streamFile(file, "text/html");
		file.close();
	}
	else
	{
		server.send(500, "text/plain", "500: Internal Server Error (Missing index.html)\n\n");
		Serial.printf("*index.html not found in the filesystem*");
	}
}

void handleNotFound()
{
	Serial.printf("HTTP request for %s from %s\n", server.uri(), server.client().remoteIP().toString().c_str());
	server.send(404, "text/plain", "404 Not Found\n\n");
}

void onWebSocketEvent(uint8_t client_num, WStype_t type, uint8_t * payload, size_t length)
{
	switch(type)
	{
		// Client has disconnected
		case WStype_DISCONNECTED:
		{
			Serial.printf("WebSocket [%u] Disconnected!\n", client_num);
		}
		break;

		// New client has connected
		case WStype_CONNECTED:
		{
			IPAddress ip = webSocket.remoteIP(client_num);
			Serial.printf("WebSocket [%u] Connection from ", client_num);
			Serial.println(ip.toString());
			
			send_current_network_config(client_num);
		}
		break;

		// Handle text messages from client
		case WStype_TEXT:
		{
			Serial.printf("WebSocket [%u] Received text: %s\n", client_num, payload);
			
			if (!check_config_format(payload))
			{
				Serial.println("*config from server: format valid*");

				if (!write_to_config_file(payload))
				{
					Serial.println("New network configuration written to config file!");
					reboot_device();
				}
			}
			else
			{
				Serial.println("*config from server: format invalid*");
			}
		}

		// For everything else: do nothing
		case WStype_BIN:
		case WStype_ERROR:
		case WStype_FRAGMENT_TEXT_START:
		case WStype_FRAGMENT_BIN_START:
		case WStype_FRAGMENT:
		case WStype_FRAGMENT_FIN:
		default:
		break;
  }
}

void send_current_network_config(uint8_t client_num)
{
	char buffor[64];

	if (network_config.dhcp == 0)
	{
		buffor[0] = '0';
		buffor[1] = '|';
		buffor[3] = '\0';
	}
	else if (network_config.dhcp == 1)
	{
		buffor[0] = '1';
		buffor[1] = '|';
		buffor[3] = '\0';
	}

	if (strcmp(eth.localIP().toString().c_str(), "(IP unset)"))
	{
		strcat(buffor, eth.localIP().toString().c_str());
	}
	
	strcat(buffor, "|");

	if (strcmp(eth.subnetMask().toString().c_str(), "(IP unset)"))
	{
		strcat(buffor, eth.subnetMask().toString().c_str());
	}
	
	strcat(buffor, "|");

	if (strcmp(eth.gatewayIP().toString().c_str(), "(IP unset)"))
	{
		strcat(buffor, eth.gatewayIP().toString().c_str());
	}
	
	strcat(buffor, "|");

	if (strcmp(eth.dnsIP(0).toString().c_str(), "(IP unset)"))
	{
		strcat(buffor, eth.dnsIP(0).toString().c_str());
	}
	
	strcat(buffor, "|");

	if (strcmp(eth.dnsIP(1).toString().c_str(), "(IP unset)"))
	{
		strcat(buffor, eth.dnsIP(1).toString().c_str());
	}
	
	strcat(buffor, "|");

	webSocket.sendTXT(client_num, buffor, strlen(buffor));
	Serial.printf("WebSocket [%u] text sent: %s\n", client_num, buffor);
}

int check_config_format(uint8_t *config)
{
	char buffor[64];
	
	strcpy(buffor, (char *) config);

	IPAddress ip_buffor;
	const char *delimeter = "|";
	char *buff;
	char *working_ptr = buffor;

	buff = strsep(&working_ptr, delimeter);
	
	if (buff == NULL || !(!strcmp(buff, "0") || !strcmp(buff, "1")))
	{
		return -1;
	}

	int i = 0;

	do
	{
		buff = strsep(&working_ptr, delimeter);

		// IPAddress class method fromString() return 1 when string meets ipv4 address format
		// and manages to assign value to given IPAddress object.
		if (strcmp(buff, "") && !(ip_buffor.fromString(buff)))
		{
			return -1;
		}
		
		i++;

	} while (working_ptr != NULL && i < 5);
	
	if (i != 4)
	{
		return -1;
	}
	
	return 0;
}

int write_to_config_file(uint8_t *config)
{
	char *buffor = (char *) config;

	File file = LittleFS.open("/network_config.txt", "w"); 

  	if (!file)
	{
		Serial.println("*failed to open /network_config.txt for writing*");
    	return -1;
  	}
	
	file.print(buffor);
	
	file.close();
	
	return 0;
}

void print_network_config()
{
	Serial.println("#=============================#");

	Serial.print("MAC: ");
	uint8_t *mac;
	eth.macAddress(mac);
	for (int i = 0; i < 6; i++)
	{
		if (mac[i] < 0xA)
		{
			Serial.print("0");
		}
		
		Serial.print(mac[i], HEX);
		if(i != 5)
		{
			Serial.print(":");
		}
	}
	Serial.println();

	Serial.print("DHCP: ");
	Serial.println((network_config.dhcp) ? "enabled" : "disabled");

	Serial.print("IPv4: ");
	Serial.println(eth.localIP());
	
	Serial.print("Subnet: ");
	Serial.println(eth.subnetMask());

	Serial.print("Gateway: ");
	Serial.println(eth.gatewayIP());

	Serial.print("DNS_1: ");
	Serial.println(eth.dnsIP(0));

	Serial.print("DNS_2: ");
	Serial.println(eth.dnsIP(1));

	Serial.println("#=============================#");
}

void reboot_device()
{
	Serial.println("Rebooting device...");

	/*
	LittleFS.end();
	udp.stop();
	server.close();
	webSocket.close();
	eth.end();
	Serial.end();
	*/

	delay(1000);
	rp2040.reboot();
}
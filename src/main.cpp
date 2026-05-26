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

void handleRoot();
void handleNotFound();

int parse_network_config_file();
void print_network_config();

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
char file_content[64]; // contents of config.txt file, parse_network_config_file() writes here while reading the file.

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

void setup()
{
	Serial.begin(9600);
	while(!Serial);
	
	if (!LittleFS.begin())
	{
    	Serial.println("*an error has occurred while mounting LittleFS*");
		while(true);
  	}
	
	if (parse_network_config_file())
	{
    	Serial.println("*an error has occured while reading network configuration file*");
		while(true);
	}

	if (!network_config.dhcp)
	{
		eth.config(network_config.ip, network_config.gateway, network_config.subnet, network_config.dns1, network_config.dns2);
		print_network_config();
	}

	eth.begin();

	while (!eth.connected())
	{
		Serial.println("*not connected*");
		delay(1000);
		// led_blink() or smth
	}
	
	Serial.println("*connected to nerwork*\n");
	
	if (network_config.dhcp)
	{
		print_network_config();
	}
	
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
	
	if (udp_server())
	{
		command_word_count = extract_command_data();
		
		Serial.print("Parsed command words: ");
		Serial.println(command_word_count);
		for (int i = 0; i < command_word_count; i++)
		{
			Serial.print("- ");
			Serial.println(command_buffer[i]);
		}
		Serial.println();
		
		if (display(command_word_count, command_buffer))
		{
			Serial.println("*display error*\n");
		}
	}

	server.handleClient();

	webSocket.loop();
}

int parse_network_config_file()
{
	File file;
	size_t bytesRead;
	char buffor[64];

	file = LittleFS.open("/network_config.txt", "r");

  	if (!file)
	{
    	return -1;
	}
	
	bytesRead = file.readBytesUntil(EOF, buffor, sizeof(buffor) - 1);
	buffor[bytesRead] = '\0';
	
	Serial.printf("\nNetwork configuration file:\n%s\n\n", buffor);
	
	strcpy(file_content, buffor);
	
	IPAddress *network_config_ptr = (IPAddress*)&network_config;
    char delimeter[] = "|";
    char *buff;

    buff = strtok(buffor, delimeter);

	if (buff != NULL)
	{
		network_config.dhcp = atoi(buff);
	}
	
    for (int i = 0; i < 5 && buff != NULL; i++)
    {
        buff = strtok(NULL, delimeter);
		(network_config_ptr + i)->fromString(buff);
    }
	
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

		Serial.print("received packet of size ");
		Serial.print(packet_size);
		Serial.print(" from ");
		IPAddress remote = udp.remoteIP();
		for (int i=0; i < 4; i++) {
			Serial.print(remote[i], DEC);
			if (i < 3) {
				Serial.print(".");
			}
		}
		Serial.println();
		Serial.print(udp_packet_buffer);
		Serial.println("\n");
	}
	delay(10);
	
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
		server.send(500, "text/plain", "500: Internal Server Error (Missing index.html)");
		Serial.printf("*index.html not found in the filesystem*");
	}
}

void handleNotFound()
{
	char message[] = "404 Not Found\n\n";
	server.send(404, "text/plain", message);
	Serial.printf("HTTP request for %s (404) from %s\n", server.uri(), server.client().remoteIP().toString().c_str());
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
			
			webSocket.sendTXT(client_num, file_content, strlen(file_content));
			Serial.printf("WebSocket [%u] text sent: %s\n", client_num, file_content);
		}
		break;

		// Handle text messages from client
		case WStype_TEXT:
		{
			Serial.printf("WebSocket [%u] Received text: %s\n", client_num, payload);
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
	
	Serial.print("subnet: ");
	Serial.println(eth.subnetMask());

	Serial.print("gateway: ");
	Serial.println(eth.gatewayIP());

	Serial.print("DNS: ");
	Serial.println(eth.dnsIP());

	Serial.println("#=============================#\n");
}
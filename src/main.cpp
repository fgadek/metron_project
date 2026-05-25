#include <Arduino.h>
#include <W5500lwIP.h>
#include <WebSocketsServer.h>
#include <string.h>
#include "config.h"

extern "C" int display(int count, char command[COMMAND_MAX_WORD_COUNT][COMMAND_MAX_WORD_SIZE]);

int udp_server();
int extract_command_data();

void onWebSocketEvent(uint8_t client_num, WStype_t type, uint8_t * payload, size_t length);

void print_network_config();

Wiznet5500lwIP eth(17);
IPAddress ip_default(192, 168, 67, 2);
IPAddress subnet_default(255, 255, 255, 0);
IPAddress gateway_default(192, 168, 67, 1);

// #==== UDP ====#
unsigned int udp_port = 8888;
char udp_packet_buffer[UDP_TX_PACKET_MAX_SIZE];
WiFiUDP udp;
// #============#

// #= WebSocket =#
WebSocketsServer webSocket = WebSocketsServer(1337);
char msg_buf[10];
// #============#

char command_buffer[COMMAND_MAX_WORD_COUNT][COMMAND_MAX_WORD_SIZE];

void setup()
{
	Serial.begin(9600);
	while(!Serial);

	eth.config(ip_default, gateway_default, subnet_default, NULL, NULL);
	eth.begin();

	while (!eth.connected())
	{
		Serial.print("*not connected*\n");
		// led_blink() or smth
	}
	
	print_network_config();
	
	udp.begin(udp_port);
	
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
			Serial.print("*display error*\n");
		}
	}

	webSocket.loop();
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

	Serial.print("IPv4: ");
	Serial.println(eth.localIP());
	
	Serial.print("subnet: ");
	Serial.println(eth.subnetMask());

	Serial.print("gateway: ");
	Serial.println(eth.gatewayIP());

	Serial.println("#=============================#\n");
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
		Serial.println();
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
			
			webSocket.sendTXT(client_num, "");
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
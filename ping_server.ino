/*
 * Project: Si4463 Radio Library for AVR and Arduino (Ping client example)
 * Author: Zak Kemble, contact@zakkemble.co.uk
 * Copyright: (C) 2017 by Zak Kemble
 * License: GNU GPL v3 (see License.txt)
 * Web: http://blog.zakkemble.co.uk/si4463-radio-library-avr-arduino/
 */

/*
 * Ping client
 *
 * Time how long it takes to send some data and get a reply
 * Should be around 5-6ms with default settings
 */

#include "Si446x.h"
#include <SPI.h>
#include <SoftwareSerial.h>

typedef enum {
	IDLE = 0,
	FIRST_CHAR,
	SECOND_CHAR,
	THIRD_CHAR,
	FOURTH_CHAR,
	FIFTH_CHAR,
	NEW_LINE_CHAR,
	SEND_DATA
} fsm_values;

int state_machine = IDLE;
int state_machine_tx = IDLE;

char dato[] = "Hola2\r\n";
char dato_ack[] = "Hola\r\n";
char cString[32];
byte chPos = 0;
byte ch = 0;
char dataStr[6];

unsigned long count = 0;
unsigned long countTrue = 0;

double perc = 0.0;

#define CHANNEL 20
#define MAX_PACKET_SIZE 10
#define TIMEOUT 1000

#define PACKET_NONE		0
#define PACKET_OK		1
#define PACKET_INVALID	2

typedef struct{
	uint8_t ready;
	uint32_t timestamp;
	int16_t rssi;
	uint8_t length;
	uint8_t buffer[MAX_PACKET_SIZE];
} pingInfo_t;

static volatile pingInfo_t pingInfo;

void SI446X_CB_RXCOMPLETE(uint8_t length, int16_t rssi){
	if(length > MAX_PACKET_SIZE)
		length = MAX_PACKET_SIZE;

	pingInfo.ready = PACKET_OK;
	pingInfo.timestamp = millis();
	pingInfo.rssi = rssi;
	pingInfo.length = length;

	Si446x_read((uint8_t*)pingInfo.buffer, length);

	// Radio will now be in idle mode
}

void SI446X_CB_RXINVALID(int16_t rssi){
	pingInfo.ready = PACKET_INVALID;
	pingInfo.rssi = rssi;
}

void set_tx_mode(){
	byte data_set_gpio[] = {0x31, CHANNEL, 0x70, 0x00, 0x00, 0x00, 0x00};
	byte data_set_gpio_cfg[] = {0x13, 0x04, 0x00, 0x21, 0x20, 0x00, 0x00, 0x00};
	while(1){
		byte check_set = 0;
		write_to_radio(data_set_gpio_cfg, 8);
		write_to_radio(data_set_gpio, 7);

		digitalWrite(SI446X_CSN, LOW);

		byte temp_result = 0;
		temp_result = SPI.transfer(0x33);

		do{
			digitalWrite(SI446X_CSN, HIGH);
			digitalWrite(SI446X_CSN, LOW);
			SPI.transfer(0x44);
			temp_result = SPI.transfer(0xFF);
		}while(temp_result != 0xFF);
		temp_result = SPI.transfer(0xFF);
		if(temp_result != 7) check_set = 1;
		temp_result = SPI.transfer(0xFF);
		if(temp_result != CHANNEL) check_set = 1;
		digitalWrite(SI446X_CSN, HIGH);
		if(check_set == 0) break;
	}
}

void set_rx_mode(){
	byte data_set_gpio[] = {0x32, CHANNEL, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	byte data_set_gpio_cfg[] = {0x13, 0x00, 0x14, 0x21, 0x20, 0x00, 0x00, 0x00};

	while(1){
		byte check_set = 0;
		write_to_radio(data_set_gpio_cfg, 8);
		write_to_radio(data_set_gpio, 8);

		digitalWrite(SI446X_CSN, LOW);

		byte temp_result = 0;
		temp_result = SPI.transfer(0x33);

		do{
			digitalWrite(SI446X_CSN, HIGH);
			digitalWrite(SI446X_CSN, LOW);
			SPI.transfer(0x44);
			temp_result = SPI.transfer(0xFF);
		}while(temp_result != 0xFF);
		temp_result = SPI.transfer(0xFF);
		if(temp_result != 8) check_set = 1;
		temp_result = SPI.transfer(0xFF);
		if(temp_result != CHANNEL) check_set = 1;
		digitalWrite(SI446X_CSN, HIGH);
		if(check_set == 0) break;
	}
}

void write_to_radio(byte *data, byte len){
	digitalWrite(SI446X_CSN, LOW);
	
	byte temp_result = 0;

	for(int i = 0; i < len; i++){
		SPI.transfer(data[i]);
	}

	do{
		digitalWrite(SI446X_CSN, HIGH);
		digitalWrite(SI446X_CSN, LOW);
		SPI.transfer(0x44);
		temp_result = SPI.transfer(0xFF);
	}while(temp_result != 0xFF);

	digitalWrite(SI446X_CSN, HIGH);
}

void read_from_radio(byte cmd, byte len){
	digitalWrite(SI446X_CSN, LOW);

	byte temp_result = 0;
	temp_result = SPI.transfer(cmd);

	Serial.print("Reading ");
	Serial.print(cmd, HEX);
	Serial.print(": ");

	do{
		digitalWrite(SI446X_CSN, HIGH);
		digitalWrite(SI446X_CSN, LOW);
		SPI.transfer(0x44);
		temp_result = SPI.transfer(0xFF);
	}while(temp_result != 0xFF);

	for(int i = 0; i < len; i++){
		temp_result = SPI.transfer(0xFF);
		Serial.print(temp_result, DEC);
		Serial.print(", ");
	}

	Serial.println();

	digitalWrite(SI446X_CSN, HIGH);
}

void read_fast_radio(byte cmd, byte len){
	digitalWrite(SI446X_CSN, LOW);

	Serial.print("Reading ");
	Serial.print(cmd, HEX);
	Serial.print(": ");

	byte temp_result = 0;
	temp_result = SPI.transfer(cmd);
	temp_result = SPI.transfer(0xFF);
	Serial.print(temp_result, DEC);

	Serial.println();

	digitalWrite(SI446X_CSN, HIGH);
}

void read_property(byte cmd, uint16_t prop, byte len){
	digitalWrite(SI446X_CSN, LOW);

	byte temp_result = 0;
	temp_result = SPI.transfer(cmd);
	temp_result = SPI.transfer((prop >> 8) & 0xFF00);
	temp_result = SPI.transfer(len);
	temp_result = SPI.transfer(prop & 0x00FF);

	Serial.print("Reading ");
	Serial.print(cmd, HEX);
	Serial.print(": ");

	do{
		digitalWrite(SI446X_CSN, HIGH);
		digitalWrite(SI446X_CSN, LOW);
		SPI.transfer(0x44);
		temp_result = SPI.transfer(0xFF);
	}while(temp_result != 0xFF);

	for(int i = 0; i < len; i++){
		temp_result = SPI.transfer(0xFF);
		Serial.print(temp_result, DEC);
		Serial.print(", ");
	}

	Serial.println();

	digitalWrite(SI446X_CSN, HIGH);
}

void tx_data(){
	state_machine_tx = IDLE;
	memset(cString, '\0', 32);
	while(1){
		set_tx_mode();
		Serial.print("TX Data: ");
		Serial.println("Chao");
		Serial.flush();
		set_rx_mode();
		Serial.println("Waiting ACK...");
		Serial.flush();
		unsigned long stop = (unsigned long)1000 + millis();
		while((signed long)(stop - millis()) > 0){
			read_fast_radio(0x50, 1);
			read_fast_radio(0x51, 1);
			while(Serial.available()){
			//read incoming char by char:
				ch = Serial.read();
				switch(state_machine_tx){
					case IDLE:
						cString[0] = ch;
						if (ch == dato_ack[0]){
							state_machine = FIRST_CHAR;
						}
						break;
					case FIRST_CHAR:
						cString[1] = ch;
						if (ch == dato_ack[1]){
							state_machine_tx = SECOND_CHAR;
						}
						else if (ch != '\0'){
							state_machine_tx = IDLE;
							Serial.println(".1");
							Serial.flush();
							memset(cString, '\0', 32);
						}
						break;
					case SECOND_CHAR:
						cString[2] = ch;
						if (ch == dato_ack[2]){
							state_machine_tx = THIRD_CHAR;
						}
						else if (ch != '\0'){
							state_machine_tx = IDLE;
							Serial.println(".2");
							Serial.flush();
							memset(cString, '\0', 32);
						}
						break;
					case THIRD_CHAR:
						cString[3] = ch;
						if (ch == dato_ack[3]){
							state_machine_tx = FOURTH_CHAR;
						}
						else if (ch != '\0'){
							state_machine_tx = IDLE;
							Serial.println(".3");
							Serial.flush();
							memset(cString, '\0', 32);
						}
						break;
					case FOURTH_CHAR:
						cString[4] = ch;
						if (ch == dato_ack[4]){
							state_machine_tx = NEW_LINE_CHAR;
						}
						else if (ch != '\0'){
							state_machine_tx = IDLE;
							Serial.println(".4");
							Serial.flush();
							memset(cString, '\0', 32);
						}
						break;
					case NEW_LINE_CHAR:
						cString[5] = ch;
						if (ch == dato_ack[5]){
							state_machine_tx = SEND_DATA;
							Serial.println(" - ACK - ");
							Serial.flush();
						}
						else if (ch != '\0'){
							state_machine_tx = IDLE;
							Serial.println(".5");
							Serial.flush();
							memset(cString, '\0', 32);
						}
						break;
				}
			}
			if(state_machine_tx == SEND_DATA){
				break;
			}
		}
		if(state_machine_tx == SEND_DATA){
			break;
		}
		state_machine_tx = IDLE;
		memset(cString, '\0', 32);
	}
}

void setup(){
	Serial.begin(19200);

	pinMode(A5, OUTPUT);

	// Start up
	Si446x_init();
	Si446x_setTxPower(SI446X_MAX_TX_POWER);
}

void loop(){
	set_rx_mode();
	Serial.println("Waiting...");
	Serial.flush();
	while(1){
		while(Serial.available()){
			ch = Serial.read();
			switch(state_machine){
				case IDLE:
					cString[0] = ch;
					if (ch == dato[0]){
						state_machine = FIRST_CHAR;
					}
					break;
				case FIRST_CHAR:
					cString[1] = ch;
					if (ch == dato[1]){
						state_machine = SECOND_CHAR;
					}
					else if (ch != '\0'){
						state_machine = IDLE;
						memset(cString, '\0', 32);
					}
					break;
				case SECOND_CHAR:
					cString[2] = ch;
					if (ch == dato[2]){
						state_machine = THIRD_CHAR;
					}
					else if (ch != '\0'){
						state_machine = IDLE;
						memset(cString, '\0', 32);
					}
					break;
				case THIRD_CHAR:
					cString[3] = ch;
					if (ch == dato[3]){
						state_machine = FOURTH_CHAR;
					}
					else if (ch != '\0'){
						state_machine = IDLE;
						memset(cString, '\0', 32);
					}
					break;
				case FOURTH_CHAR:
					cString[4] = ch;
					if (ch != '\0'){
						state_machine = FIFTH_CHAR;
					}
					else {
						state_machine = IDLE;
						memset(cString, '\0', 32);
					}
					break;
				case FIFTH_CHAR:
					cString[5] = ch;
					if(ch != '\0'){
						state_machine = NEW_LINE_CHAR;
					}
					else {
						state_machine = IDLE;
						memset(cString, '\0', 32);
					}
					break;
				case NEW_LINE_CHAR:
					cString[6] = ch;
					state_machine = SEND_DATA;
					Serial.print("RX Data: ");
					Serial.print(cString);
					Serial.flush();
					memset(cString, '\0', 32);
					break;
			}
		}
		if(state_machine == SEND_DATA){
			state_machine = IDLE;
			set_tx_mode();
			Serial.println("Chao");
			Serial.flush();
			break;
		}
	}
	//tx_data();
}

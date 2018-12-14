#ifndef F_CPU
	#define F_CPU 8000000UL
#endif

#define ADDRESS 0x20
#define REMOTE 0x30

#define BITRATE 0
#define BITRATE_PRESCALER 0

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <iic/iic.h>
#include <iic/iic_extras.h>
#include "common.h"

void setup_usart();
void out_char(char c);
void out_string(char *str);

void show_menu();

typedef enum{
	NORMAL,
	COLOR_INPUT,
	SEND_PATTERN_CMD,
	SEND_COLOR_CMD,
	READ_REMOTE_ADDRESS,
	RELOAD_MENU
} ustate_t;

volatile char hex_lut[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

volatile ustate_t usart_state = RELOAD_MENU;
volatile uint8_t color_buf[] = {0,0,0,0};
volatile uint8_t color_index = 0;
volatile uint8_t pattern_buffer = 0;

volatile uint8_t remote_address = REMOTE;

int main(void){
	setup_iic(ADDRESS, false, false, BITRATE, BITRATE_PRESCALER, 20, 0);
	enable_iic();
	setup_usart();

	sei();

	// Setup things
	while(1){
		switch(usart_state){
			case SEND_PATTERN_CMD:
				iic_write_two(remote_address, 0x21, pattern_buffer);
				//out_string("f");
				usart_state = NORMAL;
				break;
			case SEND_COLOR_CMD:
				iic_write_many(remote_address, color_buf, 4);
				while(IIC_MODULE.state != IIC_IDLE);
				out_string("\033[2J\033[HUSART connection online.\n\r * Type 0, 1, or 2 to change the slave-device pattern.\n\r * Type r to set the slave-device red value.\n\r * Type g to set the slave-device green value.\n\r * Type b to set the slave-device blue value.\n\n\r");
				usart_state = NORMAL;
				break;
			case RELOAD_MENU:
				show_menu();
				usart_state = NORMAL;
				break;
			default:
				break;
		}
	}
	return 0;
}

void setup_usart(){
	DDRD |= (1 << DDD1); // set Tx to output

	UBRR0L = 12;
	UBRR0H = 0;

	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8-bit charsize
	UCSR0B = (1 << RXCIE0) | (1 << RXEN0) | (1 << TXEN0);
	// enable RxC interrupt, enable Tx/Rx
}

void out_char(char c){
	while(!(UCSR0A & (1 << UDRE0)));
	UDR0 = c;
}

void out_string(char *str){
	for(uint8_t dex = 0; str[dex] != 0; dex++){
		out_char(str[dex]);
	}
}

void show_menu(){
	out_string("\033[2J\033[HUSART connection online [remote ");
	out_char(hex_lut[(remote_address & 0xf0) >> 4]);
	out_char(hex_lut[(remote_address & 0x0f)]);
	out_string("].\n\r * Type 0, 1, or 2 to change the slave-device pattern.\n\r * Type r to set the slave-device red value.\n\r * Type g to set the slave-device green value.\n\r * Type b to set the slave-device blue value.\n\r * Type t to set the address of the slave.\n\n\r");
}

ISR(USART0_RX_vect){
	uint8_t rx_dat = UDR0;
	switch(usart_state){
		case NORMAL:
			switch(rx_dat){
				case '0':
				case '1':
				case '2':
					usart_state = SEND_PATTERN_CMD;
					pattern_buffer = rx_dat - '0';
					break;

				case 't':
					out_string("\033[2J\033[HSet remote (2 hex digits) >>");
					usart_state = READ_REMOTE_ADDRESS;
					color_index = 0;
					pattern_buffer = 0;
					break;

				case 'r':
					out_string("\033[2J\033[HSet color (3 hex digits) >>");
					usart_state = COLOR_INPUT;
					color_buf[0] = IIC_COMMAND_LED_WRITE_WORD;
					color_buf[1] = 0;
					color_buf[2] = 0;
					color_buf[3] = 0;
					color_index = 0;
					break;
				case 'g':
					out_string("\033[2J\033[HSet color (3 hex digits) >>");
					usart_state = COLOR_INPUT;
					color_buf[0] = IIC_COMMAND_LED_WRITE_WORD;
					color_buf[1] = 1;
					color_buf[2] = 0;
					color_buf[3] = 0;
					color_index = 0;
					break;
				case 'b':
					out_string("\033[2J\033[HSet color (3 hex digits) >>");
					usart_state = COLOR_INPUT;
					color_buf[0] = IIC_COMMAND_LED_WRITE_WORD;
					color_buf[1] = 2;
					color_buf[2] = 0;
					color_buf[3] = 0;
					color_index = 0;
					break;
			}
			break;

		case COLOR_INPUT:;
			uint8_t val = 17;

			if(rx_dat >= '0' && rx_dat <= '9'){
				val = rx_dat - '0';
			}else if(rx_dat >= 'a' && rx_dat <= 'f'){
				val = rx_dat - 'a' + 10;
			}else if(rx_dat >= 'A' && rx_dat <= 'F'){
				val = rx_dat - 'A' + 10;
			}

			if(val < 16){
				out_char(rx_dat);
				switch(color_index++){
					case 0:
						color_buf[3] = (val << 4);
						break;
					case 1:
						color_buf[3] |= (val);
						break;
					case 2:
						color_buf[2] = (val << 4);
						break;
					case 3:
						color_buf[2] |= (val);
						usart_state = SEND_COLOR_CMD;
				}
			}
			break;

		case READ_REMOTE_ADDRESS:;
			uint8_t valr = 17;

			if(rx_dat >= '0' && rx_dat <= '9'){
				valr = rx_dat - '0';
			}else if(rx_dat >= 'a' && rx_dat <= 'f'){
				valr = rx_dat - 'a' + 10;
			}else if(rx_dat >= 'A' && rx_dat <= 'F'){
				valr = rx_dat - 'A' + 10;
			}

			if(valr < 16){
				out_char(rx_dat);
				switch(color_index++){
					case 0:
						pattern_buffer = (valr << 4);
						break;
					case 1:
						pattern_buffer |= (valr);
						remote_address = pattern_buffer;
						usart_state = RELOAD_MENU;
						break;
				}
			}
			break;

		default:
			break;
	}
}

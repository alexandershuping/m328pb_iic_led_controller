#ifndef F_CPU
	#define F_CPU 8000000UL
#endif

#define ADDRESS 0x20
#define REMOTE 0x30

#define BITRATE 0
#define BITRATE_PRESCALER 0

//#define INTERACTIVE

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
	SEND_INCLUDE_CMD,
	SEND_EXCLUDE_CMD,
	SEND_ISYNC_CMD, // inclusive sync
	SEND_XSYNC_CMD, // exclusive sync
	READ_HEX_BYTE,
	RELOAD_MENU
} ustate_t;

typedef enum{
	REMOTE_ADDRESS,
	INCLUDE_TARGET,
	PHASE_BUFFER,
	EXCLUDE_TARGET,
	ISYNC_PHASE
} destination_t;

volatile char hex_lut[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

volatile ustate_t usart_state = RELOAD_MENU;
volatile destination_t destination = REMOTE_ADDRESS;
volatile uint8_t ix_target_address = 0; // include/exclude address target
volatile uint8_t phase_buf = 0;

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
				usart_state = NORMAL;
				break;
			case SEND_COLOR_CMD:
				iic_write_many(remote_address, color_buf, 4);
				while(IIC_MODULE.state != IIC_IDLE);
				usart_state = RELOAD_MENU;
				break;
			case SEND_INCLUDE_CMD:
				iic_write_two(ix_target_address, IIC_COMMAND_LED_INCLUDE_DEVICE, phase_buf);
				while(IIC_MODULE.state != IIC_IDLE);
				usart_state = RELOAD_MENU;
				break;
			case SEND_EXCLUDE_CMD:
				iic_write_one(ix_target_address, IIC_COMMAND_LED_EXCLUDE_DEVICE);
				while(IIC_MODULE.state != IIC_IDLE);
				usart_state = RELOAD_MENU;
				break;
			case SEND_ISYNC_CMD:
				iic_write_two(0x00, IIC_COMMAND_LED_INCLUSIVE_SYNCHRONIZE, phase_buf);
				while(IIC_MODULE.state != IIC_IDLE);
				usart_state = RELOAD_MENU;
				break;
			case SEND_XSYNC_CMD:
				iic_write_one(0x00, IIC_COMMAND_LED_EXCLUSIVE_SYNCHRONIZE);
				while(IIC_MODULE.state != IIC_IDLE);
				usart_state = RELOAD_MENU;
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
	#ifdef INTERACTIVE
	out_string("\033[2J\033[HUSART connection online [remote ");
	out_char(hex_lut[(remote_address & 0xf0) >> 4]);
	out_char(hex_lut[(remote_address & 0x0f)]);
	out_string("].\n\r * Type 0, 1, or 2 to change the slave-device pattern.\n\r * Type r to set the slave-device red value.\n\r * Type g to set the slave-device green value.\n\r * Type b to set the slave-device blue value.\n\r * Type t to set the address of the slave.\n * Type i to include devices in the next sync command.\n * Type e to exclude devices from the next sync command.\n * Type s to perform an inclusive synchronize.\n * type S to perform an exclusive synchronize.\n\n\r");
	#endif
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
					#ifdef INTERACTIVE
					out_string("\033[2J\033[HSet remote (2 hex digits) >>");
					#endif
					usart_state = READ_HEX_BYTE;
					destination = REMOTE_ADDRESS;
					color_index = 0;
					break;

				case 'i':
					#ifdef INTERACTIVE
					out_string("\033[2J\033[HDevice address to include (2 hex digits) >>");
					#endif
					usart_state = READ_HEX_BYTE;
					destination = INCLUDE_TARGET;
					color_index = 0;
					break;

				case 'e':
					#ifdef INTERACTIVE
					out_string("\033[2J\033[HDevice address to exclude (2 hex digits) >>");
					#endif
					usart_state = READ_HEX_BYTE;
					destination = EXCLUDE_TARGET;
					color_index = 0;
					break;

				case 's':
					#ifdef INTERACTIVE
					out_string("\033[2J\033[HPhase for inclusive-sync (2 hex digits) >>");
					#endif
					usart_state = READ_HEX_BYTE;
					destination = ISYNC_PHASE;
					color_index = 0;
					break;

				case 'S':
					usart_state = SEND_XSYNC_CMD;
					break;

				case 'r':
					#ifdef INTERACTIVE
					out_string("\033[2J\033[HSet color (3 hex digits) >>");
					#endif
					usart_state = COLOR_INPUT;
					color_buf[0] = IIC_COMMAND_LED_WRITE_WORD;
					color_buf[1] = 0;
					color_buf[2] = 0;
					color_buf[3] = 0;
					color_index = 0;
					break;
				case 'g':
					#ifdef INTERACTIVE
					out_string("\033[2J\033[HSet color (3 hex digits) >>");
					#endif
					usart_state = COLOR_INPUT;
					color_buf[0] = IIC_COMMAND_LED_WRITE_WORD;
					color_buf[1] = 1;
					color_buf[2] = 0;
					color_buf[3] = 0;
					color_index = 0;
					break;
				case 'b':
					#ifdef INTERACTIVE
					out_string("\033[2J\033[HSet color (3 hex digits) >>");
					#endif
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
			}else if(rx_dat == 'x'){
				usart_state = RELOAD_MENU;
				break;
			}

			if(val < 16){
				#ifdef INTERACTIVE
				out_char(rx_dat);
				#endif
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

		case READ_HEX_BYTE:;
			uint8_t valr = 17;

			if(rx_dat >= '0' && rx_dat <= '9'){
				valr = rx_dat - '0';
			}else if(rx_dat >= 'a' && rx_dat <= 'f'){
				valr = rx_dat - 'a' + 10;
			}else if(rx_dat >= 'A' && rx_dat <= 'F'){
				valr = rx_dat - 'A' + 10;
			}else{
				usart_state = RELOAD_MENU;
				break;
			}

			if(valr < 16){
				#ifdef INTERACTIVE
				out_char(rx_dat);
				#endif
				switch(color_index++){
					case 0:
						pattern_buffer = (valr << 4);
						break;
					case 1:
						pattern_buffer |= (valr);
						switch(destination){
							case REMOTE_ADDRESS:
								remote_address = pattern_buffer;
								usart_state = RELOAD_MENU;
								break;
							case INCLUDE_TARGET:
								ix_target_address = pattern_buffer;
								#ifdef INTERACTIVE
								out_string("\033[2J\033[HSync at what phase? (2 hex digits) >>");
								#endif
								usart_state = READ_HEX_BYTE;
								destination = PHASE_BUFFER;
								color_index = 0;
								break;
							case PHASE_BUFFER:
								phase_buf = pattern_buffer;
								usart_state = SEND_INCLUDE_CMD;
								break;
							case EXCLUDE_TARGET:
								ix_target_address = pattern_buffer;
								usart_state = SEND_EXCLUDE_CMD;
								break;
							case ISYNC_PHASE:
								phase_buf = pattern_buffer;
								usart_state = SEND_ISYNC_CMD;
								break;
						}

						break;
				}
			}
			break;

		default:
			break;
	}
}

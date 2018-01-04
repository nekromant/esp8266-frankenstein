#ifndef _SYSEX_ROUTINES
#define _SYSEX_ROUTINES

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

// #include "Profiles\profiles.h"

/*
#define GET_SYSEX_DATA (0xFF)
#define RECEIVING_SYSEX_DATA (1)
#define SYSEX_DATA_ERROR (3)

void SysexSend(void* data, uint16_t len);
uint8_t IsReceivingSysExData(uint8_t state);
void ParseSysExData(uint8_t nextByte);
void SysExFlush(void);
void SysEx_ReceiveError(void);
*/




void serialWrite(char c);
void startSysex(void);
void endSysex(void);
void sendValueAsTwo7bitBytes(int value);
void sendSysex(int command, int bytec, char* bytev);
void SysexProcessInput(char c);
void sysexCallback(char command, char argc, char* argv);

typedef void (*sysex_callback_fn_t)(char command, int length, int);


#define START_SYSEX					0xF0 	// start a MIDI Sysex message
#define END_SYSEX					0xF7 	// end a MIDI Sysex message
#define SYSTEM_RESET				0xFF 	// reset from MIDI
#define SET_PIN_MODE				0xF4
#define SET_PIN						0x32
#define SEND_INT_VAL				0x33
#define COMMAND_NOT_FOUND			0x34
#define COMMAND_OK					0x35
#define L_SET_PIN_MODE				0x36
#define L_SET_PIN 					0x37
#define DEBUG_PIN_VALUE				0x39
#define L_DEBUG_PIN_VALUE			0x3A
#define SEND_KEEPALIVE				0x3B
#define SET_ENCODER_VALUE			0x40
#define GET_ENCODER_VALUE			0x41
#define ENABLE_DEBUG_PIN_VALUE		0x42
#define DISABLE_DEBUG_PIN_VALUE 	0x43
#define L_ENABLE_DEBUG_PIN_VALUE	0x44
#define L_DISABLE_DEBUG_PIN_VALUE	0x45
#define SEND_ALL_INPUTS				0x46
#define UART_CONNECTED				0x47

	
#define MAX_DATA_BYTES   	32   	// max number of data bytes in non-Sysex messages

// encoders
#define ENCODER_01 0x01
#define ENCODER_02 0x02
#define ENCODER_03 0x03
#define ENCODER_04 0x04
#define ENCODER_05 0x05

#define INPUT_COUNT 16
#define ENCODER_COUNT 5

struct encoder
{
    int id;
    char* label;
    int value;
} encoder_t;

struct encoder Inputs[INPUT_COUNT]; 
struct encoder Encoders[ENCODER_COUNT]; 


#endif


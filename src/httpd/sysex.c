/* SysEx Data Routines */

#include <string.h>
#include <osapi.h>
#include "cstdint.h"
#include "uart.h"
#include "sysex.h"
#include "stdbool.h"
#include "io.h"

// static uint16_t DataCount = 0;
static uart_t* uart0;


// input message handling
bool waitForData; // this flag says the next serial input will be data
int executeMultiByteCommand; // execute this after getting multi-byte data
int multiByteChannel; // channel data for multiByteCommands
char storedInputData[MAX_DATA_BYTES]; // multi-byte data

// sysex 
bool parsingSysex;
int sysexBytesRead;

void serialWrite(char c)
{
	uart0_transmit_char(uart0, c);
}

void startSysex(void) 
{
	serialWrite(START_SYSEX);
}

void endSysex(void)
{
	serialWrite(END_SYSEX);
}


void sendValueAsTwo7bitBytes(int value)
{
	serialWrite(value & 0xB01111111); // LSB
	serialWrite(value >> 7 & 0xB01111111); // MSB
}

void sendSysex(int command, int bytec, char* bytev)
{
	int i;
	startSysex();
	serialWrite(command);
	for(i=0; i<bytec; i++) {
		sendValueAsTwo7bitBytes(bytev[i]);
	}
	endSysex();
}

void SysexProcessInput(char c)
{
    char inputData = c; // this is 'int' to handle -1 when no data
    int command;

    // TODO make sure it handles -1 properly

    if (parsingSysex)
    {
      if(inputData == END_SYSEX)
      {
        //stop sysex byte
        parsingSysex = false;
        //fire off handler function
		/*
        if(currentSysexCallback)
          (*currentSysexCallback)(storedInputData[0], sysexBytesRead - 1, storedInputData + 1);
		*/
		sysexCallback(storedInputData[0], sysexBytesRead - 1, storedInputData + 1);
      }
      else
      {
        //normal data byte - add to buffer
        storedInputData[sysexBytesRead] = inputData;
        sysexBytesRead++;
      }
    }
    else if( (waitForData > 0) && (inputData < 128) )
    {
      waitForData--;
      storedInputData[waitForData] = inputData;
    }
    else
    {
      // remove channel info from command byte if less than 0xF0
      if(inputData < 0xF0)
      {
        command = inputData & 0xF0;
        multiByteChannel = inputData & 0x0F;
      }
      else
      {
        command = inputData;
        // commands in the 0xF* range don't use channel data
      }

      switch (command)
      {
      case START_SYSEX:
        parsingSysex = true;
        sysexBytesRead = 0;
        break;
      case SYSTEM_RESET:
        // systemReset();
        break;
      }
    }
  }
  
void sysexCallback(char command, char argc, char* argv) {
	
    // int pos = -1;
    // int val = 0;
    char senddata[2];
	senddata[0] = command;
	
    switch(command) 
    {
		case SEND_KEEPALIVE:
			ioLed(100);
			break;
		case UART_CONNECTED:
			senddata[0] = COMMAND_OK;
			sendSysex(SEND_ALL_INPUTS, 1, senddata);
			break;
		case DEBUG_PIN_VALUE:
			
			for(int i = 0; i < INPUT_COUNT; i++) {
				if((argv[0]-1) == Inputs[i].id)
				{
					Inputs[i].value = argv[2];
				}
			}
			sendSysex(COMMAND_OK, 1, senddata);
			break;
		case GET_ENCODER_VALUE:
				for(int i = 0; i < ENCODER_COUNT; i++) {
					if((argv[0]-1) == Encoders[i].id)
					{
						Encoders[i].value = argv[2];
					}
				}
				sendSysex(COMMAND_OK, 1, senddata);
			break;

		default:
			sendSysex(COMMAND_NOT_FOUND, 1, senddata);
	}
}

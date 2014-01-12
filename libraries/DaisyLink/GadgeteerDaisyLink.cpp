/*
GadgeteerDaisyLink.h - Gadgeteer DaisyLink Module library for Arduino
Copyright (c) 2012 Valentin Ivanov.  All right reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

extern "C"{
	//#include <avr/io.h>
#include <avr/interrupt.h>
#include <compat/twi.h>
}

#include "Arduino.h"
#include "GadgeteerDaisyLink.h"

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif

#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif


#define TWI_FREQ 50000L
#define TWI_BUFFER_LENGTH 32

#define DAISYLINK_DEFAULT_ADDRESS		0x7F
#define DAISYLINK_VERSION				0x04

#define REGISTER_DAISY_ADDRESS					0x00
#define REGISTER_DAISY_CONFIG					0x01
#define REGISTER_DAISY_VERSION					0x02
#define REGISTER_DAISY_MODULE_TYPE				0x03
#define REGISTER_DAISY_MODULE_VERSION			0x04
#define REGISTER_DAISY_MODULE_MANUFACTURER		0x05
#define REGISTER_DAISY_RESERVED1				0x06
#define REGISTER_DAISY_RESERVED2				0x07


#define ACK TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWINT) | _BV(TWEA);
#define NACK TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWINT);

static void (*user_onStatus)(int);
static void (*user_onUpdateRegister)(int);

extern "C"{
static void onUpstreamInterrupt();
static void onDownstreamInterrupt();
}

static uint8_t volatile * pDeviceRegisters = 0;

static uint8_t twi_rxBuffer[TWI_BUFFER_LENGTH];
static volatile uint8_t twi_rxBufferIndex;

static uint8_t twi_txBuffer[TWI_BUFFER_LENGTH];
static volatile uint8_t twi_txBufferIndex;
static volatile uint8_t twi_txBufferLength;

static uint8_t daisy_Registers[8];

static volatile int daisy_state = 0;

GadgeteerDaisyLink::GadgeteerDaisyLink()
{
}

void GadgeteerDaisyLink::initialize( uint8_t manufacturerCode, uint8_t moduleType, uint8_t moduleVersion, uint8_t volatile * deviceRegisters, uint8_t registerTableSize)
{
	daisy_state = 0;

	daisy_Registers[REGISTER_DAISY_ADDRESS] = DAISYLINK_DEFAULT_ADDRESS;
	daisy_Registers[REGISTER_DAISY_CONFIG] = 0;
	daisy_Registers[REGISTER_DAISY_VERSION] = DAISYLINK_VERSION;
	daisy_Registers[REGISTER_DAISY_MODULE_TYPE] = moduleType;
	daisy_Registers[REGISTER_DAISY_MODULE_VERSION] = moduleVersion;
	daisy_Registers[REGISTER_DAISY_MODULE_MANUFACTURER] = manufacturerCode;
	daisy_Registers[REGISTER_DAISY_RESERVED1] = 0;
	daisy_Registers[REGISTER_DAISY_RESERVED2] = 0;

	_deviceRegistersSize = registerTableSize;
	pDeviceRegisters = deviceRegisters;

	// initialize twi prescaler and bit rate
	//cbi(TWSR, TWPS0);
	//cbi(TWSR, TWPS1);
	//TWBR = ((F_CPU / TWI_FREQ) - 16) / 2;

	//Release ustream neighbour pin
	pinMode(2,INPUT);
	digitalWrite(2,LOW);

	//attach external interupt on upstream pin
	attachInterrupt(0, onUpstreamInterrupt, CHANGE);

	//Pull downstream low
	pullDownStream();

	/* twi bit rate formula from atmega128 manual pg 204
	SCL Frequency = CPU Clock Frequency / (16 + (2 * TWBR))
	note: TWBR should be 10 or higher for master mode
	It is 72 for a 16mhz Wiring board with 100kHz TWI */

	//enablePullups();

	//TWAR = daisy_Registers[0] << 1;

	// enable twi module, acks, and twi interrupt
	//TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA);

	//if( user_onStatus != 0 )
	//	user_onStatus(-1);
}


void GadgeteerDaisyLink::disablePullups()
{
	pinMode(A3, INPUT);
	digitalWrite(A3,LOW);
	pinMode(A2, INPUT);
	digitalWrite(A2,LOW);	
}

void GadgeteerDaisyLink::enablePullups()
{
	pinMode(A3, OUTPUT);
	digitalWrite(A3,HIGH);
	pinMode(A2, OUTPUT);
	digitalWrite(A2,HIGH);	
}

void GadgeteerDaisyLink::pullDownStream()
{
	detachInterrupt(1);
	pinMode(3, OUTPUT);
	digitalWrite(3, LOW);
}

void GadgeteerDaisyLink::releaseDownStream()
{
	pinMode(3, INPUT);
	digitalWrite(3, LOW);

	attachInterrupt(1, onDownstreamInterrupt, FALLING);
}


void GadgeteerDaisyLink::notifyMainBoard()
{
}

void GadgeteerDaisyLink::onUpdateRegisterService(uint8_t updatedRegister)
{
	// don't bother if user hasn't registered a callback
	if(!user_onUpdateRegister){
		return;
	}

	user_onUpdateRegister(updatedRegister);
}

void GadgeteerDaisyLink::onStatusService(uint8_t status)
{
	// don't bother if user hasn't registered a callback
	if(!user_onStatus){
		return;
	}

	user_onStatus(status);
}

// sets function called on slave write
void GadgeteerDaisyLink::onUpdateRegister( void (*function)(int) )
{
	user_onUpdateRegister = function;
}


// sets function called on slave write
void GadgeteerDaisyLink::onStatus( void (*function)(int) )
{
	user_onStatus = function;
}

static volatile uint8_t changeAddress = 0;

void GadgeteerDaisyLink::processRequests()
{
	if( changeAddress == 1 )
	{
		changeAddress = 0;
		DaisyLink.releaseDownStream();
	}
}

GadgeteerDaisyLink DaisyLink = GadgeteerDaisyLink();

static volatile uint8_t k=0;
static volatile uint8_t i=0;

extern "C"{

static void onUpstreamInterrupt()
{
	if( digitalRead(2) == LOW  && daisy_state != 1 )//Reset State
	{
		daisy_state = 1;

		//disable TWI
		TWCR = 0;
		DaisyLink.disablePullups();
		DaisyLink.pullDownStream();

				//Setup state
		daisy_Registers[REGISTER_DAISY_ADDRESS] = DAISYLINK_DEFAULT_ADDRESS;
		daisy_Registers[REGISTER_DAISY_CONFIG] = 0;

		//if( user_onStatus != 0 )
		//	user_onStatus(1);

	}
	else if( daisy_state == 1 )
	{
		daisy_state = 2;

		//if( user_onStatus != 0 )
		//	user_onStatus(2);

		DaisyLink.enablePullups();
		//Start TWI to get I2C going
		TWAR = daisy_Registers[REGISTER_DAISY_ADDRESS] << 1;
		TWCR = _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
	}
}

static void onDownstreamInterrupt()
{
	//if( user_onStatus != 0 )
	//	user_onStatus(0x7F);

	if( daisy_state == 3 )
	{
		//Propagate signal up stream

		//remove interrupt
		//from upstream pin
		detachInterrupt(0);

		//pull it down
		pinMode(2,OUTPUT);
		digitalWrite(2, LOW);

		//switch back to input
		pinMode(2, INPUT);
		digitalWrite(2, LOW);

		//reattach interrup
		attachInterrupt(0, onUpstreamInterrupt, FALLING);
	}
}

	SIGNAL(TWI_vect)
	{
		switch(TW_STATUS)
		{
		case TW_SR_SLA_ACK:
			twi_rxBufferIndex = 0;
			ACK;
			break;
		case TW_SR_DATA_ACK:
			{
				if(twi_rxBufferIndex < TWI_BUFFER_LENGTH)
				{
					twi_rxBuffer[twi_rxBufferIndex++] = TWDR;
					ACK;
				}
				else
				{
					NACK;
				}
			}
			break;
		case TW_SR_STOP:
			{
				if( twi_rxBuffer[0] >= 8 )
				{
					k = 1;
					i = twi_rxBuffer[0]-8;
					uint8_t limit = twi_rxBufferIndex;
					while( k < limit )
					{
						pDeviceRegisters[i++] = twi_rxBuffer[k++];
						//k+=1;
						//i+=1;
					}
					
					if( user_onUpdateRegister != 0 )
						user_onUpdateRegister(twi_rxBufferIndex);

					ACK;

				}else
				{
					if( twi_rxBuffer[0] == REGISTER_DAISY_ADDRESS && twi_rxBufferIndex == 2 )
					{						
						daisy_Registers[REGISTER_DAISY_ADDRESS]=twi_rxBuffer[1];
						TWAR = daisy_Registers[REGISTER_DAISY_ADDRESS]<<1;
						changeAddress = 1;
						ACK;
					}
					else if( twi_rxBuffer[0] == REGISTER_DAISY_CONFIG && twi_rxBufferIndex == 2 )
					{
					 //   if( user_onStatus != 0 )
						//  user_onStatus(twi_rxBuffer[1]);

						//if( twi_rxBuffer[1] & 0x80 )
						//{

						//}else
						////{
						////}
						ACK;
					}
					else
					{
						ACK;
					}
				}
			}
			break;
		case TW_SR_DATA_NACK:       // data received, returned nack
			{
				NACK;
			}
			break;
			//    
			//    // Slave Transmitter
		case TW_ST_SLA_ACK:          // addressed, returned ack
			{
				twi_txBufferIndex = 0;
				//Check what register has been send to us previously
				//Daisy specific registers
				if( twi_rxBuffer[0] < 8 )
				{
					twi_txBufferLength = 1;
					twi_txBuffer[0]= daisy_Registers[twi_rxBuffer[0]];
				}
				else
				{
					//ask how many bytes are needed, from module?
				}

				//      // enter slave transmitter mode
				//      twi_state = TWI_STX;
				//      // ready the tx buffer index for iteration
				//      twi_txBufferIndex = 0;
				//      // set tx buffer length to be zero, to verify if user changes it
				//      twi_txBufferLength = 0;
				//      // request for txBuffer to be filled and length to be set
				//      // note: user must call twi_transmit(bytes, length) to do this
				//      twi_onSlaveTransmit();
				//      // if they didn't change buffer & length, initialize it
				//      if(0 == twi_txBufferLength){
				//        twi_txBufferLength = 1;
				//        twi_txBuffer[0] = 0x00;
				//      }
				// transmit first byte from buffer, fall
			}
		case TW_ST_DATA_ACK: // byte sent, ack returned
			{
				TWDR = twi_txBuffer[twi_txBufferIndex++];
				//ACK;
				if(twi_txBufferIndex < twi_txBufferLength)
				{
					ACK;
				}
				else
				{
					NACK;
				}
			}
			break;
			
		case TW_ST_DATA_NACK: //0xC0 received nack, we are done 
			{
				ACK;
			}
			break;
		case TW_ST_LAST_DATA: //0xC8 received ack, but we are done already!
			{
				ACK;
			}
			break;
			//
			//    // All
		case TW_NO_INFO:   // no state information
			break;
			//    case TW_BUS_ERROR: // bus error, illegal stop/start
			//      twi_error = TW_BUS_ERROR;
			//      twi_stop();
			//      break;
			//
		}
	}
}
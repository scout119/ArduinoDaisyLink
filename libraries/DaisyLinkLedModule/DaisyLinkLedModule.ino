// Gadgeteer Smart Led Daisy Link Module Example
// by Valentin Ivanov <http://www.breakcontinue.com>

// Demonstrates use of the DaisyLink library
// DaisyLink library follows the Gadgeteer DaisyLink protocol

// Created 15 January 2012

// This example code is in the public domain.

#include <GadgeteerDaisyLink.h>

#define MANUFACTURER   0x00
#define MODULE_TYPE    0x01
#define MODULE_VESION  0x01

//
#define REGISTER_RED_VALUE   0
#define REGISTER_GREEN_VALUE 1
#define REGISTER_BLUE_VALUE  2

static byte ledRegisters[8]= { 
  0, //R value
  0, //G value
  0,  //B value
  0,
  0,
  0,
  0,
  0
};

void setup()
{
  Serial.begin(9600);
  
  pinMode(5,OUTPUT);
  pinMode(6,OUTPUT);
  pinMode(9,OUTPUT);
  
  //Turn off all colors
  digitalWrite(5,HIGH);
  digitalWrite(6,HIGH);
  digitalWrite(9,HIGH);
  
  //Setup callbacks  
  DaisyLink.onStatus(onStatusEvent);  
  DaisyLink.onUpdateRegister(onUpdateRegisterEvent);
  
  //Initialize DaisyLink module
  DaisyLink.initialize(MANUFACTURER, MODULE_TYPE, MODULE_VESION, &ledRegisters[0], 8 );
  
  Serial.println((int)&ledRegisters[0]);
  Serial.println("Started 0");  
}

void loop()
{
  DaisyLink.processRequests();
}

void onUpdateRegisterEvent(int updatedRegister)
{ 
  if( ledRegisters[5] == 0 )
    digitalWrite(5,HIGH);
  else
    digitalWrite(5,LOW);
    
  if( ledRegisters[6] == 0 )
    digitalWrite(6,HIGH);
  else
    digitalWrite(6,LOW);

  if( ledRegisters[7] == 0 )
    digitalWrite(9,HIGH);
  else
    digitalWrite(9,LOW);

  //Serial.println("!!!!");
}

void onStatusEvent(int i2cStatus)
{
  Serial.println(i2cStatus,HEX);
}


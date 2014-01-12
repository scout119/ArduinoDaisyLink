// Gadgeteer Daisy Link  Piezo Module
// by Valentin Ivanov <http://www.breakcontinue.com>

// Demonstrates use of the DaisyLink library
// DaisyLink library follows the Gadgeteer DaisyLink protocol

// Created 15 January 2012
// This code is in the public domain.

#include <GadgeteerDaisyLink.h>

#define  c3    7634
#define  d3    6803
#define  e3    6061
#define  f3    5714
#define  g3    5102
#define  a3    4545
#define  b3    4049
#define  c4    3816    // 261 Hz 
#define  d4    3401    // 294 Hz 
#define  e4    3030    // 329 Hz 
#define  f4    2865    // 349 Hz 
#define  g4    2551    // 392 Hz 
#define  a4    2272    // 440 Hz 
#define  a4s   2146
#define  b4    2028    // 493 Hz 
#define  c5    1912    // 523 Hz
#define  d5    1706
#define  d5s   1608
#define  e5    1517
#define  f5    1433
#define  g5    1276
#define  a5    1136
#define  a5s   1073
#define  b5    1012
#define  c6    955

// Define a special note, 'R', to represent a rest
#define  R     0 
#define MANUFACTURER   0x00
#define MODULE_TYPE    0x01
#define MODULE_VESION  0x01

#define PIEZO_MODE_OFF         0
#define PIEZO_MODE_ON          1
#define PIEZO_MODE_CONTINUOUS  2
#define PIEZO_MODE_MAX PIEZO_MODE_CONTINUOUS
 
#define PIEZO_VOL_LOW  0
#define PIEZO_VOL_HIGH 1

#define PIEZO_MELODY_DEBUG 0
#define PIEZO_MELODY_SW    1
#define PIEZO_MELODY_LS    2
#define PIEZO_MELODY_MAX PIEZO_MELODY_LS

static byte piezoRegisters[3]= { 
  0, //mode
  0, //melody
  0  //volume
};

// note debug
int melody0[] = { c4, d4, e4, f4, g4, a4, b4, c5 };
int beats0[]  = { 63, 64, 64, 64, 64, 64, 64, 64 };

//star wars theme
int melody1[] = {  f4,  f4, f4,  a4s,   f5,  d5s,  d5,  c5, a5s, f5, d5s,  d5,  c5, a5s, f5, d5s, d5, d5s,   c5};//
int beats1[]  = {  21,  21, 21,  128,  128,   21,  21,  21, 128, 64,  21,  21,  21, 128, 64,  21, 21,  21, 128 }; 

//little star theme
int melody2[] = { c5, c5, g5,g5, a5, a5, g5, f5, f5, e5, e5, d5, d5, c5, R  };
int beats2[] = { 30, 30, 30, 30, 30, 30, 60, 30, 30, 30, 30, 30, 30, 60, 120 };

#define SP1  5
#define SP2  6

void setup()
{
  Serial.begin(9600);
  
  pinMode(SP1,OUTPUT);
  pinMode(SP2,OUTPUT);
  digitalWrite(SP2, LOW);
  
  //Setup callbacks  
  DaisyLink.onUpdateRegister(onUpdateRegisterEvent);
  
  //Initialize DaisyLink module
  DaisyLink.initialize(MANUFACTURER, MODULE_TYPE, MODULE_VESION, &piezoRegisters[0], 3 );
  
  Serial.println("Initialized");
}

bool isPlaying = false;
int currentMelodyIndex = -1;
int * currentMelody = 0;
int * currentBeats = 0;
int currentNote = 0;
int numberOfTonesInMelody = 0;

// Set overall tempo
long tempo = 10000;
// Set length of pause between notes
int pause = 1000;
// Loop variable to increase Rest length
int rest_count = 50;

void playTone(int toneM, int beat, int volume) 
{
  long duration = beat * tempo;
  
  long elapsed_time = 0;
  
  if (toneM > 0) { // if this isn't a Rest beat, while the tone has 
    //  played less long than 'duration', pulse speaker HIGH and LOW
    while (elapsed_time < duration) {

      digitalWrite(SP1,HIGH);
      if( volume != 0 )
        digitalWrite(SP2,LOW);
        
      delayMicroseconds(toneM / 2);

      // DOWN
      digitalWrite(SP1, LOW);
      if( volume != 0 )
        digitalWrite(SP2,HIGH);

      delayMicroseconds(toneM / 2);

      // Keep track of how long we pulsed
      elapsed_time += (toneM);
    } 
  }
  else { // Rest beat; loop times delay
    for (int j = 0; j < rest_count; j++) { // See NOTE on rest_count
      delayMicroseconds(duration);  
    }                                
  }                                 
}

void switchMelody()
{
       switch( currentMelodyIndex )
       {
         case PIEZO_MELODY_DEBUG:
         {
           currentMelody = &melody0[0];
           currentBeats = &beats0[0];
           numberOfTonesInMelody = sizeof(melody0)/2;
           break;
         }
         case PIEZO_MELODY_SW:
         {
           currentMelody = &melody1[0];
           currentBeats = &beats1[0];
           numberOfTonesInMelody = sizeof(melody1)/2;
           break;
         }
         case PIEZO_MELODY_LS:
         {
           currentMelody = &melody2[0];
           currentBeats = &beats2[0];
           numberOfTonesInMelody = sizeof(melody2)/2;
           break;
         }
       }  
}

void loop()
{
  DaisyLink.processRequests();
  
  int mode = piezoRegisters[0];
  
  if( piezoRegisters[0] != 0 )
  {
    int melody = piezoRegisters[1];
    if( melody > PIEZO_MELODY_MAX )
      melody = PIEZO_MELODY_DEBUG;
      
    if( !isPlaying )
    {
      Serial.println("Not playing");
      Serial.println(piezoRegisters[1]);
      
       isPlaying = true;
       currentNote = 0;
       currentMelodyIndex = melody;
       switchMelody();
    }else
    {
      if( melody != currentMelodyIndex )
      {
        isPlaying = false;
       currentNote = 0;
       currentMelodyIndex = melody;
       switchMelody();
      }
    }

    playTone(currentMelody[currentNote], currentBeats[currentNote], piezoRegisters[2]);
    
    // A pause between notes...
    delayMicroseconds(pause);
    
    currentNote+=1;
    if( currentNote >= numberOfTonesInMelody )
    {
      if( mode == 2 )
      {
        currentNote = 0;
      }
      else
      {
        piezoRegisters[0] = 0;
        isPlaying = false;
      }
    }      
  }
  else
  {
    isPlaying = false;
    currentMelodyIndex = -1;
    currentMelody = 0;
  }  
}

void onUpdateRegisterEvent(int updatedRegister)
{
 Serial.println(updatedRegister); 
}


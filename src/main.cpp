#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stream.h>
#include <EEPROM.h>
#include <Servo.h>
#include <Snooze.h> // Controls the Teensy sleep mode
#include <TimeLib.h>
#include <TimeAlarms.h>
#include <Bounce.h>
#include <DS1307RTC.h>  // a basic DS1307 library that returns time as a time_t
#include <FreqMeasure.h>
#include <FreqCount.h>
#include <SPI.h>
#include <Arduino.h>
#include <IRremote.h>
#include "PinDefinitionsAndMore.h"


void intialise_pump(int duty, int time);
void pump_set(bool on, bool clockwise, int duty);
void blink_led(int times); 


// Load drivers
SnoozeDigital digital;
SnoozeAlarm  alarm;
SnoozeTimer timer; 
SnoozeBlock config_off_sleep(timer); 


int flag = 1;
int counter = 0; //when counter = 2880 stop
int analog_write_freq = 980;
int duty_cycle = 90;
int MAX_INIT_SECONDS = 120; 


// Setup IR Receiver 
int RECV_PIN = 1; // Define input pin on arduino 
unsigned long IRCode = 0; // Initialise IRCode (to be received from IR Remote)
#define ONE 0xBA45FF00   // HEX code for the 1 button
#define TWO 0xB946FF00 // HEX code for the 2 button
#define THREE 0xB847FF00 // HEX code for the 3 button
#define FOUR 0xBB44FF00 
#define FIVE 0xBF40FF00
#define SIX 0xBC43FF00
#define SEVEN 0xF807FF00
#define EIGHT 0xEA15FF00
#define NINE 0xF609FF00
#define STAR 0xE916FF00
#define ZERO 0xE619FF00
#define HASH 0xF20DFF00 // HEX code for the # button 
#define OK 0xE31CFF00  // HEX code for the OK button

// Setup Motor Pins
#define AN_SPEED_PIN 14
#define EN_MOTOR_PIN 24
#define DIR_MOTOR_PIN 25



enum SamplingState {
  SAMPLING_IDLE, 
  SAMPLING_WAIT_START, 
  SAMPLING_PUMP_ON, 
  SAMPLING_PUMP_OFF, 
  SAMPLING_DONE, 
  INITIALISE 
};

struct SamplingController { 
  SamplingState state; 
  uint32_t startDelayMs; 
  uint32_t onTimeMs; 
  uint32_t offTimeMs; 
  uint32_t lastTransitionMs; 
  uint32_t cyclesDone; 
  uint32_t maxCycles; 
  int duty;
};

time_t getTeensy3Time() {
	return Teensy3Clock.get();
}

SamplingController sampling; 

void start_intermittent_sampling(uint32_t delay, uint32_t onTimeSec, uint32_t offTimeSec, int dutycycle){ 
  sampling.startDelayMs = delay * 3600000UL;
  sampling.onTimeMs = onTimeSec * 1000UL; 
  sampling.offTimeMs = offTimeSec * 1000UL; 
  sampling.lastTransitionMs = millis(); 
  sampling.cyclesDone = 0; 
  sampling.maxCycles = 24UL * 3600UL / (onTimeSec + offTimeSec); 
  sampling.duty = dutycycle; 
  sampling.state = SAMPLING_WAIT_START;
}

void intermittent_sampling_update() { 
  uint32_t now = millis(); 

  switch(sampling.state) { 

    case SAMPLING_IDLE: 
      break; 
    
    case INITIALISE: 
      if (now - sampling.lastTransitionMs >= MAX_INIT_SECONDS * 1000UL) { 
        sampling.state = SAMPLING_DONE; 
        pump_set(false, true, 0);
      }else{
        pump_set(true, true, 90); 
      }
      break;

    case SAMPLING_WAIT_START:{
      if (sampling.startDelayMs == 0) { 
        Serial.println("Pump Turning On");
        pump_set(true, true, sampling.duty); 
        sampling.lastTransitionMs = millis(); 
        sampling.state = SAMPLING_PUMP_ON; 
        break;
      }
      uint32_t startDelaySec = sampling.startDelayMs / 1000UL; 
      timer.setTimer(startDelaySec); 
      Snooze.deepSleep(config_off_sleep); 
      pump_set(true, true, sampling.duty); 
      sampling.lastTransitionMs = millis(); 
      sampling.state = SAMPLING_PUMP_ON;
      break; 
    }
    
    case SAMPLING_PUMP_ON: {
      uint32_t onSeconds = sampling.onTimeMs / 1000UL;
      Serial.println("In turn ON");
      if (onSeconds == 0) onSeconds = 1;
      timer.setTimer(onSeconds);
      Snooze.sleep(config_off_sleep);   
      pump_set(true, true, 0);
      sampling.lastTransitionMs = millis();
      sampling.state = SAMPLING_PUMP_OFF;
      break;
    }


    case SAMPLING_PUMP_OFF:{
      uint32_t offSeconds = sampling.offTimeMs / 1000UL; 
      if (offSeconds == 0) offSeconds = 1; 
      Serial.println("In turn OFF");
      timer.setTimer(offSeconds); 
      Snooze.sleep(config_off_sleep); 

      sampling.cyclesDone++; 
      if (sampling.cyclesDone >= sampling.maxCycles){ 
        sampling.state = SAMPLING_DONE; 
      }else { 
        pump_set(true, true, sampling.duty); 
        sampling.lastTransitionMs =millis();
        sampling.state = SAMPLING_PUMP_ON; 
      }
      
      break;
    }

    case SAMPLING_DONE: 
      pump_set(false, true, 0); 
      sampling.state = SAMPLING_IDLE; 
      break; 
  }
}

void blink_led(int times) { 
  for (int i=0; i<times; i++) { 
    delay(1000); 
    digitalWrite(LED_BUILTIN, HIGH); 
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW); 
  }
}

void handle_ir() { 
  if (!IrReceiver.decode()){ 
    return; 
  }
  unsigned long irCode = IrReceiver.decodedIRData.decodedRawData; 
  
  if (irCode == HASH) { 
    if (sampling.state == INITIALISE) { 
      sampling.state = SAMPLING_DONE; 
    }else if (sampling.state == SAMPLING_IDLE) {
      sampling.state = INITIALISE;
      sampling.lastTransitionMs = millis();
    }
    
  }else if (irCode  == OK){ 
    blink_led(1); 
    start_intermittent_sampling(0, 3, 27, duty_cycle); 
  }else if (irCode == ONE) { 
    blink_led(2); 
    start_intermittent_sampling(12, 3, 27, duty_cycle);
  }else if (irCode == TWO) { 
    blink_led(4); 
    start_intermittent_sampling(24, 3, 27, duty_cycle); 
  }else if (irCode == THREE) { 
    blink_led(6); 
    start_intermittent_sampling(48, 3, 27, duty_cycle); 
  }else if (irCode == STAR) { 
    sampling.state = SAMPLING_IDLE;
  }
  IrReceiver.resume();
}

void pump_set(bool on, bool clockwise, int duty) { //todo: direction
  digitalWrite(EN_MOTOR_PIN, on ? HIGH : LOW);
  digitalWrite(DIR_MOTOR_PIN, clockwise ? LOW : HIGH); 
  analogWrite(AN_SPEED_PIN, on ? duty * 1023 / 100 : 0); 
}


void intialise_pump(int duty) { //todo: direction
  pump_set(true, true, duty);  
  pump_set(true, true, 0);
}


void setup() {
  analogWriteResolution(10);
  Serial.begin(9600);
  alarm.setRtcTimer(0,0,2);
  setSyncProvider(getTeensy3Time); // Gets the time from the Serial5 monitor
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(DIR_MOTOR_PIN, OUTPUT);
  pinMode(EN_MOTOR_PIN, OUTPUT);
  pinMode(AN_SPEED_PIN, OUTPUT);


  digitalWrite(LED_BUILTIN, HIGH);
  delay(2000); 
  digitalWrite(LED_BUILTIN, LOW);

  analogWriteFrequency(AN_SPEED_PIN, analog_write_freq); 

  IrReceiver.begin(RECV_PIN, DISABLE_LED_FEEDBACK); // Start the receiver

  pump_set(false, true, 0);      // EN low, PWM 0 → motor off
  sampling = {};                 // zero all fields
  sampling.state = SAMPLING_IDLE;
}


void loop() {
  handle_ir(); 
  intermittent_sampling_update(); 
}


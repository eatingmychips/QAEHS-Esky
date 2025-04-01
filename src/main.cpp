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



// Load drivers
SnoozeDigital digital;
SnoozeUSBSerial usb;
SnoozeAlarm  alarm;

SnoozeBlock config_teensy40( usb, alarm, digital);
int led = 13;
int flag = 1;
int counter = 0; //when counter = 2880 stop

int start_delay = 12; // Specify delay in hours 

void stepper_act(int pin, int dir_pin, int clockwise, int en_pin, int duty);
void intialise_pump(int pin, int dir_pin, int clockwise, int en_pin, int duty);
void intermittent_sampling(int start_delay, int on_time, int off_time, int duty);


// Setup IR Receiver 
int RECV_PIN = 6; // Define input pin on arduino 
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



time_t getTeensy3Time() {
	return Teensy3Clock.get();
}

void setup() {
  analogWriteResolution(10);
  Serial.begin(9600);
  alarm.setRtcTimer(0,0,2); // hour, min, sec (0,10,0)
  setSyncProvider(getTeensy3Time); // Guess??????? it gets the time from the Serial5 monitor
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(22, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(led, OUTPUT);

  digitalWrite(led, HIGH);
  delay(3000); 
  digitalWrite(led, LOW);

  IrReceiver.begin(RECV_PIN, LED_FEEDBACK_DISABLED_COMPLETELY); // Start the receiver
}

// stepper_act(int pin, int dir_pin, int clockwise, int en_pin, int rpm)

void loop() {
    // Wait for IR receiver to get message from remote
  while (IRCode == 0){ 
    if (IrReceiver.decode()){
      IRCode = IrReceiver.decodedIRData.decodedRawData;
      Serial.println(IRCode, HEX);
      if ((IRCode == OK || IRCode == ONE || IRCode == TWO || IRCode == THREE)){ // If IR value received is valid
        continue;
      }
      else if (IRCode == HASH) {
        intialise_pump(22, 3, 1, 4, 90);
        IRCode = 0;
      }
      else {
        IRCode = 0;
        Serial.println(IRCode);
      }
      IrReceiver.resume(); // Receive the next value
    }
  }

  if (IRCode == OK) { // Start immediately Intermittent Sampling
    delay(1000);
    digitalWrite(led, HIGH);
    delay(1000); 
    digitalWrite(led, LOW);
    delay(1000);

  } else if (IRCode == ONE) { // Start immediately Old Code  
    for (int i = 1; i <= 2; i++) {
      digitalWrite(led, HIGH);
      delay(500); 
      digitalWrite(led, LOW);
      delay(500);
    }
    intermittent_sampling(0, 3, 17, 60);

  } else if (IRCode == TWO) { // Start immediately Continuous sampling
    for (int i = 1; i <= 4; i++) {
      digitalWrite(led, HIGH);
      delay(500); 
      digitalWrite(led, LOW);
      delay(500);
    }
  

  } else if (IRCode == THREE) { // Delay for 48 hours 
    for (int i = 1; i <= 6; i++) {
      digitalWrite(led, HIGH);
      delay(500); 
      digitalWrite(led, LOW);
      delay(500);
    }

  }
}



//------------------Set RTC -------------------------------
// Sets the RTC from the serial monitor
// 
void setRTC(void) {
  if (timeStatus() != timeSet) {
    Serial5.println("Unable to sync with the RTC\r");
  } else {
    Serial5.println("RTC has set the system time\r");
  }
}

/**stepper motor control

*/
void stepper_act(int pin, int dir_pin, int clockwise, int en_pin, int duty) { //todo: direction
  //enable the stepper motor pin to hold the torque
  //int timer = 0;
  digitalWrite(en_pin, HIGH);
  //from Pico_1.4_Peristaltic_Pump_Driver.pdf
  //Open (or +5.0 V) = direction anti-clockwise / GND = direction clockwise
  if (clockwise){
    digitalWrite(dir_pin, LOW);
  } else{
    digitalWrite(dir_pin, HIGH);
  }
  if (duty == 0) {
    digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
    //disable the drive pin
    analogWrite(pin, 0);
    //disable the enable pin
    digitalWrite(en_pin, LOW);
  } else {
      digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
      analogWrite(pin, duty * 1023 / 100);    
    }
}


void intialise_pump(int pin, int dir_pin, int clockwise, int en_pin, int duty) { //todo: direction
  //enable the stepper motor pin to hold the torque
  //int timer = 0;
  digitalWrite(en_pin, HIGH);
  //from Pico_1.4_Peristaltic_Pump_Driver.pdf
  //Open (or +5.0 V) = direction anti-clockwise / GND = direction clockwise
  if (clockwise){
    digitalWrite(dir_pin, LOW);
  } else{
    digitalWrite(dir_pin, HIGH);
  }
  if (duty == 0) {
    digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
    //disable the drive pin
    analogWrite(pin, 0);
    //disable the enable pin
    digitalWrite(en_pin, LOW);
  } else {
      digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
      analogWrite(pin, duty * 1023 / 100);    
    }
  delay(120000);
  stepper_act(22,3,1,4,0);
}

// Implementation of intermittent sampling
// start_delay: Hours   on_time: seconds    off_time: seconds   duty: 1-100
void intermittent_sampling(int start_delay, int on_time, int off_time, int duty){
  delay(start_delay*1000*60*60);
  while (true){
    if (counter == 24*60*60/(on_time + off_time) + 1) { //24 Hour runtime
      // Stop the loop after 2880 iterations
      while (true) {
        // Infinite loop to halt execution
      }
    }
    else if (counter == 0) { 
      delay(1000);
    }
  
    else if (flag == 1) {
      stepper_act(22, 3, 1, 4, duty); // Turn pump on
      delay(on_time*1000); // 3s
      flag = 2; // Send system to 2nd flag (wait for 29.3 seconds)
    }
    
  
    else if(flag == 2){
      stepper_act(22, 3, 1, 4, 0); // Turn pump off
      counter++; // Iterate the counter
      delay(off_time*1000); // Delay for 17 seconds
      flag = 1; // Send system back to pump on (flag = 1)
    }
  }
}




// OLD CODE 
void stepper_act_old(int pin, int dir_pin, int clockwise, int en_pin, int rpm) { //todo: direction
  //enable the stepper motor pin to hold the torque
  //int timer = 0;
  digitalWrite(en_pin, HIGH);
  //from Pico_1.4_Peristaltic_Pump_Driver.pdf
  //Open (or +5.0 V) = direction anti-clockwise / GND = direction clockwise
  if (clockwise){
    digitalWrite(dir_pin, LOW);
  } else{
    digitalWrite(dir_pin, HIGH);
  }
  if (rpm == 0) {
    digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
    //disable the drive pin
    analogWrite(pin, 0);
    //disable the enable pin
    digitalWrite(en_pin, LOW);
  } else {
      digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
      //1/256 micro stepping mode: 1 rotation = 51,200 (200 x 256) pulses. 51.2 kHz = 60 rpm, 512 kHz = 600 rpm
      analogWriteFrequency(pin, rpm*51200/60); // 83rpm x 51200/60
      // analogWriteResolution(12);
      analogWrite(pin, 120);    
      delay(1000);
    }
}

void old_sampling(){ 
  if (counter == 300) {
    // Stop the loop after 300 iterations
    while (true) {
      // Infinite loop to halt execution
    }
  }

  else if (flag == 1) {
    stepper_act_old(22, 3, 1, 4, 350);
    delay(3000);
    flag = 2;
    
  } 

  else if(flag == 2){
    stepper_act_old(22, 3, 1, 4, 0);
    flag = 1;
    counter++;
    delay(300000);
  }
}
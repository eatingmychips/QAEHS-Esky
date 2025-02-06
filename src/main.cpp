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



// Load drivers
SnoozeDigital digital;
SnoozeUSBSerial usb;
SnoozeAlarm  alarm;

SnoozeBlock config_teensy40( usb, alarm, digital);
int led = 13;
int flag = 0;
int counter = 0; //when counter = 6*4 stop

void stepper_act(int pin, int dir_pin, int clockwise, int en_pin, int rpm);

time_t getTeensy3Time() {
	return Teensy3Clock.get();
}

void setup() {

  Serial.begin(9600);
  alarm.setRtcTimer(0,0,2); // hour, min, sec (0,10,0)
  setSyncProvider(getTeensy3Time); // Guess??????? it gets the time from the Serial5 monitor
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(22, OUTPUT);
  pinMode(10, OUTPUT);
  pinMode(led, OUTPUT);

}

// stepper_act(int pin, int dir_pin, int clockwise, int en_pin, int rpm)

void loop() {
  // int who=Snooze.hibernate( config_teensy40 );
  if (counter == 2881) { //24 Hou r runtime
    // Stop the loop after 300 iterations
    while (true) {
      // Infinite loop to halt execution
    }
  }

  if (flag == 0) {
    stepper_act(22, 3, 1, 4, 350);
    delay(25000);//40000
    flag = 2;
    
  }

  else if (flag == 1) {
    stepper_act(22, 3, 1, 4, 250);
    delay(600);//20000
    flag = 2;
  }

  else if(flag == 2){
    stepper_act(22, 3, 0, 4, 0);
    flag = 1;
    counter++;
    delay(29300);
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
void stepper_act(int pin, int dir_pin, int clockwise, int en_pin, int rpm) { //todo: direction
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

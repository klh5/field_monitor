#include <TimeLib.h>
#include <Wire.h>
#include <DS1307RTC.h>
#include <LowPower.h>

//The pin used to control the MOSFET which turns power to the Raspberry Pi on and off
#define PI_PIN  4

//Pin 9 is used to control the onboard LED
#define LED_PIN 9

void setup() {

  Serial.begin(115200);

  //OUTPUT and LOW mean that the MOSFET is off
  pinMode(PI_PIN, OUTPUT);
  digitalWrite(PI_PIN, LOW);

  //OUTPUT and LOW mean that the LED is off
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  setSyncProvider(RTC.get);

  fetch_time();

}

void loop() {

  byte i;

  fetch_time();

  //Turn on power to the pi
  digitalWrite(PI_PIN, HIGH);   

  //Turn on the LED
  digitalWrite(LED_PIN, HIGH);

  //Go to sleep for a bit
  for(i=0; i<4; i++) {                                        
      LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_ON);
  }

  //Send fake data to the pi
  send_timestamp();

  Serial.println("a b c d e f g h");
  delay(120);
  Serial.println("28 200 100 200 300 400 500 600 700 800 -24");
  delay(120);
  Serial.println("29 200 100 200 300 400 500 600 700 800 -56");

  //Send End flag
  Serial.println("E");
  Serial.flush();

  //Sleep for a bit to let Pi shut down
  for(i=0; i<2; i++) {
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_ON);
  }

  //Turn off power to the pi
  digitalWrite(PI_PIN, LOW);   

  //Turn off the LED
  digitalWrite(LED_PIN, LOW);

  //Sleep to simulate gap between readings
  for(i=0; i<10; i++) {
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_ON);
  }
  
}

void send_timestamp() {

  fetch_time();
  
  time_t curr_time = now(); 
  
  Serial.print("T");                                //RPi looks for the "T" to know that it's a timestamp
  
  Serial.print(hour(curr_time));
  Serial.print(":");
  Serial.print(minute(curr_time));
  Serial.print(":");
  Serial.print(second(curr_time));
  Serial.print("_");
  Serial.print(day(curr_time));
  Serial.print("-");
  Serial.print(month(curr_time));
  Serial.print("-");
  Serial.print(year(curr_time)); 
  Serial.println();

}

/*
 * Request the current time from the DS1307 real time clock.
 */
void fetch_time() {

  setTime(RTC.get());
}




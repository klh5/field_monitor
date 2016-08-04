
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

}

void loop() {

  byte i;
  char test_time[20] = "20-08-16_11:12:13";

  //Turn on power to the pi
  digitalWrite(PI_PIN, HIGH);   

  //Turn on the LED
  digitalWrite(LED_PIN, HIGH);

  //Go to sleep for a bit
  for(i=0; i<4; i++) {                                        
      LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_ON);
  }

  //Send fake data to the pi
  Serial.print("T");
  Serial.println(test_time);

  Serial.println("a b c d e f g h");
  LowPower.powerDown(SLEEP_120MS, ADC_OFF, BOD_ON);
  Serial.println("28 200 4.5 100 200 300 400 500 600 700 800 -24");
  LowPower.powerDown(SLEEP_120MS, ADC_OFF, BOD_ON);
  Serial.println("29 200 4.7 100 200 300 400 500 600 700 800 -56");

  //Send End flag
  Serial.println("E");

  //Sleep for a bit to let Pi shut down
  for(i=0; i<3; i++) {
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


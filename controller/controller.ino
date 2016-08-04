/*
 * The controller is attached to the Raspberry Pi. It coordinates taking readings from all of the loggers, and outputs the data from the loggers to the RPi through the USB port.
 * 
 * It also turns the Raspberry Pi on and off, so that it can record data and capture images from the camera.
 * 
 */

/*
 * Additional libraries that we need - don't worry about these.
 */
#include <SPI.h>
#include <RFM69.h>
#include <TimeLib.h>
#include <LowPower.h>
#include <Wire.h>
#include <DS1307RTC.h>

/*******************************Things you might want to change*************************************************************/

 //Set the frequency of readings. This needs to be a 5 minute interval, so 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, or 00 for every hour.
#define READ_FREQ   5

/*
 * These variables relate to the radio. 
 */
#define LOGGER_ID     1                   //The ID of this radio on the network. The controller acts as a gateway, so it's always 1
#define NETWORK_ID    202                 //The ID of the network that all related radios are on. This needs to be the same for all radios that talk to each other
#define FREQUENCY     RF69_433MHZ         //The frequency of the radio
#define ENCRYPTKEY    "as86HbM097Ljqd93"  //The 16-digit encyption key - must be the same on all radios that talk to each other!

//The maximum number of photodiodes that a logger on this network has
#define MAX_PHOTODIODES   8

/***************************************************************************************************************************/

//The pin used to control the MOSFET which turns power to the Raspberry Pi on and off
#define PI_PIN  4

//The amount of time allocated to listening for packets once a send command has been issued (in milliseconds)
#define WAIT_TIME 35000                  

//Declare variables
RFM69 radio;
char take_reading[2] = "R";
char init_nodes[2] = "I";
char sleep_nodes[2] = "S";
byte i;

typedef struct {
  float light_readings[MAX_PHOTODIODES];       
} data_packet;

/*
Setup only runs once, when the logger starts up
*/
void setup() {
  
  byte i;
  
  //Set unused digital pins to OUTPUT and LOW, so that they don't float
  for(i=3; i<=9; i++) {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }

  /*
   * These checks result in LED flashes if they fail. The LED will flash a different number of times depending on the problem. In all cases, the LED will flash rapidly several
   * times followed by a one second gap.
   * 
   * 3 flashes - the network ID is too high (more than 253) or too low (less than 1)
   * 4 flashes - the logger ID is too high (more than 253) or too low (less than 2)
   * 5 flashes - the maximum number of photodiodes has been changed, and does not equal 8
   */
  if(NETWORK_ID < 1 || NETWORK_ID > 254) {                                //Check that the network ID is reasonable
    while(1) {
      flash_led(3);
      LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_ON);
    }
  }
  else if(LOGGER_ID < 2 || LOGGER_ID > 254) {                                  //Check that the logger ID is reasonable
    while(1) {
      flash_led(4);
      LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_ON);
    }
  }
  else if(MAX_PHOTODIODES != 8) {                                             //Check that MAX_PHOTODIODES is 8
    while(1) {
      flash_led(5);
      LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_ON);
    }
  }

  //Setup radio
  radio.initialize(FREQUENCY, LOGGER_ID, NETWORK_ID);   //Initialize the radio with the above settings
  radio.encrypt(ENCRYPTKEY);                            //Encrypt transmissions
  radio.promiscuous(false);
  radio.sleep();                                        //Sleep the radio to save power

  fetch_time();

  while(true) {
    if(second() == 30 && minute() == (READ_FREQ-1)) {
      break;
    }
  }

  radio.send(255, init_nodes, strlen(init_nodes));  //Tell all of the loggers to reset
}

/*
Loop() runs over and over again forever.
*/
void loop() {

  fetch_time();                                                 //Get an up to date time stamp from the Pi
  time_t curr_time = now();                                     //Create variable curr_time to hold time so that it doesn't change during loop execution

  if(second(curr_time) == 0) {                                  //Check if second is 0             
    if(minute(curr_time) == READ_FREQ) {                        //Check if the minute is the same as READ_FREQ
      send_timestamp();
      radio.send(255, take_reading, strlen(take_reading));      //Broadcast a read command
      waitForReadings();                                        //Wait for readings to come back from loggers
      radio.sleep();                                            //Sleep the radio to save power
      shut_down_pi();
      for(i=0; i<25; i++) {                                     //Sleep for ~3.5 minutes to save power. ~1 minute is already used waiting for data packets and the Pi
        LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_ON);
      }
    }
    else if((minute(curr_time)+5) % 5 == 0) {                   //Check if the minute is a multiple of 5 - loggers wake up every five minutes, and need to be told to sleep again
      radio.send(255, sleep_nodes, strlen(sleep_nodes));
      radio.sleep();
      for(i=0; i<33; i++) {                                     //Sleep for ~4 minutes to save power - need to wake at least 30 seconds before next 5 minute interval
        LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_ON);
      }
    }
  }
  else if(second(curr_time) == 30) {
    if(minute(curr_time) == READ_FREQ-1) {                      //Check if we are 30 seconds before readings need to be taken
      digitalWrite(PI_PIN, HIGH);                               //Turn on Raspberry Pi so that it has time to boot
    }
    for(i=0; i<3; i++) {                                        //Save power by sleeping for about 24 seconds
      LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_ON);
    }
  }
}

/*
 * After a read command is issued, the controller sets time aside to listen for returning data. This amount of time is based on the maximum number of loggers that can exist on one
 * network, which is 254. Because each logger has it's own window in which to send, it can take a long time for all of the data to be sent.
 */
void waitForReadings() {
  
  unsigned long start_time = millis();
  data_packet data_in;

  //Loop for the amount of time stated by WAIT_TIME
  while(millis() - start_time < WAIT_TIME) {

    //Check if a packet has been received
    if(radio.receiveDone()) {

      //If a packet has been received, check that it's the right size
      if (radio.DATALEN == sizeof(data_in)) {

        //Copy the data into memory, and then print it. Data is copied first in case more data arrives while we are printing
        data_in = *(data_packet*)radio.DATA;
        byte sender_id = radio.SENDERID;
        int sender_rssi = radio.readRSSI();
      
        Serial.print(sender_id); Serial.print(" ");
        Serial.print(NETWORK_ID); Serial.print(" ");

        for(i=0; i<MAX_PHOTODIODES; i++) {
          Serial.print(data_in.light_readings[i]); Serial.print(" ");
        }

        //End with a newline character, so that the Python script knows that we are done printing this data
        Serial.println(sender_rssi);
      }
    }
  }
}

/*
 * Sends an "E" to tell the Pi that loggers have finished sending, then turns off the Pi.
 */
void shut_down_pi() {

  Serial.println("E");
  
  for(i=0; i<3; i++) {
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_ON);  //Allow ~24 seconds for the Pi to take a picture and shut down
  }
  
  digitalWrite(PI_PIN, LOW);                        //Turn off the Raspberry Pi
}

/*
 * Send a timestamp over serial port to the Raspberry Pi, so that images and data have the correct timestamp.
 */
void send_timestamp() {

  time_t timestamp = now();
  Serial.print("T");                                //RPi looks for the "T" to know that it's a timestamp
  Serial.println(timestamp);                        //RPi reads until end of line, so print the timestamp straight after the "T", followed by a new line
}

/*
 * Request the current time from the DS1307 real time clock.
 */
void fetch_time() {

  while(timeStatus()!= timeSet) {
    setSyncProvider(RTC.get);
  }
}

/*
 * Flash the onboard LED num_flashes amount of times. Different numbers of flashes indicate different problems.
 */
void flash_led(int num_flashes)
{

  for(i=0; i<num_flashes; i++) {
    digitalWrite(9, HIGH);
    delay(50);
    digitalWrite(9, LOW);
  }
}



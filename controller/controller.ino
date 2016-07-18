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
#include <RFM69_ATC.h>
#include <TimeLib.h>
#include <LowPower.h>
#include <Wire.h>
#include <DS1307RTC.h>

/*******************************Things you might want to change*************************************************************/
 
/*
 * These variables relate to the radio. 
 */
#define NODEID        1                   //The ID of this radio on the network. The controller acts as a gateway, so it's always 1
#define NETWORKID     202                 //The ID of the network that all related radios are on. This needs to be the same for all radios that talk to each other
#define FREQUENCY     RF69_433MHZ         //The frequency of the radio
#define ENCRYPTKEY    "as86HbM097Ljqd93"  //The 16-digit encyption key - must be the same on all radios that talk to each other!
#define ENABLE_ATC                        //Enables automatic power adjustment - no need to change this

//Set the frequency of readings. This needs to be a 5 minute interval, so 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, or 00 for every hour.
#define READ_FREQ   5

//Number of photodiodes - they will be read from in order, 0-N
#define NUM_PHOTODIODES 8

/***************************************************************************************************************************/

//The pin used to control the MOSFET which turns power to the Raspberry Pi on and off
#define PI_PIN  4

//The amount of time allocated to listening for packets once a send command has been issued (in milliseconds)
#define WAIT_TIME 35000                  

//Declare variables
RFM69_ATC radio;
char take_reading[2] = "R";
char init_nodes[2] = "I";
char sleep_nodes[2] = "S";

typedef struct {
  float light_readings[NUM_PHOTODIODES];            //There are 8 analog pins, so maximum 8 photodiodes
} data_packet;

/*
Setup only runs once, when the logger starts up
*/
void setup() {

  Serial.begin(115200);

  //Set PI_PIN to output so that we can control the MOSFET
  pinMode(PI_PIN, OUTPUT);

  //Pull PI_PIN low to turn the MOSFET off so that the Pi isn't switched on yet
  digitalWrite(PI_PIN, LOW);

  //Setup radio
  radio.initialize(FREQUENCY, NODEID, NETWORKID);   //Initialize the radio with the above settings
  radio.encrypt(ENCRYPTKEY);                        //Encrypt transmissions
  radio.enableAutoPower(-70);                       //Set auto power level
  radio.promiscuous(false);
  radio.sleep();                                    //Sleep the radio to save power

  fetch_time();

  while(true) {
    if(minute() == READ_FREQ && second() == 0) {
      break;
    }
  }

  Serial.println("Resetting nodes");
  radio.send(255, init_nodes, strlen(init_nodes));  //Tell all of the loggers to reset
}

/*
Loop() runs over and over again forever.
*/
void loop() {

  byte i;
  fetch_time();               //Get an up to date time stamp from the Pi
  time_t curr_time = now();   //Create variable curr_time to hold time so that it doesn't change during loop execution

  if(second(curr_time) == 0) {                                  //Check if second is 0             
    if(minute(curr_time) == READ_FREQ) {                        //Check if the minute is the same as READ_FREQ
      Serial.println("Telling nodes to send readings...");
      digitalWrite(PI_PIN, HIGH);                               //Turn on Raspberry Pi so that it can boot
      radio.send(255, take_reading, strlen(take_reading));      //Broadcast a read comman
      waitForReadings();                                        //Wait for readings to come back from loggers
      radio.sleep();                                            //Sleep the radio to save power
      send_pi_time();
      for(i=0; i<25; i++) {                                     //Sleep for ~3.5 minutes to save power. The Moteino can only sleep for 8 seconds at a time
        LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_ON);
      }
    }
    else if((minute(curr_time)+5) % 5 == 0) {                   //Check if the minute is a multiple of 5 - loggers wake up every five minutes, and need to be told to sleep again
      Serial.println("Telling nodes to keep sleeping...");
      radio.send(255, sleep_nodes, strlen(sleep_nodes));
      radio.sleep();
      for(i=0; i<30; i++) {                                     //Sleep for 4 minutes to save power
        LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_ON);
      }
    }
  }
}

/*
 * After a read command is issued, the controller sets time aside to listen for returning data. This amount of time is based on the maximum number of loggers that can exist on one
 * network, which is 254. Because each logger has it's own window in which to send, it can take a long time for all of the data to be sent.
 */
void waitForReadings() {
  
  byte i;
  unsigned long start_time = millis();
  data_packet data_in;

  //Loop for the amount of time stated by WAIT_TIME
  while(millis() - start_time < WAIT_TIME) {

    if(radio.receiveDone()) {

      if (radio.DATALEN != sizeof(data_in)) {
        Serial.println("INVALID PACKET");
      }
      
      else {
        data_in = *(data_packet*)radio.DATA;
        byte sender_id = radio.SENDERID;
        int sender_rssi = radio.readRSSI();
      
        Serial.print(sender_id); Serial.print(" ");
        Serial.print(NETWORKID); Serial.print(" ");

        for(i=0; i<NUM_PHOTODIODES; i++) {
          Serial.print(data_in.light_readings[i]); Serial.print(" ");
        }
        
        Serial.println(sender_rssi);
      }
    }
  }
}

/*
 * Send a timestamp over serial port to the Raspberry Pi, so that images have the correct timestamp.
 */
void send_pi_time() {

  time_t timestamp = now();
  Serial.print("T");              //RPi looks for the "T" to know that it's a timestamp
  Serial.println(timestamp);      //RPi reads until end of line, so print the timestamp straight after the "T", followed by a new line
  
}

/*
 * Request the current time from the DS1307 real time clock.
 */
void fetch_time() {

  while(timeStatus()!= timeSet) {
    setSyncProvider(RTC.get);
  }
}





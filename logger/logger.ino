/*
 * The logger collects data from sensors. It sends this data to the controller when the controller requests it.
 */


/*
 * Additional libraries that we need - don't worry about these.
 */
#include <SPI.h>
#include <RFM69.h>
#include <LowPower.h>

/*******************************Things you might want to change*************************************************************/
/*
 * These variables relate to the radio. 
 */
#define LOGGER_ID       2                     //The ID of this radio on the network. This needs to be unique for each logger!
#define NETWORK_ID      202                   //The ID of the network that all related radios are on. This needs to be the same for all radios that talk to each other
#define CONTROLLER_ID   1                     //The ID of the gateway node, also known as the controller. Usually 1
#define FREQUENCY       RF69_433MHZ           //The frequency of the radio
#define ENCRYPTKEY      "as86HbM097Ljqd93"    //The 16-digit encyption key - must be the same on all radios that talk to each other!

#define NUM_PHOTODIODES 8                     //Number of photodiodes - they will be read from in order, 0-N

/***************************************************************************************************************************/

#define PHOTO_PIN  4                          //Pin that powers the photodiodes. They need very little current so it's safe to power them from a digital pin
#define MAX_PHOTODIODES 8                     //The maximum number of photodiodes we can have - 8

//Declare variables
RFM69 radio;                                  //Set up radio object
bool init_logger;                             //Tells the logger if it's initialized or not

//Create a data structure to hold all of the readings
typedef struct {
  float light_readings[MAX_PHOTODIODES];      //There are 8 analog pins, so maximum 8 photodiodes
} data_packet;

//Declare an empty structure that we can copy data into
data_packet sensor_readings;

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
   * 2 flashes - the number of photodiodes is too high (more than 8) or too low (less than 0)
   * 3 flashes - the network ID is too high (more than 253) or too low (less than 1)
   * 4 flashes - the logger ID is too high (more than 253) or too low (less than 2)
   * 5 flashes - the maximum number of photodiodes has been changed, and does not equal 8
   */
  if(NUM_PHOTODIODES < 0 || NUM_PHOTODIODES > 8) {                             //Check that the number of photodiodes is reasonable 
    while(1) {
        flash_led(2);
        LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_ON);
    }
  }
  else if(NETWORK_ID < 1 || NETWORK_ID > 254) {                                //Check that the network ID is reasonable
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

  //All loggers start off uninitialized
  init_logger = false;

  //Setup radio
  radio.initialize(FREQUENCY, LOGGER_ID, NETWORK_ID);   //Initialize the radio with the above settings
  radio.encrypt(ENCRYPTKEY);                            //Encrypt transmissions
}

/*
Loop runs over and over again forever
*/
void loop() {

  char radio_in[2];

  radio.receiveDone();                                  //Wake up the radio so that it's in receiving (TX) mode
  LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_ON);   //Sleep the Moteino 
  
  if(radio.receiveDone()) {
    if(radio.DATALEN == 1) {
      sscanf((const char*)radio.DATA, "%s", radio_in);  //Copy contents of the radio packet into radio_in

      //The packet contains a read command
      if((strcmp(radio_in, "R") == 0) ) {
        
        take_readings();                      //Read from all sensors
        send_packet();                        //Send the readings back to the controller
        reset_data();                         //Reset all of the data
      }

      //The packet contains a sleep command
      else if(radio_in == "S") {
        go_to_sleep();
      }
    
    }
  }

}

/*
 * Sleep for 5 minutes, with the radio off.
 */
void go_to_sleep() {

  byte i;

  //Sleep for 34*8 seconds, or 272 seconds
  for(i=0; i<34; i++) {
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_ON);
  }
  
}

/*
 * Function to take readings from all sensors. If functions are added for more sensors, this function can be used to call them all in turn.
 */
void take_readings() {

  read_light_sensors();
  
}
 
/*
This function reads from all of the light sensors in turn, and stores their readings in the struct.
*/
void read_light_sensors() {

  int i;
  unsigned long start_time;
  float total;
  float num_readings;
  float curr_reading;

  //Set PHOTO_PIN to allow power to the photodiodes
  digitalWrite(PHOTO_PIN, HIGH);

  //Cycle through the photodiodes. For each one, take readings over 1 second, and average them
  for(i=0; i<NUM_PHOTODIODES; i++) {
    start_time = millis();
    
    while(millis() - start_time < 1000) {
        total += analogRead(i);
    }
    
    curr_reading = total / num_readings;
    sensor_readings.light_readings[i] = curr_reading;
    total = 0.0;
  }

  //Set TRANSISTOR_PIN back to low to save power
  digitalWrite(PHOTO_PIN, LOW);
}

/*
Send the sensor_readings packet to the gateway. Loggers sleep for 120ms multiplied by their NODEID-2 (1 is the controller). So logger 2 sleeps for 0ms, logger 3 sleeps for 120ms,
logger 4 sleeps for 240ms, and so on. This gives every logger in the network a 120ms window in which to send.
*/
void send_packet() {

  byte i;
  int num_sleeps = 254 - LOGGER_ID;

  //Max number of sleeps is 252, or 30240ms
  for(i=2; i<LOGGER_ID; i++) {
    LowPower.powerDown(SLEEP_120MS, ADC_OFF, BOD_ON);
  }
  
  radio.send(CONTROLLER_ID, (const void*)(&sensor_readings), sizeof(sensor_readings));
  radio.sleep();

  //Sleep for up to another 30240 seconds. This is how long it would take for all of the loggers to send on a full network of 254 loggers
  for(i=0; i<num_sleeps; i++) {
    LowPower.powerDown(SLEEP_120MS, ADC_OFF, BOD_ON);
  }

  //Sleep until the next 5 minute interval. As the WDT is quite inaccurate (+/-10%), this sleeps for 240 seconds instead of 270
  for(i=0; i<30; i++) {
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_ON);
  }
}

/*
Reset all of the data in the packet to 0, ready for the next set of readings.
*/
void reset_data() {

  int i;

  for(i=0; i<NUM_PHOTODIODES; i++) {
    sensor_readings.light_readings[i] = 0.0;
  }
}

/*
 * Flash the onboard LED num_flashes amount of times. Different numbers of flashes indicate different problems.
 */
void flash_led(int num_flashes)
{
  byte i;

  for(i=0; i<num_flashes; i++) {
    digitalWrite(9, HIGH);
    delay(200);
    digitalWrite(9, LOW);
    delay(200);
  }
}

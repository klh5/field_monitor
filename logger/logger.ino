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

#define TRANSISTOR_PIN  4                     //Pin for enabling transistor that allows power to the photodiodes
#define MAX_PHOTODIODES 8                     //The maximum number of photodiodes we can have - 8

//Declare variables
RFM69 radio;          //Set up radio object
bool init_logger;     //Tells the logger if it's initialized or not

//Create a data structure to hold all of the readings
typedef struct {
  float light_readings[MAX_PHOTODIODES];            //There are 8 analog pins, so maximum 8 photodiodes
} data_packet;

//Declare an empty structure that we can copy data into
data_packet sensor_readings;

/*
Setup only runs once, when the logger starts up
*/
void setup() {

  //Set TRANSISTOR_PIN to output so we can use it to control power to the photodiodes
  pinMode(TRANSISTOR_PIN, OUTPUT);

  //Set TRANSISTOR_PIN to LOW (0V) so that the NPN transistor is not saturated
  digitalWrite(TRANSISTOR_PIN, LOW);

  //All loggers start off uninitialized
  init_logger = false;

  //Setup radio
  radio.initialize(FREQUENCY, LOGGER_ID, NETWORK_ID);   //Initialize the radio with the above settings
  radio.encrypt(ENCRYPTKEY);                            //Encrypt transmissions

  if((MAX_PHOTODIODES - NUM_PHOTODIODES) < 0 || NUM_PHOTODIODES > 8) {         //Check that the number of photodiodes is reasonable
    radio.sleep();                                                             //If not, sleep the radio and then the logger
    while(1) {
        LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_ON);
    }
  }
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

      //The packet contains an init command
      if((strcmp(radio_in, "I") == 0) ) {
        reset_data();
        init_logger = true;
        go_to_sleep();
      }

      //The packet contains a read command
      else if((strcmp(radio_in, "R") == 0) ) {

        //The logger is already initialized
        if(init_logger == true) {
          take_readings();
          send_packet();
          reset_data();
        }

        //The logger isn't initialized yet
        else {
          reset_data();
          init_logger = true;
          go_to_sleep();
        }
        
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

  //Set TRANSISTOR_PIN to high to saturate the transistor and allow voltage across the photodiodes
  digitalWrite(TRANSISTOR_PIN, HIGH);

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
  digitalWrite(TRANSISTOR_PIN, LOW);
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

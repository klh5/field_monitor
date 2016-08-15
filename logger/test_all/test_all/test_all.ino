
void setup() {

  byte i;
  
  //Set unused digital pins to OUTPUT and LOW, so that they don't float
  for(i=3; i<=9; i++) {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }

  Serial.begin(115200);

}

void loop() {

  read_light_sensors();
  delay(2000);
  Serial.println();

}

void read_light_sensors() {

  int i;
  unsigned long start_time;
  float total;
  float num_readings;
  float curr_reading;

  //Set PHOTO_PIN to allow power to the photodiodes
  digitalWrite(4, HIGH);

  //Cycle through the photodiodes. For each one, take readings over 1 second, and average them
  for(i=0; i<8; i++) {
    start_time = millis();
    
    while(millis() - start_time < 1000) {
        total = total + analogRead(i);                      //Add each new reading to the total
        num_readings++;                                     //Add one to the number of readings
    }
    
    curr_reading = total / num_readings;                    //Find the average by dividing the total by the number of readings
    Serial.println(curr_reading);
    total = 0.0;                                            //Reset the total and number of readings, ready for the next sensor
    num_readings = 0.0;
  }

  //Set TRANSISTOR_PIN back to low to save power
  digitalWrite(4, LOW);
}

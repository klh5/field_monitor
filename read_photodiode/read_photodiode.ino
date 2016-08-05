#define PAR_PIN A1

void setup() {

  //Set up serial comms
  Serial.begin(115200);
  
}

void loop() {

  float total = 0;
  int num_readings = 0;
  unsigned long start_time;
  float light_level;

  start_time = millis();

  //Take one second's worth of averages
  while(millis() - start_time < 1000) {
    int reading = analogRead(PAR_PIN);
    float voltage = reading * (3.3 / 1023);
    //Serial.println(voltage);
    total = total + reading;
    num_readings++;
  }
  
  light_level = total/num_readings;
  Serial.println(light_level);

  delay(20);

}

void setup() {

  Serial.begin(115200);

  Serial.println("ad jh tg ak pa");
  Serial.println("25 200 4.5 100 200 300 400 500 600 700 800 -34");
  
  delay(10000);
  
  unsigned long curr_time = millis();
  
  Serial.print("T");
  Serial.println(curr_time);
  
}

void loop() {

}


void setup() {

  Serial.begin(115200);
}

void loop() {

  unsigned long curr_time = millis();
  
  Serial.print("T");
  Serial.println(curr_time);

  delay(500);
}


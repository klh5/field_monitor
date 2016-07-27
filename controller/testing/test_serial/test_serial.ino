void setup() {

  Serial.begin(115200);

  delay(30000);
  
  char test_time[20] = "20-08-16_10:42:06";
  
  Serial.print("T");
  Serial.println(test_time);

  Serial.println("ad jh tg ak pa");
  Serial.println("25 200 4.5 100 200 300 400 500 600 700 800 -34");
  
  delay(10000);

  Serial.println("E");
  
}

void loop() {

}


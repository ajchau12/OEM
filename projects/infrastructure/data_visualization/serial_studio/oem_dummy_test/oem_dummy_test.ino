int gyroscope, random_2, random_3, random_4, random_5, altitude, pressure, random_8, random_9, random_10, random_11;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(19200);
}

void loop() {
  gyroscope = random(10);
  random_2 = random(10);
  random_3 = random(10);
  random_4 = random(10);
  random_5 = random(10);
  altitude = random(10);
  pressure = random(10);
  random_8 = random(10);
  random_9 = random(10);
  random_10 = random(10);
  random_11 = random(10);
  Serial.print("/*");
  Serial.print(gyroscope);
  Serial.print(",");
  Serial.print(random_2);
  Serial.print(",");
  Serial.print(random_3);
  Serial.print(",");
  Serial.print(random_4);
  Serial.print(",");
  Serial.print(random_5);
  Serial.print(",");
  Serial.print(altitude);
  Serial.print(",");
  Serial.print(pressure);
  Serial.print(",");
  Serial.print(random_8);
  Serial.print(",");
  Serial.print(random_9);
  Serial.print(",");
  Serial.print(random_10);
  Serial.print(",");
  Serial.print(random_11);
  Serial.print(",");
  Serial.println("*/");
  delay(100);
}

#include <Wire.h>

void setup() {
  Serial.begin(9600);
  Wire.begin();
  Serial.println("Scan du bus I2C...");
  int n = 0;
  for (byte adresse = 1; adresse < 127; adresse++) {
    Wire.beginTransmission(adresse);
    if (Wire.endTransmission() == 0) {   // 0 = un périphérique a répondu
      Serial.print("Peripherique trouve a 0x");
      Serial.println(adresse, HEX);
      n++;
    }
  }
  Serial.print(n);
  Serial.println(" peripherique(s)");
}

void loop() {}
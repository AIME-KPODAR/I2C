#include <Wire.h>
#include <RTClib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const byte EEPROM_ADDR = 0x57;
const unsigned int TAILLE_ENR = 6;      // 4 octets horodatage + 2 octets température
const unsigned int MAX_ENR = 680;       // (4096 - 2) / 6 pour une AT24C32
const unsigned long INTERVALLE_S = 60;  // un enregistrement par minute

RTC_DS3231 rtc;
Adafruit_SSD1306 ecran(128, 64, &Wire, -1);
unsigned long dernierAffichage = 0, dernierEnr = 0;

// --- EEPROM (adressage sur 2 octets) ---
void eepromEcrire(unsigned int adr, byte val) {
  Wire.beginTransmission(EEPROM_ADDR);
  Wire.write(adr >> 8);
  Wire.write(adr & 0xFF);
  Wire.write(val);
  Wire.endTransmission();
  delay(6);                              // durée d'écriture interne de la puce
}

byte eepromLire(unsigned int adr) {
  Wire.beginTransmission(EEPROM_ADDR);
  Wire.write(adr >> 8);
  Wire.write(adr & 0xFF);
  Wire.endTransmission();
  Wire.requestFrom(EEPROM_ADDR, (byte)1);
  return Wire.available() ? Wire.read() : 0xFF;
}

// Le compteur d'enregistrements est stocké aux adresses 0 et 1
unsigned int lireCompteur() {
  return eepromLire(0) | ((unsigned int)eepromLire(1) << 8);
}
void ecrireCompteur(unsigned int n) {
  eepromEcrire(0, n & 0xFF);
  eepromEcrire(1, n >> 8);
}

void enregistrer() {
  unsigned int n = lireCompteur();
  if (n >= MAX_ENR) return;              // mémoire pleine
  uint32_t ts = rtc.now().unixtime();
  int16_t temp = (int16_t)round(rtc.getTemperature() * 10);
  unsigned int adr = 2 + n * TAILLE_ENR;
  for (byte i = 0; i < 4; i++) eepromEcrire(adr + i, (ts >> (8 * i)) & 0xFF);
  eepromEcrire(adr + 4, temp & 0xFF);
  eepromEcrire(adr + 5, (temp >> 8) & 0xFF);
  ecrireCompteur(n + 1);
}

void exporter() {
  unsigned int n = lireCompteur();
  Serial.println(F("temps;temperature"));
  for (unsigned int i = 0; i < n; i++) {
    unsigned int adr = 2 + i * TAILLE_ENR;
    uint32_t ts = 0;
    for (byte k = 0; k < 4; k++) ts |= (uint32_t)eepromLire(adr + k) << (8 * k);
    int16_t temp = (int16_t)((uint16_t)eepromLire(adr + 5) << 8 | eepromLire(adr + 4));
    Serial.print(ts);
    Serial.print(';');
    Serial.println(temp / 10.0, 1);
  }
  Serial.println(F("FIN"));
}

void afficher() {
  DateTime t = rtc.now();
  char buf[20];
  snprintf(buf, sizeof(buf), "%02d/%02d/%04d %02d:%02d:%02d",
           t.day(), t.month(), t.year(), t.hour(), t.minute(), t.second());
  ecran.clearDisplay();
  ecran.setTextColor(SSD1306_WHITE);
  ecran.setTextSize(1);
  ecran.setCursor(0, 0);
  ecran.print(buf);
  ecran.setTextSize(2);
  ecran.setCursor(0, 20);
  ecran.print(rtc.getTemperature(), 1);
  ecran.print(F(" C"));
  ecran.setTextSize(1);
  ecran.setCursor(0, 52);
  ecran.print(lireCompteur());
  ecran.print(F(" mesures"));
  ecran.display();
}

void setup() {
  Serial.begin(9600);
  Wire.begin();
  if (!rtc.begin()) { Serial.println(F("DS3231 introuvable")); while (true); }
  if (rtc.lostPower()) rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  if (!ecran.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { Serial.println(F("OLED introuvable")); while (true); }
  if (lireCompteur() > MAX_ENR) ecrireCompteur(0);   // puce vierge (0xFFFF)
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'D') exporter();                          // D = dump CSV
    if (c == 'E') { ecrireCompteur(0); Serial.println(F("Efface")); }
  }
  if (millis() - dernierAffichage >= 1000) {
    dernierAffichage = millis();
    afficher();
  }
  if (millis() - dernierEnr >= INTERVALLE_S * 1000UL) {
    dernierEnr = millis();
    enregistrer();
  }
}
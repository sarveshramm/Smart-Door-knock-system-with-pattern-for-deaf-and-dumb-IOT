/* Knock Identification System + WiFi for NodeMCU (ESP12E)
   Components:
    - Vibration sensor DO -> D5
    - I2C 16x2 LCD -> SDA D2, SCL D1 (address 0x27)
    - Buzzer -> D7
    - LED -> D6
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>     // ✅ FINAL WORKING LIBRARY
#include <ESP8266WiFi.h>

// ======= WIFI CONFIG ==========
const char* WIFI_SSID = "Sarvesh";      
const char* WIFI_PASS = "244466666";    
// =============================

// ======= SENSOR CONFIG =======
#define VIB_PIN        D5
#define BUZZER_PIN     D7
#define LED_PIN        D6

const unsigned long KNOCK_WINDOW = 2500;
const unsigned long DEBOUNCE_MS  = 60;
const int LCD_ADDR = 0x27;

LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);   // ✅ FINAL OBJECT

volatile unsigned long lastKnockTime = 0;
volatile unsigned long lastDebounceTime = 0;
volatile int knockCount = 0;
bool useFallingEdge = false;

// ======= FUNCTIONS ===========
void beep(int times, int onMs=120, int offMs=80) {
  for (int i = 0; i < times; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(onMs);
    digitalWrite(BUZZER_PIN, LOW);
    delay(offMs);
  }
}

void showName(String line1, String line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

// =============== WIFI CONNECT =====================
void connectWiFi() {
  Serial.println("\nConnecting to WiFi...");
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Connecting WiFi");

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  int dot = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    lcd.setCursor(dot, 1);
    lcd.print(".");
    dot++;
    if (dot > 15) dot = 0;
  }

  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("WiFi Connected");
  lcd.setCursor(0,1);
  lcd.print(WiFi.localIP().toString());
  delay(1500);
}

// ===================================================

void setup() {
  Serial.begin(115200);
  pinMode(VIB_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0,0);
  lcd.print("KNOCK SYSTEM");
  lcd.setCursor(0,1);
  lcd.print("Booting...");
  delay(1500);

  connectWiFi();

  lcd.clear();
  lcd.print("Ready!");
}

void loop() {
  static int lastState = LOW;
  int state = digitalRead(VIB_PIN);

  static bool calibrated = false;
  static unsigned long calibStart = 0;
  static int highCount = 0, lowCount = 0;

  if (!calibrated) {
    if (calibStart == 0) calibStart = millis();

    if (state == HIGH) highCount++;
    else lowCount++;

    if (millis() - calibStart > 1200) {
      useFallingEdge = (highCount > lowCount);
      calibrated = true;

      Serial.print("Calibration done. useFallingEdge=");
      Serial.println(useFallingEdge ? "true" : "false");
    }
  }

  bool pulseDetected = false;

  if (!useFallingEdge) {
    if (lastState == LOW && state == HIGH) pulseDetected = true;
  } else {
    if (lastState == HIGH && state == LOW) pulseDetected = true;
  }

  if (pulseDetected) {
    unsigned long now = millis();

    if (now - lastDebounceTime > DEBOUNCE_MS) {
      lastDebounceTime = now;
      knockCount++;
      lastKnockTime = now;

      Serial.print("Knock #");
      Serial.println(knockCount);

      digitalWrite(LED_PIN, HIGH);
      digitalWrite(BUZZER_PIN, HIGH);
      delay(80);
      digitalWrite(BUZZER_PIN, LOW);
      digitalWrite(LED_PIN, LOW);
    }
  }

  lastState = state;

  if (knockCount > 0 && (millis() - lastKnockTime) > KNOCK_WINDOW) {
    Serial.print("Final knock count: ");
    Serial.println(knockCount);

    if (knockCount == 3) {
      showName("HUSBAND ARRIVED", "Welcome Home");
      beep(2);
    } 
    else if (knockCount == 4) {
      showName("SON ARRIVED", "Say Hi!");
      beep(3);
    } 
    else if (knockCount == 5) {
      showName("WANTED PERSON", "ALERT!");
      digitalWrite(LED_PIN, HIGH);
      beep(8, 80, 50);
      digitalWrite(LED_PIN, LOW);
    } 
    else {
      showName("UNKNOWN PATTERN", "Count: " + String(knockCount));
      beep(1);
    }

    delay(3000);
    lcd.clear();
    knockCount = 0;
  }

  delay(10);
}

#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// RS485 control pins
#define RE 5
#define DE 4

HardwareSerial mod(2);  // UART2 for RS485

// WiFi credentials
#define WIFI_SSID "your ssid"
#define WIFI_PASSWORD "your password"

// Firebase credentials
#define API_KEY "your API key"
#define DATABASE_URL "your database url"
#define USER_EMAIL "your email"
#define USER_PASSWORD "your password"

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
FirebaseJson json;

// Modbus commands
const byte reqN[] = {0x01, 0x03, 0x00, 0x1E, 0x00, 0x01, 0xE4, 0x0C};
const byte reqP[] = {0x01, 0x03, 0x00, 0x1F, 0x00, 0x01, 0xB5, 0xCC};
const byte reqK[] = {0x01, 0x03, 0x00, 0x20, 0x00, 0x01, 0x85, 0xC0};

void setup() {
  Serial.begin(115200);
  mod.begin(9600, SERIAL_8N1, 16, 17);  // RX=16, TX=17

  pinMode(RE, OUTPUT);
  pinMode(DE, OUTPUT);
  digitalWrite(RE, LOW);
  digitalWrite(DE, LOW);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.println("IP Address: " + WiFi.localIP().toString());

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  Firebase.reconnectWiFi(true);
  Firebase.begin(&config, &auth);
  Serial.println("Firebase initialized.");
}

void loop() {
  uint16_t nitrogen = readSingleRegister(reqN, "Nitrogen");
  delay(300);
  uint16_t phosphorus = readSingleRegister(reqP, "Phosphorus");
  delay(300);
  uint16_t potassium = readSingleRegister(reqK, "Potassium");
  delay(300);

  Serial.println("\n========== Final Readings ==========");
  Serial.print("Nitrogen   : "); Serial.print(nitrogen); Serial.println(" mg/kg");
  Serial.print("Phosphorus : "); Serial.print(phosphorus); Serial.println(" mg/kg");
  Serial.print("Potassium  : "); Serial.print(potassium); Serial.println(" mg/kg");
  Serial.println("====================================");

  json.clear();  
  json.set("Nitrogen", nitrogen);
  json.set("Phosphorus", phosphorus);
  json.set("Potassium", potassium);

  // Indicate sensor status
  if (nitrogen == 0 && phosphorus == 0 && potassium == 0) {
    json.set("status", "Sensor disconnected or dry soil");
    Serial.println("Sensor likely disconnected or dry.");
  } else {
    json.set("status", "OK");
  }

  // Push data to Firebase
  if (Firebase.ready()) {
    if (Firebase.RTDB.setJSON(&fbdo, "/NPK", &json)) {
      Serial.println("Data sent to Firebase.");
    } else {
      Serial.print("Firebase Error: ");
      Serial.println(fbdo.errorReason());
    }
  } else {
    Serial.println("Firebase not ready.");
  }

  delay(10000);  // Every 10 seconds
}

// Read a register from the sensor
uint16_t readSingleRegister(const byte* request, const char* label) {
  byte response[11];
  int i = 0;

  digitalWrite(RE, HIGH);
  digitalWrite(DE, HIGH);
  delay(2);
  mod.write(request, 8);
  mod.flush();

  digitalWrite(RE, LOW);
  digitalWrite(DE, LOW);
  delay(100);

  unsigned long start = millis();
  while (mod.available() == 0 && millis() - start < 500);

  while (mod.available() && i < 11) {
    response[i++] = mod.read();
  }

  Serial.print(label); Serial.print(" raw: ");
  for (int j = 0; j < i; j++) {
    Serial.printf("0x%02X ", response[j]);
  }
  Serial.println();

  if (i >= 5) {
    return (response[3] << 8) | response[4];
  } else {
    Serial.println(String(label) + " read failed.");
    return 0;
  }
}

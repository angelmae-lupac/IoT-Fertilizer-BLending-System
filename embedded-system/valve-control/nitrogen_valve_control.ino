#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// -------------------------
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

// Relay pin (active LOW)
const int relayN = 15;

// Dispense state
bool dispensing = false;
unsigned long dispenseStart = 0;
unsigned long dispenseDuration = 0;

// -------------------------
// HARD-CODED valve open times (milliseconds)
unsigned long getNitrogenTime(int level) {
  if (level == 1) return 51300; // LOW
  if (level == 2) return 32100; // MEDIUM
  if (level == 3) return 25700; // HIGH
  return 0;
}

// -------------------------
// Convert INTEGER N → category
int nitrogenLevel(int N) {
  if (N < 20) return 1;        // LOW
  else if (N <= 40) return 2; // MEDIUM
  else return 3;              // HIGH
}

// -------------------------
void setup() {
  Serial.begin(115200);

  pinMode(relayN, OUTPUT);
  digitalWrite(relayN, HIGH); // Relay OFF

  // WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  // Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.token_status_callback = tokenStatusCallback;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  while (!Firebase.ready()) {
    delay(300);
  }
  Serial.println("Firebase ready");
}

// -------------------------
void loop() {
  if (!Firebase.ready()) return;

  // Read Nitrogen sensor (INTEGER)
  int N = 0;
  if (Firebase.RTDB.getInt(&fbdo, "/NPK/Nitrogen")) {
    N = fbdo.intData();
  }

  int level = nitrogenLevel(N);

  // Read valve command from app
  int cmd = 0;
  if (Firebase.RTDB.getInt(&fbdo, "/valve/N")) {
    cmd = fbdo.intData();
  }

  // START dispensing
  if (cmd == 1 && !dispensing) {
    dispenseDuration = getNitrogenTime(level);

    digitalWrite(relayN, LOW); // Valve ON
    dispensing = true;
    dispenseStart = millis();

    Serial.println("Dispensing Nitrogen...");
  }

  // STOP dispensing
  if (dispensing && millis() - dispenseStart >= dispenseDuration) {
    digitalWrite(relayN, HIGH); // Valve OFF
    dispensing = false;

    // Reset valve command after dispensing
    Firebase.RTDB.setInt(&fbdo, "/valve/N", 0);

    Serial.println("Nitrogen dispense completed");
  }

  delay(200);
}

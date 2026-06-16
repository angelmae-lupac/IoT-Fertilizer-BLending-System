/**
 * Phosphorus Dispensing Controller
 */

#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseESP32.h>

#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

/* 1. WiFi credentials */
#define WIFI_SSID "your ssid"
#define WIFI_PASSWORD "your password"

/* 2. Firebase credentials */
#define API_KEY "your API key"
#define DATABASE_URL "your database url"
#define USER_EMAIL "your email"
#define USER_PASSWORD "your password"

/* 3. Hardware */
#define PHOSPHORUS_RELAY_PIN 16   // ACTIVE LOW valve

/* 4. Firebase objects */
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

/* 5. Timing */
unsigned long firebasePrevMillis = 0;
const unsigned long FIREBASE_INTERVAL = 500;

unsigned long valveStartMillis = 0;
unsigned long valveOpenTime = 0;

bool valveRunning = false;
bool firstBoot = true;

/* -------------------------
   Phosphorus valve open times (ms)
------------------------- */
unsigned long getPhosphorusTime(int level)
{
  if (level == 1) return 51300;  // Low
  if (level == 2) return 32100;  // Medium
  if (level == 3) return 12830;  // High
  return 0;
}

/* Convert P reading to category */
int phosphorusLevel(int P)
{
  if (P < 10) return 1;
  else if (P <= 25) return 2;
  else return 3;
}

void setup()
{
  Serial.begin(115200);

  pinMode(PHOSPHORUS_RELAY_PIN, OUTPUT);
  digitalWrite(PHOSPHORUS_RELAY_PIN, HIGH); // Valve OFF

  /* WiFi */
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected, IP: ");
  Serial.println(WiFi.localIP());

  /* Firebase setup */
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  config.token_status_callback = tokenStatusCallback;

  Firebase.reconnectNetwork(true);
  fbdo.setBSSLBufferSize(4096, 1024);

  Firebase.begin(&config, &auth);

  Serial.println("Firebase initialized");
}

void loop()
{
  /* REQUIRED */
  if (!Firebase.ready())
    return;

  /* Reset fertilized flag ONCE on boot */
  if (firstBoot)
  {
    Firebase.setInt(fbdo, F("/fertilized/valveP"), 0);
    firstBoot = false;
    Serial.println("Phosphorus valve armed");
  }

  /* ---- Firebase polling ---- */
  if (millis() - firebasePrevMillis >= FIREBASE_INTERVAL)
  {
    firebasePrevMillis = millis();

    int phosphorusValue = 0;
    int command = 0;
    int done = 0;

    /* Read Phosphorus sensor */
    if (Firebase.getInt(fbdo, F("/NPK/Phosphorus")))
      phosphorusValue = fbdo.to<int>();

    /* Read valve command */
    if (Firebase.getInt(fbdo, F("/valve/P")))
      command = fbdo.to<int>();

    /* Read fertilized status */
    if (Firebase.getInt(fbdo, F("/fertilized/valveP")))
      done = fbdo.to<int>();

    int level = phosphorusLevel(phosphorusValue);

    /* Start dispensing */
    if (command == 1 && !valveRunning && done == 0)
    {
      valveOpenTime = getPhosphorusTime(level);
      valveRunning = true;
      valveStartMillis = millis();

      digitalWrite(PHOSPHORUS_RELAY_PIN, LOW); // Valve ON

      Firebase.setInt(fbdo, F("/valve/P"), 0); // reset command

      Serial.println("Dispensing Phosphorus...");
    }
  }

  /* ---- Valve timing (NO delay) ---- */
  if (valveRunning && millis() - valveStartMillis >= valveOpenTime)
  {
    digitalWrite(PHOSPHORUS_RELAY_PIN, HIGH); // Valve OFF
    valveRunning = false;

    Firebase.setInt(fbdo, F("/fertilized/valveP"), 1);

    Serial.println("Phosphorus dispense completed");
  }
}

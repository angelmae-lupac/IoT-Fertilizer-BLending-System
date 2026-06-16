/**
 * Potassium Valve Control
 */

#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseESP32.h>

#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

/* 1. WiFi credentials */
#define WIFI_SSID "your SSID"
#define WIFI_PASSWORD "your wifi password"

/* 2. Firebase credentials */
#define API_KEY "your API Key"
#define DATABASE_URL "your database url"
#define USER_EMAIL "your email"
#define USER_PASSWORD "your password"

/* 3. Hardware */
#define POTASSIUM_RELAY_PIN 15   // ACTIVE LOW

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
   Fixed dispense times (ms)
   Flow rate = 67.8 mL/s
------------------------- */
unsigned long getPresetDispenseTime(int level)
{
  if (level == 1) return 39230; // LOW  (2.66 L)
  if (level == 2) return 29500; // MED  (2.00 L)
  if (level == 3) return 29500; // HIGH (2.00 L)
  return 0;
}

/* -------------------------
   Potassium category
------------------------- */
int determineLevel(int K)
{
  if (K < 20) return 1;
  else if (K <= 60) return 2;
  else return 3;
}

void setup()
{
  Serial.begin(115200);

  pinMode(POTASSIUM_RELAY_PIN, OUTPUT);
  digitalWrite(POTASSIUM_RELAY_PIN, HIGH); // Valve OFF

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
    Firebase.setInt(fbdo, F("/fertilized/valveK"), 0);
    firstBoot = false;
    Serial.println("Potassium valve armed");
  }

  /* ---- Firebase polling ---- */
  if (millis() - firebasePrevMillis >= FIREBASE_INTERVAL)
  {
    firebasePrevMillis = millis();

    int potassiumValue = 0;
    int command = 0;
    int done = 0;

    /* Read potassium sensor */
    if (Firebase.getInt(fbdo, F("/NPK/Potassium")))
      potassiumValue = fbdo.to<int>();

    /* Read valve command */
    if (Firebase.getInt(fbdo, F("/valve/K")))
      command = fbdo.to<int>();

    /* Read fertilized status */
    if (Firebase.getInt(fbdo, F("/fertilized/valveK")))
      done = fbdo.to<int>();

    int level = determineLevel(potassiumValue);

    /* Start dispensing */
    if (command == 1 && !valveRunning && done == 0)
    {
      valveOpenTime = getPresetDispenseTime(level);

      if (valveOpenTime > 0)
      {
        valveRunning = true;
        valveStartMillis = millis();

        digitalWrite(POTASSIUM_RELAY_PIN, LOW); // Valve ON
        Firebase.setInt(fbdo, F("/valve/K"), 0); // reset command

        Serial.println("Dispensing Potassium...");
      }
    }
  }

  /* ---- Valve timing (NO delay) ---- */
  if (valveRunning && millis() - valveStartMillis >= valveOpenTime)
  {
    digitalWrite(POTASSIUM_RELAY_PIN, HIGH); // Valve OFF
    valveRunning = false;

    Firebase.setInt(fbdo, F("/fertilized/valveK"), 1);

    Serial.println("Potassium dispense completed");
  }
}

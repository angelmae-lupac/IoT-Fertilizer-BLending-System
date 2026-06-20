#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// ----------- WiFi credentials -------------
#define WIFI_SSID "your ssid"
#define WIFI_PASSWORD "your password"

// ----------- Firebase credentials -------------
#define API_KEY "your API key"
#define DATABASE_URL "your database url"
#define USER_EMAIL "your email"
#define USER_PASSWORD "your password"

// ----------- Firebase objects -------------
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ----------- Pin definitions -------------
#define MOTOR_RELAY_PIN 16   // ESP GPIO 16 connected to transistor base

void setup() {
  Serial.begin(115200);

  pinMode(MOTOR_RELAY_PIN, OUTPUT);
  digitalWrite(MOTOR_RELAY_PIN, LOW);  // Motor OFF initially

  // Connect to Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n Wi-Fi connected!");

  // Firebase setup
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("Firebase ready.");
}

void loop() {
  // Read the value of "/mix/command"
  if (Firebase.RTDB.getInt(&fbdo, "/mix/command")) {
    int command = fbdo.intData();  // 0 = OFF, 1 = ON

    Serial.print("Firebase /mix/command = ");
    Serial.println(command);

    if (command == 1) {
      Serial.println("Motor ON for 3 seconds...");
      digitalWrite(MOTOR_RELAY_PIN, HIGH);  // Motor ON
      delay(3000);                           // Run motor for 3 seconds
      digitalWrite(MOTOR_RELAY_PIN, LOW);   // Motor OFF
      Serial.println("Motor OFF");

      // Reset Firebase command to 0
      Firebase.RTDB.setInt(&fbdo, "/mix/command", 0);
    }

  } else {
    Serial.print("Firebase read failed: ");
    Serial.println(fbdo.errorReason());
  }

  delay(500); // check twice per second
}

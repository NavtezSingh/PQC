#include <ESP8266WiFi.h>

const char* ssid = "Redmni 13 5G";
const char* password = "987654321";

// 👇 REPLACE THIS WITH ESP1'S IP (from its Serial Monitor)
const char* SERVER_IP = ""; 
const int SERVER_PORT = 8888;

WiFiClient client;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== ESP2: TCP CLIENT ===");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");

  Serial.println("Connecting to ESP1 server...");
  while (!client.connect(SERVER_IP, SERVER_PORT)) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\n✅ Connected to ESP1!\n");
}

void loop() {
  // Reconnect if lost
  if (!client.connected()) {
    Serial.println("⚠ Lost connection. Reconnecting...");
    while (!client.connect(SERVER_IP, SERVER_PORT)) {
      delay(1000);
    }
    Serial.println("✅ Reconnected!");
  }

  // Read from Serial (user input) and send to server (ESP1)
  if (Serial.available()) {
    String msg = Serial.readStringUntil('\n');
    msg.trim();
    if (msg.length() > 0) {
      client.println(msg);
      Serial.println("[Me] " + msg); // Echo locally
    }
  }

  // Read from server (ESP1) and print to Serial
  if (client.available()) {
    String msg = client.readStringUntil('\n');
    msg.trim();
    if (msg.length() > 0) {
      Serial.println("[ESP1] " + msg);
    }
  }

  delay(10);
}
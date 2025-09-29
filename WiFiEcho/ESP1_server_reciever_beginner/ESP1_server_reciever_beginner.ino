#include <ESP8266WiFi.h>

const char* ssid = "Redmi 13 5G";
const char* password = "987654321";

WiFiServer server(8888);
WiFiClient client;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== ESP1: TCP SERVER ===");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.begin();
  Serial.println("Server started on port 8888. Waiting for client...\n");
}

void loop() {
  // Accept new client if not connected
  if (!client || !client.connected()) {
    client = server.available();
    if (client) {
      Serial.println("✅ ESP2 connected!");
    }
  }

  // Read from Serial (user input) and send to client
  if (Serial.available()) {
    String msg = Serial.readStringUntil('\n');
    msg.trim();
    if (msg.length() > 0 && client && client.connected()) {
      client.println(msg);
      Serial.println("[Me] " + msg); // Echo locally
    }
  }

  // Read from client and print to Serial
  if (client && client.connected() && client.available()) {
    String msg = client.readStringUntil('\n');
    msg.trim();
    if (msg.length() > 0) {
      Serial.println("[ESP2] " + msg);
    }
  }

  delay(10); // Small delay to avoid watchdog
}
#include <ESP8266WiFi.h>
extern "C" {
  #include <espnow.h>
}

// 👇 REPLACE WITH THE OTHER ESP'S MAC ADDRESS (in hex)
// Example: if THIS is ESP1, put ESP2's MAC here
uint8_t peerMac[] = {0x80, 0x7D, 0x3A, 0x6E, 0xEF, 0x41}; // ← CHANGE THIS!

// Buffer to hold incoming messages
#define MAX_MSG_LEN 250
char incomingMsg[MAX_MSG_LEN + 1];

// Callback when data is received
void onDataRecv(uint8_t *mac, uint8_t *data, uint8_t len) {
  // Prevent buffer overflow
  if (len > MAX_MSG_LEN) {
    len = MAX_MSG_LEN;
  }

  // Copy and null-terminate
  memcpy(incomingMsg, data, len);
  incomingMsg[len] = '\0';

  // Print sender's MAC (optional)
  Serial.print("Received from: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X%s", mac[i], (i < 5) ? ":" : "");
  }
  Serial.print(" → ");
  Serial.println(incomingMsg);
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== ESP-NOW CHAT ===");
  Serial.println("Type a message and press Enter to send.\n");

  // Set Wi-Fi to STA mode (required for ESP-NOW)
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Initialize ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("❌ ESP-NOW init failed!");
    return;
  }

  // Register receive callback
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO); // Can send & receive
  esp_now_register_recv_cb(onDataRecv);

  // Add peer (the other ESP)
  esp_now_add_peer(peerMac, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  Serial.println("✅ ESP-NOW ready! Ready to chat.\n");
}

void loop() {
  // Check for user input on Serial
  if (Serial.available()) {
    String msg = Serial.readStringUntil('\n');
    msg.trim();

    if (msg.length() > 0) {
      if (msg.length() > MAX_MSG_LEN) {
        Serial.println("⚠ Message too long! Max 250 chars.");
        return;
      }

      // Send message to peer
      esp_now_send(peerMac, (uint8_t*)msg.c_str(), msg.length());
      Serial.println("[Me] " + msg);
    }
  }

  delay(10);
}
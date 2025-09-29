#include <ESP8266WiFi.h>
void setup() {
  Serial.begin(115200);
  Serial.println("\nMAC Address: " + WiFi.macAddress());
}
void loop() {}
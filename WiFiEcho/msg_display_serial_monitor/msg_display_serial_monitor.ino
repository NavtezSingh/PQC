/*
  WiFiEcho - Echo server

  released to public domain
*/

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <PolledTimeout.h>
#include <algorithm>  // std::min

#ifndef STASSID
#define STASSID "Redmi 13 5G"
#define STAPSK "987654321"
#endif

constexpr int port = 23;

WiFiServer server(port);
WiFiClient client;

constexpr size_t sizes[] = { 0, 512, 384, 256, 128, 64, 16, 8, 4 };
constexpr uint32_t breathMs = 200;
esp8266::polledTimeout::oneShotFastMs enoughMs(breathMs);
esp8266::polledTimeout::periodicFastMs test(2000);
int t = 1;  // test (1, 2 or 3, see below)
int s = 0;  // sizes[] index

void setup() {

  Serial.begin(115200);
  Serial.println(ESP.getFullVersion());

  WiFi.mode(WIFI_STA);
  WiFi.begin(STASSID, STAPSK);
  Serial.print("\nConnecting to ");
  Serial.println(STASSID);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }
  Serial.println();
  Serial.print("connected, address=");
  Serial.println(WiFi.localIP());

  server.begin();

  MDNS.begin("echo23");

  Serial.printf("Ready!\n"
                "- Use 'telnet/nc echo23.local %d' to try echo\n\n"
                "- Use 'python3 echo-client.py' bandwidth meter to compare transfer APIs\n\n"
                "  and try typing 1, 1, 1, 2, 2, 2, 3, 3, 3 on console during transfers\n\n",
                port);
}



void loop() {
  MDNS.update();

  static uint32_t tot = 0;
  static uint32_t cnt = 0;
  if (test && cnt) {
    Serial.printf("measured-block-size=%u min-free-stack=%u", tot / cnt, ESP.getFreeContStack());
    if (t == 2 && sizes[s]) { Serial.printf(" (blocks: at most %d bytes)", sizes[s]); }
    if (t == 3 && sizes[s]) { Serial.printf(" (blocks: exactly %d bytes)", sizes[s]); }
    if (t == 3 && !sizes[s]) { Serial.printf(" (blocks: any size)"); }
    Serial.printf("\n");
    tot = cnt = 0;
  }

  if (server.hasClient()) {
    if (client && client.connected()) {
      client.stop(); // disconnect old client if any
    }
    client = server.accept();
    Serial.println("New client connected");
  }

  if (Serial.available()) {
    s = (s + 1) % (sizeof(sizes) / sizeof(sizes[0]));
    switch (Serial.read()) {
      case '1':
        if (t != 1) s = 0;
        t = 1;
        Serial.println("Mode 1: byte-by-byte (visible in Serial)");
        break;
      case '2':
        if (t != 2) s = 1;
        t = 2;
        Serial.printf("Mode 2: buffered (max %d bytes)\n", sizes[s]);
        break;
      case '3':
        if (t != 3) s = 0;
        t = 3;
        Serial.printf("Mode 3: sendAvailable (no Serial log!)\n");
        break;
      case '4':
        t = 4;
        Serial.println("Mode 4: sendAll (no Serial log, blocks until disconnect)");
        break;
    }
    ESP.resetFreeContStack();
  }

  enoughMs.reset(breathMs);

  if (!client || !client.connected()) {
    return;
  }

  if (t == 1) {
    while (client.available() && client.availableForWrite() && !enoughMs) {
      uint8_t c = client.read();
      client.write(c);
      Serial.write(c); // 👈 Visible in Serial Monitor!
      Serial.write('\t'); 
      if (c == '\n') Serial.flush(); // optional: flush on newline
      cnt++;
      tot++;
    }
  }

  else if (t == 2) {
    while (client.available() && client.availableForWrite() && !enoughMs) {
      size_t avail = static_cast<size_t>(client.available());
      size_t availWrite = static_cast<size_t>(client.availableForWrite());
      size_t maxTo = std::min(std::min(avail, availWrite), sizes[s]);
      uint8_t buf[maxTo];
      size_t got = client.read(buf, maxTo);
      if (got > 0) {
        client.write(buf, got);
        Serial.write(buf, got);  // 👈 Visible!
        Serial.write('\t');
        if (buf[got - 1] == '\n') Serial.flush();
        tot += got;
        cnt++;
      }
    }
  }

  else if (t == 3) {
    // No easy way to log — use Mode 1 or 2 for visibility
    if (sizes[s]) {
      tot += client.sendSize(&client, sizes[s]);
    } else {
      tot += client.sendAvailable(&client);
    }
    cnt++;
    // Handle send report if needed...
  }

  else if (t == 4) {
    tot += client.sendAll(&client);
    cnt++;
  }
}
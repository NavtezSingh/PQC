#include <Arduino.h>
#include <uECC.h>
#include "esp_system.h"  // for esp_random()

int my_rng(uint8_t *dest, unsigned size) {
  for (unsigned i = 0; i < size; ++i) {
    dest[i] = esp_random() & 0xFF;
  }
  return 1;
}

#define TOTAL_RUNS 1000
volatile int run = 0;
uint32_t total_sign = 0;
uint32_t total_verify = 0;

uint8_t private_key[32];
uint8_t public_key[64];
uint8_t hash[32] = {0};
uint8_t signature[64];

enum State { IDLE, SIGNING, VERIFYING, DONE } state = IDLE;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== ECDSA Benchmark (secp256r1) ===");

  uECC_set_rng(my_rng);

  if (!uECC_make_key(public_key, private_key, uECC_secp256r1())) {
    Serial.println("Key gen failed!");
    return;
  }

  const char* msg = "Hello micro-ecc";
  size_t len = strlen(msg);
  if (len > 32) len = 32;
  memcpy(hash, msg, len);

  state = SIGNING;
  run = 0;
  total_sign = 0;
  total_verify = 0;
}

void loop() {
  if (state == DONE) return;

  switch (state) {
    case SIGNING:
      if (run < TOTAL_RUNS) {
        uint32_t start = micros();
        if (!uECC_sign(private_key, hash, 32, signature, uECC_secp256r1())) {
          Serial.println("Sign failed!");
          state = DONE;
          return;
        }
        total_sign += (micros() - start);
        run++;

        if (run % 50 == 0)
          Serial.printf("[SIGN] Completed %d / %d\n", run, TOTAL_RUNS);

        yield();
      } else {
        Serial.printf("Signing done (%d runs)\n", TOTAL_RUNS);
        state = VERIFYING;
        run = 0;
      }
      break;

    case VERIFYING:
      if (run < TOTAL_RUNS) {
        uint32_t start = micros();
        if (!uECC_verify(public_key, hash, 32, signature, uECC_secp256r1())) {
          Serial.println("Verify failed!");
          state = DONE;
          return;
        }
        total_verify += (micros() - start);
        run++;

        if (run % 50 == 0)
          Serial.printf("[VERIFY] Completed %d / %d\n", run, TOTAL_RUNS);

        yield();
      } else {
        float avg_sign = (float)total_sign / TOTAL_RUNS;
        float avg_verify = (float)total_verify / TOTAL_RUNS;

        Serial.printf("\n Results (%d runs):\n", TOTAL_RUNS);
        Serial.printf("Avg Sign Time:   %.2f µs\n", avg_sign);
        Serial.printf("Avg Verify Time: %.2f µs\n", avg_verify);
        state = DONE;
      }
      break;

    default:
      break;
  }
}

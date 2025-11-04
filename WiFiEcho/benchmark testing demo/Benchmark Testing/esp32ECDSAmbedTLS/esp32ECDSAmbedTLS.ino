#include <Arduino.h>
#include "mbedtls/ecdsa.h"
#include "mbedtls/ecp.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/sha256.h"

#define TOTAL_RUNS 1000

volatile int runCount = 0;
uint32_t total_sign = 0;
uint32_t total_verify = 0;

enum State { IDLE, SIGNING, VERIFYING, DONE } state = IDLE;

mbedtls_ecdsa_context ecdsa;
mbedtls_ctr_drbg_context ctr_drbg;
mbedtls_entropy_context entropy;

uint8_t hash[32] = {0};
uint8_t sig_buf[128];
size_t sig_len = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== ECDSA Benchmark (secp256r1) using mbedTLS ===");

  mbedtls_ecdsa_init(&ecdsa);
  mbedtls_ctr_drbg_init(&ctr_drbg);
  mbedtls_entropy_init(&entropy);

  const char *pers = "esp32_ecdsa_bench";
  if (mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                            (const unsigned char *)pers, strlen(pers)) != 0) {
    Serial.println("ctr_drbg_seed failed");
    while (1) delay(1000);
  }

  // Generate ECDSA keypair (secp256r1)
  Serial.println("Generating ECDSA keypair...");
  int ret = mbedtls_ecdsa_genkey(&ecdsa, MBEDTLS_ECP_DP_SECP256R1,
                                 mbedtls_ctr_drbg_random, &ctr_drbg);
  if (ret != 0) {
    Serial.printf("Key generation failed: -0x%04X\n", -ret);
    while (1) delay(1000);
  }
  Serial.println("Keypair ready.");

  // Hash message using new mbedTLS SHA256 API
  const char *msg = "Hello mbedTLS ECDSA";
  mbedtls_sha256_context sha_ctx;
  mbedtls_sha256_init(&sha_ctx);
  mbedtls_sha256_starts(&sha_ctx, 0);  // 0 = SHA-256 (not 224)
  mbedtls_sha256_update(&sha_ctx, (const unsigned char *)msg, strlen(msg));
  mbedtls_sha256_finish(&sha_ctx, hash);
  mbedtls_sha256_free(&sha_ctx);

  state = SIGNING;
  runCount = 0;
  total_sign = 0;
  total_verify = 0;
}

void loop() {
  if (state == DONE) return;

  switch (state) {
    case SIGNING:
      if (runCount < TOTAL_RUNS) {
        uint32_t t0 = micros();
        sig_len = sizeof(sig_buf);
        // Corrected function call signature for ESP-IDF v5.x
        int ret = mbedtls_ecdsa_write_signature(&ecdsa, MBEDTLS_MD_SHA256,
                                                hash, sizeof(hash),
                                                sig_buf, sizeof(sig_buf),
                                                &sig_len,
                                                mbedtls_ctr_drbg_random, &ctr_drbg);
        if (ret != 0) {
          Serial.printf("Sign failed: -0x%04X\n", -ret);
          state = DONE;
          return;
        }
        total_sign += (micros() - t0);
        runCount++;

        if (runCount % 50 == 0)
          Serial.printf("[SIGN] Completed %d / %d\n", runCount, TOTAL_RUNS);

        yield();
      } else {
        Serial.printf("Signing done (%d runs)\n", TOTAL_RUNS);
        state = VERIFYING;
        runCount = 0;
      }
      break;

    case VERIFYING:
      if (runCount < TOTAL_RUNS) {
        uint32_t t0 = micros();
        int ret = mbedtls_ecdsa_read_signature(&ecdsa,
                                               hash, sizeof(hash),
                                               sig_buf, sig_len);
        if (ret != 0) {
          Serial.printf("Verify failed: -0x%04X\n", -ret);
          state = DONE;
          return;
        }
        total_verify += (micros() - t0);
        runCount++;

        if (runCount % 50 == 0)
          Serial.printf("[VERIFY] Completed %d / %d\n", runCount, TOTAL_RUNS);

        yield();
      } else {
        float avg_sign = (float)total_sign / TOTAL_RUNS;
        float avg_verify = (float)total_verify / TOTAL_RUNS;

        Serial.printf("\nResults (%d runs):\n", TOTAL_RUNS);
        Serial.printf("Avg Sign Time:   %.2f µs\n", avg_sign);
        Serial.printf("Avg Verify Time: %.2f µs\n", avg_verify);
        Serial.printf("DER Signature length (last run): %u bytes\n", (unsigned)sig_len);

        state = DONE;

        mbedtls_ecdsa_free(&ecdsa);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        mbedtls_entropy_free(&entropy);
      }
      break;

    default:
      break;
  }
}

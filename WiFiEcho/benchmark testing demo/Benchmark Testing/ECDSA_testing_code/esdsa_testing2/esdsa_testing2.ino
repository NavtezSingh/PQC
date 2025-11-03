#include <Arduino.h>
#include <bearssl/bearssl.h> // Include BearSSL for AES

// Define the curve for micro-ecc
#define uECC_SUPPORTS_secp256r1 1
#include <uECC.h>

extern "C" {
  #include "osapi.h"
}

// --- RNG Setup ---
int my_rng(uint8_t *dest, unsigned size) {
  for (unsigned i = 0; i < size; ++i) {
    dest[i] = os_random() & 0xFF;
  }
  return 1;
}

// --- Benchmark Configuration ---
#define TOTAL_RUNS 100 // Number of iterations for each benchmark
#define AES_BLOCK_SIZE 16 // Keep as a constant integer
#define AES_KEY_SIZE 16 // For AES-128

// --- Global Variables ---
// ECDSA Keys and Data
uint8_t private_key_sign[32];
uint8_t public_key_sign[64];
uint8_t private_key_echd[32]; // Separate key pair for ECDH as per paper
uint8_t public_key_echd[64];
uint8_t public_key_echd_peer[64]; // Peer's public key (simulated by generating another key pair)
uint8_t hash[32] = {0};
uint8_t signature[64];

// ECDH Shared Secret
uint8_t shared_secret[32];

// AES Data (using BearSSL)
uint8_t aes_key[AES_KEY_SIZE] = {0};
uint8_t aes_iv[AES_BLOCK_SIZE] = {0};
uint8_t plaintext[AES_BLOCK_SIZE] = {0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x41, 0x45, 0x53, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // "Hello AES!" + padding
uint8_t ciphertext[AES_BLOCK_SIZE];
// BearSSL encryptor context
br_aes_small_cbcenc_keys aes_enc_ctx; // Use the smaller implementation suitable for microcontrollers

// Timing Variables
volatile int run = 0;
uint32_t total_time = 0;

enum Operation { IDLE, SIGNING, VERIFYING, ECDH, AES_ENC, DONE } current_op = IDLE;
const char* op_names[] = {"", "SIGNING", "VERIFYING", "ECDH", "AES_ENC"};

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== Cryptographic Benchmark (secp256r1 / AES-128) - BearSSL ===");
  Serial.printf("Running %d iterations per operation...\n", TOTAL_RUNS);

  uECC_set_rng(my_rng);

  // --- Generate Key Pairs ---
  Serial.println("Generating key pairs...");
  if (!uECC_make_key(public_key_sign, private_key_sign, uECC_secp256r1())) {
    Serial.println("ECDSA Key gen failed!");
    return;
  }
  if (!uECC_make_key(public_key_echd_peer, private_key_echd, uECC_secp256r1())) { // Generate peer key pair
    Serial.println("ECDH Peer Key gen failed!");
    return;
  }
  // Copy the *public* key of the peer into our local peer_public variable for the ECDH calc
  memcpy(public_key_echd, public_key_echd_peer, 64);
  Serial.println("Key generation complete.");

  // Prepare hash for ECDSA
  const char* msg = "Hello IoT Security";
  size_t len = strlen(msg);
  if (len > 32) len = 32;
  memcpy(hash, msg, len);

  // Prepare AES key (simplified: using first 16 bytes of one of the private keys)
  memcpy(aes_key, private_key_sign, AES_KEY_SIZE);

  // Initialize BearSSL AES context *once* before the benchmark loop
  br_aes_small_cbcenc_init(&aes_enc_ctx, aes_key, AES_KEY_SIZE * 8); // Key size in bits

  // Start the benchmark sequence
  current_op = SIGNING;
  Serial.println("Starting benchmark sequence...");
  run = 0;
  total_time = 0;
}

int n =0;

void loop() {
  if (current_op == DONE && n<2) {
    Serial.println("\n=== Benchmark Complete ===");
    n++;
    return; // Stop after all operations
  }

  switch (current_op) {
    case SIGNING:
      if (run < TOTAL_RUNS) {
        uint32_t start = micros();
        if (!uECC_sign(private_key_sign, hash, 32, signature, uECC_secp256r1())) {
          Serial.println("ECDSA Sign failed!");
          current_op = DONE;
          return;
        }
        total_time += (micros() - start);
        run++;

        if (run % 100 == 0) { // Print progress every 100 runs
          Serial.printf("[%s] Completed %d / %d\n", op_names[current_op], run, TOTAL_RUNS);
        }
        yield();
      } else {
        float avg_time = (float)total_time / TOTAL_RUNS;
        Serial.printf("[%s] Avg Time: %.2f µs (%.6f ms)\n", op_names[current_op], avg_time, avg_time / 1000.0);
        // Move to next operation
        current_op = VERIFYING;
        run = 0;
        total_time = 0;
      }
      break;

    case VERIFYING:
      // Ensure signature is valid before benchmarking verification
      if (run == 0) {
         uECC_sign(private_key_sign, hash, 32, signature, uECC_secp256r1()); // Generate one valid signature first
      }
      if (run < TOTAL_RUNS) {
        uint32_t start = micros();
        if (!uECC_verify(public_key_sign, hash, 32, signature, uECC_secp256r1())) {
          Serial.println("ECDSA Verify failed!");
          current_op = DONE;
          return;
        }
        total_time += (micros() - start);
        run++;

        if (run % 100 == 0) {
          Serial.printf("[%s] Completed %d / %d\n", op_names[current_op], run, TOTAL_RUNS);
        }
        yield();
      } else {
        float avg_time = (float)total_time / TOTAL_RUNS;
        Serial.printf("[%s] Avg Time: %.2f µs (%.6f ms)\n", op_names[current_op], avg_time, avg_time / 1000.0);
        current_op = ECDH;
        run = 0;
        total_time = 0;
      }
      break;

    case ECDH:
      if (run < TOTAL_RUNS) {
        uint32_t start = micros();
        if (!uECC_shared_secret(public_key_echd, private_key_echd, shared_secret, uECC_secp256r1())) { // Using peer's public key and local private key
          Serial.println("ECDH Shared Secret failed!");
          current_op = DONE;
          return;
        }
        total_time += (micros() - start);
        run++;

        if (run % 100 == 0) {
          Serial.printf("[%s] Completed %d / %d\n", op_names[current_op], run, TOTAL_RUNS);
        }
        yield();
      } else {
        float avg_time = (float)total_time / TOTAL_RUNS;
        Serial.printf("[%s] Avg Time: %.2f µs (%.6f ms)\n", op_names[current_op], avg_time, avg_time / 1000.0);
        current_op = AES_ENC;
        run = 0;
        total_time = 0;
      }
      break;

    case AES_ENC:
      if (run < TOTAL_RUNS) {
        // Use the initialized context. IV should ideally change, but using static here for simplicity in benchmarking.
        uint32_t start = micros();
        // BearSSL CBC encrypt function. Note: it encrypts in-place, so ciphertext buffer is used.
        // Copy plaintext to the ciphertext buffer first for this run.
        memcpy(ciphertext, plaintext, AES_BLOCK_SIZE);
        // Corrected order: ctx, iv, data, len
        br_aes_small_cbcenc_run(&aes_enc_ctx, aes_iv, ciphertext, (size_t)AES_BLOCK_SIZE); // Encrypts the ciphertext buffer in place
        total_time += (micros() - start);
        run++;

        if (run % 100 == 0) {
          Serial.printf("[%s] Completed %d / %d\n", op_names[current_op], run, TOTAL_RUNS);
        }
        yield();
      } else {
        float avg_time = (float)total_time / TOTAL_RUNS;
        Serial.printf("[%s] Avg Time: %.2f µs (%.6f ms)\n", op_names[current_op], avg_time, avg_time / 1000.0);
        current_op = DONE; // Finished all operations
      }
      break;

    default:
      break;
  }
}
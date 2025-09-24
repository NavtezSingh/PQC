#include "uECC.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

// Simple XOR "encryption" using shared secret
void xor_encrypt_decrypt(uint8_t *data, size_t len, uint8_t *key, size_t keylen) {
    for (size_t i = 0; i < len; i++) {
        data[i] ^= key[i % keylen];
    }
}

// Helper to print hex
void print_hex(const char *label, const uint8_t *data, size_t len) {
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02X", data[i]);
    }
    printf("\n");
}

int main() {
    // List available curves
    const struct uECC_Curve_t *curves[5];
    const char *curve_names[5];
    int num_curves = 0;

#if uECC_SUPPORTS_secp160r1
    curves[num_curves] = uECC_secp160r1();
    curve_names[num_curves++] = "secp160r1";
#endif
#if uECC_SUPPORTS_secp192r1
    curves[num_curves] = uECC_secp192r1();
    curve_names[num_curves++] = "secp192r1";
#endif
#if uECC_SUPPORTS_secp224r1
    curves[num_curves] = uECC_secp224r1();
    curve_names[num_curves++] = "secp224r1";
#endif
#if uECC_SUPPORTS_secp256r1
    curves[num_curves] = uECC_secp256r1();
    curve_names[num_curves++] = "secp256r1";
#endif
#if uECC_SUPPORTS_secp256k1
    curves[num_curves] = uECC_secp256k1();
    curve_names[num_curves++] = "secp256k1";
#endif

    // Show choices
    printf("Available curves:\n");
    for (int i = 0; i < num_curves; i++) {
        printf("%d: %s\n", i + 1, curve_names[i]);
    }

    int choice;
    printf("Choose a curve (1-%d): ", num_curves);
    scanf("%d", &choice);
    if (choice < 1 || choice > num_curves) {
        printf("Invalid choice\n");
        return 1;
    }
    const struct uECC_Curve_t *curve = curves[choice - 1];
    printf("Using curve: %s\n", curve_names[choice - 1]);

    // Input message
    char message[256];
    printf("Enter a message: ");
    getchar(); // consume newline
    fgets(message, sizeof(message), stdin);
    size_t msg_len = strlen(message);
    if (message[msg_len - 1] == '\n') message[msg_len - 1] = '\0';

    printf("Message: %s\n", message);

    // Buffers for keys
    int priv_size = uECC_curve_private_key_size(curve);
    int pub_size = uECC_curve_public_key_size(curve);

    uint8_t private[32];
    uint8_t public[64];

    // Generate keypair
    if (!uECC_make_key(public, private, curve)) {
        printf("uECC_make_key() failed\n");
        return 1;
    }

    print_hex("Private key", private, priv_size);
    print_hex("Public key", public, pub_size);

    // Sign message
    uint8_t sig[64];
    if (!uECC_sign(private, (uint8_t *)message, msg_len, sig, curve)) {
        printf("uECC_sign() failed\n");
        return 1;
    }
    print_hex("Signature", sig, uECC_curve_private_key_size(curve) * 2);

    // Verify signature
    if (uECC_verify(public, (uint8_t *)message, msg_len, sig, curve)) {
        printf("Signature verification SUCCESS ✅\n");
    } else {
        printf("Signature verification FAILED ❌\n");
    }

    // ---- Demonstrate simple encryption/decryption with ECDH ----
    uint8_t private2[32], public2[64];
    uint8_t secret1[32], secret2[32];

    // Generate second keypair
    uECC_make_key(public2, private2, curve);

    // Derive shared secret
    uECC_shared_secret(public2, private, secret1, curve);
    uECC_shared_secret(public, private2, secret2, curve);

    if (memcmp(secret1, secret2, priv_size) != 0) {
        printf("Shared secret mismatch!\n");
        return 1;
    }
    print_hex("Shared secret", secret1, priv_size);

    // Encrypt message with shared secret
    uint8_t encrypted[256];
    strcpy((char *)encrypted, message);
    xor_encrypt_decrypt(encrypted, msg_len, secret1, priv_size);
    print_hex("Encrypted", encrypted, msg_len);

    // Decrypt back
    xor_encrypt_decrypt(encrypted, msg_len, secret2, priv_size);
    printf("Decrypted: %s\n", encrypted);

    return 0;
}

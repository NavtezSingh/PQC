/* Copyright 2014, Kenneth MacKay. Licensed under the BSD 2-clause license. */

#include "uECC.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

static long elapsed_ns(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) * 1000000000L + (end.tv_nsec - start.tv_nsec);
}

int main() {
    int i, c;
    uint8_t private[32] = {0};
    uint8_t public[64] = {0};
    uint8_t hash[32] = {0};
    uint8_t sig[64] = {0};

    const struct uECC_Curve_t * curves[5];
    const char * curve_names[5];
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

    printf("Benchmarking 256 iterations per curve:\n");

    for (c = 0; c < num_curves; ++c) {
        long total_keygen = 0, total_sign = 0, total_verify = 0;

        printf("\nCurve: %s\n", curve_names[c]);
        for (i = 0; i < 256; ++i) {
            struct timespec start, end;

            // --- Key Generation ---
            clock_gettime(CLOCK_MONOTONIC, &start);
            if (!uECC_make_key(public, private, curves[c])) {
                printf("uECC_make_key() failed\n");
                return 1;
            }
            clock_gettime(CLOCK_MONOTONIC, &end);
            total_keygen += elapsed_ns(start, end);

            memcpy(hash, public, sizeof(hash));

            // --- Sign ---
            clock_gettime(CLOCK_MONOTONIC, &start);
            if (!uECC_sign(private, hash, sizeof(hash), sig, curves[c])) {
                printf("uECC_sign() failed\n");
                return 1;
            }
            clock_gettime(CLOCK_MONOTONIC, &end);
            total_sign += elapsed_ns(start, end);

            // --- Verify ---
            clock_gettime(CLOCK_MONOTONIC, &start);
            if (!uECC_verify(public, hash, sizeof(hash), sig, curves[c])) {
                printf("uECC_verify() failed\n");
                return 1;
            }
            clock_gettime(CLOCK_MONOTONIC, &end);
            total_verify += elapsed_ns(start, end);
        }

        printf("Average timings over 256 runs:\n");
        printf("  Keygen : %.2f ms\n", total_keygen / 256.0 / 1e6);
        printf("  Sign   : %.2f ms\n", total_sign   / 256.0 / 1e6);
        printf("  Verify : %.2f ms\n", total_verify / 256.0 / 1e6);
    }

    return 0;
}

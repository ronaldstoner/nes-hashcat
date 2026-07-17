// SHA-256 (FIPS 180-4). No allocation; writes a 32-byte digest.
#ifndef HASHCAT_NES_SHA256_H
#define HASHCAT_NES_SHA256_H

#include <stdint.h>

void sha256(const uint8_t *data, uint16_t length, uint8_t out[32]);
void sha256_block1(const uint8_t blk[64], uint8_t out[32]);  // brute-force fast path

#endif

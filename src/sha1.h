#ifndef HASHCAT_NES_SHA1_H
#define HASHCAT_NES_SHA1_H
#include <stdint.h>
void sha1(const uint8_t *data, uint16_t length, uint8_t out[20]);
void sha1_block1(const uint8_t blk[64], uint8_t out[20]);  // brute-force fast path
#endif

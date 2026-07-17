#ifndef HASHCAT_NES_MD_H
#define HASHCAT_NES_MD_H
#include <stdint.h>
void md5(const uint8_t *data, uint16_t length, uint8_t out[16]);
void md4(const uint8_t *data, uint16_t length, uint8_t out[16]);  // backs NTLM
// Single pre-padded 64-byte block (brute-force fast path).
void md5_block1(const uint8_t blk[64], uint8_t out[16]);
void md4_block1(const uint8_t blk[64], uint8_t out[16]);
#endif

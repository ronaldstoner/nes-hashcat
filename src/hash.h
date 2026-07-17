// Unified interface over the real hashcat modes this ROM ships.
#ifndef HASHCAT_NES_HASH_H
#define HASHCAT_NES_HASH_H

#include <stdint.h>

// Menu order. Values are indices, not hashcat mode numbers (see algo_mode()).
#define ALGO_MD5    0
#define ALGO_SHA1   1
#define ALGO_SHA256 2
#define ALGO_NTLM   3
#define ALGO_CRC32  4
#define ALGO_COUNT  5

// Digest is stored in a fixed 32-byte buffer, zero-padded past the algorithm's
// real length so a full 32-byte compare works for every mode.
void hash_compute(uint8_t algo, const uint8_t *data, uint16_t len, uint8_t out[32]);

uint8_t hash_digest_len(uint8_t algo);   // significant bytes
uint8_t digest_equal(const uint8_t *a, const uint8_t *b);
uint8_t digest_equal_n(const uint8_t *a, const uint8_t *b, uint8_t n);

// Fast path: hash_begin() pre-builds the block for (algo,len) and returns the
// digest length; hash_next() hashes a candidate (falls back for >1 block/CRC32).
uint8_t hash_begin(uint8_t algo, uint8_t len);
void hash_next(const uint8_t *cand, uint8_t out[32]);
const char *algo_name(uint8_t algo);     // "MD5", "SHA-256", ...
uint16_t algo_mode(uint8_t algo);        // hashcat -m number

#endif

// SHA-1 (FIPS 180-4). Big-endian, 64-byte blocks. hashcat -m 100.
#include "sha1.h"

static inline uint32_t rotl(uint32_t v, uint8_t n) {
  return (v << n) | (v >> (32 - n));
}

static void sha1_block(uint32_t s[5], const uint8_t *block) {
  uint32_t w[80];
  uint8_t i;
  for (i = 0; i < 16; ++i) {
    uint8_t j = i * 4;
    w[i] = ((uint32_t)block[j] << 24) | ((uint32_t)block[j + 1] << 16) |
           ((uint32_t)block[j + 2] << 8) | (uint32_t)block[j + 3];
  }
  for (i = 16; i < 80; ++i)
    w[i] = rotl(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);

  uint32_t a = s[0], b = s[1], c = s[2], d = s[3], e = s[4];
  for (i = 0; i < 80; ++i) {
    uint32_t f, k;
    if (i < 20) { f = (b & c) | (~b & d); k = 0x5a827999UL; }
    else if (i < 40) { f = b ^ c ^ d; k = 0x6ed9eba1UL; }
    else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8f1bbcdcUL; }
    else { f = b ^ c ^ d; k = 0xca62c1d6UL; }
    uint32_t t = rotl(a, 5) + f + e + k + w[i];
    e = d; d = c; c = rotl(b, 30); b = a; a = t;
  }
  s[0] += a; s[1] += b; s[2] += c; s[3] += d; s[4] += e;
}

void sha1(const uint8_t *data, uint16_t length, uint8_t out[20]) {
  uint32_t s[5] = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0};
  uint16_t full = length >> 6, blk;
  for (blk = 0; blk < full; ++blk) sha1_block(s, data + blk * 64);

  uint16_t rem = length - full * 64;
  uint8_t tail[128];
  uint8_t i;
  for (i = 0; i < 128; ++i) tail[i] = 0;
  for (i = 0; i < rem; ++i) tail[i] = data[full * 64 + i];
  tail[rem] = 0x80;
  uint8_t blocks = (rem >= 56) ? 2 : 1;
  uint32_t bits = (uint32_t)length * 8;
  uint8_t off = blocks * 64 - 8;  // big-endian length; high 4 bytes stay zero
  tail[off + 4] = (uint8_t)(bits >> 24);
  tail[off + 5] = (uint8_t)(bits >> 16);
  tail[off + 6] = (uint8_t)(bits >> 8);
  tail[off + 7] = (uint8_t)bits;
  for (i = 0; i < blocks; ++i) sha1_block(s, tail + i * 64);

  for (i = 0; i < 5; ++i) {
    out[i * 4] = (uint8_t)(s[i] >> 24);
    out[i * 4 + 1] = (uint8_t)(s[i] >> 16);
    out[i * 4 + 2] = (uint8_t)(s[i] >> 8);
    out[i * 4 + 3] = (uint8_t)s[i];
  }
}

// Single pre-padded 64-byte block (brute-force fast path).
void sha1_block1(const uint8_t blk[64], uint8_t out[20]) {
  uint32_t s[5] = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0};
  sha1_block(s, blk);
  for (uint8_t i = 0; i < 5; ++i) {
    out[i * 4] = (uint8_t)(s[i] >> 24);
    out[i * 4 + 1] = (uint8_t)(s[i] >> 16);
    out[i * 4 + 2] = (uint8_t)(s[i] >> 8);
    out[i * 4 + 3] = (uint8_t)s[i];
  }
}

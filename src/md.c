// MD4 and MD5 (RFC 1320 / RFC 1321). Little-endian, 64-byte blocks. Both loops
// rotate the a/b/c/d register roles each step; 48 and 64 are multiples of 4, so
// the naming realigns before the final add-back.
#include "md.h"

static inline uint32_t rotl(uint32_t v, uint8_t n) {
  return (v << n) | (v >> (32 - n));
}

// Load 16 little-endian words from a 64-byte block.
static void load_le(const uint8_t *block, uint32_t m[16]) {
  uint8_t i;
  for (i = 0; i < 16; ++i) {
    uint8_t j = i * 4;
    m[i] = (uint32_t)block[j] | ((uint32_t)block[j + 1] << 8) |
           ((uint32_t)block[j + 2] << 16) | ((uint32_t)block[j + 3] << 24);
  }
}

// ---- MD5 -------------------------------------------------------------------

static const uint32_t MD5_T[64] = {
    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
    0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
    0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
    0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
    0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
    0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
    0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
    0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391};

static const uint8_t MD5_S[64] = {
    7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
    5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
    4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
    6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};

static void md5_block(uint32_t s[4], const uint8_t *block) {
  uint32_t m[16];
  load_le(block, m);
  uint32_t a = s[0], b = s[1], c = s[2], d = s[3];
  uint8_t i;
  for (i = 0; i < 64; ++i) {
    uint32_t f;
    uint8_t g;
    if (i < 16) { f = (b & c) | (~b & d); g = i; }
    else if (i < 32) { f = (d & b) | (~d & c); g = (5 * i + 1) & 15; }
    else if (i < 48) { f = b ^ c ^ d; g = (3 * i + 5) & 15; }
    else { f = c ^ (b | ~d); g = (7 * i) & 15; }
    f = f + a + MD5_T[i] + m[g];
    a = d; d = c; c = b;
    b = b + rotl(f, MD5_S[i]);
  }
  s[0] += a; s[1] += b; s[2] += c; s[3] += d;
}

// ---- MD4 -------------------------------------------------------------------

static const uint8_t MD4_K2[16] = {0, 4, 8, 12, 1, 5, 9, 13, 2, 6, 10, 14, 3, 7, 11, 15};
static const uint8_t MD4_K3[16] = {0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15};
static const uint8_t MD4_S1[4] = {3, 7, 11, 19};
static const uint8_t MD4_S2[4] = {3, 5, 9, 13};
static const uint8_t MD4_S3[4] = {3, 9, 11, 15};

static void md4_block(uint32_t s[4], const uint8_t *block) {
  uint32_t m[16];
  load_le(block, m);
  uint32_t a = s[0], b = s[1], c = s[2], d = s[3];
  uint8_t i;
  for (i = 0; i < 16; ++i) {
    uint32_t f = (b & c) | (~b & d);
    f = f + a + m[i];
    a = d; d = c; c = b;
    b = rotl(f, MD4_S1[i & 3]);
  }
  for (i = 0; i < 16; ++i) {
    uint32_t f = (b & c) | (b & d) | (c & d);
    f = f + a + m[MD4_K2[i]] + 0x5a827999UL;
    a = d; d = c; c = b;
    b = rotl(f, MD4_S2[i & 3]);
  }
  for (i = 0; i < 16; ++i) {
    uint32_t f = b ^ c ^ d;
    f = f + a + m[MD4_K3[i]] + 0x6ed9eba1UL;
    a = d; d = c; c = b;
    b = rotl(f, MD4_S3[i & 3]);
  }
  s[0] += a; s[1] += b; s[2] += c; s[3] += d;
}

// ---- shared driver ---------------------------------------------------------

// Both algorithms share MD padding: 0x80, zero fill, then a 64-bit
// little-endian bit length. is_md5 selects the compression function.
static void md_hash(uint8_t is_md5, const uint8_t *data, uint16_t length, uint8_t out[16]) {
  uint32_t s[4] = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476};
  uint16_t full = length >> 6, blk;
  for (blk = 0; blk < full; ++blk) {
    if (is_md5) md5_block(s, data + blk * 64);
    else md4_block(s, data + blk * 64);
  }

  uint16_t rem = length - full * 64;
  uint8_t tail[128];
  uint8_t i;
  for (i = 0; i < 128; ++i) tail[i] = 0;
  for (i = 0; i < rem; ++i) tail[i] = data[full * 64 + i];
  tail[rem] = 0x80;
  uint8_t blocks = (rem >= 56) ? 2 : 1;
  uint32_t bits = (uint32_t)length * 8;
  uint8_t off = blocks * 64 - 8;  // low 4 length bytes (LE); high 4 stay zero
  tail[off] = (uint8_t)bits;
  tail[off + 1] = (uint8_t)(bits >> 8);
  tail[off + 2] = (uint8_t)(bits >> 16);
  tail[off + 3] = (uint8_t)(bits >> 24);
  for (i = 0; i < blocks; ++i) {
    if (is_md5) md5_block(s, tail + i * 64);
    else md4_block(s, tail + i * 64);
  }

  for (i = 0; i < 4; ++i) {
    out[i * 4] = (uint8_t)s[i];
    out[i * 4 + 1] = (uint8_t)(s[i] >> 8);
    out[i * 4 + 2] = (uint8_t)(s[i] >> 16);
    out[i * 4 + 3] = (uint8_t)(s[i] >> 24);
  }
}

void md5(const uint8_t *data, uint16_t length, uint8_t out[16]) { md_hash(1, data, length, out); }
void md4(const uint8_t *data, uint16_t length, uint8_t out[16]) { md_hash(0, data, length, out); }

// Fast path: hash one pre-padded 64-byte block (caller mutates only the
// candidate bytes), so per-hash cost is init + one compression + finalize.
static void md_finalize(uint32_t s[4], uint8_t out[16]) {
  for (uint8_t i = 0; i < 4; ++i) {
    out[i * 4] = (uint8_t)s[i];
    out[i * 4 + 1] = (uint8_t)(s[i] >> 8);
    out[i * 4 + 2] = (uint8_t)(s[i] >> 16);
    out[i * 4 + 3] = (uint8_t)(s[i] >> 24);
  }
}

void md5_block1(const uint8_t blk[64], uint8_t out[16]) {
  uint32_t s[4] = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476};
  md5_block(s, blk);
  md_finalize(s, out);
}

void md4_block1(const uint8_t blk[64], uint8_t out[16]) {
  uint32_t s[4] = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476};
  md4_block(s, blk);
  md_finalize(s, out);
}

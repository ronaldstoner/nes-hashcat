// SHA-256 (FIPS 180-4). Candidates are short, so a uint16_t length and a
// fixed 128-byte tail buffer cover every case.
#include "sha256.h"

static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

static inline uint32_t rotr(uint32_t v, uint8_t n) {
  return (v >> n) | (v << (32 - n));
}

static void process_block(uint32_t state[8], const uint8_t block[64]) {
  uint32_t w[64];
  uint8_t i;

  for (i = 0; i < 16; ++i) {
    uint8_t j = i * 4;
    w[i] = ((uint32_t)block[j] << 24) | ((uint32_t)block[j + 1] << 16) |
           ((uint32_t)block[j + 2] << 8) | (uint32_t)block[j + 3];
  }
  for (i = 16; i < 64; ++i) {
    uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
    uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
    w[i] = w[i - 16] + s0 + w[i - 7] + s1;
  }

  uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
  uint32_t e = state[4], f = state[5], g = state[6], h = state[7];

  for (i = 0; i < 64; ++i) {
    uint32_t s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
    uint32_t ch = (e & f) ^ (~e & g);
    uint32_t temp1 = h + s1 + ch + K[i] + w[i];
    uint32_t s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
    uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
    uint32_t temp2 = s0 + maj;
    h = g; g = f; f = e; e = d + temp1;
    d = c; c = b; b = a; a = temp1 + temp2;
  }

  state[0] += a; state[1] += b; state[2] += c; state[3] += d;
  state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

void sha256(const uint8_t *data, uint16_t length, uint8_t out[32]) {
  uint32_t state[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                       0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
  uint8_t i;

  uint16_t full_blocks = length >> 6;
  uint16_t b;
  for (b = 0; b < full_blocks; ++b)
    process_block(state, data + (b * 64));

  uint16_t remaining = length - (full_blocks * 64);
  uint8_t tail[128];
  for (i = 0; i < 128; ++i) tail[i] = 0;
  for (i = 0; i < remaining; ++i) tail[i] = data[full_blocks * 64 + i];
  tail[remaining] = 0x80;

  uint8_t tail_blocks = (remaining >= 56) ? 2 : 1;
  uint32_t bit_length = (uint32_t)length * 8;  // < 2^32; message is short
  uint8_t off = (tail_blocks * 64) - 8;
  // High 4 length bytes are always 0 for our inputs; write the low 4.
  tail[off + 4] = (uint8_t)(bit_length >> 24);
  tail[off + 5] = (uint8_t)(bit_length >> 16);
  tail[off + 6] = (uint8_t)(bit_length >> 8);
  tail[off + 7] = (uint8_t)(bit_length);

  for (i = 0; i < tail_blocks; ++i)
    process_block(state, tail + (i * 64));

  for (i = 0; i < 8; ++i) {
    out[i * 4] = (uint8_t)(state[i] >> 24);
    out[i * 4 + 1] = (uint8_t)(state[i] >> 16);
    out[i * 4 + 2] = (uint8_t)(state[i] >> 8);
    out[i * 4 + 3] = (uint8_t)(state[i]);
  }
}

// Single pre-padded 64-byte block (brute-force fast path).
void sha256_block1(const uint8_t blk[64], uint8_t out[32]) {
  uint32_t state[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                       0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
  process_block(state, blk);
  for (uint8_t i = 0; i < 8; ++i) {
    out[i * 4] = (uint8_t)(state[i] >> 24);
    out[i * 4 + 1] = (uint8_t)(state[i] >> 16);
    out[i * 4 + 2] = (uint8_t)(state[i] >> 8);
    out[i * 4 + 3] = (uint8_t)(state[i]);
  }
}

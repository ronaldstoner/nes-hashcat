#include "hash.h"
#include "md.h"
#include "sha1.h"
#include "sha256.h"
#include "crc32.h"

// Shared (never used at once): NTLM's UTF-16LE buffer and the fast path's
// prepared 64-byte block. Overlapping them saves 64 bytes of RAM.
static uint8_t scratch[208];
#define utf16 scratch
#define g_blk scratch

void hash_compute(uint8_t algo, const uint8_t *data, uint16_t len, uint8_t out[32]) {
  uint8_t i;
  for (i = 0; i < 32; ++i) out[i] = 0;

  switch (algo) {
    case ALGO_MD5:
      md5(data, len, out);
      break;
    case ALGO_SHA1:
      sha1(data, len, out);
      break;
    case ALGO_SHA256:
      sha256(data, len, out);
      break;
    case ALGO_NTLM: {
      uint16_t n = 0, k;
      for (k = 0; k < len && n < 206; ++k) {
        utf16[n++] = data[k];
        utf16[n++] = 0;
      }
      md4(utf16, n, out);
      break;
    }
    case ALGO_CRC32: {
      uint32_t c = crc32(data, len);
      out[0] = (uint8_t)(c >> 24);
      out[1] = (uint8_t)(c >> 16);
      out[2] = (uint8_t)(c >> 8);
      out[3] = (uint8_t)c;
      break;
    }
  }
}

uint8_t hash_digest_len(uint8_t algo) {
  switch (algo) {
    case ALGO_MD5:
    case ALGO_NTLM: return 16;
    case ALGO_SHA1: return 20;
    case ALGO_SHA256: return 32;
    case ALGO_CRC32: return 4;
  }
  return 32;
}

uint8_t digest_equal(const uint8_t *a, const uint8_t *b) {
  return digest_equal_n(a, b, 32);
}

uint8_t digest_equal_n(const uint8_t *a, const uint8_t *b, uint8_t n) {
  uint8_t i;
  for (i = 0; i < n; ++i)
    if (a[i] != b[i]) return 0;
  return 1;
}

// Fast path: pre-pad the block once per crack, then per hash overwrite only the
// candidate bytes (0x80 marker, zero fill and length field stay put).
static uint8_t g_algo, g_len, g_fast, g_dl;

uint8_t hash_begin(uint8_t algo, uint8_t len) {
  g_algo = algo;
  g_len = len;
  g_dl = hash_digest_len(algo);

  uint16_t msglen = (algo == ALGO_NTLM) ? (uint16_t)len * 2 : len;
  // Single-block only (message + 9 bytes of padding must fit in 64).
  g_fast = (algo != ALGO_CRC32 && msglen <= 55);
  if (g_fast) {
    uint8_t i;
    for (i = 0; i < 64; ++i) g_blk[i] = 0;
    g_blk[msglen] = 0x80;
    uint32_t bits = (uint32_t)msglen * 8;
    if (algo == ALGO_MD5 || algo == ALGO_NTLM) {   // MD family: LE length @56
      g_blk[56] = (uint8_t)bits;
      g_blk[57] = (uint8_t)(bits >> 8);
      g_blk[58] = (uint8_t)(bits >> 16);
      g_blk[59] = (uint8_t)(bits >> 24);
    } else {                                        // SHA family: BE length @60
      g_blk[60] = (uint8_t)(bits >> 24);
      g_blk[61] = (uint8_t)(bits >> 16);
      g_blk[62] = (uint8_t)(bits >> 8);
      g_blk[63] = (uint8_t)bits;
    }
  }
  return g_dl;
}

void hash_next(const uint8_t *cand, uint8_t out[32]) {
  if (!g_fast) {
    hash_compute(g_algo, cand, g_len, out);
    return;
  }
  uint8_t i;
  if (g_algo == ALGO_NTLM) {
    for (i = 0; i < g_len; ++i) g_blk[i * 2] = cand[i];  // odd bytes stay 0
    md4_block1(g_blk, out);
  } else {
    for (i = 0; i < g_len; ++i) g_blk[i] = cand[i];
    switch (g_algo) {
      case ALGO_MD5:    md5_block1(g_blk, out); break;
      case ALGO_SHA1:   sha1_block1(g_blk, out); break;
      default:          sha256_block1(g_blk, out); break;  // ALGO_SHA256
    }
  }
}

const char *algo_name(uint8_t algo) {
  switch (algo) {
    case ALGO_MD5: return "MD5";
    case ALGO_SHA1: return "SHA1";
    case ALGO_SHA256: return "SHA-256";
    case ALGO_NTLM: return "NTLM";
    case ALGO_CRC32: return "CRC32";
  }
  return "?";
}

uint16_t algo_mode(uint8_t algo) {
  switch (algo) {
    case ALGO_MD5: return 0;
    case ALGO_SHA1: return 100;
    case ALGO_SHA256: return 1400;
    case ALGO_NTLM: return 1000;
    case ALGO_CRC32: return 11500;
  }
  return 0;
}

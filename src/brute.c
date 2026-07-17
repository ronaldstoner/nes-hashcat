#include "brute.h"
#include <neslib.h>

uint8_t brute_cand[BRUTE_MAX];
uint8_t brute_len;

void brute_reset(uint8_t len) {
  if (len > BRUTE_MAX) len = BRUTE_MAX;
  brute_len = len;
  for (uint8_t i = 0; i < len; ++i) brute_cand[i] = 'a';
}

uint8_t brute_next(void) {
  uint8_t i = brute_len;
  while (i) {
    --i;
    if (brute_cand[i] < 'z') {
      ++brute_cand[i];
      return 1;
    }
    brute_cand[i] = 'a';   // carry into the next column
  }
  return 0;                // wrapped past "zzz..." -> space exhausted
}

void brute_secret(uint8_t *dst) {
  for (uint8_t i = 0; i < brute_len; ++i)
    dst[i] = 'a' + (rand8() % 26);
}

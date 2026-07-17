// Brute-force candidate generator: an odometer over 'a'..'z' of a chosen length.
#ifndef HASHCAT_NES_BRUTE_H
#define HASHCAT_NES_BRUTE_H

#include <stdint.h>

#define BRUTE_MAX 100

extern uint8_t brute_cand[BRUTE_MAX];  // current candidate, chars 'a'..'z'
extern uint8_t brute_len;

// Reset the odometer to "aaaa..." of the given length.
void brute_reset(uint8_t len);

// Advance to the next candidate. Returns 0 when the whole space has wrapped
// (exhausted); realistically only reachable for tiny lengths.
uint8_t brute_next(void);

// Fill dst with a random secret of brute_len ('a'..'z') using neslib's RNG.
void brute_secret(uint8_t *dst);

#endif

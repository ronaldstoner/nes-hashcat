// Tiny frame-driven APU sound engine (no music data / famitone needed).
#ifndef HASHCAT_NES_SOUND_H
#define HASHCAT_NES_SOUND_H

#include <stdint.h>

void snd_init(void);
void snd_update(void);        // call exactly once per frame

void snd_move(void);          // menu cursor blip
void snd_select(void);        // menu confirm
void snd_victory(void);       // triumphant fanfare on crack
void snd_tick(void);          // faint "working" crunch during cracking
void snd_silence(void);       // cut all voices

#endif

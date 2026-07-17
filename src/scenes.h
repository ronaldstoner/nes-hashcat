#ifndef HASHCAT_NES_SCENES_H
#define HASHCAT_NES_SCENES_H
#include <stdint.h>

// Scene ids, also mirrored into g_scene for headless RAM-watching.
#define SC_TITLE  0
#define SC_MENU   1
#define SC_CRACK  2

extern volatile uint8_t g_scene;   // current scene
extern volatile uint8_t g_found;   // set to 1 the instant a hash matches

void game_run(void);               // never returns

#endif

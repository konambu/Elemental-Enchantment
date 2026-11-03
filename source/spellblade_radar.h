#ifndef SPELLBLADE_RADAR_H
#define SPELLBLADE_RADAR_H
#include "spellblade_affinity.h"
/* RGBA, top-to-bottom, tightly packed. No engine or font dependency. */
int sb_radar_draw(unsigned char* pixels,int width,int height,const SBAffinity* state);
#endif

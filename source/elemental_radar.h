#ifndef ELEMENTAL_RADAR_H
#define ELEMENTAL_RADAR_H
#include "elemental_affinity.h"
/* RGBA, top-to-bottom, tightly packed. No engine or font dependency. */
int ee_radar_draw(unsigned char *pixels, int width, int height, const EEAffinity *state);
#endif

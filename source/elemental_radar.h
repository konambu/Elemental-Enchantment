#ifndef ELEMENTAL_RADAR_H
#define ELEMENTAL_RADAR_H
#include "elemental_affinity.h"
/** @file elemental_radar.h
 * @brief Draw the aptitude chart into a tightly packed MyGUI BGRA canvas.
 */
/** @brief Draw top-to-bottom pixels without using game or font APIs. */
int ee_radar_draw(unsigned char *pixels, int width, int height, const EEAffinity *state);
#endif

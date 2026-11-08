#include "elemental_radar.h"

/* The sentinels guard the maximum supported canvas against out-of-bounds writes. */
__declspec(dllexport) int ee_run_radar_tests(void) {
    static unsigned char canvas[512 * 512 * 4 + 2];
    EEAffinity affinity;
    unsigned int i;
    if (!ee_affinity_create(&affinity, 123, 1))
        return 1;
    canvas[0] = 0x5a;
    canvas[sizeof(canvas) - 1] = 0xa5;
    if (!ee_radar_draw(canvas + 1, 512, 512, &affinity) || canvas[0] != 0x5a ||
        canvas[sizeof(canvas) - 1] != 0xa5)
        return 2;
    for (i = 0; i < 5; ++i)
        affinity.rates[i] = 0;
    canvas[32 * 32 * 4 + 1] = 0xa5;
    if (!ee_radar_draw(canvas + 1, 32, 32, &affinity) ||
        canvas[0] != 0x5a || canvas[32 * 32 * 4 + 1] != 0xa5 ||
        ee_radar_draw(canvas + 1, 31, 32, &affinity) ||
        ee_radar_draw(canvas + 1, 513, 512, &affinity))
        return 3;
    affinity.rates[0] = 101;
    if (ee_radar_draw(canvas + 1, 32, 32, &affinity))
        return 4;
    return 0;
}

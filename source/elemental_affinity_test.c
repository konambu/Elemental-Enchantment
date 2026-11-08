#include "elemental_affinity.h"

__declspec(dllexport) int ee_run_affinity_threshold_tests(void) {
    static const unsigned char expected_ring[5] = {0, 2, 4, 3, 1};
    EEAffinity affinity;
    unsigned int level, expected, i;
    if (!ee_affinity_create(&affinity, 123, 1))
        return 1;
    for (i = 0; i < EE_AFFINITY_COUNT; ++i)
        if (ee_affinity_ring_order[i] != expected_ring[i])
            return 2;
    for (level = 0; level <= 100; ++level) {
        expected = 4u * (20u * level + level * (level - 1u));
        if (ee_affinity_xp_for_level(level) != expected)
            return 3;
        affinity.xp[0] = (unsigned short)expected;
        if (ee_affinity_level(&affinity, 0) != level)
            return 4;
        if (level > 0) {
            affinity.xp[0] = (unsigned short)(expected - 1u);
            if (ee_affinity_level(&affinity, 0) != level - 1u)
                return 5;
        }
    }
    if (ee_affinity_xp_for_level(101) != EE_AFFINITY_MAX_XP)
        return 6;
    return 0;
}

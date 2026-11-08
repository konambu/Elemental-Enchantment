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
        affinity.total_xp = (unsigned short)expected;
        if (ee_affinity_level(&affinity) != level)
            return 4;
        if (level > 0) {
            affinity.total_xp = (unsigned short)(expected - 1u);
            if (ee_affinity_level(&affinity) != level - 1u)
                return 5;
        }
    }
    if (ee_affinity_xp_for_level(101) != EE_AFFINITY_MAX_XP)
        return 6;
    return 0;
}

__declspec(dllexport) int ee_run_affinity_migration_tests(void) {
    EEAffinity decoded, roundtrip;
    static const unsigned char expected_rates[5] = {100, 80, 80, 60, 60};
    unsigned int legacy[5];
    int record[3], encoded[3], encoded_again[3];
    unsigned int i, highest_slot, version;

    /* Versions 1 and 2 stored aptitude rates without XP. */
    for (version = 1; version <= 2; ++version) {
        record[0] = (int)((version << 8) | 3u);
        record[1] = (int)(100u | (80u << 7) | (60u << 14) | (60u << 21));
        record[2] = 80;
        if (!ee_affinity_decode(&decoded, record) || decoded.selected != 3 || decoded.total_xp != 0)
            return 7;
        for (i = 0; i < 5; ++i)
            if (decoded.rates[i] != expected_rates[i])
                return 8;
    }

    /* Version 3 stored five XP values; the highest becomes shared XP. */
    for (highest_slot = 0; highest_slot < 5; ++highest_slot) {
        for (i = 0; i < 5; ++i)
            legacy[i] = i == highest_slot ? 46000u : 100u + i;
        record[0] = (int)((legacy[0] << 16) | (3u << 8) | 1u);
        record[1] = (int)(legacy[1] | (legacy[2] << 16));
        record[2] = (int)(legacy[3] | (legacy[4] << 16));
        if (!ee_affinity_decode(&decoded, record) || decoded.selected != 1 ||
            decoded.total_xp != 46000u || decoded.secondary != 0)
            return 1;
    }
    decoded.total_xp = 1234;
    record[1] = 60000;
    if (ee_affinity_decode(&decoded, record) || decoded.total_xp != 1234)
        return 2;

    /* Current version must retain its three-integer wire format. */
    if (!ee_affinity_create(&decoded, 123, 1))
        return 3;
    for (i = 0; i < 5; ++i)
        decoded.rates[i] = expected_rates[i];
    decoded.total_xp = (unsigned short)ee_affinity_xp_for_level(50);
    decoded.secondary = 2;
    decoded.xp_fraction = 37;
    if (!ee_affinity_encode(&decoded, encoded) || !ee_affinity_decode(&roundtrip, encoded) ||
        !ee_affinity_encode(&roundtrip, encoded_again))
        return 4;
    if (encoded[0] != 0x20401 || encoded[1] != 13800 || encoded[2] != 37)
        return 9;
    for (i = 0; i < 3; ++i)
        if (encoded[i] != encoded_again[i])
            return 5;
    return 0;
}

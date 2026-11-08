#ifndef EE_BOSS_PROFILES_H
#define EE_BOSS_PROFILES_H

/** @file elemental_boss_profiles.h
 * @brief Static affinity presets assigned to selected named
 * Kenshi bosses.
 */

/** A boss's stable mod ID and starting elemental profile. */
typedef struct EEBossProfile {
    const char *id;
    int primary;
    int secondary;
    int level;
} EEBossProfile;

/** @brief Return the number of configured boss profiles. */
unsigned int ee_boss_profile_count(void);
/** @brief Get a profile by zero-based table index, or NULL if out of range. */
const EEBossProfile *ee_boss_profile_at(unsigned int index);

#endif

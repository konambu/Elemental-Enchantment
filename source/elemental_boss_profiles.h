#ifndef EE_BOSS_PROFILES_H
#define EE_BOSS_PROFILES_H

typedef struct EEBossProfile {
    const char* id;
    int primary;
    int secondary;
    int level;
} EEBossProfile;

unsigned int ee_boss_profile_count(void);
const EEBossProfile* ee_boss_profile_at(unsigned int index);

#endif

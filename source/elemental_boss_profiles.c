#include "elemental_boss_profiles.h"

static const EEBossProfile profiles[] = {
    {"17225-Dialogue.mod", 1, 2, 80},
    {"59164-Dialogue.mod", 2, 1, 65},
    {"59165-Dialogue.mod", 1, 2, 65},
    {"51751-Dialogue.mod", 2, 5, 60},
    {"43159-Dialogue.mod", 4, 3, 80},
    {"59030-Dialogue.mod", 3, 2, 90},
    {"1533858-rebirth.mod", 2, 5, 100},
    {"51333-rebirth.mod", 5, 4, 90},
    {"50350-rebirth.mod", 3, 5, 70},                  /* Moll */
    {"56727-Dialogue.mod", 4, 1, 70},                 /* Flying Bull */
    {"56730-Dialogue.mod", 1, 3, 65},                 /* Ghost */
    {"98164-Dialogue.mod", 4, 3, 65},                 /* Tora */
    {"62981-Dialogue.mod", 5, 2, 85},                 /* Savant */
    {"95756-__december17.mod", 1, 4, 80},             /* Valamon */
    {"62827-rebirth.mod", 4, 5, 80},                  /* Crab Queen */
    {"62807-rebirth.mod", 2, 5, 85},                  /* Skeleton Bandits' Elder */
    {"96407-Dialogue.mod", 4, 5, 75},                 /* King Gurgler */
    {"48447-Dialogue.mod", 4, 1, 65},                 /* Big Grim */
    {"56576-Dialogue.mod", 2, 4, 85},                 /* Eyegore */
    {"56365-Dialogue.mod", 2, 5, 70},                 /* Longen */
    {"65004-Dialogue.mod", 3, 1, 60},                 /* Boss Simion */
    {"56824-Dialogue.mod", 1, 2, 65},                 /* The Preacher */
    {"56887-Dialogue.mod", 3, 5, 60},                 /* Buzan */
    {"56888-Dialogue.mod", 5, 3, 60},                 /* Dimak */
    {"56889-Dialogue.mod", 1, 4, 60},                 /* Red Sabre Boss */
    {"48718-Dialogue.mod", 5, 4, 60},                 /* Big Gray */
    {"48738-Dialogue.mod", 4, 1, 60},                 /* Big Al */
    {"63884-Dialogue.mod", 3, 5, 65},                 /* Shade */
    {"96409-rebirth.mod", 2, 1, 80},                  /* Screamer the False */
    {"96407-rebirth.mod", 5, 2, 80},                  /* Ponk */
    {"1533326-__world reactions Shek.mod", 4, 1, 85}, /* Mukai */
    {"50519-rebirth.mod", 1, 3, 75}                   /* Cannibal Grand Wizard */
};

unsigned int ee_boss_profile_count(void) {
    return (unsigned int)(sizeof(profiles) / sizeof(profiles[0]));
}

const EEBossProfile *ee_boss_profile_at(unsigned int index) {
    return index < ee_boss_profile_count() ? &profiles[index] : 0;
}

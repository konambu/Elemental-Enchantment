#include "spellblade_state.h"
int _fltused = 0;

/* Exposed to an isolated test harness; does not open Kenshi. */
__declspec(dllexport) int sb_run_tests(void)
{
    SpellbladeState a, b;
    sb_init(&a); sb_init(&b);
    if (sb_effects(&a, 1, 1, 1, 1) != 0) return 1;
    if (!sb_select(&a, SB_FIRE)) return 2;
    if (sb_effects(&a, 0, 1, 1, 1) != 0) return 3;
    if (sb_effects(&a, 1, 0, 1, 1) != 0) return 4;
    if (sb_effects(&a, 1, 1, 0, 0) != SB_EFFECT_BLADE) return 5;
    if (sb_effects(&a, 1, 1, 1, 0) != (SB_EFFECT_BLADE | SB_EFFECT_SWING)) return 6;
    if (sb_effects(&a, 1, 1, 1, 1) != (SB_EFFECT_BLADE | SB_EFFECT_SWING | SB_EFFECT_HIT)) return 7;
    if (sb_effects(&b, 1, 1, 1, 1) != 0) return 8;
    if (!sb_restore(&b, SB_FIRE, 42.0f, 12.0f) || b.level != 42.0f || b.experience != 12.0f) return 9;
    if (sb_restore(&b, 7, 42.0f, 12.0f)) return 10;
    if (sb_restore(&b, SB_FIRE, 101.0f, 12.0f)) return 11;
    if (b.level != 42.0f || b.element != SB_FIRE) return 12;
    if (!sb_select(&a, SB_OFF) || sb_effects(&a, 1, 1, 1, 1) != 0) return 13;
    sb_init(&a);
    if (sb_gain_experience(&a, 9.0f) != 0 || a.level != 0.0f || a.experience != 9.0f) return 14;
    if (sb_gain_experience(&a, 13.0f) != 2 || a.level != 2.0f || a.experience != 0.0f) return 15;
    if (sb_gain_experience(&a, -1.0f) != -1 || a.level != 2.0f) return 16;
    if (!sb_restore(&a, SB_FIRE, 99.0f, 0.0f)) return 17;
    if (sb_gain_experience(&a, 1000.0f) != 1 || a.level != 100.0f || a.experience != 0.0f) return 18;
    if (sb_combat_effects(&a,0,0,1,1)!=SB_EFFECT_NONE) return 19;
    if (sb_combat_effects(&a,1,0,0,0)!=SB_EFFECT_HANDS) return 20;
    if (sb_combat_effects(&a,1,0,1,1)!=(SB_EFFECT_HANDS|SB_EFFECT_SWING|SB_EFFECT_HIT)) return 21;
    if (sb_combat_effects(&a,1,1,1,1)!=(SB_EFFECT_BLADE|SB_EFFECT_SWING|SB_EFFECT_HIT)) return 22;
    if (!sb_select(&a,SB_OFF) || sb_combat_effects(&a,1,0,1,1)!=SB_EFFECT_NONE) return 23;
    sb_init(&a); sb_init(&b);
    if (!sb_assign_initial(&a,1) || a.element!=SB_FIRE || !a.affinity_assigned) return 24;
    if (sb_assign_initial(&a,0) || a.element!=SB_FIRE) return 25;
    if (!sb_assign_initial(&b,0) || b.element!=SB_OFF || !b.affinity_assigned) return 26;
    if (sb_assign_initial(&b,1) || b.element!=SB_OFF) return 27;
    if (!sb_restore(&b,SB_FIRE,1.0f,0.0f) || !b.affinity_assigned || sb_assign_initial(&b,0)) return 28;
    return 0;
}

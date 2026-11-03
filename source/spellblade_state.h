#ifndef SPELLBLADE_STATE_H
#define SPELLBLADE_STATE_H

/* Pure gameplay decisions. Host integration supplies the selected character,
 * actual equipped weapon and successful melee-hit events. */
typedef struct SpellbladeState {
    unsigned int version;
    unsigned int element;
    float level;
    float experience;
    unsigned int affinity_assigned;
} SpellbladeState;

enum { SB_OFF = 0, SB_FIRE = 1 };
enum { SB_EFFECT_NONE = 0, SB_EFFECT_BLADE = 1, SB_EFFECT_SWING = 2, SB_EFFECT_HIT = 4, SB_EFFECT_HANDS = 8 };

void sb_init(SpellbladeState* state);
/* The host supplies a random draw once when a character first receives an
 * affinity, and must persist the resulting element, not the random draw. */
int sb_assign_initial(SpellbladeState* state, unsigned int random_draw);
int sb_select(SpellbladeState* state, unsigned int element);
unsigned int sb_effects(const SpellbladeState* state, int has_melee_weapon,
                        int weapon_drawn, int attacking, int valid_hit);
unsigned int sb_combat_effects(const SpellbladeState* state, int combat_active,
                               int has_drawn_melee_weapon, int attacking, int valid_hit);
int sb_restore(SpellbladeState* state, unsigned int element, float level, float experience);
int sb_gain_experience(SpellbladeState* state, float amount);

#endif

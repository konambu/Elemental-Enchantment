#include "spellblade_state.h"

void sb_init(SpellbladeState* state)
{
    if (!state) return;
    state->version = 1;
    state->element = SB_OFF;
    state->level = 0.0f;
    state->experience = 0.0f;
    state->affinity_assigned = 0;
}

int sb_assign_initial(SpellbladeState* state, unsigned int random_draw)
{
    if (!state || state->affinity_assigned) return 0;
    state->element = (random_draw & 1u) ? SB_FIRE : SB_OFF;
    state->affinity_assigned = 1;
    return 1;
}

int sb_select(SpellbladeState* state, unsigned int element)
{
    if (!state || (element != SB_OFF && element != SB_FIRE)) return 0;
    state->element = element;
    state->affinity_assigned = 1;
    return 1;
}

unsigned int sb_effects(const SpellbladeState* state, int has_melee_weapon,
                        int weapon_drawn, int attacking, int valid_hit)
{
    unsigned int result = SB_EFFECT_NONE;
    if (!state || state->element != SB_FIRE || !has_melee_weapon || !weapon_drawn)
        return result;
    result |= SB_EFFECT_BLADE;
    if (attacking) result |= SB_EFFECT_SWING;
    if (valid_hit) result |= SB_EFFECT_HIT;
    return result;
}

unsigned int sb_combat_effects(const SpellbladeState* state, int combat_active,
                               int has_drawn_melee_weapon, int attacking, int valid_hit)
{
    unsigned int result;
    if (!state || state->element != SB_FIRE || !combat_active) return SB_EFFECT_NONE;
    result = has_drawn_melee_weapon ? SB_EFFECT_BLADE : SB_EFFECT_HANDS;
    if (attacking) result |= SB_EFFECT_SWING;
    if (valid_hit) result |= SB_EFFECT_HIT;
    return result;
}

int sb_restore(SpellbladeState* state, unsigned int element, float level, float experience)
{
    if (!state || (element != SB_OFF && element != SB_FIRE)) return 0;
    /* NaN and infinities fail these range checks too. */
    if (!(level >= 0.0f && level <= 100.0f && experience >= 0.0f && experience <= 1000000.0f))
        return 0;
    state->version = 1;
    state->element = element;
    state->level = level;
    state->experience = experience;
    state->affinity_assigned = 1;
    return 1;
}

int sb_gain_experience(SpellbladeState* state, float amount)
{
    float threshold;
    int levels = 0;
    /* Prototype balance only; actual event sources still require integration. */
    if (!state || !(amount >= 0.0f && amount <= 1000000.0f) ||
        !(state->level >= 0.0f && state->level <= 100.0f) ||
        !(state->experience >= 0.0f && state->experience <= 1000000.0f)) return -1;
    if (state->level >= 100.0f) { state->experience = 0.0f; return 0; }
    state->experience += amount;
    while (state->level < 100.0f) {
        threshold = 10.0f + state->level * 2.0f;
        if (state->experience < threshold) break;
        state->experience -= threshold;
        state->level += 1.0f;
        ++levels;
    }
    if (state->level >= 100.0f) { state->level = 100.0f; state->experience = 0.0f; }
    return levels;
}

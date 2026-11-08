#ifndef ELEMENTAL_AFFINITY_H
#define ELEMENTAL_AFFINITY_H

/** @file elemental_affinity.h
 * @brief Game-independent affinity, leveling, aptitude, and combat rules.
 */
/* Versioned data only. Game integration must store this in each character's
 * own save record. Attribute order: fire, lightning, wind, water, ice. */
#define EE_AFFINITY_COUNT 5
#define EE_AFFINITY_MAX_LEVEL 100u
#define EE_AFFINITY_MAX_XP 47600u
/** Zero-based element indices in aptitude and radar display order. */
extern const unsigned char ee_affinity_ring_order[EE_AFFINITY_COUNT];
typedef struct EEAffinity {
    unsigned int selected;                  /* 0 = off, 1..5 = element */
    unsigned char rates[EE_AFFINITY_COUNT]; /* 0..100 */
    unsigned short xp[EE_AFFINITY_COUNT];   /* total XP in quarter-point units */
    unsigned int secondary;                 /* 0 = off, 1..5 = element */
    unsigned int xp_fraction;               /* hundredths of one XP unit */
} EEAffinity;

/* On first assignment, preserve the selected initial element when valid.
 * The random seed is only used once; save the resulting values. */
/** @brief Initialize levels and aptitude from a seed; initial_element may be 0..5. */
int ee_affinity_create(EEAffinity *, unsigned int seed, unsigned int initial_element);
/** @brief Select a primary element, clearing an identical secondary choice. */
int ee_affinity_select(EEAffinity *, unsigned int element);
/** @brief Select or clear a secondary element, available from level 50. */
int ee_affinity_select_secondary(EEAffinity *, unsigned int element);
/** @brief Return the secondary element's current share of its combat bonus. */
unsigned int ee_affinity_secondary_percent(const EEAffinity *);
/** @brief Award XP using the selected element's aptitude rate. */
int ee_affinity_award_hit_rate(EEAffinity *s, int all_aptitudes);
/** @brief Award XP with a configurable multiplier in tenths (0..200). */
int ee_affinity_award_hit_rate_scaled(EEAffinity *s, int all_aptitudes, unsigned int tenths);
int ee_affinity_award_hit(EEAffinity *);
/** @brief Compute the shared Elemental level from accumulated total XP. */
unsigned int ee_affinity_level(const EEAffinity *);
/** @brief Total XP required for a level, capped at the maximum level. */
unsigned int ee_affinity_xp_for_level(unsigned int level);
typedef struct EEDamage {
    float cut, blunt, pierce, stun, bleed, armour;
} EEDamage;
/** @brief Apply current combat bonuses to the Kenshi damage components. */
int ee_affinity_damage(const EEAffinity *, EEDamage *);
/** @brief Return combat speed after applying the wind bonus to the base speed. */
float ee_affinity_combat_speed(const EEAffinity *, float base_speed);
/** @brief Return attack speed after applying the elemental bonus to the base speed. */
float ee_affinity_attack_speed(const EEAffinity *, float base_speed);
/** @brief Apply combat bonuses scaled by the MCM strength multiplier. */
int ee_affinity_damage_scaled(const EEAffinity *, EEDamage *, float multiplier);
/** @brief Return combat speed with the elemental bonus scaled by multiplier. */
float ee_affinity_combat_speed_scaled(const EEAffinity *, float base_speed, float multiplier);
/** @brief Return attack speed with the elemental bonus scaled by multiplier. */
float ee_affinity_attack_speed_scaled(const EEAffinity *, float base_speed, float multiplier);
/** @brief Encode the current version into the fixed three-integer save payload. */
int ee_affinity_encode(const EEAffinity *, int out_three[3]);
/** @brief Decode only supported data; reject corrupt or newer-version records. */
int ee_affinity_decode(EEAffinity *, const int in_three[3]);
#endif

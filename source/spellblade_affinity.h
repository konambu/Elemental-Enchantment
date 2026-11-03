#ifndef SPELLBLADE_AFFINITY_H
#define SPELLBLADE_AFFINITY_H

/* Versioned data only. Game integration must store this in each character's
 * own save record. Attribute order: fire, lightning, wind, water, ice. */
#define SB_AFFINITY_COUNT 5
typedef struct SBAffinity {
    unsigned int selected; /* 0 = off, 1..5 = element */
    unsigned char rates[SB_AFFINITY_COUNT]; /* 0..100 */
    unsigned short xp[SB_AFFINITY_COUNT]; /* total XP in quarter-point units */
    unsigned int secondary; /* 0 = off, 1..5 = element */
    unsigned int xp_fraction; /* hundredths of one XP unit */
} SBAffinity;

/* On first assignment, preserve the currently visible trial element when it
 * is valid. The random seed is only used once; save the resulting values. */
int sb_affinity_create(SBAffinity*, unsigned int seed, unsigned int trial_element);
int sb_affinity_select(SBAffinity*, unsigned int element);
int sb_affinity_select_secondary(SBAffinity*, unsigned int element);
unsigned int sb_affinity_secondary_percent(const SBAffinity*);
int sb_affinity_award_hit(SBAffinity*);
unsigned int sb_affinity_level(const SBAffinity*,unsigned int element);
typedef struct SBDamage {float cut,blunt,pierce,stun,bleed,armour;} SBDamage;
int sb_affinity_damage(const SBAffinity*,SBDamage*);
float sb_affinity_combat_speed(const SBAffinity*,float base_speed);
float sb_affinity_attack_speed(const SBAffinity*,float base_speed);
int sb_affinity_encode(const SBAffinity*, int out_three[3]);
/* A corrupt or unknown-version record must not overwrite a current state. */
int sb_affinity_decode(SBAffinity*, const int in_three[3]);
#endif

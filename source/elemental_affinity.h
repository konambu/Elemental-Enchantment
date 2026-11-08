#ifndef ELEMENTAL_AFFINITY_H
#define ELEMENTAL_AFFINITY_H

/* Versioned data only. Game integration must store this in each character's
 * own save record. Attribute order: fire, lightning, wind, water, ice. */
#define EE_AFFINITY_COUNT 5
typedef struct EEAffinity {
    unsigned int selected; /* 0 = off, 1..5 = element */
    unsigned char rates[EE_AFFINITY_COUNT]; /* 0..100 */
    unsigned short xp[EE_AFFINITY_COUNT]; /* total XP in quarter-point units */
    unsigned int secondary; /* 0 = off, 1..5 = element */
    unsigned int xp_fraction; /* hundredths of one XP unit */
} EEAffinity;

/* On first assignment, preserve the currently visible trial element when it
 * is valid. The random seed is only used once; save the resulting values. */
int ee_affinity_create(EEAffinity*, unsigned int seed, unsigned int trial_element);
int ee_affinity_select(EEAffinity*, unsigned int element);
int ee_affinity_select_secondary(EEAffinity*, unsigned int element);
unsigned int ee_affinity_secondary_percent(const EEAffinity*);
int ee_affinity_award_hit_rate(EEAffinity* s,int all_aptitudes);
int ee_affinity_award_hit_rate_scaled(EEAffinity* s,int all_aptitudes,unsigned int tenths);
int ee_affinity_award_hit(EEAffinity*);
unsigned int ee_affinity_level(const EEAffinity*,unsigned int element);
typedef struct EEDamage {float cut,blunt,pierce,stun,bleed,armour;} EEDamage;
int ee_affinity_damage(const EEAffinity*,EEDamage*);
float ee_affinity_combat_speed(const EEAffinity*,float base_speed);
float ee_affinity_attack_speed(const EEAffinity*,float base_speed);
int ee_affinity_damage_scaled(const EEAffinity*,EEDamage*,float multiplier);
float ee_affinity_combat_speed_scaled(const EEAffinity*,float base_speed,float multiplier);
float ee_affinity_attack_speed_scaled(const EEAffinity*,float base_speed,float multiplier);
int ee_affinity_encode(const EEAffinity*, int out_three[3]);
/* A corrupt or unknown-version record must not overwrite a current state. */
int ee_affinity_decode(EEAffinity*, const int in_three[3]);
#endif

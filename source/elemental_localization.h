#ifndef EE_LOCALIZATION_H
#define EE_LOCALIZATION_H
typedef struct EELocale {const char* id;const char* element[6];const char* main;const char* sub;const char* locked;const char* opener;const char* elemental;const char* level;const char* master;const char* next;const char* trail;const char* glow;const char* dual;const char* second;const char* nextlevel;const char* maxlevel;const char* readonly;const char* unavailable;const char* disabled;const char* noelement;const char* choose;const char* base;const char* winddetail;const char* bleed;const char* bolt;const char* attackblock;const char* mcm_section;const char* mcm_label;const char* mcm_tip;const char* armour;const char* blunt;const char* cut;} EELocale;
void ee_locale_set(const char* id);
const EELocale* ee_locale_get(void);
#endif

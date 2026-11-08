#ifndef EE_EFFECT_TEMPLATES_H
#define EE_EFFECT_TEMPLATES_H

enum {
    EE_TEMPLATE_HAND=0,
    EE_TEMPLATE_BLADE=1,
    EE_TEMPLATE_IMPACT=2,
    EE_TEMPLATE_CROSSBOW=3
};

typedef struct EEEffectTemplate {
    const char* resource;
    unsigned int size;
    unsigned int capacity;
} EEEffectTemplate;

const EEEffectTemplate* ee_effect_template(int kind,int element);

#endif

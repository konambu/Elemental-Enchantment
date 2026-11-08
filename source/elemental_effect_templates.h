#ifndef EE_EFFECT_TEMPLATES_H
#define EE_EFFECT_TEMPLATES_H

/** @file elemental_effect_templates.h
 * @brief Maps element IDs to the particle resources used by
 * each visual.
 */

/** Visual context passed to ee_effect_template(). */
enum {
    EE_TEMPLATE_HAND = 0,
    EE_TEMPLATE_BLADE = 1,
    EE_TEMPLATE_IMPACT = 2,
    EE_TEMPLATE_CROSSBOW = 3
};

/** A particle resource name in the Kenshi/Ogre string format. */
typedef struct EEEffectTemplate {
    const char *resource;
    unsigned int size;
    unsigned int capacity;
} EEEffectTemplate;

/**
 * @brief Find the resource used for one element and visual context.
 * @param kind One of
 * EE_TEMPLATE_HAND, BLADE, IMPACT, or CROSSBOW.
 * @param element Element ID in the stable
 * range 1..5.
 * @return Resource descriptor, or NULL when the context or element is invalid.
 */
const EEEffectTemplate *ee_effect_template(int kind, int element);

#endif

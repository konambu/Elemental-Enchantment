#include "elemental_effect_templates.h"

/* String literals include a trailing NUL. Kenshi stores up to 15 bytes inline. */
#define TEMPLATE(name) {name, sizeof(name) - 1u, sizeof(name) <= 16u ? 15u : sizeof(name) - 1u}

/* Rows are hand, blade, impact, and crossbow; columns follow element IDs 1..5. */
static const EEEffectTemplate templates[4][5] = {{TEMPLATE("SB_Prototype_HandFire"),
                                                  TEMPLATE("SB_Prototype_LightningHand"),
                                                  TEMPLATE("SB_Prototype_WindHand"),
                                                  TEMPLATE("SB_Prototype_WaterHand"),
                                                  TEMPLATE("SB_Prototype_IceHand")},
                                                 {TEMPLATE("SB_Prototype_Fire"),
                                                  TEMPLATE("SB_Prototype_LightningBlade"),
                                                  TEMPLATE("SB_Prototype_WindBlade"),
                                                  TEMPLATE("SB_Prototype_WaterBlade"),
                                                  TEMPLATE("SB_Prototype_IceBlade")},
                                                 {TEMPLATE("EE_Impact_Fire"),
                                                  TEMPLATE("EE_Impact_Lightning"),
                                                  TEMPLATE("EE_Impact_Wind"),
                                                  TEMPLATE("EE_Impact_Water"),
                                                  TEMPLATE("EE_Impact_Ice")},
                                                 {TEMPLATE("SB_Prototype_HandFire"),
                                                  TEMPLATE("EE_Bolt_Lightning"),
                                                  TEMPLATE("EE_Bolt_Wind"),
                                                  TEMPLATE("EE_Bolt_Water"),
                                                  TEMPLATE("EE_Bolt_Ice")}};
#undef TEMPLATE

const EEEffectTemplate *ee_effect_template(int kind, int element) {
    if (kind < EE_TEMPLATE_HAND || kind > EE_TEMPLATE_CROSSBOW || element < 1 || element > 5)
        return 0;
    return &templates[kind][element - 1];
}

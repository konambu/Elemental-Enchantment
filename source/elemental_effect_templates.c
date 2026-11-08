#include "elemental_effect_templates.h"

static const EEEffectTemplate templates[4][5]={
    {
        {"SB_Prototype_HandFire",21,21},
        {"SB_Prototype_LightningHand",26,26},
        {"SB_Prototype_WindHand",21,21},
        {"SB_Prototype_WaterHand",22,22},
        {"SB_Prototype_IceHand",20,20}
    },
    {
        {"SB_Prototype_Fire",17,17},
        {"SB_Prototype_LightningBlade",27,27},
        {"SB_Prototype_WindBlade",22,22},
        {"SB_Prototype_WaterBlade",23,23},
        {"SB_Prototype_IceBlade",21,21}
    },
    {
        {"EE_Impact_Fire",14,15},
        {"EE_Impact_Lightning",19,19},
        {"EE_Impact_Wind",14,15},
        {"EE_Impact_Water",15,15},
        {"EE_Impact_Ice",13,15}
    },
    {
        {"SB_Prototype_HandFire",21,21},
        {"EE_Bolt_Lightning",17,17},
        {"EE_Bolt_Wind",12,15},
        {"EE_Bolt_Water",13,15},
        {"EE_Bolt_Ice",11,15}
    }
};

const EEEffectTemplate* ee_effect_template(int kind,int element)
{
    if(kind<EE_TEMPLATE_HAND||kind>EE_TEMPLATE_CROSSBOW||element<1||element>5)return 0;
    return &templates[kind][element-1];
}

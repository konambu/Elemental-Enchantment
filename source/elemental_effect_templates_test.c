#include "elemental_effect_templates.h"

/* Test-only access to the same descriptors consumed by the runtime. */
__declspec(dllexport) const EEEffectTemplate *ee_test_effect_template(int kind, int element) {
    return ee_effect_template(kind, element);
}

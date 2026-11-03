#include "spellblade_lifecycle.h"
__declspec(dllexport) int sb_run_lifecycle_tests(void)
{
    SBBindings b;
    SBFlameBinding out;
    int i;
    sb_bindings_init(&b);
    if (!sb_binding_add(&b,(void*)1,(void*)2,(void*)3,(void*)4)) return 1;
    if (sb_binding_add(&b,(void*)1,(void*)2,(void*)5,(void*)4)) return 2;
    if (!sb_binding_add(&b,(void*)1,(void*)6,(void*)7,(void*)4)) return 3;
    if (sb_binding_take_blade(&b,(void*)8,(void*)2,&out)) return 4;
    if (!sb_binding_take_blade(&b,(void*)1,(void*)2,&out) || out.effect!=(void*)3) return 5;
    if (!sb_binding_take_scene(&b,(void*)4,&out) || out.effect!=(void*)7) return 6;
    if (sb_binding_take_owner(&b,(void*)1,&out)) return 7;
    for (i=0;i<SB_BINDING_LIMIT;++i)
        if (!sb_binding_add(&b,(void*)1,(void*)(unsigned long long)(i+10),(void*)(unsigned long long)(i+100),(void*)4)) return 8;
    if (sb_binding_add(&b,(void*)1,(void*)999,(void*)998,(void*)4)) return 9;
    for (i=0;i<SB_BINDING_LIMIT;++i) if (!sb_binding_take_owner(&b,(void*)1,&out)) return 10;
    if (sb_binding_take_owner(&b,(void*)1,&out)) return 11;
    return 0;
}

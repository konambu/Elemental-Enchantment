#include "spellblade_flame.h"
typedef struct TestBackend { int creates, attaches, detaches, destroys, fail_create, fail_attach; } TestBackend;
static void* make(void* c, void* scene)
{ TestBackend* t=(TestBackend*)c; (void)scene; ++t->creates; return t->fail_create ? 0 : (void*)(unsigned long long)(100+t->creates); }
static int attach(void* c, void* owner, void* effect, const void* pose)
{ TestBackend* t=(TestBackend*)c; (void)owner; (void)effect; (void)pose; ++t->attaches; return !t->fail_attach; }
static void detach(void* c, void* owner, void* effect)
{ TestBackend* t=(TestBackend*)c; (void)owner; (void)effect; ++t->detaches; }
static void destroy(void* c, void* scene, void* effect)
{ TestBackend* t=(TestBackend*)c; (void)scene; (void)effect; ++t->destroys; }
__declspec(dllexport) int sb_run_flame_tests(void)
{
    SBBindings b; TestBackend t={0}; SBFlameBackend api={&t,make,attach,detach,destroy};
    sb_bindings_init(&b);
    if (!sb_flame_attach(&b,&api,(void*)1,(void*)2,(void*)3,0)) return 1;
    if (sb_flame_attach(&b,&api,(void*)1,(void*)2,(void*)3,0) || t.creates!=1) return 2;
    if (!sb_flame_attach(&b,&api,(void*)1,(void*)4,(void*)3,0)) return 3;
    sb_flame_remove_blade(&b,&api,(void*)1,(void*)2);
    if (t.detaches!=1 || t.destroys!=1) return 4;
    sb_flame_remove_scene(&b,&api,(void*)3);
    if (t.detaches!=2 || t.destroys!=2) return 5;
    t.fail_attach=1;
    if (sb_flame_attach(&b,&api,(void*)1,(void*)2,(void*)3,0) || t.destroys!=3 || t.detaches!=2) return 6;
    t.fail_attach=0; t.fail_create=1;
    if (sb_flame_attach(&b,&api,(void*)1,(void*)2,(void*)3,0) || t.destroys!=3) return 7;
    t.fail_create=0;
    if (!sb_flame_attach(&b,&api,(void*)1,(void*)2,(void*)3,0)) return 8;
    if (!sb_flame_attach(&b,&api,(void*)5,(void*)6,(void*)3,0)) return 9;
    sb_flame_remove_owner(&b,&api,(void*)1);
    if (t.detaches!=3 || t.destroys!=4) return 10;
    sb_flame_remove_scene(&b,&api,(void*)3);
    if (t.detaches!=4 || t.destroys!=5) return 11;
    return 0;
}

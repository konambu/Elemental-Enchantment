#include "spellblade_flame.h"

static int valid(const SBFlameBackend* api)
{ return api && api->create && api->attach && api->detach && api->destroy; }

int sb_flame_attach(SBBindings* bindings, const SBFlameBackend* api,
                    void* owner, void* blade, void* scene, const void* pose)
{
    int i, free_slot=0;
    void* effect;
    if (!bindings || !valid(api) || !owner || !blade || !scene) return 0;
    for (i=0; i<SB_BINDING_LIMIT; ++i) {
        if (bindings->slots[i].effect && bindings->slots[i].blade==blade) return 0;
        if (!bindings->slots[i].effect) free_slot=1;
    }
    if (!free_slot) return 0;
    effect=api->create(api->context, scene);
    if (!effect) return 0;
    if (!api->attach(api->context, owner, effect, pose)) {
        api->destroy(api->context, scene, effect);
        return 0;
    }
    if (!sb_binding_add(bindings, owner, blade, effect, scene)) {
        api->detach(api->context, owner, effect);
        api->destroy(api->context, scene, effect);
        return 0;
    }
    return 1;
}

static void release(const SBFlameBackend* api, const SBFlameBinding* binding)
{
    api->detach(api->context, binding->owner, binding->effect);
    api->destroy(api->context, binding->scene, binding->effect);
}
void sb_flame_remove_blade(SBBindings* b, const SBFlameBackend* a, void* owner, void* blade)
{ SBFlameBinding x; if (valid(a) && sb_binding_take_blade(b,owner,blade,&x)) release(a,&x); }
void sb_flame_remove_owner(SBBindings* b, const SBFlameBackend* a, void* owner)
{ SBFlameBinding x; if (valid(a)) while (sb_binding_take_owner(b,owner,&x)) release(a,&x); }
void sb_flame_remove_scene(SBBindings* b, const SBFlameBackend* a, void* scene)
{ SBFlameBinding x; if (valid(a)) while (sb_binding_take_scene(b,scene,&x)) release(a,&x); }

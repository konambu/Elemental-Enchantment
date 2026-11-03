#include "spellblade_lifecycle.h"

static void clear_slot(SBFlameBinding* p) { p->owner=0; p->blade=0; p->effect=0; p->scene=0; }
void sb_bindings_init(SBBindings* b) { int i; if (b) for (i=0;i<SB_BINDING_LIMIT;++i) clear_slot(&b->slots[i]); }

int sb_binding_add(SBBindings* b, void* owner, void* blade, void* effect, void* scene)
{
    int i, free_slot=-1;
    if (!b || !owner || !blade || !effect || !scene) return 0;
    for (i=0;i<SB_BINDING_LIMIT;++i) {
        SBFlameBinding* p=&b->slots[i];
        if (p->effect && (p->blade==blade || p->effect==effect)) return 0;
        if (!p->effect && free_slot<0) free_slot=i;
    }
    if (free_slot<0) return 0;
    b->slots[free_slot].owner=owner; b->slots[free_slot].blade=blade;
    b->slots[free_slot].effect=effect; b->slots[free_slot].scene=scene;
    return 1;
}

static int take(SBBindings* b, void* owner, void* blade, void* scene, SBFlameBinding* out)
{
    int i;
    if (!b || !out || (!owner && !scene)) return 0;
    for (i=0;i<SB_BINDING_LIMIT;++i) {
        SBFlameBinding* p=&b->slots[i];
        if (p->effect && (!owner || p->owner==owner) && (!blade || p->blade==blade) &&
            (!scene || p->scene==scene)) { *out=*p; clear_slot(p); return 1; }
    }
    return 0;
}
int sb_binding_take_blade(SBBindings* b, void* owner, void* blade, SBFlameBinding* out)
{ return owner && blade ? take(b, owner, blade, 0, out) : 0; }
int sb_binding_take_owner(SBBindings* b, void* owner, SBFlameBinding* out)
{ return owner ? take(b, owner, 0, 0, out) : 0; }
int sb_binding_take_scene(SBBindings* b, void* scene, SBFlameBinding* out)
{ return scene ? take(b, 0, 0, scene, out) : 0; }

#ifndef SPELLBLADE_LIFECYCLE_H
#define SPELLBLADE_LIFECYCLE_H

/* Opaque, borrowed engine identities. The host owns synchronization and must
 * release an effect before its owner or scene is destroyed. */
#define SB_BINDING_LIMIT 64
typedef struct SBFlameBinding {
    void* owner;
    void* blade;
    void* effect;
    void* scene;
} SBFlameBinding;
typedef struct SBBindings { SBFlameBinding slots[SB_BINDING_LIMIT]; } SBBindings;

void sb_bindings_init(SBBindings* bindings);
int sb_binding_add(SBBindings* bindings, void* owner, void* blade, void* effect, void* scene);
int sb_binding_take_blade(SBBindings* bindings, void* owner, void* blade, SBFlameBinding* out);
int sb_binding_take_owner(SBBindings* bindings, void* owner, SBFlameBinding* out);
int sb_binding_take_scene(SBBindings* bindings, void* scene, SBFlameBinding* out);

#endif

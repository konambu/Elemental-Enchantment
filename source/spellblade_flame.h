#ifndef SPELLBLADE_FLAME_H
#define SPELLBLADE_FLAME_H
#include "spellblade_lifecycle.h"

/* A single-threaded adapter. The host must ensure all calls happen while the
 * engine objects are alive, and prevent re-entry from backend callbacks. */
typedef struct SBFlameBackend {
    void* context;
    void* (*create)(void* context, void* scene);
    int (*attach)(void* context, void* owner, void* effect, const void* pose);
    void (*detach)(void* context, void* owner, void* effect);
    void (*destroy)(void* context, void* scene, void* effect);
} SBFlameBackend;

int sb_flame_attach(SBBindings*, const SBFlameBackend*, void* owner, void* blade,
                    void* scene, const void* pose);
void sb_flame_remove_blade(SBBindings*, const SBFlameBackend*, void* owner, void* blade);
void sb_flame_remove_owner(SBBindings*, const SBFlameBackend*, void* owner);
void sb_flame_remove_scene(SBBindings*, const SBFlameBackend*, void* scene);
#endif

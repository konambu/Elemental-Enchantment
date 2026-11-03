#ifndef SPELLBLADE_SAVE_H
#define SPELLBLADE_SAVE_H
#include "spellblade_affinity.h"

/* A backend reads/writes one namespaced triple in one character's save data.
 * Read returns 1 if present, 0 if absent, -1 on failure. Write returns 1 on
 * success. These functions do not interact with Kenshi by themselves. */
typedef struct SBSaveBackend {
    void* context;
    int (*read)(void* context,int out[3]);
    int (*write)(void* context,const int values[3]);
} SBSaveBackend;
enum { SB_SAVE_ERROR=-1, SB_SAVE_MISSING=0, SB_SAVE_OK=1, SB_SAVE_UNSUPPORTED=2 };
int sb_save_load(const SBSaveBackend*,SBAffinity*);
int sb_save_store(const SBSaveBackend*,const SBAffinity*);
#endif

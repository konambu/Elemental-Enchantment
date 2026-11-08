#include "elemental_persistence.h"
typedef struct LocalRead {
    int values[3];
} LocalRead;
static int local_read(void *c, int out[3]) {
    LocalRead *x = (LocalRead *)c;
    int i;
    for (i = 0; i < 3; ++i)
        out[i] = x->values[i];
    return 1;
}
/* A reused stats pointer must not retain data staged by an earlier load. */
static void forget_pending(EEPersistence *p, void *stats) {
    int i;
    if (!p || !stats)
        return;
    for (i = 0; i < EE_PENDING_LIMIT; ++i)
        if (p->pending[i].stats == stats)
            p->pending[i].stats = 0;
}
void ee_persistence_init(EEPersistence *p) {
    int i;
    if (!p)
        return;
    ee_characters_init(&p->characters);
    p->stage_overflow = 0;
    for (i = 0; i < EE_PENDING_LIMIT; ++i)
        p->pending[i].stats = 0;
}
int ee_persistence_stage(EEPersistence *p, void *stats, const EESaveBackend *backend) {
    int values[3], status, i, unsupported, slot = -1;
    EEAffinity check;
    if (!p || !stats || !backend || !backend->read)
        return EE_SAVE_ERROR;
    status = backend->read(backend->context, values);
    if (!status) {
        forget_pending(p, stats);
        return EE_SAVE_MISSING;
    }
    /* Preserve a guarded record when an existing field could not be read, so
     * later character binding cannot silently invent writable data. */
    if (status < 0) {
        values[0] = -1;
        values[1] = 0;
        values[2] = 0;
    }
    for (i = 0; i < EE_PENDING_LIMIT; ++i) {
        if (p->pending[i].stats == stats) {
            slot = i;
            break;
        }
        if (slot < 0 && !p->pending[i].stats)
            slot = i;
    }
    if (slot < 0) {
        /* Some loaded record cannot be retained. Fail closed for this session:
         * a later bind must not invent state and overwrite its saved data. */
        p->stage_overflow = 1;
        return EE_SAVE_ERROR;
    }
    p->pending[slot].stats = stats;
    for (i = 0; i < 3; ++i)
        p->pending[slot].values[i] = values[i];
    unsupported = !ee_affinity_decode(&check, values);
    if (status < 0)
        return EE_SAVE_ERROR;
    return unsupported ? EE_SAVE_UNSUPPORTED : EE_SAVE_OK;
}
EECharacterRecord *ee_persistence_bind(EEPersistence *p,
                                       void *stats,
                                       void *character,
                                       unsigned int index,
                                       unsigned int serial,
                                       unsigned int seed,
                                       unsigned int initial_element) {
    int i;
    EECharacterRecord *r;
    if (!p || !character || p->stage_overflow)
        return 0;
    if (stats)
        for (i = 0; i < EE_PENDING_LIMIT; ++i)
            if (p->pending[i].stats == stats) {
                LocalRead local;
                EESaveBackend backend;
                int j, status;
                for (j = 0; j < 3; ++j)
                    local.values[j] = p->pending[i].values[j];
                backend.context = &local;
                backend.read = local_read;
                backend.write = 0;
                status = ee_character_load(&p->characters, character, index, serial, &backend);
                if (status == EE_SAVE_ERROR)
                    return 0;
                p->pending[i].stats = 0;
                break;
            }
    r = ee_character_observe(&p->characters, character, index, serial, seed, initial_element);
    return r;
}
int ee_persistence_save(EEPersistence *p,
                        void *character,
                        unsigned int index,
                        unsigned int serial,
                        const EESaveBackend *backend) {
    return p && !p->stage_overflow
               ? ee_character_save(&p->characters, character, index, serial, backend)
               : EE_SAVE_ERROR;
}
void ee_persistence_forget(EEPersistence *p, void *stats, void *character) {
    if (!p)
        return;
    ee_character_forget(&p->characters, character);
    forget_pending(p, stats);
}

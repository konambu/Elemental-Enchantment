#include "elemental_save.h"

int ee_save_load(const EESaveBackend *backend, EEAffinity *out) {
    int values[3], status;
    if (!backend || !backend->read || !out)
        return EE_SAVE_ERROR;
    status = backend->read(backend->context, values);
    if (status < 0)
        return EE_SAVE_ERROR;
    if (!status)
        return EE_SAVE_MISSING;
    return ee_affinity_decode(out, values) ? EE_SAVE_OK : EE_SAVE_UNSUPPORTED;
}
int ee_save_store(const EESaveBackend *backend, const EEAffinity *state) {
    EEAffinity previous;
    int old[3], values[3], status;
    if (!backend || !backend->read || !backend->write || !state)
        return EE_SAVE_ERROR;
    if (!ee_affinity_encode(state, values))
        return EE_SAVE_ERROR;
    status = backend->read(backend->context, old);
    if (status < 0)
        return EE_SAVE_ERROR;
    /* Never silently erase data from a newer or corrupt version. */
    if (status > 0 && !ee_affinity_decode(&previous, old))
        return EE_SAVE_UNSUPPORTED;
    return backend->write(backend->context, values) ? EE_SAVE_OK : EE_SAVE_ERROR;
}

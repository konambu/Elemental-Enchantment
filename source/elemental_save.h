#ifndef ELEMENTAL_SAVE_H
#define ELEMENTAL_SAVE_H
#include "elemental_affinity.h"

/** @file elemental_save.h
 * @brief Versioned affinity serialization over a caller-provided backend.
 */
/* A backend reads/writes one namespaced triple in one character's save data.
 * Read returns a positive value if present, 0 if absent, or a negative value
 * on failure. Write returns a positive value on success; zero or negative
 * values indicate failure. The backend is responsible for accessing Kenshi. */
typedef struct EESaveBackend {
    void *context;
    int (*read)(void *context, int out[3]);
    int (*write)(void *context, const int values[3]);
} EESaveBackend;
enum { EE_SAVE_ERROR = -1, EE_SAVE_MISSING = 0, EE_SAVE_OK = 1, EE_SAVE_UNSUPPORTED = 2 };
/** @brief Load and validate one character's stored affinity record. */
int ee_save_load(const EESaveBackend *, EEAffinity *);
/** @brief Store affinity without overwriting corrupt or newer-version data. */
int ee_save_store(const EESaveBackend *, const EEAffinity *);
#endif

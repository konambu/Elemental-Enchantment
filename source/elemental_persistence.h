#ifndef ELEMENTAL_PERSISTENCE_H
#define ELEMENTAL_PERSISTENCE_H
#include "elemental_characters.h"
/** @file elemental_persistence.h
 * @brief Coordinates staged GameData reads and per-character save
 * lifecycle.
 */
#define EE_PENDING_LIMIT 512
typedef struct EEPendingSave {
    void *stats;
    int values[3];
} EEPendingSave;
typedef struct EEPersistence {
    EECharacters characters;
    EEPendingSave pending[EE_PENDING_LIMIT];
} EEPersistence;
/* Caller owns synchronization. A loaded stats object may not yet have a
 * usable character handle,
 * so valid data is staged until that actor appears. */
void ee_persistence_init(EEPersistence *);
/** @brief Stage a stats record until its character handle is available. */
int ee_persistence_stage(EEPersistence *, void *stats, const EESaveBackend *);
/** @brief Bind loaded or new affinity state to the live character. */
EECharacterRecord *ee_persistence_bind(EEPersistence *,
                                       void *stats,
                                       void *character,
                                       unsigned int index,
                                       unsigned int serial,
                                       unsigned int seed,
                                       unsigned int initial_element);
/** @brief Persist one character's writable affinity record. */
int ee_persistence_save(EEPersistence *,
                        void *character,
                        unsigned int index,
                        unsigned int serial,
                        const EESaveBackend *);
/** @brief Drop cached state after the stats or character object is destroyed. */
void ee_persistence_forget(EEPersistence *, void *stats, void *character);
#endif

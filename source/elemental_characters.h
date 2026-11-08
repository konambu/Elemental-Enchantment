#ifndef ELEMENTAL_CHARACTERS_H
#define ELEMENTAL_CHARACTERS_H
#include "elemental_save.h"
/** @file elemental_characters.h
 * @brief Per-character affinity state keyed by live identity and save handle.
 */
#define EE_CHARACTER_LIMIT 512
/** One character's loaded or newly generated affinity state. */
typedef struct EECharacterRecord {
    void *character;
    unsigned int index, serial;
    EEAffinity affinity;
    int read_only;
    unsigned int handle_change_depth;
} EECharacterRecord;
typedef struct EECharacters {
    EECharacterRecord records[EE_CHARACTER_LIMIT];
} EECharacters;
/* Caller serializes access. Pointer and handle are jointly used to avoid
 * carrying a dead character's state onto a recycled game object. */
void ee_characters_init(EECharacters *characters);
/* Optional entropy for newly created records only; never used for loaded data. */
void ee_characters_set_seed_provider(unsigned int (*provider)(unsigned int));
/** @brief Find or create the in-memory record for a character identity.
 * @param character Live game object; must not be NULL.
 * @param index Game handle index for the character.
 * @param serial Game handle generation, paired with index.
 * @param seed Seed used only when creating a new affinity record.
 * @param initial_element Initial active element, 0 for none or 1..5.
 * @return The record, or NULL for invalid input or a full record table.
 */
EECharacterRecord *ee_character_observe(
    EECharacters *characters, void *character, unsigned int index, unsigned int serial,
    unsigned int seed, unsigned int initial_element);
/** @brief Load the character's affinity; unsupported versions become read-only. */
int ee_character_load(EECharacters *, void *, unsigned int, unsigned int, const EESaveBackend *);
/** @brief Save a writable character record. */
int ee_character_save(EECharacters *, void *, unsigned int, unsigned int, const EESaveBackend *);
/** @brief Change the primary element; returns false for missing/read-only records. */
int ee_character_select(EECharacters *, void *, unsigned int, unsigned int, unsigned int);
/** @brief Change the secondary element after level and choice validation. */
int ee_character_select_secondary(EECharacters *, void *, unsigned int, unsigned int, unsigned int);
/** @brief Forget a character when Kenshi destroys its live object. */
void ee_character_forget(EECharacters *, void *);
/** @brief Mark the start of a handle transition using the old index and serial.
 * @return 1 if a record was found and the transition started, otherwise 0.
 */
int ee_character_handle_begin(EECharacters *characters, void *character,
                              unsigned int index, unsigned int serial);
/** @brief Finish a started transition with the new index and serial.
 * Only call after ee_character_handle_begin() returns 1. */
void ee_character_handle_end(EECharacters *characters, void *character,
                             unsigned int index, unsigned int serial);
#endif

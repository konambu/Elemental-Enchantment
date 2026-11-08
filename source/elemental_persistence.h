#ifndef ELEMENTAL_PERSISTENCE_H
#define ELEMENTAL_PERSISTENCE_H
#include "elemental_characters.h"
#define EE_PENDING_LIMIT 512
typedef struct EEPendingSave {void* stats;int values[3];int unsupported;} EEPendingSave;
typedef struct EEPersistence {EECharacters characters;EEPendingSave pending[EE_PENDING_LIMIT];} EEPersistence;
/* Caller owns synchronization. A loaded stats object may not yet have a
 * usable character handle, so valid data is staged until that actor appears. */
void ee_persistence_init(EEPersistence*);
int ee_persistence_stage(EEPersistence*,void* stats,const EESaveBackend*);
EECharacterRecord* ee_persistence_bind(EEPersistence*,void* stats,void* character,unsigned int index,unsigned int serial,unsigned int seed,unsigned int trial);
int ee_persistence_save(EEPersistence*,void* character,unsigned int index,unsigned int serial,const EESaveBackend*);
void ee_persistence_forget(EEPersistence*,void* stats,void* character);
#endif

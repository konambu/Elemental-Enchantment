#ifndef SPELLBLADE_PERSISTENCE_H
#define SPELLBLADE_PERSISTENCE_H
#include "spellblade_characters.h"
#define SB_PENDING_LIMIT 512
typedef struct SBPendingSave {void* stats;int values[3];int unsupported;} SBPendingSave;
typedef struct SBPersistence {SBCharacters characters;SBPendingSave pending[SB_PENDING_LIMIT];} SBPersistence;
/* Caller owns synchronization. A loaded stats object may not yet have a
 * usable character handle, so valid data is staged until that actor appears. */
void sb_persistence_init(SBPersistence*);
int sb_persistence_stage(SBPersistence*,void* stats,const SBSaveBackend*);
SBCharacterRecord* sb_persistence_bind(SBPersistence*,void* stats,void* character,unsigned int index,unsigned int serial,unsigned int seed,unsigned int trial);
int sb_persistence_save(SBPersistence*,void* character,unsigned int index,unsigned int serial,const SBSaveBackend*);
void sb_persistence_forget(SBPersistence*,void* stats,void* character);
#endif

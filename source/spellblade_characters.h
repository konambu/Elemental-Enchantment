#ifndef SPELLBLADE_CHARACTERS_H
#define SPELLBLADE_CHARACTERS_H
#include "spellblade_save.h"
#define SB_CHARACTER_LIMIT 512
typedef struct SBCharacterRecord {
    void* character;
    unsigned int index,serial;
    SBAffinity affinity;
    int read_only;
    unsigned int handle_change_depth;
} SBCharacterRecord;
typedef struct SBCharacters { SBCharacterRecord records[SB_CHARACTER_LIMIT]; } SBCharacters;
/* Caller serializes access. Pointer and handle are jointly used to avoid
 * carrying a dead character's state onto a recycled game object. */
void sb_characters_init(SBCharacters*);
SBCharacterRecord* sb_character_observe(SBCharacters*,void*,unsigned int,unsigned int,unsigned int,unsigned int);
int sb_character_load(SBCharacters*,void*,unsigned int,unsigned int,const SBSaveBackend*);
int sb_character_save(SBCharacters*,void*,unsigned int,unsigned int,const SBSaveBackend*);
int sb_character_select(SBCharacters*,void*,unsigned int,unsigned int,unsigned int);
int sb_character_select_secondary(SBCharacters*,void*,unsigned int,unsigned int,unsigned int);
void sb_character_forget(SBCharacters*,void*);
int sb_character_handle_begin(SBCharacters*,void*,unsigned int,unsigned int);
void sb_character_handle_end(SBCharacters*,void*,unsigned int,unsigned int);
#endif

#ifndef ELEMENTAL_CHARACTERS_H
#define ELEMENTAL_CHARACTERS_H
#include "elemental_save.h"
#define EE_CHARACTER_LIMIT 512
typedef struct EECharacterRecord {
    void* character;
    unsigned int index,serial;
    EEAffinity affinity;
    int read_only;
    unsigned int handle_change_depth;
} EECharacterRecord;
typedef struct EECharacters { EECharacterRecord records[EE_CHARACTER_LIMIT]; } EECharacters;
/* Caller serializes access. Pointer and handle are jointly used to avoid
 * carrying a dead character's state onto a recycled game object. */
void ee_characters_init(EECharacters*);
/* Optional entropy for newly created records only; never used for loaded data. */
void ee_characters_set_seed_provider(unsigned int (*provider)(unsigned int));
EECharacterRecord* ee_character_observe(EECharacters*,void*,unsigned int,unsigned int,unsigned int,unsigned int);
int ee_character_load(EECharacters*,void*,unsigned int,unsigned int,const EESaveBackend*);
int ee_character_save(EECharacters*,void*,unsigned int,unsigned int,const EESaveBackend*);
int ee_character_select(EECharacters*,void*,unsigned int,unsigned int,unsigned int);
int ee_character_select_secondary(EECharacters*,void*,unsigned int,unsigned int,unsigned int);
void ee_character_forget(EECharacters*,void*);
int ee_character_handle_begin(EECharacters*,void*,unsigned int,unsigned int);
void ee_character_handle_end(EECharacters*,void*,unsigned int,unsigned int);
#endif

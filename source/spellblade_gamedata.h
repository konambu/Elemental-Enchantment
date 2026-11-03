#ifndef SPELLBLADE_GAMEDATA_H
#define SPELLBLADE_GAMEDATA_H
#include "spellblade_save.h"

/* The Kenshi VC2010 string ABI used by KenshiLib. Only constant, small ASCII
 * keys are constructed here; borrowed names are read with bounds checks. */
typedef struct SBGameString {
    union { char small[16]; const char* heap; } text;
    unsigned long long size,capacity,allocator;
} SBGameString;
typedef struct SBGameDataAPI {
    unsigned char (*list_exists)(void*,const SBGameString*);
    int (*list_size)(void*,const SBGameString*);
    const SBGameString* (*list_name)(void*,const SBGameString*,int);
    const int* (*list_values)(void*,const SBGameString*,int);
    void* (*find_reference)(void*,const SBGameString*,const SBGameString*);
    void (*add_reference)(void*,const SBGameString*,const SBGameString*,int,int,int);
} SBGameDataAPI;
typedef struct SBGameDataContext {void* data;const SBGameDataAPI* api;} SBGameDataContext;
int sb_gamedata_read(void*,int out[3]);
int sb_gamedata_write(void*,const int values[3]);
#endif

#ifndef ELEMENTAL_GAMEDATA_H
#define ELEMENTAL_GAMEDATA_H
#include "elemental_save.h"
#include "elemental_game_string.h"

/** @file elemental_gamedata.h
 * @brief Minimal Kenshi GameData ABI adapter used by the save
 * backend.
 */
typedef struct EEGameDataAPI {
    unsigned char (*list_exists)(void *, const EEGameString *);
    int (*list_size)(void *, const EEGameString *);
    const EEGameString *(*list_name)(void *, const EEGameString *, int);
    const int *(*list_values)(void *, const EEGameString *, int);
    void *(*find_reference)(void *, const EEGameString *, const EEGameString *);
    void (*add_reference)(void *, const EEGameString *, const EEGameString *, int, int, int);
} EEGameDataAPI;
typedef struct EEGameDataContext {
    void *data;
    const EEGameDataAPI *api;
} EEGameDataContext;
/** @brief Read the existing SB_Affinity/SB_Data payload; 0 means absent. */
int ee_gamedata_read(void *, int out[3]);
/** @brief Write the payload while leaving unrelated GameData references intact. */
int ee_gamedata_write(void *, const int values[3]);
#endif

#ifndef ELEMENTAL_GAME_STRING_H
#define ELEMENTAL_GAME_STRING_H

/** @file elemental_game_string.h
 * @brief Kenshi's VC2010 string layout as used by KenshiLib and MyGUI.
 * @details Capacity below 16 stores characters inline; larger strings use
 * a borrowed heap pointer. This layout is part of the game ABI.
 */
typedef struct EEGameString {
    union {
        char small[16];
        const char *heap;
    } text;
    unsigned long long size, capacity, allocator;
} EEGameString;

/** @brief Build a game string, copying short text into inline storage.
 * @param out Destination string object; must not be NULL.
 * @param text NUL-terminated text; long text remains borrowed from this pointer.
 * @param length Number of bytes before the terminator.
 * @param capacity Game string capacity; must be at least length.
 * @return 1 on success; 0 for invalid arguments or an invalid capacity.
 */
int ee_game_string_init(EEGameString *out,
                        const char *text,
                        unsigned long long length,
                        unsigned long long capacity);

#endif

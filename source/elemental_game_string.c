#include "elemental_game_string.h"

int ee_game_string_init(EEGameString *out,
                        const char *text,
                        unsigned long long length,
                        unsigned long long capacity) {
    if (!out || !text || capacity < length)
        return 0;
    if (capacity < sizeof(out->text.small)) {
        unsigned long long i;
        volatile char *small = out->text.small;
        for (i = 0; i < length; ++i)
            small[i] = text[i];
        small[length] = '\0';
    } else {
        out->text.heap = text;
    }
    out->size = length;
    out->capacity = capacity;
    out->allocator = 0;
    return 1;
}

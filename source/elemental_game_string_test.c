#include "elemental_game_string.h"

__declspec(dllexport) int ee_run_game_string_tests(void) {
    static const char short_text[] = "123456789012345";
    static const char long_text[] = "1234567890123456";
    EEGameString value;
    unsigned int i;
    if (!ee_game_string_init(&value, "", 0, 15) || value.text.small[0] != '\0')
        return 1;
    if (!ee_game_string_init(&value, short_text, 15, 15) || value.size != 15 ||
        value.capacity != 15)
        return 2;
    for (i = 0; i <= 15; ++i)
        if (value.text.small[i] != short_text[i])
            return 3;
    if (!ee_game_string_init(&value, long_text, 16, 16) || value.text.heap != long_text ||
        value.size != 16 || value.capacity != 16)
        return 4;
    if (ee_game_string_init(&value, long_text, 16, 15) ||
        ee_game_string_init(0, long_text, 16, 16) ||
        ee_game_string_init(&value, 0, 0, 15))
        return 5;
    return 0;
}

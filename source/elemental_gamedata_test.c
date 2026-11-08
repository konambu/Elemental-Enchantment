#include "elemental_gamedata.h"

typedef struct TestEntry {
    EEGameString name;
    int values[3];
} TestEntry;
typedef struct TestData {
    int exists, count, adds;
    TestEntry entries[3];
} TestData;

static unsigned char list_exists(void *data, const EEGameString *name) {
    (void)name;
    return (unsigned char)((TestData *)data)->exists;
}
static int list_size(void *data, const EEGameString *name) {
    (void)name;
    return ((TestData *)data)->count;
}
static const EEGameString *entry_name(void *data, const EEGameString *name, int index) {
    (void)name;
    return &((TestData *)data)->entries[index].name;
}
static const int *entry_values(void *data, const EEGameString *name, int index) {
    (void)name;
    return ((TestData *)data)->entries[index].values;
}
static void *find_reference(void *data, const EEGameString *list, const EEGameString *name) {
    TestData *test = (TestData *)data;
    int i;
    (void)list;
    (void)name;
    for (i = 0; i < test->count && i < 3; ++i)
        if (test->entries[i].name.size == 7 && test->entries[i].name.text.small[0] == 'S')
            return test->entries[i].values;
    return 0;
}
static void add_reference(void *data, const EEGameString *list, const EEGameString *name,
                          int a, int b, int c) {
    TestData *test = (TestData *)data;
    TestEntry *entry = &test->entries[test->count++];
    (void)list;
    entry->name = *name;
    entry->values[0] = a;
    entry->values[1] = b;
    entry->values[2] = c;
    test->exists = 1;
    ++test->adds;
}

__declspec(dllexport) int ee_run_gamedata_tests(void) {
    static TestData data;
    EEGameDataAPI api = {list_exists, list_size, entry_name, entry_values, find_reference,
                         add_reference};
    EEGameDataContext context = {&data, &api};
    int values[3] = {11, 22, 33}, out[3];
    if (ee_gamedata_read(&context, out) != 0 || !ee_gamedata_write(&context, values) ||
        data.adds != 1 || ee_gamedata_read(&context, out) != 1 || out[0] != 11 || out[2] != 33)
        return 1;
    data.entries[1].name = data.entries[0].name;
    ee_game_string_init(&data.entries[0].name, "Other", 5, 15);
    data.entries[0].values[0] = 99;
    data.count = 2;
    values[0] = 44;
    if (!ee_gamedata_write(&context, values) || data.adds != 1 ||
        data.entries[0].values[0] != 99 || data.entries[1].values[0] != 44)
        return 2;
    data.entries[2].name = data.entries[1].name;
    data.count = 3;
    if (ee_gamedata_read(&context, out) != -1 || ee_gamedata_write(&context, values) ||
        data.adds != 1)
        return 3;
    data.count = 513;
    if (ee_gamedata_read(&context, out) != -1 || ee_gamedata_write(&context, values))
        return 4;
    data.count = 1;
    data.entries[0].name.size = 7;
    data.entries[0].name.capacity = 2;
    if (ee_gamedata_read(&context, out) != -1 || ee_gamedata_write(&context, values) ||
        data.adds != 1 || data.entries[0].values[0] != 99)
        return 5;
    return 0;
}

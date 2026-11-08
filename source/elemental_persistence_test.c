#include "elemental_persistence.h"

typedef struct MockRead {
    int status;
    int values[3];
} MockRead;

static int mock_read(void *context, int out[3]) {
    MockRead *mock = (MockRead *)context;
    int i;
    if (mock->status <= 0)
        return mock->status;
    for (i = 0; i < 3; ++i)
        out[i] = mock->values[i];
    return 1;
}

__declspec(dllexport) int ee_run_persistence_tests(void) {
    static EEPersistence persistence;
    MockRead mock = {1, {0, 0, 0}};
    EESaveBackend backend = {&mock, mock_read, 0};
    EEAffinity saved;
    EECharacterRecord *record;
    void *stats = (void *)1;
    void *character = (void *)2;

    ee_persistence_init(&persistence);
    if (!ee_affinity_create(&saved, 123, 4) || !ee_affinity_encode(&saved, mock.values))
        return 1;
    if (ee_persistence_stage(&persistence, stats, &backend) != EE_SAVE_OK)
        return 2;
    mock.status = 0;
    if (ee_persistence_stage(&persistence, stats, &backend) != EE_SAVE_MISSING)
        return 3;
    record = ee_persistence_bind(&persistence, stats, character, 42, 7, 123, 2);
    if (!record || record->read_only || record->affinity.selected != 2)
        return 4;

    ee_persistence_init(&persistence);
    mock.status = -1;
    if (ee_persistence_stage(&persistence, stats, &backend) != EE_SAVE_ERROR)
        return 5;
    record = ee_persistence_bind(&persistence, stats, character, 42, 7, 123, 2);
    if (!record || !record->read_only)
        return 6;
    return 0;
}

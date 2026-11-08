#include "elemental_characters.h"
static unsigned int (*new_record_seed)(unsigned int);
void ee_characters_set_seed_provider(unsigned int (*provider)(unsigned int)) {
    new_record_seed = provider;
}
static void clear(EECharacterRecord *r) {
    r->character = 0;
    r->index = 0;
    r->serial = 0;
    r->read_only = 0;
    r->handle_change_depth = 0;
}
void ee_characters_init(EECharacters *c) {
    int i;
    if (c)
        for (i = 0; i < EE_CHARACTER_LIMIT; ++i)
            clear(&c->records[i]);
}
static EECharacterRecord *
lookup(EECharacters *c, void *ch, unsigned int index, unsigned int serial, int create) {
    int i, free_slot = -1;
    if (!c || !ch)
        return 0;
    for (i = 0; i < EE_CHARACTER_LIMIT; ++i) {
        EECharacterRecord *r = &c->records[i];
        if (r->character == ch) {
            if ((r->index == index && r->serial == serial) || r->handle_change_depth)
                return r;
            clear(r);
            if (free_slot < 0)
                free_slot = i;
        } else if (!r->character && free_slot < 0)
            free_slot = i;
    }
    if (!create || free_slot < 0)
        return 0;
    c->records[free_slot].character = ch;
    c->records[free_slot].index = index;
    c->records[free_slot].serial = serial;
    c->records[free_slot].read_only = 0;
    c->records[free_slot].handle_change_depth = 0;
    return &c->records[free_slot];
}
EECharacterRecord *ee_character_observe(EECharacters *c,
                                        void *ch,
                                        unsigned int index,
                                        unsigned int serial,
                                        unsigned int seed,
                                        unsigned int initial_element) {
    EECharacterRecord *r = lookup(c, ch, index, serial, 0);
    if (r)
        return r;
    if (initial_element > 5)
        return 0;
    r = lookup(c, ch, index, serial, 1);
    if (!r)
        return 0;
    if (new_record_seed)
        seed = new_record_seed(seed);
    if (!ee_affinity_create(&r->affinity, seed, initial_element)) {
        clear(r);
        return 0;
    }
    return r;
}
int ee_character_load(EECharacters *c,
                      void *ch,
                      unsigned int index,
                      unsigned int serial,
                      const EESaveBackend *backend) {
    EEAffinity loaded;
    EECharacterRecord *r;
    int status = ee_save_load(backend, &loaded);
    if (status == EE_SAVE_ERROR || status == EE_SAVE_MISSING)
        return status;
    r = lookup(c, ch, index, serial, 1);
    if (!r)
        return EE_SAVE_ERROR;
    if (status == EE_SAVE_UNSUPPORTED) {
        ee_affinity_create(&r->affinity, index ^ serial, 0);
        r->read_only = 1;
        return status;
    }
    r->affinity = loaded;
    r->read_only = 0;
    return EE_SAVE_OK;
}
int ee_character_save(EECharacters *c,
                      void *ch,
                      unsigned int index,
                      unsigned int serial,
                      const EESaveBackend *backend) {
    EECharacterRecord *r = lookup(c, ch, index, serial, 0);
    if (!r)
        return EE_SAVE_MISSING;
    if (r->read_only)
        return EE_SAVE_UNSUPPORTED;
    return ee_save_store(backend, &r->affinity);
}
int ee_character_select(
    EECharacters *c, void *ch, unsigned int index, unsigned int serial, unsigned int element) {
    EECharacterRecord *r = lookup(c, ch, index, serial, 0);
    return r && !r->read_only && ee_affinity_select(&r->affinity, element);
}
int ee_character_select_secondary(
    EECharacters *c, void *ch, unsigned int index, unsigned int serial, unsigned int element) {
    EECharacterRecord *r = lookup(c, ch, index, serial, 0);
    return r && !r->read_only && ee_affinity_select_secondary(&r->affinity, element);
}
void ee_character_forget(EECharacters *c, void *ch) {
    int i;
    if (!c || !ch)
        return;
    for (i = 0; i < EE_CHARACTER_LIMIT; ++i)
        if (c->records[i].character == ch)
            clear(&c->records[i]);
}
int ee_character_handle_begin(EECharacters *c, void *ch, unsigned int index, unsigned int serial) {
    EECharacterRecord *r = lookup(c, ch, index, serial, 0);
    if (!r)
        return 0;
    ++r->handle_change_depth;
    return 1;
}
void ee_character_handle_end(EECharacters *c, void *ch, unsigned int index, unsigned int serial) {
    int i;
    if (!c || !ch)
        return;
    for (i = 0; i < EE_CHARACTER_LIMIT; ++i) {
        EECharacterRecord *r = &c->records[i];
        if (r->character == ch && r->handle_change_depth) {
            r->index = index;
            r->serial = serial;
            --r->handle_change_depth;
            return;
        }
    }
}

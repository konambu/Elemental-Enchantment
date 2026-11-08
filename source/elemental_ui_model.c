#include "elemental_ui_model.h"
void ee_ui_reset(EEUIModel *u) {
    if (!u)
        return;
    u->window = 0;
    u->character = 0;
    u->index = 0;
    u->serial = 0;
    u->visible = 0;
    u->editable = 0;
    u->has_affinity = 0;
}
void ee_ui_bind(EEUIModel *u,
                void *window,
                void *character,
                unsigned int index,
                unsigned int serial,
                const EEAffinity *affinity,
                int editable) {
    if (!u)
        return;
    if (!window || !character || !affinity) {
        ee_ui_reset(u);
        return;
    }
    if (u->window != window)
        u->visible = 0;
    u->window = window;
    u->character = character;
    u->index = index;
    u->serial = serial;
    u->affinity = *affinity;
    u->editable = editable ? 1 : 0;
    u->has_affinity = 1;
}
int ee_ui_toggle(EEUIModel *u) {
    if (!u || !u->window || !u->character || !u->has_affinity)
        return 0;
    u->visible = !u->visible;
    return 1;
}
int ee_ui_request_selection(const EEUIModel *u,
                            unsigned int element,
                            void **character,
                            unsigned int *index,
                            unsigned int *serial) {
    if (!u || !u->window || !u->character || !u->visible || !u->editable || !u->has_affinity ||
        element > 5 || !character || !index || !serial)
        return 0;
    *character = u->character;
    *index = u->index;
    *serial = u->serial;
    return 1;
}
void ee_ui_forget_window(EEUIModel *u, void *window) {
    if (u && u->window == window)
        ee_ui_reset(u);
}

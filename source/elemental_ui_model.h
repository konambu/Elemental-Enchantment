#ifndef ELEMENTAL_UI_MODEL_H
#define ELEMENTAL_UI_MODEL_H
#include "elemental_affinity.h"
/** @file elemental_ui_model.h
 * @brief UI state transitions independent of MyGUI and game
 * widgets.
 */
/* View state only; it does not dereference or own game pointers. */
typedef struct EEUIModel {
    void *window;
    void *character;
    unsigned int index, serial;
    EEAffinity affinity;
    int visible, editable, has_affinity;
} EEUIModel;
void ee_ui_reset(EEUIModel *);
/** @brief Bind a window to the currently selected character and affinity. */
void ee_ui_bind(EEUIModel *,
                void *window,
                void *character,
                unsigned int index,
                unsigned int serial,
                const EEAffinity *,
                int editable);
/** @brief Toggle the panel's visibility; return 1 on success, or 0 if unbound. */
int ee_ui_toggle(EEUIModel *);
/* Select validates view state and returns the desired value; the host must
 * commit it to its
 * character store. It does not mutate saved state itself. */
int ee_ui_request_selection(const EEUIModel *,
                            unsigned int element,
                            void **character,
                            unsigned int *index,
                            unsigned int *serial);
void ee_ui_forget_window(EEUIModel *, void *window);
#endif

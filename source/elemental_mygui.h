#ifndef ELEMENTAL_MYGUI_H
#define ELEMENTAL_MYGUI_H
#include "elemental_ui_view.h"
#include "elemental_gamedata.h"
typedef struct EEMyGUICoord {
    int left, top, width, height;
} EEMyGUICoord;
typedef struct EEMyGUI {
    void *(*gui_instance)(void);
    void *(*create_widget)(void *,
                           const EEGameString *,
                           const EEGameString *,
                           const EEMyGUICoord *,
                           int,
                           const EEGameString *);
    void (*destroy_widget)(void *, void *);
    void (*set_visible)(void *, unsigned char);
    void (*set_caption)(void *, const EEGameString *);
    void (*set_selected)(void *, unsigned char);
    unsigned char (*canvas_created)(void *);
    void (*canvas_create)(void *, int, int, int, int, int);
    int (*canvas_width)(void *);
    int (*canvas_height)(void *);
    void *(*canvas_lock)(void *, int);
    void (*canvas_unlock)(void *);
    void (*set_font)(void *, const EEGameString *);
    void (*set_style)(void *, int, const EEGameString *);
    void (*set_size)(void *, int, int);
    void (*set_position)(void *, int, int);
    void (*set_property)(void *, const EEGameString *, const EEGameString *);
    int (*get_font_height)(void *);
    void (*set_font_height)(void *, int);
    unsigned int scale_tenths;
    unsigned int scale_x100, scale_y100;
} EEMyGUI;
/* Construct the generic view backend for the supplied resolved API. UI calls
 * must run on the
 * MyGUI thread. */
void ee_mygui_backend(EEMyGUI *, EEUIViewAPI *);
#endif

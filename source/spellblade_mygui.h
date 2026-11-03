#ifndef SPELLBLADE_MYGUI_H
#define SPELLBLADE_MYGUI_H
#include "spellblade_ui_view.h"
#include "spellblade_gamedata.h"
typedef struct SBMyGUICoord {int left,top,width,height;} SBMyGUICoord;
typedef struct SBMyGUI {
    void* (*gui_instance)(void);
    void* (*create_widget)(void*,const SBGameString*,const SBGameString*,const SBMyGUICoord*,int,const SBGameString*);
    void (*destroy_widget)(void*,void*);
    void (*set_visible)(void*,unsigned char);
    void (*set_caption)(void*,const SBGameString*);
    void (*set_selected)(void*,unsigned char);
    unsigned char (*canvas_created)(void*);
    void (*canvas_create)(void*,int,int,int,int,int);
    int (*canvas_width)(void*);
    int (*canvas_height)(void*);
    void* (*canvas_lock)(void*,int);
    void (*canvas_unlock)(void*);
    void (*set_font)(void*,const SBGameString*);
    void (*set_style)(void*,int,const SBGameString*);
    void (*set_size)(void*,int,int);
    void (*set_property)(void*,const SBGameString*,const SBGameString*);
} SBMyGUI;
/* Construct the generic view backend for the supplied resolved API. UI calls
 * must run on the MyGUI thread. */
void sb_mygui_backend(SBMyGUI*,SBUIViewAPI*);
#endif

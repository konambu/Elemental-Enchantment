#ifndef ELEMENTAL_UI_VIEW_H
#define ELEMENTAL_UI_VIEW_H
#include "elemental_ui_model.h"
typedef struct EEUIViewAPI {
    void* context;
    void* (*create)(void*,void* parent,int kind,int x,int y,int w,int h);
    void (*destroy)(void*,void* widget);
    void (*visible)(void*,void* widget,int visible);
    void (*caption)(void*,void* widget,const char* text);
    void (*selected)(void*,void* widget,int selected);
    int (*radar)(void*,void* widget,const EEAffinity*);
    void (*progress)(void*,void* widget,unsigned int percent);
} EEUIViewAPI;
enum { EE_UI_PANEL=1,EE_UI_BUTTON=2,EE_UI_LABEL=3,EE_UI_RADAR=4,EE_UI_BAR_BG=5,EE_UI_BAR_FILL=6,EE_UI_WINDOW_BODY=7,EE_UI_WINDOW_HEADER=8,EE_UI_WINDOW_TITLE=9,EE_UI_WINDOW_CLOSE=10,EE_UI_AXIS_LABEL=11,EE_UI_PAPER=12,EE_UI_BAR_TEXT=13,EE_UI_OPENER=14 };
typedef struct EEUIView {
    EEUIModel model;
    void* parent;void* opener;void* panel;void* body;void* paper;void* header;void* window_title;void* graph;void* close;void* status;void* bar;void* fill;void* title;void* effect;void* detail;
    void* labels[5];void* axis_labels[5];void* buttons[6];void* mode_main;void* mode_second;void* milestone;void* xp_label;void* xp_percent;int second_mode;int graph_ready;unsigned int built_scale_tenths;EEAffinity graph_state;
} EEUIView;
void ee_ui_view_init(EEUIView*);
void ee_ui_view_set_effect_multiplier(float multiplier);
void ee_ui_view_set_scale(unsigned int tenths);
void ee_ui_view_set_geometry_scale(unsigned int x,unsigned int y,unsigned int manual);
int ee_ui_view_build(EEUIView*,const EEUIViewAPI*,void* parent,int width,int height);
void ee_ui_view_dispose(EEUIView*,const EEUIViewAPI*);
void ee_ui_view_sync(EEUIView*,const EEUIViewAPI*,void* window,void* character,unsigned int index,unsigned int serial,const EEAffinity*,int editable);
/* Returns 0 for no selection, 1..6 for requested element 0..5. */
int ee_ui_view_click(EEUIView*,const EEUIViewAPI*,void* widget);
#endif

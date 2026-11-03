#ifndef SPELLBLADE_UI_VIEW_H
#define SPELLBLADE_UI_VIEW_H
#include "spellblade_ui_model.h"
typedef struct SBUIViewAPI {
    void* context;
    void* (*create)(void*,void* parent,int kind,int x,int y,int w,int h);
    void (*destroy)(void*,void* widget);
    void (*visible)(void*,void* widget,int visible);
    void (*caption)(void*,void* widget,const char* text);
    void (*selected)(void*,void* widget,int selected);
    int (*radar)(void*,void* widget,const SBAffinity*);
    void (*progress)(void*,void* widget,unsigned int percent);
} SBUIViewAPI;
enum { SB_UI_PANEL=1,SB_UI_BUTTON=2,SB_UI_LABEL=3,SB_UI_RADAR=4,SB_UI_BAR_BG=5,SB_UI_BAR_FILL=6,SB_UI_WINDOW_BODY=7,SB_UI_WINDOW_HEADER=8,SB_UI_WINDOW_TITLE=9,SB_UI_WINDOW_CLOSE=10,SB_UI_AXIS_LABEL=11,SB_UI_PAPER=12,SB_UI_BAR_TEXT=13 };
typedef struct SBUIView {
    SBUIModel model;
    void* parent;void* opener;void* panel;void* body;void* paper;void* header;void* window_title;void* graph;void* close;void* status;void* bar;void* fill;void* title;void* effect;void* detail;
    void* labels[5];void* axis_labels[5];void* buttons[6];void* mode_main;void* mode_second;void* milestone;void* xp_label;void* xp_percent;int second_mode;int graph_ready;SBAffinity graph_state;
} SBUIView;
void sb_ui_view_init(SBUIView*);
int sb_ui_view_build(SBUIView*,const SBUIViewAPI*,void* parent,int width,int height);
void sb_ui_view_dispose(SBUIView*,const SBUIViewAPI*);
void sb_ui_view_sync(SBUIView*,const SBUIViewAPI*,void* window,void* character,unsigned int index,unsigned int serial,const SBAffinity*,int editable);
/* Returns 0 for no selection, 1..6 for requested element 0..5. */
int sb_ui_view_click(SBUIView*,const SBUIViewAPI*,void* widget);
#endif

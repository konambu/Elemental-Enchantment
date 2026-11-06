#include "spellblade_ui_model.h"
void sb_ui_reset(SBUIModel* u) {if(!u)return;u->window=0;u->character=0;u->index=0;u->serial=0;u->visible=0;u->editable=0;u->has_affinity=0;}
void sb_ui_bind(SBUIModel* u,void* window,void* character,unsigned int index,unsigned int serial,const SBAffinity* affinity,int editable)
{
    if(!u)return;
    if(!window||!character||!affinity){sb_ui_reset(u);return;}
    if(u->window!=window)u->visible=0;
    u->window=window;u->character=character;u->index=index;u->serial=serial;
    u->affinity=*affinity;u->editable=editable?1:0;u->has_affinity=1;
}
int sb_ui_toggle(SBUIModel* u){if(!u||!u->window||!u->character||!u->has_affinity)return 0;u->visible=!u->visible;return 1;}
int sb_ui_request_selection(const SBUIModel* u,unsigned int element,void** character,unsigned int* index,unsigned int* serial)
{
    if(!u||!u->window||!u->character||!u->visible||!u->editable||!u->has_affinity||element>5||!character||!index||!serial)return 0;
    *character=u->character;*index=u->index;*serial=u->serial;return 1;
}
void sb_ui_forget_window(SBUIModel* u,void* window){if(u&&u->window==window)sb_ui_reset(u);}

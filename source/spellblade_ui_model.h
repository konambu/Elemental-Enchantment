#ifndef SPELLBLADE_UI_MODEL_H
#define SPELLBLADE_UI_MODEL_H
#include "spellblade_affinity.h"
/* View state only; it does not dereference or own game pointers. */
typedef struct SBUIModel {
    void* window;
    void* character;
    unsigned int index,serial;
    SBAffinity affinity;
    int visible,editable,has_affinity;
} SBUIModel;
void sb_ui_reset(SBUIModel*);
void sb_ui_bind(SBUIModel*,void* window,void* character,unsigned int index,unsigned int serial,const SBAffinity*,int editable);
int sb_ui_toggle(SBUIModel*);
/* Select validates view state and returns the desired value; the host must
 * commit it to its character store. It does not mutate saved state itself. */
int sb_ui_request_selection(const SBUIModel*,unsigned int element,void** character,unsigned int* index,unsigned int* serial);
void sb_ui_forget_window(SBUIModel*,void* window);
#endif

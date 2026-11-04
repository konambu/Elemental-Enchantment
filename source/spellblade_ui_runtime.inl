/* Experimental native UI. All UI methods run from MyGUI / stats-window hooks.
 * The host hooks window clear/destruction before the native children die. */
#define SB_UI_WINDOWS 16
typedef struct SBStatsView {void* window;SBUIView view;} SBStatsView;
static SBStatsView stats_views[SB_UI_WINDOWS];
static SBMyGUI mygui_api;
static SBUIViewAPI ui_api;
static int (*stats_window_width)(void*);
static int (*stats_window_height)(void*);
static unsigned char (*stats_window_visible)(void*);
static void* (*input_focus)(void*);
static void* (*widget_parent)(void*);
static void (*original_stats_window_update)(void*);
static void (*original_stats_window_clear)(void*);
static void (*original_stats_window_destroy)(void*);
static unsigned char (*original_mouse_press)(void*,int,int,int);
static int ui_failed,ui_locale_ready;
static void* (*locale_manager_instance)(void);
static void* (*locale_current)(void*);
static void select_game_locale(void) {
    void* manager;void* locale;const String* id;const char* text;
    if(ui_locale_ready||!locale_manager_instance||!locale_current)return;
    __try {manager=locale_manager_instance();if(!manager)return;locale=locale_current(manager);if(!locale)return;id=(const String*)((unsigned char*)locale+8);if(id->size==0||id->size>32||id->capacity<id->size)return;text=id->capacity<16?id->text.small:id->text.heap;if(!text)return;sb_locale_set(text);ui_locale_ready=1;}__except(1){ui_locale_ready=1;}
}
static int ui_find(void* window) {int i;for(i=0;i<SB_UI_WINDOWS;++i)if(stats_views[i].window==window)return i;return -1;}
static int ui_find_or_add(void* window) {int i=ui_find(window);if(i>=0)return i;for(i=0;i<SB_UI_WINDOWS;++i)if(!stats_views[i].window){stats_views[i].window=window;sb_ui_view_init(&stats_views[i].view);return i;}return -1;}
static void ui_forget(void* window) {int i=ui_find(window);if(i>=0){sb_ui_view_init(&stats_views[i].view);stats_views[i].window=0;}}
static void stats_window_update_hook(void* window)
{
    void* root;void* character;void* stats;const unsigned char* h;unsigned int index,serial_number;int i,editable=0,have=0;SBAffinity affinity;
    original_stats_window_update(window);
    if(!active||ui_failed||!window||!mygui_api.gui_instance||!mygui_api.gui_instance()||!stats_window_visible(window))return;
    __try {
        select_game_locale();
        root=*(void**)((unsigned char*)window+0x08);character=*(void**)((unsigned char*)window+0xd0);
        if(!root||stats_window_width(window)<460||stats_window_height(window)<380)return;
        i=ui_find_or_add(window);if(i<0)return;
        if(!sb_ui_view_build(&stats_views[i].view,&ui_api,root,stats_window_width(window),stats_window_height(window))){ui_failed=1;logline("UI: failed to create Spellblade controls.\r\n");return;}
        if(character&&character_is_player(character)){
            h=(const unsigned char*)character_handle(character);stats=character_stats(character);
            if(h){index=*(const unsigned int*)(h+0x14);serial_number=*(const unsigned int*)(h+0x18);
                AcquireSRWLockExclusive(&guard);
                __try {SBCharacterRecord* r=sb_persistence_bind(&persistence,stats,character,index,serial_number,index^(serial_number*0x9e3779b9u),(unsigned int)element_from_character(character));if(r){affinity=r->affinity;editable=!r->read_only;have=1;}}
                __finally {ReleaseSRWLockExclusive(&guard);}
            }
        }
        if(have&&all_aptitudes){int rate_index;for(rate_index=0;rate_index<5;++rate_index)affinity.rates[rate_index]=100;}
        if(have)sb_ui_view_sync(&stats_views[i].view,&ui_api,window,character,index,serial_number,&affinity,editable);
        else sb_ui_view_sync(&stats_views[i].view,&ui_api,0,0,0,0,0,0);
    } __except(1) {ui_failed=1;logline("UI: disabled after native UI error.\r\n");}
}
static void stats_window_clear_hook(void* window){ui_forget(window);original_stats_window_clear(window);}
static void stats_window_destroy_hook(void* window){ui_forget(window);original_stats_window_destroy(window);}
static unsigned char mouse_press_hook(void* input,int x,int y,int button)
{
    unsigned char result=original_mouse_press(input,x,y,button);void* focus;int i,depth;
    if(!active||ui_failed||button!=0||!input)return result;
    __try {focus=input_focus(input);for(depth=0;focus&&depth<12;++depth){
        for(i=0;i<SB_UI_WINDOWS;++i)if(stats_views[i].window){SBUIView* view=&stats_views[i].view;int action=sb_ui_view_click(view,&ui_api,focus);
            if(action){void* character;unsigned int index,serial_number;
                unsigned int element=(unsigned int)((action-1)%6);
                if(sb_ui_request_selection(&view->model,element,&character,&index,&serial_number)){
                    int accepted,k;AcquireSRWLockExclusive(&guard);__try {accepted=action>6?sb_character_select_secondary(&persistence.characters,character,index,serial_number,element):sb_character_select(&persistence.characters,character,index,serial_number,element);if(accepted){SBCharacterRecord* r=sb_character_observe(&persistence.characters,character,index,serial_number,0,element);if(r)for(k=0;k<SB_ALLY_LIMIT;++k)if(allies[k].character==character){allies[k].element=(int)r->affinity.selected;allies[k].secondary=(int)r->affinity.secondary;break;}}} __finally {ReleaseSRWLockExclusive(&guard);}
                }return result;
            }
            if(focus==view->opener||focus==view->close||focus==view->mode_main||focus==view->mode_second)return result;
        }
        focus=widget_parent(focus);
    }} __except(1) {ui_failed=1;logline("UI: disabled after mouse input error.\r\n");}
    return result;
}

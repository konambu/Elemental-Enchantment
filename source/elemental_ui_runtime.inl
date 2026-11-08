/* Experimental native UI. All UI methods run from MyGUI / stats-window hooks.
 * The host hooks window clear/destruction before the native children die. */
#define EE_UI_WINDOWS 16
typedef struct EEStatsView {void* window;EEUIView view;} EEStatsView;
static EEStatsView stats_views[EE_UI_WINDOWS];
static EEMyGUI mygui_api;
static EEUIViewAPI ui_api;
static int (*stats_window_width)(void*);
static int (*stats_window_height)(void*);
static unsigned char (*stats_window_visible)(void*);
static void* (*input_focus)(void*);
static void* (*widget_parent)(void*);
static void (*original_stats_window_update)(void*);
static void (*original_stats_window_clear)(void*);
static void (*original_stats_window_destroy)(void*);
static unsigned char (*original_mouse_press)(void*,int,int,int);
static unsigned char (*original_mouse_move)(void*,int,int,int);
static unsigned char (*original_mouse_release)(void*,int,int,int);
static int ui_dragging,ui_drag_x,ui_drag_y,ui_offset_x,ui_offset_y;
static void* ui_drag_window;
static void ui_position_panel(EEStatsView* s) {
    unsigned int sx,sy,scale;int x,y;
    if(!s||!s->window||!s->view.panel||!mygui_api.set_position)return;
    sx=mygui_api.scale_x100?mygui_api.scale_x100:100;sy=mygui_api.scale_y100?mygui_api.scale_y100:100;
    scale=mygui_api.scale_tenths>=8&&mygui_api.scale_tenths<=20?mygui_api.scale_tenths:10;
    sx=sx*scale/10;sy=sy*scale/10;
    x=stats_window_width(s->window)+(12*(int)sx+50)/100+ui_offset_x;
    y=(10*(int)sy+50)/100+ui_offset_y;
    mygui_api.set_position(s->view.panel,x,y);
}
static int ui_failed,ui_locale_ready,ui_panel_preference=1;
static void** ui_game_gui_slot;
#ifdef ELEMENTAL_NATIVE_TEST
static float ui_test_native_x=1.0f,ui_test_native_y=1.0f;
#endif
static void ui_game_scale(unsigned int* sx,unsigned int* sy){
    float x=1.0f,y=1.0f;*sx=100;*sy=100;
#ifdef ELEMENTAL_NATIVE_TEST
    x=ui_test_native_x;y=ui_test_native_y;
#else
    if(!ui_game_gui_slot){MODULE lib=GetModuleHandleA("KenshiLib.dll");if(lib)ui_game_gui_slot=(void**)GetProcAddress(lib,"?gui@@3PEAVForgottenGUI@@EA");}
    if(ui_game_gui_slot&&*ui_game_gui_slot){void* gui=*ui_game_gui_slot;x=*(float*)((unsigned char*)gui+0x3c);y=*(float*)((unsigned char*)gui+0x40);}
#endif
    if(x>=0.4f&&x<=3.0f&&y>=0.4f&&y<=3.0f){*sx=(unsigned int)(x*100.0f+0.5f);*sy=(unsigned int)(y*100.0f+0.5f);}
}
static void* (*locale_manager_instance)(void);
static void* (*locale_current)(void*);
static void select_game_locale(void) {
    void* manager;void* locale;const String* id;const char* text;
    if(ui_locale_ready||!locale_manager_instance||!locale_current)return;
    __try {manager=locale_manager_instance();if(!manager)return;locale=locale_current(manager);if(!locale)return;id=(const String*)((unsigned char*)locale+8);if(id->size==0||id->size>32||id->capacity<id->size)return;text=id->capacity<16?id->text.small:id->text.heap;if(!text)return;ee_locale_set(text);ui_locale_ready=1;}__except(1){ui_locale_ready=1;}
}
static int ui_find(void* window) {int i;for(i=0;i<EE_UI_WINDOWS;++i)if(stats_views[i].window==window)return i;return -1;}
static int ui_find_or_add(void* window) {int i=ui_find(window);if(i>=0)return i;for(i=0;i<EE_UI_WINDOWS;++i)if(!stats_views[i].window){stats_views[i].window=window;ee_ui_view_init(&stats_views[i].view);return i;}return -1;}
static void ui_forget(void* window) {int i=ui_find(window);if(ui_drag_window==window){ui_dragging=0;ui_drag_window=0;}if(i>=0){ee_ui_view_init(&stats_views[i].view);stats_views[i].window=0;}}
static void stats_window_update_hook(void* window)
{
    void* root;void* character;void* stats;const unsigned char* h;unsigned int index,serial_number,sx,sy;int i,editable=0,have=0;EEAffinity affinity;
    original_stats_window_update(window);
    if(!active||ui_failed||!window||!mygui_api.gui_instance||!mygui_api.gui_instance()||!stats_window_visible(window))return;
    __try {
        select_game_locale();
        root=*(void**)((unsigned char*)window+0x08);character=*(void**)((unsigned char*)window+0xd0);
        if(!root||stats_window_width(window)<460||stats_window_height(window)<380)return;
        i=ui_find_or_add(window);if(i<0)return;
        ui_game_scale(&sx,&sy);mygui_api.scale_x100=sx;mygui_api.scale_y100=sy;ee_ui_view_set_geometry_scale(sx,sy,ui_scale_tenths);
        mygui_api.scale_tenths=ui_scale_tenths;
        if(!ee_ui_view_build(&stats_views[i].view,&ui_api,root,stats_window_width(window),stats_window_height(window))){ui_failed=1;logline("UI: failed to create Elemental controls.\r\n");return;}
        if(ui_offset_x||ui_offset_y)ui_position_panel(&stats_views[i]);
        if(character&&character_is_player(character)){
            h=(const unsigned char*)character_handle(character);stats=character_stats(character);
            if(h){index=*(const unsigned int*)(h+0x14);serial_number=*(const unsigned int*)(h+0x18);
                AcquireSRWLockExclusive(&guard);
                __try {EECharacterRecord* r=ee_persistence_bind(&persistence,stats,character,index,serial_number,index^(serial_number*0x9e3779b9u),(unsigned int)element_from_character(character));if(r){affinity=r->affinity;editable=!r->read_only;have=1;}}
                __finally {ReleaseSRWLockExclusive(&guard);}
            }
        }
        if(have&&all_aptitudes){int rate_index;for(rate_index=0;rate_index<5;++rate_index)affinity.rates[rate_index]=100;}
        if(have){EEUIView* view=&stats_views[i].view;ee_ui_view_sync(view,&ui_api,window,character,index,serial_number,&affinity,editable);if(view->model.visible!=ui_panel_preference){view->model.visible=ui_panel_preference;ui_api.visible(ui_api.context,view->panel,view->model.visible);}}
        else ee_ui_view_sync(&stats_views[i].view,&ui_api,0,0,0,0,0,0);
    } __except(1) {ui_failed=1;logline("UI: disabled after native UI error.\r\n");}
}
static void stats_window_clear_hook(void* window){ui_forget(window);original_stats_window_clear(window);}
static void stats_window_destroy_hook(void* window){ui_forget(window);original_stats_window_destroy(window);}
static unsigned char mouse_press_hook(void* input,int x,int y,int button)
{
    unsigned char result=original_mouse_press(input,x,y,button);void* focus;int i,depth;
    if(!active||ui_failed||button!=0||!input)return result;
    __try {focus=input_focus(input);for(depth=0;focus&&depth<12;++depth){
        for(i=0;i<EE_UI_WINDOWS;++i)if(stats_views[i].window){EEUIView* view=&stats_views[i].view;int action=ee_ui_view_click(view,&ui_api,focus);
            if(view->model.visible&&mygui_api.set_position&&(focus==view->header||focus==view->window_title)){ui_dragging=1;ui_drag_x=x;ui_drag_y=y;ui_drag_window=stats_views[i].window;return result;}
            if(action){void* character;unsigned int index,serial_number;
                unsigned int element=(unsigned int)((action-1)%6);
                if(ee_ui_request_selection(&view->model,element,&character,&index,&serial_number)){
                    int accepted,k;AcquireSRWLockExclusive(&guard);__try {accepted=action>6?ee_character_select_secondary(&persistence.characters,character,index,serial_number,element):ee_character_select(&persistence.characters,character,index,serial_number,element);if(accepted){EECharacterRecord* r=ee_character_observe(&persistence.characters,character,index,serial_number,0,element);if(r)for(k=0;k<EE_ALLY_LIMIT;++k)if(allies[k].character==character){allies[k].element=(int)r->affinity.selected;allies[k].secondary=(int)r->affinity.secondary;break;}}} __finally {ReleaseSRWLockExclusive(&guard);}
                }return result;
            }
            if(focus==view->opener||focus==view->close){ui_panel_preference=view->model.visible;return result;}
            if(focus==view->mode_main||focus==view->mode_second)return result;
        }
        focus=widget_parent(focus);
    }} __except(1) {ui_failed=1;logline("UI: disabled after mouse input error.\r\n");}
    return result;
}
static unsigned char mouse_move_hook(void* input,int x,int y,int z){
    unsigned char result=original_mouse_move(input,x,y,z);int i;
    if(!active||ui_failed||!ui_dragging)return result;
    __try {i=ui_find(ui_drag_window);if(i<0||!stats_views[i].view.model.visible){ui_dragging=0;ui_drag_window=0;return result;}
        ui_offset_x+=x-ui_drag_x;ui_offset_y+=y-ui_drag_y;ui_drag_x=x;ui_drag_y=y;
        if(ui_offset_x>3000)ui_offset_x=3000;if(ui_offset_x< -3000)ui_offset_x= -3000;
        if(ui_offset_y>3000)ui_offset_y=3000;if(ui_offset_y< -3000)ui_offset_y= -3000;
        ui_position_panel(&stats_views[i]);
    } __except(1) {ui_dragging=0;ui_drag_window=0;logline("UI: dragging disabled after input error.\r\n");}
    return result;
}
static unsigned char mouse_release_hook(void* input,int x,int y,int button){
    unsigned char result=original_mouse_release(input,x,y,button);
    if(button==0){ui_dragging=0;ui_drag_window=0;}return result;
}

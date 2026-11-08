#include "elemental_ui_model.h"
__declspec(dllexport) int ee_run_ui_model_tests(void)
{
    EEUIModel u;EEAffinity a={2,{20,30,40,50,60}};void* ch;unsigned int index,serial;
    ee_ui_reset(&u);if(ee_ui_toggle(&u))return 1;
    ee_ui_bind(&u,(void*)1,(void*)2,3,4,&a,1);
    if(ee_ui_request_selection(&u,3,&ch,&index,&serial))return 2;
    if(!ee_ui_toggle(&u)||!u.visible||!ee_ui_request_selection(&u,5,&ch,&index,&serial)||ch!=(void*)2||index!=3||serial!=4)return 3;
    if(ee_ui_request_selection(&u,6,&ch,&index,&serial))return 4;
    ee_ui_bind(&u,(void*)1,(void*)2,3,4,&a,0);if(!u.visible||ee_ui_request_selection(&u,1,&ch,&index,&serial))return 5;
    ee_ui_bind(&u,(void*)1,(void*)3,5,6,&a,1);if(!u.visible||!ee_ui_request_selection(&u,1,&ch,&index,&serial)||ch!=(void*)3||index!=5||serial!=6)return 6;
    ee_ui_forget_window(&u,(void*)7);if(!u.visible)return 7;
    ee_ui_forget_window(&u,(void*)1);if(u.window||u.character||u.visible)return 8;
    return 0;
}

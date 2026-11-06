#include "spellblade_ui_view.h"
#include "spellblade_localization.h"
typedef struct Mock {int created,destroyed,radars,visible,selected,captions,opener_y,progress_pct;char effect[256],detail[256],milestone[256],title[256],status[256],xp_label[256],xp_percent[256];} Mock;
static void* create(void* c,void* p,int k,int x,int y,int w,int h){Mock* m=(Mock*)c;(void)p;(void)k;(void)x;(void)w;(void)h;if(!m->created)m->opener_y=y;return (void*)(unsigned long long)(++m->created);}
static void destroy(void* c,void* w){(void)w;++((Mock*)c)->destroyed;}
static void visible(void* c,void* w,int v){(void)w;(void)v;++((Mock*)c)->visible;}
static void caption(void* c,void* w,const char* s){Mock* m=(Mock*)c;int i;char* out=0;if(w==(void*)29)out=m->effect;else if(w==(void*)30)out=m->detail;else if(w==(void*)32)out=m->milestone;else if(w==(void*)28)out=m->title;else if(w==(void*)9)out=m->status;else if(w==(void*)34)out=m->xp_label;else if(w==(void*)35)out=m->xp_percent;++m->captions;if(out){for(i=0;i<255&&s[i];++i)out[i]=s[i];out[i]=0;}}
static int same(const char* a,const char* b){while(*a&&*a==*b){++a;++b;}return *a==*b;}
static void selected(void* c,void* w,int s){(void)w;if(s)++((Mock*)c)->selected;}
static int radar(void* c,void* w,const SBAffinity* s){(void)w;(void)s;++((Mock*)c)->radars;return 1;}
static void bar(void* c,void* w,unsigned int pct){(void)w;((Mock*)c)->progress_pct=(int)pct;}
__declspec(dllexport) int sb_run_ui_view_tests(void)
{
    static Mock m;SBUIViewAPI api={&m,create,destroy,visible,caption,selected,radar,bar};SBUIView view;SBAffinity a={2,{10,20,30,40,50}};
    sb_ui_view_init(&view);if(!sb_ui_view_build(&view,&api,(void*)100,700,500)||m.created!=35||m.opener_y!=260)return 1;
    a.xp[0]=40;
    sb_ui_view_sync(&view,&api,(void*)101,(void*)102,1,2,&a,1);if(view.model.visible||m.radars)return 2;
    if(sb_ui_view_click(&view,&api,view.buttons[1]))return 3;
    sb_ui_view_click(&view,&api,view.opener);if(!view.model.visible)return 4;
    sb_ui_view_sync(&view,&api,(void*)101,(void*)102,1,2,&a,1);if(m.radars!=1||m.selected!=2||m.progress_pct!=50||!same(m.milestone,"LV50 0/50")||!same(m.title,"MAIN  Lightning     SUB  Locked")||!same(m.status,"ELEMENTAL   0")||!same(m.xp_label,"Next Level")||!same(m.xp_percent,"50%"))return 5;
    sb_ui_view_sync(&view,&api,(void*)101,(void*)102,1,2,&a,1);if(m.radars!=1)return 9;
    if(sb_ui_view_click(&view,&api,view.buttons[4])!=5)return 6;
    a.selected=4;sb_ui_view_sync(&view,&api,(void*)101,(void*)102,1,2,&a,1);{const char* expected="Cutting damage +5.0%";int j=0;while(expected[j]&&m.effect[j]==expected[j])++j;if(expected[j]||m.effect[j])return 10;}
    a.xp[0]=a.xp[1]=a.xp[4]=47600;a.selected=1;sb_ui_view_sync(&view,&api,(void*)101,(void*)102,1,2,&a,1);if(!same(m.effect,"Blunt damage +15.0%")||!same(m.status,"ELEMENTAL   100")||!same(m.xp_label,"MAX LEVEL")||!same(m.xp_percent,"100%"))return 11;
    a.selected=2;sb_ui_view_sync(&view,&api,(void*)101,(void*)102,1,2,&a,1);if(!same(m.effect,"Armour penetration +15.0%")||!same(m.detail,"Attack / bolt speed +5.0%"))return 12;
    a.selected=5;sb_ui_view_sync(&view,&api,(void*)101,(void*)102,1,2,&a,1);if(!same(m.effect,"Bleed multiplier +0.50")||!same(m.detail,"Cutting damage +10.0%"))return 13;
    if(!same(m.milestone,"SUB 75%"))return 17;sb_ui_view_click(&view,&api,view.mode_second);if(!view.second_mode)return 14;if(sb_ui_view_click(&view,&api,view.buttons[2])!=9)return 15;
    a.secondary=2;sb_ui_view_sync(&view,&api,(void*)101,(void*)102,1,2,&a,1);if(!same(m.effect,"Armour penetration +11.3%")||!same(m.detail,"Attack / bolt speed +3.8%")||!same(m.title,"MAIN  Ice     SUB  Lightning"))return 16;
    sb_ui_view_click(&view,&api,view.mode_main);sb_ui_view_sync(&view,&api,(void*)101,(void*)102,1,2,&a,1);if(!same(m.title,"MAIN  Ice     SUB  Lightning"))return 18;
    sb_ui_view_sync(&view,&api,(void*)101,(void*)103,3,4,&a,1);if(!view.model.visible||view.model.character!=(void*)103)return 7;
    sb_ui_view_dispose(&view,&api);if(m.destroyed!=2||view.parent)return 8;
    return 0;
}

__declspec(dllexport) const char* sb_test_locale_render(const char* id,int which,unsigned int element,unsigned int level){
    static Mock m;static SBUIViewAPI api={&m,create,destroy,visible,caption,selected,radar,bar};static SBUIView view;static SBAffinity a;unsigned int i;
    for(i=0;i<sizeof(m);++i)((volatile unsigned char*)&m)[i]=0;for(i=0;i<sizeof(a);++i)((volatile unsigned char*)&a)[i]=0;sb_locale_set(id);a.selected=element;if(level>100)level=100;a.xp[0]=(unsigned short)(4u*(20u*level+level*(level-1u)));for(i=0;i<5;++i)a.rates[i]=100;
    sb_ui_view_init(&view);if(!sb_ui_view_build(&view,&api,(void*)100,700,500))return 0;sb_ui_view_sync(&view,&api,(void*)101,(void*)102,1,2,&a,1);sb_ui_view_click(&view,&api,view.opener);sb_ui_view_sync(&view,&api,(void*)101,(void*)102,1,2,&a,1);
    if(which==0)return m.effect;if(which==1)return m.detail;if(which==2)return m.title;if(which==3)return m.status;return m.xp_label;
}

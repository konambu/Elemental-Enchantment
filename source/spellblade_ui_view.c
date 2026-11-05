#include "spellblade_ui_view.h"
#include "spellblade_localization.h"
static const int ring_order[5]={0,2,4,3,1};
static unsigned int ui_effect_half_steps=2;
void sb_ui_view_set_effect_multiplier(float multiplier){ui_effect_half_steps=!(multiplier>0.0f)?0:multiplier>=5.0f?10:(unsigned int)(multiplier*2.0f+0.5f);}
static int add(char* out,int pos,int cap,const char* text){while(text&&*text&&pos<cap-1)out[pos++]=*text++;out[pos]=0;return pos;}
static int num(char* out,int pos,int cap,unsigned value){char tmp[12];int n=0;do{tmp[n++]=(char)('0'+value%10);value/=10;}while(value&&n<11);while(n&&pos<cap-1)out[pos++]=tmp[--n];out[pos]=0;return pos;}
static void decimal(char* out,int cap,const char* label,unsigned value,int percent){int n=0;n=add(out,n,cap,label);n=add(out,n,cap," +");n=num(out,n,cap,value/10);n=add(out,n,cap,".");n=num(out,n,cap,value%10);if(percent)add(out,n,cap,"%");}
static unsigned int ui_level(const SBAffinity* a){unsigned int lo=0,hi=100;while(lo<hi){unsigned int mid=(lo+hi+1u)/2u;if(a->xp[0]>=4u*(20u*mid+mid*(mid-1u)))lo=mid;else hi=mid-1u;}return lo;}
static unsigned int ui_secondary_percent(unsigned int l){return l<50?0:l<60?25:l<70?35:l<80?45:l<90?55:l<100?65:75;}
static void selection_title(char out[256],const SBAffinity* a,int second_mode)
{const SBLocale* l=sb_locale_get();int i=0;unsigned e=a->selected;(void)second_mode;i=add(out,i,256,l->main);i=add(out,i,256,"  ");i=add(out,i,256,e<=5?l->element[e]:l->element[0]);i=add(out,i,256,"     ");i=add(out,i,256,l->sub);i=add(out,i,256,"  ");e=a->secondary;add(out,i,256,ui_level(a)<50?l->locked:(e<=5?l->element[e]:l->element[0]));}
void sb_ui_view_init(SBUIView* v) {int i;if(!v)return;sb_ui_reset(&v->model);v->parent=0;v->opener=0;v->panel=0;v->body=0;v->paper=0;v->header=0;v->window_title=0;v->graph=0;v->close=0;v->status=0;v->bar=0;v->fill=0;v->title=0;v->effect=0;v->detail=0;v->mode_main=0;v->mode_second=0;v->milestone=0;v->xp_label=0;v->xp_percent=0;v->second_mode=0;v->graph_ready=0;for(i=0;i<5;++i){v->labels[i]=0;v->axis_labels[i]=0;}for(i=0;i<6;++i)v->buttons[i]=0;}
static int valid(const SBUIViewAPI* a){return a&&a->create&&a->destroy&&a->visible&&a->caption&&a->selected&&a->radar&&a->progress;}
void sb_ui_view_dispose(SBUIView* v,const SBUIViewAPI* a){if(!v||!valid(a))return;if(v->panel)a->destroy(a->context,v->panel);if(v->opener)a->destroy(a->context,v->opener);sb_ui_view_init(v);}
int sb_ui_view_build(SBUIView* v,const SBUIViewAPI* a,void* parent,int width,int height)
{
    int i,x;if(!v||!valid(a)||!parent||width<300||height<260)return 0;
    if(v->parent==parent&&v->opener&&v->panel)return 1;
    if(v->parent)sb_ui_view_dispose(v,a);v->parent=parent;
    x=width+12;
    /* Place inside the unused area below the attributes list. The outer
     * window height includes borders and is not the child client height. */
    v->opener=a->create(a->context,parent,SB_UI_BUTTON,15,260,170,26);
    v->panel=a->create(a->context,parent,SB_UI_PANEL,x,10,530,448);
    if(!v->opener||!v->panel)goto fail;
    a->caption(a->context,v->opener,sb_locale_get()->opener);a->visible(a->context,v->panel,0);
    v->body=a->create(a->context,v->panel,SB_UI_WINDOW_BODY,0,38,530,410);
    v->header=a->create(a->context,v->panel,SB_UI_WINDOW_HEADER,0,0,530,38);
    if(!v->body||!v->header)goto fail;
    v->paper=a->create(a->context,v->body,SB_UI_PAPER,8,4,514,400);
    if(!v->paper)goto fail;
    v->window_title=a->create(a->context,v->header,SB_UI_WINDOW_TITLE,18,5,440,28);
    v->close=a->create(a->context,v->header,SB_UI_WINDOW_CLOSE,488,4,31,32);
    v->graph=a->create(a->context,v->body,SB_UI_RADAR,20,50,210,200);
    v->status=a->create(a->context,v->body,SB_UI_LABEL,260,20,240,25);
    if(!v->window_title||!v->graph||!v->close||!v->status)goto fail;a->caption(a->context,v->window_title,"ELEMENTAL ENCHANTMENT");
    {const int xy[5][2]={{100,28},{193,72},{188,230},{20,230},{15,72}};
    for(i=0;i<5;++i){v->axis_labels[i]=a->create(a->context,v->body,i?SB_UI_AXIS_LABEL:SB_UI_LABEL,xy[i][0],xy[i][1],100,22);if(!v->axis_labels[i])goto fail;a->caption(a->context,v->axis_labels[i],sb_locale_get()->element[ring_order[i]+1]);}}
    for(i=0;i<5;++i){v->labels[i]=a->create(a->context,v->body,SB_UI_LABEL,400,57+i*35,110,23);if(!v->labels[i])goto fail;}
    for(i=0;i<6;++i){int bx=260,by=i?52+(i-1)*35:252,bw=i?125:240;v->buttons[i]=a->create(a->context,v->body,SB_UI_BUTTON,bx,by,bw,27);if(!v->buttons[i])goto fail;a->caption(a->context,v->buttons[i],sb_locale_get()->element[i]);}
    v->bar=a->create(a->context,v->body,SB_UI_BAR_BG,20,300,490,14);
    if(!v->bar)goto fail;v->fill=a->create(a->context,v->bar,SB_UI_BAR_FILL,0,0,490,14);if(!v->fill)goto fail;
    v->title=a->create(a->context,v->body,SB_UI_LABEL,20,320,490,22);
    v->effect=a->create(a->context,v->body,SB_UI_LABEL,20,345,490,22);
    v->detail=a->create(a->context,v->body,SB_UI_LABEL,20,370,490,22);
    if(!v->title||!v->effect||!v->detail)goto fail;a->caption(a->context,v->title,"ACTIVE EFFECT");
    v->mode_main=a->create(a->context,v->body,SB_UI_BUTTON,20,265,95,26);
    v->mode_second=a->create(a->context,v->body,SB_UI_BUTTON,120,265,130,26);
    v->milestone=a->create(a->context,v->body,SB_UI_LABEL,20,8,230,18);
    v->xp_label=a->create(a->context,v->body,SB_UI_BAR_TEXT,26,298,180,18);
    v->xp_percent=a->create(a->context,v->body,SB_UI_BAR_TEXT,450,298,60,18);
    if(!v->mode_main||!v->mode_second||!v->milestone||!v->xp_label||!v->xp_percent)goto fail;a->caption(a->context,v->mode_main,sb_locale_get()->main);
    return 1;
fail:sb_ui_view_dispose(v,a);return 0;
}
static void label(char out[32],unsigned int rate){int i=0;out[i++]='X';out[i++]='P';out[i++]=' ';if(rate>=100)out[i++]='1';if(rate>=10)out[i++]=(char)('0'+rate/10%10);out[i++]=(char)('0'+rate%10);out[i++]='%';out[i]=0;}
static void level_text(char out[128],const SBAffinity* a){int i=0;i=add(out,i,128,sb_locale_get()->elemental);i=add(out,i,128,"   ");num(out,i,128,ui_level(a));}
static unsigned int progress(char out[32],const SBAffinity* a){unsigned int n,level=0,xp,base,need,pct;int i=0;const char* p="NEXT LEVEL ";if(!a->selected||a->selected>5){p="CHOOSE ELEMENT";while(*p)out[i++]=*p++;out[i]=0;return 0;}xp=a->xp[0];while(level<100){n=level+1;if(xp<4u*(20u*n+n*(n-1u)))break;++level;}if(level==100){p="LEVEL MAX";while(*p)out[i++]=*p++;out[i]=0;return 100;}base=4u*(20u*level+level*(level-1u));need=4u*(20u+2u*level);pct=(xp-base)*100u/need;while(*p)out[i++]=*p++;out[i++]=(char)('0'+pct/10);out[i++]=(char)('0'+pct%10);out[i++]='%';out[i]=0;return pct;}
static void effect_text(char out[256],char detail_out[256],const char** detail,const SBAffinity* a,unsigned int weight)
{
    const SBLocale* l=sb_locale_get();unsigned int level=ui_level(a),value,e=a->selected;
    weight*=ui_effect_half_steps;
    if(!e||e>5){add(out,0,256,l->noelement);*detail=l->choose;return;}
    if(e==1){value=50u+level;decimal(out,256,l->blunt,(value*weight+100u)/200u,1);*detail=l->base;}
    else if(e==2){value=50u+level;decimal(out,256,l->armour,(value*weight+100u)/200u,1);decimal(detail_out,256,l->bolt,((10u+(4u*level+5u)/10u)*weight+100u)/200u,1);*detail=detail_out;}
    else if(e==3){value=20u+(8u*level+5u)/10u;decimal(out,256,l->attackblock,(value*weight+100u)/200u,1);*detail=l->winddetail;}
    else if(e==4){value=50u+level;decimal(out,256,l->cut,(value*weight+100u)/200u,1);*detail=l->base;}
    else {int n=0;value=(10u+(40u*level)/100u)*weight;value=(value+100u)/200u;n=add(out,n,256,l->bleed);n=add(out,n,256," +");n=num(out,n,256,value/100);n=add(out,n,256,".");if(value%100<10)n=add(out,n,256,"0");num(out,n,256,value%100);decimal(detail_out,256,l->cut,((20u+(8u*level+5u)/10u)*weight+100u)/200u,1);*detail=detail_out;}
}
#ifdef SPELLBLADE_NATIVE_TEST
static int ui_test_has(const char* s,const char* part){int i,j;for(i=0;s[i];++i){for(j=0;part[j]&&s[i+j]==part[j];++j){}if(!part[j])return 1;}return 0;}
__declspec(dllexport) int probeTestHalfEffectDisplay(void){SBAffinity a;char out[256],detail_out[256];const char* detail;sb_locale_set("en_GB");sb_affinity_create(&a,1,1);a.xp[0]=47600;sb_ui_view_set_effect_multiplier(0.5f);effect_text(out,detail_out,&detail,&a,100);if(!ui_test_has(out,"7.5%"))return 1;sb_affinity_select(&a,5);effect_text(out,detail_out,&detail,&a,100);if(!ui_test_has(out,"0.25"))return 2;sb_ui_view_set_effect_multiplier(1.0f);return 0;}
#endif
static void milestone_text(char out[256],unsigned int level){const SBLocale* l=sb_locale_get();int n=0;unsigned int dest=0,pct=0;const char* what=0;if(level>=100){add(out,0,256,l->master);return;}if(level<20){what=l->trail;dest=20;}else if(level<40){what=l->glow;dest=40;}else if(level<50){what=l->dual;dest=50;}else {what=l->second;if(level<60){dest=60;pct=35;}else if(level<70){dest=70;pct=45;}else if(level<80){dest=80;pct=55;}else if(level<90){dest=90;pct=65;}else {dest=100;pct=75;}}n=add(out,n,256,l->next);n=add(out,n,256,": ");n=add(out,n,256,what);if(pct){n=add(out,n,256," ");n=num(out,n,256,pct);n=add(out,n,256,"%");}n=add(out,n,256," ");n=add(out,n,256,l->level);num(out,n,256,dest);}
void sb_ui_view_sync(SBUIView* v,const SBUIViewAPI* a,void* window,void* character,unsigned int index,unsigned int serial,const SBAffinity* affinity,int editable)
{
    int i,changed;if(!v||!valid(a)||!v->panel)return;
    sb_ui_bind(&v->model,window,character,index,serial,affinity,editable);
    a->visible(a->context,v->opener,v->model.has_affinity);a->visible(a->context,v->panel,v->model.visible);
    if(!v->model.visible)return;
    {char next[256];milestone_text(next,ui_level(&v->model.affinity));a->caption(a->context,v->milestone,next);}
    {unsigned int level=ui_level(&v->model.affinity),pct=ui_secondary_percent(level);char text[128],title[256];int j=0;if(level<50){j=add(text,j,128,"LV50 ");j=num(text,j,128,level);j=add(text,j,128,"/50");v->second_mode=0;}else{j=add(text,j,128,sb_locale_get()->second);j=add(text,j,128," ");j=num(text,j,128,pct);j=add(text,j,128,"%");}a->caption(a->context,v->mode_second,text);a->selected(a->context,v->mode_main,!v->second_mode);a->selected(a->context,v->mode_second,v->second_mode);selection_title(title,&v->model.affinity,v->second_mode);a->caption(a->context,v->title,title);}
    {char text[128],number[8];unsigned int pct=progress(text,&v->model.affinity),level=ui_level(&v->model.affinity);int n=0;a->progress(a->context,v->fill,pct);level_text(text,&v->model.affinity);a->caption(a->context,v->status,editable?text:sb_locale_get()->readonly);a->caption(a->context,v->xp_label,level==100?sb_locale_get()->maxlevel:sb_locale_get()->nextlevel);if(pct>=100)number[n++]='1';if(pct>=10)number[n++]=(char)('0'+pct/10%10);number[n++]=(char)('0'+pct%10);number[n++]='%';number[n]=0;a->caption(a->context,v->xp_percent,number);}
    {char text[256],detail_buf[256];const char* detail;SBAffinity shown=v->model.affinity;unsigned int weight=v->second_mode?ui_secondary_percent(ui_level(&shown)):100;if(v->second_mode)shown.selected=shown.secondary;effect_text(text,detail_buf,&detail,&shown,weight);if(!editable){a->caption(a->context,v->effect,sb_locale_get()->unavailable);a->caption(a->context,v->detail,sb_locale_get()->disabled);}else {a->caption(a->context,v->effect,text);a->caption(a->context,v->detail,detail);}}
    for(i=0;i<5;++i){char text[32];label(text,v->model.affinity.rates[i]);a->caption(a->context,v->labels[i],text);}
    for(i=0;i<6;++i)a->selected(a->context,v->buttons[i],(v->second_mode?v->model.affinity.secondary:v->model.affinity.selected)==(unsigned int)i);
    changed=!v->graph_ready||v->graph_state.selected!=v->model.affinity.selected;
    for(i=0;i<5;++i)if(!v->graph_ready||v->graph_state.rates[i]!=v->model.affinity.rates[i])changed=1;
    if(changed){v->graph_ready=a->radar(a->context,v->graph,&v->model.affinity);if(v->graph_ready)v->graph_state=v->model.affinity;}
}
int sb_ui_view_click(SBUIView* v,const SBUIViewAPI* a,void* widget)
{
    int i;void* character;unsigned int index,serial;
    if(!v||!valid(a)||!widget)return 0;
    if(widget==v->opener){if(sb_ui_toggle(&v->model))a->visible(a->context,v->panel,v->model.visible);return 0;}
    if(widget==v->close&&v->model.visible){v->model.visible=0;a->visible(a->context,v->panel,0);return 0;}
    if(widget==v->mode_main&&v->model.visible){v->second_mode=0;return 0;}
    if(widget==v->mode_second&&v->model.visible){if(ui_level(&v->model.affinity)>=50)v->second_mode=1;return 0;}
    for(i=0;i<6;++i)if(widget==v->buttons[i]&&sb_ui_request_selection(&v->model,(unsigned int)i,&character,&index,&serial))return i+1+(v->second_mode?6:0);
    return 0;
}

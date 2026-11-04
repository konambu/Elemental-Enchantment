#include "spellblade_ui_view.h"
static const char* labels[5]={"Fire","Lightning","Wind","Water","Ice"};
static const int ring_order[5]={0,2,4,3,1};
static const char* buttons[6]={"None","Fire","Lightning","Wind","Water","Ice"};
static unsigned int ui_level(const SBAffinity* a){unsigned int lo=0,hi=100;while(lo<hi){unsigned int mid=(lo+hi+1u)/2u;if(a->xp[0]>=4u*(20u*mid+mid*(mid-1u)))lo=mid;else hi=mid-1u;}return lo;}
static unsigned int ui_secondary_percent(unsigned int l){return l<50?0:l<60?25:l<70?35:l<80?45:l<90?55:l<100?65:75;}
static void selection_title(char out[64],const SBAffinity* a,int second_mode)
{int i=0;const char* p="MAIN  ";unsigned int e=a->selected;(void)second_mode;while(*p&&i<63)out[i++]=*p++;p=e&&e<=5?labels[e-1]:"None";while(*p&&i<63)out[i++]=*p++;p="     SUB  ";while(*p&&i<63)out[i++]=*p++;e=a->secondary;p=ui_level(a)<50?"Locked":(e&&e<=5?labels[e-1]:"None");while(*p&&i<63)out[i++]=*p++;out[i]=0;}
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
    v->opener=a->create(a->context,parent,SB_UI_BUTTON,15,260,150,26);
    v->panel=a->create(a->context,parent,SB_UI_PANEL,x,10,530,448);
    if(!v->opener||!v->panel)goto fail;
    a->caption(a->context,v->opener,"ENCHANTMENT");a->visible(a->context,v->panel,0);
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
    for(i=0;i<5;++i){v->axis_labels[i]=a->create(a->context,v->body,i?SB_UI_AXIS_LABEL:SB_UI_LABEL,xy[i][0],xy[i][1],100,22);if(!v->axis_labels[i])goto fail;a->caption(a->context,v->axis_labels[i],labels[ring_order[i]]);}}
    for(i=0;i<5;++i){v->labels[i]=a->create(a->context,v->body,SB_UI_LABEL,400,57+i*35,110,23);if(!v->labels[i])goto fail;}
    for(i=0;i<6;++i){int bx=260,by=i?52+(i-1)*35:252,bw=i?125:240;v->buttons[i]=a->create(a->context,v->body,SB_UI_BUTTON,bx,by,bw,27);if(!v->buttons[i])goto fail;a->caption(a->context,v->buttons[i],buttons[i]);}
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
    if(!v->mode_main||!v->mode_second||!v->milestone||!v->xp_label||!v->xp_percent)goto fail;a->caption(a->context,v->mode_main,"MAIN");
    return 1;
fail:sb_ui_view_dispose(v,a);return 0;
}
static void label(char out[32],unsigned int rate){int i=0;out[i++]='X';out[i++]='P';out[i++]=' ';if(rate>=100)out[i++]='1';if(rate>=10)out[i++]=(char)('0'+rate/10%10);out[i++]=(char)('0'+rate%10);out[i++]='%';out[i]=0;}
static void level_text(char out[32],const SBAffinity* a){unsigned int level=ui_level(a);int i=0;const char* p="ELEMENTAL   ";while(*p)out[i++]=*p++;if(level>=100)out[i++]='1';if(level>=10)out[i++]=(char)('0'+level/10%10);out[i++]=(char)('0'+level%10);out[i]=0;}
static unsigned int progress(char out[32],const SBAffinity* a){unsigned int n,level=0,xp,base,need,pct;int i=0;const char* p="NEXT LEVEL ";if(!a->selected||a->selected>5){p="CHOOSE ELEMENT";while(*p)out[i++]=*p++;out[i]=0;return 0;}xp=a->xp[0];while(level<100){n=level+1;if(xp<4u*(20u*n+n*(n-1u)))break;++level;}if(level==100){p="LEVEL MAX";while(*p)out[i++]=*p++;out[i]=0;return 100;}base=4u*(20u*level+level*(level-1u));need=4u*(20u+2u*level);pct=(xp-base)*100u/need;while(*p)out[i++]=*p++;out[i++]=(char)('0'+pct/10);out[i++]=(char)('0'+pct%10);out[i++]='%';out[i]=0;return pct;}
static void detail_cut(char out[64],unsigned int value)
{const char* text="Cut damage +";int i=0;while(*text)out[i++]=*text++;if(value>=100u)out[i++]=(char)('0'+value/100u);out[i++]=(char)('0'+(value/10u)%10u);out[i++]='.';out[i++]=(char)('0'+value%10u);out[i++]='%';out[i]=0;}
static void detail_attack(char out[64],unsigned int value)
{const char* text="Attack speed +";int i=0;while(*text)out[i++]=*text++;out[i++]=(char)('0'+(value/10u)%10u);out[i++]='.';out[i++]=(char)('0'+value%10u);out[i++]='%';out[i]=0;}
static void effect_text(char out[64],char detail_out[64],const char** detail,const SBAffinity* a,unsigned int weight)
{
    unsigned int level=ui_level(a),value;int i=0;const char* text;unsigned int e=a->selected;
    if(!e||e>5){text="No active element";*detail="Select an element above.";while(*text)out[i++]=*text++;out[i]=0;return;}
    if(e==1){text="Blunt damage +";value=50u+level;*detail="Percent of base cut + blunt damage.";}
    else if(e==2){text="Armour penetration +";value=50u+level;detail_attack(detail_out,((10u+(4u*level+5u)/10u)*weight+50u)/100u);*detail=detail_out;}
    else if(e==3){text="Attack / block speed +";value=20u+(8u*level+5u)/10u;*detail="Applied to current combat speed.";}
    else if(e==4){text="Cut damage +";value=50u+level;*detail="Percent of base cut + blunt damage.";}
    else {text="Bleed multiplier +0.";value=10u+(40u*level)/100u;value=(value*weight+50u)/100u;detail_cut(detail_out,((20u+(8u*level+5u)/10u)*weight+50u)/100u);*detail=detail_out;while(*text)out[i++]=*text++;out[i++]=(char)('0'+value/10u);out[i++]=(char)('0'+value%10u);out[i]=0;return;}
    value=(value*weight+50u)/100u;while(*text)out[i++]=*text++;if(value>=100u)out[i++]=(char)('0'+value/100u);out[i++]=(char)('0'+(value/10u)%10u);out[i++]='.';out[i++]=(char)('0'+value%10u);out[i++]='%';out[i]=0;
}
void sb_ui_view_sync(SBUIView* v,const SBUIViewAPI* a,void* window,void* character,unsigned int index,unsigned int serial,const SBAffinity* affinity,int editable)
{
    int i,changed;if(!v||!valid(a)||!v->panel)return;
    sb_ui_bind(&v->model,window,character,index,serial,affinity,editable);
    a->visible(a->context,v->opener,v->model.has_affinity);a->visible(a->context,v->panel,v->model.visible);
    if(!v->model.visible)return;
    {unsigned int level=ui_level(&v->model.affinity);const char* next=level<20?"NEXT: TRAIL LV20":level<40?"NEXT: HIT GLOW LV40":level<50?"NEXT: DUAL LV50":level<60?"NEXT: SECOND 35% LV60":level<70?"NEXT: SECOND 45% LV70":level<80?"NEXT: SECOND 55% LV80":level<90?"NEXT: SECOND 65% LV90":level<100?"NEXT: SECOND 75% LV100":"ELEMENTAL MASTER";a->caption(a->context,v->milestone,next);}
    {unsigned int level=ui_level(&v->model.affinity),pct=ui_secondary_percent(level);char text[32],title[64];int j=0;const char* p=level<50?"LV50 ":"SUB ";while(*p)text[j++]=*p++;if(level<50){if(level>=10)text[j++]=(char)('0'+level/10);text[j++]=(char)('0'+level%10);text[j++]='/';text[j++]='5';text[j++]='0';v->second_mode=0;}else{if(pct>=10)text[j++]=(char)('0'+pct/10);text[j++]=(char)('0'+pct%10);text[j++]='%';}text[j]=0;a->caption(a->context,v->mode_second,text);a->selected(a->context,v->mode_main,!v->second_mode);a->selected(a->context,v->mode_second,v->second_mode);selection_title(title,&v->model.affinity,v->second_mode);a->caption(a->context,v->title,title);}
    {char text[32],number[8];unsigned int pct=progress(text,&v->model.affinity),level=ui_level(&v->model.affinity);int n=0;a->progress(a->context,v->fill,pct);level_text(text,&v->model.affinity);a->caption(a->context,v->status,editable?text:"READ ONLY: save data");a->caption(a->context,v->xp_label,level==100?"MAX LEVEL":"NEXT LEVEL");if(pct>=100)number[n++]='1';if(pct>=10)number[n++]=(char)('0'+pct/10%10);number[n++]=(char)('0'+pct%10);number[n++]='%';number[n]=0;a->caption(a->context,v->xp_percent,number);}
    {char text[64],detail_buf[64];const char* detail;SBAffinity shown=v->model.affinity;unsigned int weight=v->second_mode?ui_secondary_percent(ui_level(&shown)):100;if(v->second_mode)shown.selected=shown.secondary;effect_text(text,detail_buf,&detail,&shown,weight);if(!editable){a->caption(a->context,v->effect,"Saved data unavailable.");a->caption(a->context,v->detail,"Effect is disabled.");}else {a->caption(a->context,v->effect,text);a->caption(a->context,v->detail,detail);}}
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

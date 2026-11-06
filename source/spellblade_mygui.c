#include "spellblade_mygui.h"
#include "spellblade_radar.h"
static void str(SBGameString* s,const char* p)
{unsigned long long n=0,i;while(p[n])++n;s->size=n;s->capacity=n<16?15:n;s->allocator=0;if(n<16)for(i=0;i<=n;++i)s->text.small[i]=p[i];else s->text.heap=p;}
static void* make(void* c,void* parent,int kind,int x,int y,int w,int h)
{
    SBMyGUI* a=(SBMyGUI*)c;SBGameString type,skin,name,font;SBMyGUICoord rect={x,y,w,h};void* widget;unsigned int scale,sx,sy;
    if(!a||!a->create_widget||!parent)return 0;
    scale=a->scale_tenths>=8&&a->scale_tenths<=20?a->scale_tenths:10;
    sx=a->scale_x100?a->scale_x100:100;sy=a->scale_y100?a->scale_y100:100;
    if(kind==SB_UI_OPENER){rect.left=(x*(int)sx+50)/100;rect.top=(y*(int)sy+50)/100;rect.width=(w*(int)sx+50)/100;rect.height=(h*(int)sy+50)/100;}
    else {sx=sx*scale/10;sy=sy*scale/10;if(kind==SB_UI_PANEL)rect.left=x-12+(12*(int)sx+50)/100;else rect.left=(x*(int)sx+50)/100;rect.top=(y*(int)sy+50)/100;rect.width=(w*(int)sx+50)/100;rect.height=(h*(int)sy+50)/100;}
    if(kind==SB_UI_PANEL){str(&type,"Widget");str(&skin,"PanelEmpty");}
    else if(kind==SB_UI_BUTTON||kind==SB_UI_OPENER){str(&type,"Button");str(&skin,"Kenshi_Button1Skin");}
    else if(kind==SB_UI_LABEL){str(&type,"TextBox");str(&skin,"Kenshi_TextboxStandardText");}
    else if(kind==SB_UI_AXIS_LABEL||kind==SB_UI_BAR_TEXT){str(&type,"TextBox");str(&skin,"Kenshi_TextBoxEmptySkin");}
    else if(kind==SB_UI_RADAR){str(&type,"Canvas");str(&skin,"Canvas");}
    else if(kind==SB_UI_BAR_BG){str(&type,"Widget");str(&skin,"Kenshi_ProgressBarEmptySkin");}
    else if(kind==SB_UI_BAR_FILL){str(&type,"Widget");str(&skin,"Kenshi_ProgressBarFilledSkin");}
    else if(kind==SB_UI_WINDOW_BODY){str(&type,"Widget");str(&skin,"Kenshi_GenericWindowSkin");}
    else if(kind==SB_UI_PAPER){str(&type,"Widget");str(&skin,"Kenshi_GenericTextBoxSkin");}
    else if(kind==SB_UI_WINDOW_HEADER){str(&type,"Widget");str(&skin,"Kenshi_GenericWindowHeaderSkin");}
    else if(kind==SB_UI_WINDOW_TITLE){str(&type,"TextBox");str(&skin,"Kenshi_TextBoxEmptySkin");}
    else if(kind==SB_UI_WINDOW_CLOSE){str(&type,"Button");str(&skin,"Kenshi_CloseButtonSkin");}
    else return 0;
    str(&name,"");widget=a->create_widget(parent,&type,&skin,&rect,10,&name);
    /* A popup retains its parent lifecycle while escaping its clipping area. */
    if(widget&&a->set_style&&kind==SB_UI_PANEL){SBGameString layer;str(&layer,"Popup");a->set_style(widget,1,&layer);}
    if(widget&&a->set_font&&(kind==SB_UI_BUTTON||kind==SB_UI_OPENER||kind==SB_UI_LABEL||kind==SB_UI_AXIS_LABEL||kind==SB_UI_BAR_TEXT||kind==SB_UI_WINDOW_TITLE)){str(&font,"Kenshi_PaintedTextFont_Medium");a->set_font(widget,&font);if(kind!=SB_UI_OPENER&&scale!=10&&a->get_font_height&&a->set_font_height){int size=a->get_font_height(widget);if(size>0&&size<200)a->set_font_height(widget,(size*(int)scale+5)/10);}}
    if(widget&&a->set_property&&kind==SB_UI_AXIS_LABEL){SBGameString key,value;str(&key,"TextColour");str(&value,"0.0784314 0.0313726 0.0235294");a->set_property(widget,&key,&value);}
    if(widget&&a->set_property&&kind==SB_UI_BAR_TEXT){SBGameString key,value;str(&key,"TextColour");str(&value,"0.90 0.86 0.76");a->set_property(widget,&key,&value);}
    if(widget&&a->set_property&&(kind==SB_UI_BUTTON||kind==SB_UI_OPENER)){SBGameString key,value;str(&key,"TextAlign");str(&value,"Center");a->set_property(widget,&key,&value);}
    if(widget&&a->set_property&&kind==SB_UI_WINDOW_TITLE){SBGameString key,value;str(&key,"TextColour");str(&value,"0.78 0.72 0.62");a->set_property(widget,&key,&value);str(&key,"TextAlign");str(&value,"Center");a->set_property(widget,&key,&value);}
    return widget;
}
static void remove_widget(void* c,void* widget){SBMyGUI* a=(SBMyGUI*)c;void* gui;if(!a||!a->gui_instance||!a->destroy_widget||!widget)return;gui=a->gui_instance();if(gui)a->destroy_widget(gui,widget);}
static void visible(void* c,void* widget,int show){SBMyGUI* a=(SBMyGUI*)c;if(a&&a->set_visible&&widget)a->set_visible(widget,(unsigned char)(show!=0));}
static void caption(void* c,void* widget,const char* text){SBMyGUI* a=(SBMyGUI*)c;SBGameString value;if(!a||!a->set_caption||!widget||!text)return;str(&value,text);a->set_caption(widget,&value);}
static void selected(void* c,void* widget,int on){SBMyGUI* a=(SBMyGUI*)c;if(a&&a->set_selected&&widget)a->set_selected(widget,(unsigned char)(on!=0));}
static void progress(void* c,void* widget,unsigned int pct){SBMyGUI* a=(SBMyGUI*)c;unsigned int scale,sx,sy;if(!a||!widget||!a->set_size||!a->set_visible)return;scale=a->scale_tenths>=8&&a->scale_tenths<=20?a->scale_tenths:10;sx=(a->scale_x100?a->scale_x100:100)*scale/10;sy=(a->scale_y100?a->scale_y100:100)*scale/10;if(pct>100)pct=100;a->set_visible(widget,(unsigned char)(pct!=0));if(pct)a->set_size(widget,(int)(490u*sx*pct/10000u),(int)((14u*sy+50u)/100u));}
static int radar(void* c,void* widget,const SBAffinity* affinity)
{
    SBMyGUI* a=(SBMyGUI*)c;unsigned char* pixels;int ok;
    if(!a||!widget||!affinity||!a->canvas_created||!a->canvas_create||!a->canvas_width||!a->canvas_height||!a->canvas_lock||!a->canvas_unlock)return 0;
    /* A fixed power-of-two texture avoids resize churn. The MyGUI texture
     * interface presents a linear buffer; actual D3D compatibility must be
     * verified in game before shipping this backend. */
    if(!a->canvas_created(widget))a->canvas_create(widget,256,256,0,18,4);
    if(a->canvas_width(widget)!=256||a->canvas_height(widget)!=256)return 0;
    pixels=(unsigned char*)a->canvas_lock(widget,16);if(!pixels)return 0;
    __try {ok=sb_radar_draw(pixels,256,256,affinity);} __finally {a->canvas_unlock(widget);}
    return ok;
}
void sb_mygui_backend(SBMyGUI* a,SBUIViewAPI* out)
{if(!out)return;out->context=a;out->create=make;out->destroy=remove_widget;out->visible=visible;out->caption=caption;out->selected=selected;out->radar=radar;out->progress=progress;}

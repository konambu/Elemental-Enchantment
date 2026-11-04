#include "spellblade_mygui.h"
typedef struct FakeWidget {int type,visible,selected;} FakeWidget;
static FakeWidget widgets[40];static int count,destroyed,texture_created,locks,unlocks,bad_skin,fonts,overlaps,centred,darklabels,paper;
static unsigned char texture[256*256*4];
static void* gui(void){return (void*)1;}
static void* create(void* parent,const SBGameString* type,const SBGameString* skin,const SBMyGUICoord* r,int align,const SBGameString* name)
{FakeWidget* w;const char* p=skin->capacity<16?skin->text.small:skin->text.heap;(void)parent;(void)r;(void)name;if(align!=10||count>=40)return 0;if(type->text.small[0]=='B'&&skin->size!=18&&skin->size!=22)bad_skin=1;if(type->text.small[0]=='T'&&skin->size!=23&&skin->size!=26)bad_skin=1;if(skin->size==25&&p&&p[0]=='K')++paper;if(!p)bad_skin=1;w=&widgets[count++];w->type=type->text.small[0];return w;}
static void set_font(void* w,const SBGameString* name){(void)w;if(name->size==29)++fonts;}
static void set_style(void* w,int style,const SBGameString* layer){(void)w;if(style==1&&layer->size==5)++overlaps;}
static void set_size(void* w,int width,int height){(void)w;(void)width;(void)height;}
static void set_property(void* w,const SBGameString* key,const SBGameString* val){(void)w;if(key->size==9&&val->size==6&&val->text.small[0]=='C')++centred;if(key->size==10&&val->size==29&&val->text.heap&&val->text.heap[2]=='0')++darklabels;}
static void destroy(void* g,void* w){(void)g;(void)w;++destroyed;}
static void set_visible(void* w,unsigned char v){((FakeWidget*)w)->visible=v;}
static void caption(void* w,const SBGameString* text){(void)w;(void)text;}
static void selected(void* w,unsigned char v){((FakeWidget*)w)->selected=v;}
static unsigned char created(void* w){(void)w;return (unsigned char)texture_created;}
static void create_texture(void* w,int width,int height,int mode,int usage,int format){(void)w;if(width==256&&height==256&&mode==0&&usage==18&&format==4)texture_created=1;}
static int dimension(void* w){(void)w;return 256;}
static void* lock(void* w,int access){(void)w;if(access!=16)return 0;++locks;return texture;}
static void unlock(void* w){(void)w;++unlocks;}
__declspec(dllexport) int sb_run_mygui_tests(void)
{
    SBMyGUI engine={gui,create,destroy,set_visible,caption,selected,created,create_texture,dimension,dimension,lock,unlock,set_font,set_style,set_size,set_property};SBUIViewAPI api;SBUIView view;SBAffinity state={3,{30,50,70,90,60}};
    count=destroyed=texture_created=locks=unlocks=centred=darklabels=paper=0;sb_mygui_backend(&engine,&api);sb_ui_view_init(&view);
    if(!sb_ui_view_build(&view,&api,(void*)2,700,500)||count!=35||bad_skin||fonts!=27||overlaps!=1||centred!=10||darklabels!=4||paper!=1)return 1;
    sb_ui_view_sync(&view,&api,(void*)3,(void*)4,5,6,&state,1);
    sb_ui_view_click(&view,&api,view.opener);
    sb_ui_view_sync(&view,&api,(void*)3,(void*)4,5,6,&state,1);
    if(!view.graph_ready||!texture_created||locks!=1||unlocks!=1||texture[0]!=0x65)return 2;
    if(!((FakeWidget*)view.buttons[3])->selected||sb_ui_view_click(&view,&api,view.buttons[5])!=6)return 3;
    sb_ui_view_dispose(&view,&api);if(destroyed!=2)return 4;
    return 0;
}

#include "spellblade_gamedata.h"
typedef struct TestData {int count,adds,vals[3];} TestData;
static const SBGameString own={{"SB_Data"},7,15,0};
static const SBGameString other={{"Unrelated"},9,15,0};
static unsigned char exists(void* d,const SBGameString* name){(void)name;return ((TestData*)d)->count>0;}
static int size(void* d,const SBGameString* name){(void)name;return ((TestData*)d)->count;}
static const SBGameString* name(void* d,const SBGameString* list,int i){(void)d;(void)list;return i==0?&other:&own;}
static const int* values(void* d,const SBGameString* list,int i){(void)list;(void)i;return ((TestData*)d)->vals;}
static void* find(void* d,const SBGameString* list,const SBGameString* id){(void)list;(void)id;return ((TestData*)d)->vals;}
static void add(void* d,const SBGameString* list,const SBGameString* id,int a,int b,int c){TestData* t=(TestData*)d;(void)list;(void)id;t->vals[0]=a;t->vals[1]=b;t->vals[2]=c;t->count=2;++t->adds;}
__declspec(dllexport) int sb_run_gamedata_tests(void)
{
    TestData d={1,0,{0}};SBGameDataAPI api={exists,size,name,values,find,add};SBGameDataContext ctx={&d,&api};SBSaveBackend backend={&ctx,sb_gamedata_read,sb_gamedata_write};SBAffinity a,b;int i;
    if(sb_save_load(&backend,&b)!=SB_SAVE_MISSING)return 1;
    if(!sb_affinity_create(&a,17,3)||sb_save_store(&backend,&a)!=SB_SAVE_OK||d.adds!=1)return 2;
    if(sb_save_load(&backend,&b)!=SB_SAVE_OK||b.selected!=3)return 3;
    for(i=0;i<5;++i)if(a.rates[i]!=b.rates[i])return 4;
    if(!sb_affinity_select(&a,5)||sb_save_store(&backend,&a)!=SB_SAVE_OK||d.adds!=1||d.count!=2)return 5;
    if(sb_save_load(&backend,&b)!=SB_SAVE_OK||b.selected!=5)return 6;
    d.vals[0]=0x501;if(sb_save_store(&backend,&a)!=SB_SAVE_UNSUPPORTED||d.vals[0]!=0x501)return 7;
    d.count=513;if(sb_save_load(&backend,&b)!=SB_SAVE_ERROR)return 8;
    return 0;
}

#include "spellblade_characters.h"
typedef struct Mock {int present,value[3],writes;} Mock;
static int read_(void* p,int out[3]){Mock* m=(Mock*)p;int i;if(!m->present)return 0;for(i=0;i<3;++i)out[i]=m->value[i];return 1;}
static int write_(void* p,const int v[3]){Mock* m=(Mock*)p;int i;for(i=0;i<3;++i)m->value[i]=v[i];m->present=1;++m->writes;return 1;}
static SBCharacters chars;
__declspec(dllexport) int sb_run_characters_tests(void)
{
    Mock m={0},other={0};SBSaveBackend b={&m,read_,write_},b2={&other,read_,write_};SBCharacterRecord *a,*x;unsigned char saved[5];int i;
    sb_characters_init(&chars);a=sb_character_observe(&chars,(void*)1,10,20,99,3);if(!a||a->affinity.selected!=3)return 1;
    for(i=0;i<5;++i)saved[i]=a->affinity.rates[i];
    if(!sb_character_select(&chars,(void*)1,10,20,4)||sb_character_save(&chars,(void*)1,10,20,&b)!=SB_SAVE_OK)return 2;
    x=sb_character_observe(&chars,(void*)2,11,20,111,2);if(!x||x->affinity.selected!=2)return 3;
    if(sb_character_save(&chars,(void*)2,11,20,&b2)!=SB_SAVE_OK)return 4;
    sb_character_forget(&chars,(void*)1);
    if(sb_character_load(&chars,(void*)3,30,40,&b)!=SB_SAVE_OK)return 5;
    a=sb_character_observe(&chars,(void*)3,30,40,1,1);if(!a||a->affinity.selected!=4)return 6;
    for(i=0;i<5;++i)if(a->affinity.rates[i]!=saved[i])return 7;
    if(sb_character_observe(&chars,(void*)3,30,41,2,1)->affinity.selected!=1)return 8;
    m.value[0]=0x501;
    if(sb_character_load(&chars,(void*)4,50,60,&b)!=SB_SAVE_UNSUPPORTED)return 9;
    if(sb_character_select(&chars,(void*)4,50,60,2)||sb_character_save(&chars,(void*)4,50,60,&b)!=SB_SAVE_UNSUPPORTED)return 10;
    a=sb_character_observe(&chars,(void*)2,11,20,2,1);if(!a||a->affinity.selected!=2)return 11;
    if(!sb_character_handle_begin(&chars,(void*)2,11,20))return 12;
    a=sb_character_observe(&chars,(void*)2,111,222,8,5);if(!a||a->affinity.selected!=2)return 13;
    sb_character_handle_end(&chars,(void*)2,111,222);
    a=sb_character_observe(&chars,(void*)2,111,222,8,5);if(!a||a->affinity.selected!=2)return 14;
    return 0;
}

#include "spellblade_characters.h"
typedef struct Mock {int present,value[3],writes;} Mock;
static int read_(void* p,int out[3]){Mock* m=(Mock*)p;int i;if(!m->present)return 0;for(i=0;i<3;++i)out[i]=m->value[i];return 1;}
static int write_(void* p,const int v[3]){Mock* m=(Mock*)p;int i;for(i=0;i<3;++i)m->value[i]=v[i];m->present=1;++m->writes;return 1;}
static SBCharacters chars;
static unsigned int seed_calls;
static unsigned int test_new_seed(unsigned int seed){return seed^(++seed_calls*0x9e3779b9u);}
__declspec(dllexport) int sb_test_new_aptitudes(void)
{
    unsigned int counts[5]={0},all=0,i,j;unsigned char saved[5];SBCharacterRecord* r;Mock m={0};SBSaveBackend b={&m,read_,write_};
    sb_characters_set_seed_provider(test_new_seed);sb_characters_init(&chars);seed_calls=0;
    for(i=0;i<5000;++i){
        r=sb_character_observe(&chars,(void*)1,10,20,99,0);if(!r||r->affinity.selected!=0)return 1;
        for(j=0;j<5&&r->affinity.rates[j]==100;++j){}
        if(j==5)++all;else for(j=0;j<5;++j)if(r->affinity.rates[j]==100){++counts[j];break;}
        if(sb_character_observe(&chars,(void*)1,10,20,1,5)!=r||seed_calls!=i+1)return 2;
        sb_character_forget(&chars,(void*)1);
    }
    for(i=0;i<5;++i)if(counts[i]<800||counts[i]>1200)return 3;
    if(all<20||all>100)return 4;
    r=sb_character_observe(&chars,(void*)1,10,20,99,0);if(!r)return 5;
    for(i=0;i<5;++i)saved[i]=r->affinity.rates[i];r->affinity.xp[0]=1234;
    if(sb_character_save(&chars,(void*)1,10,20,&b)!=SB_SAVE_OK)return 6;
    sb_character_forget(&chars,(void*)1);
    if(sb_character_load(&chars,(void*)1,10,20,&b)!=SB_SAVE_OK)return 7;
    i=seed_calls;r=sb_character_observe(&chars,(void*)1,10,20,999,5);
    if(!r||r->affinity.selected!=0||r->affinity.xp[0]!=1234||seed_calls!=i)return 8;
    for(j=0;j<5;++j)if(r->affinity.rates[j]!=saved[j])return 9;
    sb_characters_set_seed_provider(0);return 0;
}
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

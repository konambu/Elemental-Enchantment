#include "elemental_characters.h"
typedef struct Mock {int present,value[3],writes;} Mock;
static int read_(void* p,int out[3]){Mock* m=(Mock*)p;int i;if(!m->present)return 0;for(i=0;i<3;++i)out[i]=m->value[i];return 1;}
static int write_(void* p,const int v[3]){Mock* m=(Mock*)p;int i;for(i=0;i<3;++i)m->value[i]=v[i];m->present=1;++m->writes;return 1;}
static EECharacters chars;
static unsigned int seed_calls;
static unsigned int test_new_seed(unsigned int seed){return seed^(++seed_calls*0x9e3779b9u);}
__declspec(dllexport) int ee_test_new_aptitudes(void)
{
    unsigned int counts[5]={0},all=0,i,j;unsigned char saved[5];EECharacterRecord* r;Mock m={0};EESaveBackend b={&m,read_,write_};
    ee_characters_set_seed_provider(test_new_seed);ee_characters_init(&chars);seed_calls=0;
    for(i=0;i<5000;++i){
        r=ee_character_observe(&chars,(void*)1,10,20,99,0);if(!r||r->affinity.selected!=0)return 1;
        for(j=0;j<5&&r->affinity.rates[j]==100;++j){}
        if(j==5)++all;else for(j=0;j<5;++j)if(r->affinity.rates[j]==100){++counts[j];break;}
        if(ee_character_observe(&chars,(void*)1,10,20,1,5)!=r||seed_calls!=i+1)return 2;
        ee_character_forget(&chars,(void*)1);
    }
    for(i=0;i<5;++i)if(counts[i]<800||counts[i]>1200)return 3;
    if(all<20||all>100)return 4;
    r=ee_character_observe(&chars,(void*)1,10,20,99,0);if(!r)return 5;
    for(i=0;i<5;++i)saved[i]=r->affinity.rates[i];r->affinity.xp[0]=1234;
    if(ee_character_save(&chars,(void*)1,10,20,&b)!=EE_SAVE_OK)return 6;
    ee_character_forget(&chars,(void*)1);
    if(ee_character_load(&chars,(void*)1,10,20,&b)!=EE_SAVE_OK)return 7;
    i=seed_calls;r=ee_character_observe(&chars,(void*)1,10,20,999,5);
    if(!r||r->affinity.selected!=0||r->affinity.xp[0]!=1234||seed_calls!=i)return 8;
    for(j=0;j<5;++j)if(r->affinity.rates[j]!=saved[j])return 9;
    ee_characters_set_seed_provider(0);return 0;
}
__declspec(dllexport) int ee_run_characters_tests(void)
{
    Mock m={0},other={0};EESaveBackend b={&m,read_,write_},b2={&other,read_,write_};EECharacterRecord *a,*x;unsigned char saved[5];int i;
    ee_characters_init(&chars);a=ee_character_observe(&chars,(void*)1,10,20,99,3);if(!a||a->affinity.selected!=3)return 1;
    for(i=0;i<5;++i)saved[i]=a->affinity.rates[i];
    if(!ee_character_select(&chars,(void*)1,10,20,4)||ee_character_save(&chars,(void*)1,10,20,&b)!=EE_SAVE_OK)return 2;
    x=ee_character_observe(&chars,(void*)2,11,20,111,2);if(!x||x->affinity.selected!=2)return 3;
    if(ee_character_save(&chars,(void*)2,11,20,&b2)!=EE_SAVE_OK)return 4;
    ee_character_forget(&chars,(void*)1);
    if(ee_character_load(&chars,(void*)3,30,40,&b)!=EE_SAVE_OK)return 5;
    a=ee_character_observe(&chars,(void*)3,30,40,1,1);if(!a||a->affinity.selected!=4)return 6;
    for(i=0;i<5;++i)if(a->affinity.rates[i]!=saved[i])return 7;
    if(ee_character_observe(&chars,(void*)3,30,41,2,1)->affinity.selected!=1)return 8;
    m.value[0]=0x501;
    if(ee_character_load(&chars,(void*)4,50,60,&b)!=EE_SAVE_UNSUPPORTED)return 9;
    if(ee_character_select(&chars,(void*)4,50,60,2)||ee_character_save(&chars,(void*)4,50,60,&b)!=EE_SAVE_UNSUPPORTED)return 10;
    a=ee_character_observe(&chars,(void*)2,11,20,2,1);if(!a||a->affinity.selected!=2)return 11;
    if(!ee_character_handle_begin(&chars,(void*)2,11,20))return 12;
    a=ee_character_observe(&chars,(void*)2,111,222,8,5);if(!a||a->affinity.selected!=2)return 13;
    ee_character_handle_end(&chars,(void*)2,111,222);
    a=ee_character_observe(&chars,(void*)2,111,222,8,5);if(!a||a->affinity.selected!=2)return 14;
    return 0;
}

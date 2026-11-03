#include "spellblade_persistence.h"
typedef struct Mock {int present,value[3],writes,failure;} Mock;
static int read_(void* p,int out[3]){Mock* m=(Mock*)p;int i;if(m->failure)return -1;if(!m->present)return 0;for(i=0;i<3;++i)out[i]=m->value[i];return 1;}
static int write_(void* p,const int v[3]){Mock* m=(Mock*)p;int i;for(i=0;i<3;++i)m->value[i]=v[i];m->present=1;++m->writes;return 1;}
static SBPersistence state;
__declspec(dllexport) int sb_run_persistence_tests(void)
{
    Mock m={0};SBSaveBackend backend={&m,read_,write_};SBCharacterRecord* r;unsigned char expected[5];int i;
    sb_persistence_init(&state);
    if(sb_persistence_stage(&state,(void*)5,&backend)!=SB_SAVE_MISSING)return 1;
    r=sb_persistence_bind(&state,(void*)5,(void*)1,11,12,13,4);if(!r||r->affinity.selected!=4)return 2;
    for(i=0;i<5;++i)expected[i]=r->affinity.rates[i];
    if(!sb_character_select(&state.characters,(void*)1,11,12,2)||!sb_affinity_award_hit(&r->affinity)||sb_persistence_save(&state,(void*)1,11,12,&backend)!=SB_SAVE_OK)return 3;
    sb_persistence_forget(&state,(void*)5,(void*)1);
    if(sb_persistence_stage(&state,(void*)6,&backend)!=SB_SAVE_OK)return 4;
    r=sb_persistence_bind(&state,(void*)6,(void*)2,21,22,999,5);if(!r||r->affinity.selected!=2||r->affinity.xp[0]!=(unsigned short)((r->affinity.rates[1]*4)/100))return 5;
    for(i=0;i<5;++i)if(r->affinity.rates[i]!=expected[i])return 6;
    /* Rebinding cannot re-apply a consumed old value over a newer choice. */
    if(!sb_character_select(&state.characters,(void*)2,21,22,3))return 7;
    r=sb_persistence_bind(&state,(void*)6,(void*)2,21,22,999,5);if(!r||r->affinity.selected!=3)return 8;
    m.value[0]=0x501;if(sb_persistence_stage(&state,(void*)7,&backend)!=SB_SAVE_UNSUPPORTED)return 9;
    r=sb_persistence_bind(&state,(void*)7,(void*)3,31,32,55,1);if(!r||!r->read_only)return 10;
    if(sb_persistence_save(&state,(void*)3,31,32,&backend)!=SB_SAVE_UNSUPPORTED)return 11;
    m.failure=1;if(sb_persistence_stage(&state,(void*)8,&backend)!=SB_SAVE_ERROR)return 12;
    r=sb_persistence_bind(&state,(void*)8,(void*)4,41,42,55,1);if(!r||!r->read_only)return 13;
    if(sb_persistence_save(&state,(void*)4,41,42,&backend)!=SB_SAVE_UNSUPPORTED)return 14;
    return 0;
}

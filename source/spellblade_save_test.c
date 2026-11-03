#include "spellblade_save.h"
typedef struct Mock {int present,failed,values[3],writes;} Mock;
static int read_record(void* c,int out[3]) {Mock* m=(Mock*)c;int i;if(m->failed)return -1;if(!m->present)return 0;for(i=0;i<3;++i)out[i]=m->values[i];return 1;}
static int write_record(void* c,const int values[3]) {Mock* m=(Mock*)c;int i;if(m->failed)return 0;for(i=0;i<3;++i)m->values[i]=values[i];m->present=1;++m->writes;return 1;}
__declspec(dllexport) int sb_run_save_tests(void)
{
    Mock a={0},b={0};SBSaveBackend ba={&a,read_record,write_record},bb={&b,read_record,write_record};SBAffinity x,y;int i;
    if(!sb_affinity_create(&x,123,3)||sb_save_load(&ba,&y)!=SB_SAVE_MISSING)return 1;
    if(sb_save_store(&ba,&x)!=SB_SAVE_OK||sb_save_load(&ba,&y)!=SB_SAVE_OK||y.selected!=3)return 2;
    for(i=0;i<5;++i)if(x.rates[i]!=y.rates[i])return 3;
    if(sb_save_load(&bb,&y)!=SB_SAVE_MISSING||b.writes)return 4;
    if(!sb_affinity_select(&y,2)||sb_save_store(&ba,&y)!=SB_SAVE_OK)return 5;
    if(sb_save_load(&ba,&x)!=SB_SAVE_OK||x.selected!=2)return 6;
    a.values[0]=0x501;
    if(sb_save_load(&ba,&x)!=SB_SAVE_UNSUPPORTED||x.selected!=2)return 7;
    if(sb_save_store(&ba,&y)!=SB_SAVE_UNSUPPORTED||a.writes!=2)return 8;
    b.failed=1;
    if(sb_save_store(&bb,&y)!=SB_SAVE_ERROR||b.writes)return 9;
    return 0;
}

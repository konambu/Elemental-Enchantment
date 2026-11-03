#include "spellblade_persistence.h"
typedef struct LocalRead {int values[3];} LocalRead;
static int local_read(void* c,int out[3]) {LocalRead* x=(LocalRead*)c;int i;for(i=0;i<3;++i)out[i]=x->values[i];return 1;}
void sb_persistence_init(SBPersistence* p){int i;if(!p)return;sb_characters_init(&p->characters);for(i=0;i<SB_PENDING_LIMIT;++i)p->pending[i].stats=0;}
int sb_persistence_stage(SBPersistence* p,void* stats,const SBSaveBackend* backend)
{
    int values[3],status,i,slot=-1;SBAffinity check;
    if(!p||!stats||!backend||!backend->read)return SB_SAVE_ERROR;
    status=backend->read(backend->context,values);
    if(!status)return SB_SAVE_MISSING;
    /* Preserve a guarded record even when an existing custom field could not
     * be read. Later character binding must not silently invent writable data. */
    if(status<0){values[0]=-1;values[1]=0;values[2]=0;}
    for(i=0;i<SB_PENDING_LIMIT;++i){if(p->pending[i].stats==stats){slot=i;break;}if(slot<0&&!p->pending[i].stats)slot=i;}
    if(slot<0)return SB_SAVE_ERROR;
    p->pending[slot].stats=stats;for(i=0;i<3;++i)p->pending[slot].values[i]=values[i];
    p->pending[slot].unsupported=!sb_affinity_decode(&check,values);
    if(status<0)return SB_SAVE_ERROR;
    return p->pending[slot].unsupported?SB_SAVE_UNSUPPORTED:SB_SAVE_OK;
}
SBCharacterRecord* sb_persistence_bind(SBPersistence* p,void* stats,void* character,unsigned int index,unsigned int serial,unsigned int seed,unsigned int trial)
{
    int i;SBCharacterRecord* r;
    if(!p||!character)return 0;
    if(stats)for(i=0;i<SB_PENDING_LIMIT;++i)if(p->pending[i].stats==stats){
        LocalRead local;SBSaveBackend backend;int j,status;
        for(j=0;j<3;++j)local.values[j]=p->pending[i].values[j];
        backend.context=&local;backend.read=local_read;backend.write=0;
        status=sb_character_load(&p->characters,character,index,serial,&backend);
        if(status==SB_SAVE_ERROR)return 0;
        p->pending[i].stats=0;break;
    }
    r=sb_character_observe(&p->characters,character,index,serial,seed,trial);return r;
}
int sb_persistence_save(SBPersistence* p,void* character,unsigned int index,unsigned int serial,const SBSaveBackend* backend)
{return p?sb_character_save(&p->characters,character,index,serial,backend):SB_SAVE_ERROR;}
void sb_persistence_forget(SBPersistence* p,void* stats,void* character)
{int i;if(!p)return;sb_character_forget(&p->characters,character);if(stats)for(i=0;i<SB_PENDING_LIMIT;++i)if(p->pending[i].stats==stats)p->pending[i].stats=0;}

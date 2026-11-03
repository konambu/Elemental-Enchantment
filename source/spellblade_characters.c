#include "spellblade_characters.h"
static void clear(SBCharacterRecord* r){r->character=0;r->index=0;r->serial=0;r->read_only=0;r->handle_change_depth=0;}
void sb_characters_init(SBCharacters* c){int i;if(c)for(i=0;i<SB_CHARACTER_LIMIT;++i)clear(&c->records[i]);}
static SBCharacterRecord* lookup(SBCharacters* c,void* ch,unsigned int index,unsigned int serial,int create)
{
    int i,free_slot=-1;
    if(!c||!ch)return 0;
    for(i=0;i<SB_CHARACTER_LIMIT;++i){SBCharacterRecord* r=&c->records[i];
        if(r->character==ch){if((r->index==index&&r->serial==serial)||r->handle_change_depth)return r;clear(r);if(free_slot<0)free_slot=i;}
        else if(!r->character&&free_slot<0)free_slot=i;
    }
    if(!create||free_slot<0)return 0;
    c->records[free_slot].character=ch;c->records[free_slot].index=index;c->records[free_slot].serial=serial;c->records[free_slot].read_only=0;c->records[free_slot].handle_change_depth=0;
    return &c->records[free_slot];
}
SBCharacterRecord* sb_character_observe(SBCharacters* c,void* ch,unsigned int index,unsigned int serial,unsigned int seed,unsigned int trial)
{
    SBCharacterRecord* r=lookup(c,ch,index,serial,0);
    if(r)return r;
    if(trial>5)return 0;
    r=lookup(c,ch,index,serial,1);if(!r)return 0;
    if(!sb_affinity_create(&r->affinity,seed,trial)){clear(r);return 0;}return r;
}
int sb_character_load(SBCharacters* c,void* ch,unsigned int index,unsigned int serial,const SBSaveBackend* backend)
{
    SBAffinity loaded;SBCharacterRecord* r;int status=sb_save_load(backend,&loaded);
    if(status==SB_SAVE_ERROR||status==SB_SAVE_MISSING)return status;
    r=lookup(c,ch,index,serial,1);if(!r)return SB_SAVE_ERROR;
    if(status==SB_SAVE_UNSUPPORTED){sb_affinity_create(&r->affinity,index^serial,0);r->read_only=1;return status;}
    r->affinity=loaded;r->read_only=0;return SB_SAVE_OK;
}
int sb_character_save(SBCharacters* c,void* ch,unsigned int index,unsigned int serial,const SBSaveBackend* backend)
{
    SBCharacterRecord* r=lookup(c,ch,index,serial,0);
    if(!r)return SB_SAVE_MISSING;if(r->read_only)return SB_SAVE_UNSUPPORTED;
    return sb_save_store(backend,&r->affinity);
}
int sb_character_select(SBCharacters* c,void* ch,unsigned int index,unsigned int serial,unsigned int element)
{SBCharacterRecord* r=lookup(c,ch,index,serial,0);return r&&!r->read_only&&sb_affinity_select(&r->affinity,element);}
int sb_character_select_secondary(SBCharacters* c,void* ch,unsigned int index,unsigned int serial,unsigned int element)
{SBCharacterRecord* r=lookup(c,ch,index,serial,0);return r&&!r->read_only&&sb_affinity_select_secondary(&r->affinity,element);}
void sb_character_forget(SBCharacters* c,void* ch){int i;if(!c||!ch)return;for(i=0;i<SB_CHARACTER_LIMIT;++i)if(c->records[i].character==ch)clear(&c->records[i]);}
int sb_character_handle_begin(SBCharacters* c,void* ch,unsigned int index,unsigned int serial)
{SBCharacterRecord* r=lookup(c,ch,index,serial,0);if(!r)return 0;++r->handle_change_depth;return 1;}
void sb_character_handle_end(SBCharacters* c,void* ch,unsigned int index,unsigned int serial)
{int i;if(!c||!ch)return;for(i=0;i<SB_CHARACTER_LIMIT;++i){SBCharacterRecord* r=&c->records[i];if(r->character==ch&&r->handle_change_depth){r->index=index;r->serial=serial;--r->handle_change_depth;return;}}}

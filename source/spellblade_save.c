#include "spellblade_save.h"

int sb_save_load(const SBSaveBackend* backend,SBAffinity* out)
{
    int values[3],status;
    if(!backend||!backend->read||!out)return SB_SAVE_ERROR;
    status=backend->read(backend->context,values);
    if(status<0)return SB_SAVE_ERROR;
    if(!status)return SB_SAVE_MISSING;
    return sb_affinity_decode(out,values)?SB_SAVE_OK:SB_SAVE_UNSUPPORTED;
}
int sb_save_store(const SBSaveBackend* backend,const SBAffinity* state)
{
    SBAffinity previous;int old[3],values[3],status;
    if(!backend||!backend->read||!backend->write||!state)return SB_SAVE_ERROR;
    if(!sb_affinity_encode(state,values))return SB_SAVE_ERROR;
    status=backend->read(backend->context,old);
    if(status<0)return SB_SAVE_ERROR;
    /* Never silently erase data from a newer or corrupt version. */
    if(status>0&&!sb_affinity_decode(&previous,old))return SB_SAVE_UNSUPPORTED;
    return backend->write(backend->context,values)?SB_SAVE_OK:SB_SAVE_ERROR;
}

#include "elemental_gamedata.h"
static const EEGameString list_name={{"SB_Affinity"},11,15,0};
static const EEGameString item_name={{"SB_Data"},7,15,0};
static int valid(const EEGameDataContext* c)
{return c&&c->data&&c->api&&c->api->list_exists&&c->api->list_size&&c->api->list_name&&c->api->list_values&&c->api->find_reference&&c->api->add_reference;}
static int matches(const EEGameString* s,const EEGameString* target)
{
    unsigned long long i;const char* p;
    if(!s||s->size!=target->size||s->capacity<s->size||s->size>1024)return 0;
    p=s->capacity<16?s->text.small:s->text.heap;if(!p)return 0;
    for(i=0;i<s->size;++i)if(p[i]!=target->text.small[i])return 0;return 1;
}
static int locate(EEGameDataContext* c,int* index)
{
    int i,n,found=-1;
    if(!valid(c))return -1;
    if(!c->api->list_exists(c->data,&list_name)){*index=-1;return 0;}
    n=c->api->list_size(c->data,&list_name);if(n<0||n>512)return -1;
    for(i=0;i<n;++i)if(matches(c->api->list_name(c->data,&list_name,i),&item_name)){
        if(found>=0)return -1;found=i;
    }
    *index=found;return found<0?0:1;
}
int ee_gamedata_read(void* ctx,int out[3])
{
    EEGameDataContext* c=(EEGameDataContext*)ctx;int index,status;const int* data;
    if(!out)return -1;status=locate(c,&index);if(status<=0)return status;
    data=c->api->list_values(c->data,&list_name,index);if(!data)return -1;
    out[0]=data[0];out[1]=data[1];out[2]=data[2];return 1;
}
int ee_gamedata_write(void* ctx,const int values[3])
{
    EEGameDataContext* c=(EEGameDataContext*)ctx;int index,status;int* ref;
    if(!values)return 0;status=locate(c,&index);if(status<0)return 0;
    if(!status){c->api->add_reference(c->data,&list_name,&item_name,values[0],values[1],values[2]);return 1;}
    /* GameDataReference::values is its first field (TripleInt: three ints).
     * Only the value owned by this mod is touched; other references survive. */
    ref=(int*)c->api->find_reference(c->data,&list_name,&item_name);if(!ref)return 0;
    ref[0]=values[0];ref[1]=values[1];ref[2]=values[2];return 1;
}

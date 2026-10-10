#include "decal_instance.h"
#include <string.h>
int uc_decal_instance_begin(UCDecalInstance *o,uint32_t budget,const float identity[16],const UCDecalInstanceMemory *m){
    uint32_t count=budget/76;
    if(!o||o->phase||!identity||!m||!m->allocate||!m->release||count<2||count>65536)return 1;
    o->memory=*m;o->phase=3;memcpy(o->transform,identity,64);o->enabled=1;o->ring.capacity=count;
    UCDecalEntry *entries=m->allocate(m->context,count);
    if(!entries)return 2;
    o->ring.first=o->ring.next=o->auxiliary=0;o->ring.entries=entries;o->phase=1;return 0;
}
int uc_decal_instance_reset(UCDecalInstance *o,UCDecalPrimary primary,void *context){
    if(!o||o->phase!=1||!primary)return 1;
    uint32_t removed;
    if(uc_scene_model_group_remove(primary(context),o,&removed)||uc_decal_ring_clear(&o->ring)||
       uc_scene_model_group_append(primary(context),o,1)){o->phase=2;return 2;}
    return 0;
}
int uc_decal_instance_end(UCDecalInstance *o,UCDecalPrimary primary,void *context){
    if(!o||(o->phase!=1&&o->phase!=2)||!primary||!o->memory.release)return 1;
    o->phase=3;o->memory.release(o->memory.context,o->ring.entries);
    uint32_t removed;return uc_scene_model_group_remove(primary(context),o,&removed)?2:0;
}
int uc_decal_instance_collect(UCDecalInstance *o,UCDecalCollectParameters *p,
    void (*append)(void *,UCDecalInstance *,const UCDecalCollectParameters *,uint32_t),void *context){
    if(!o||o->phase!=1||!p||!append)return 1;
    if(!(p->initialized&1)){p->initialized|=1;p->type=2;p->flags=0x30;p->depth=0;}
    append(context,o,p,0);return 0;
}

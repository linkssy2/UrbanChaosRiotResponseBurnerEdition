#include "entity_slots.h"
int uc_entity_slots_initialize(UCEntityEventRegistry*r,uint32_t*next){
    if(!r||!next)return 1;
    for(unsigned i=0;i<1024;i++)r->slots[i].id=-1;
    *next=0;return 0;
}
int uc_entity_slot_release(UCEntityEventRegistry*r,int32_t id){
    if(!r)return 1;
    if(id!=-1 && r->slots[(uint32_t)id&1023].id==id)
        r->slots[(uint32_t)id&1023].id=-1;
    return 0;
}
int uc_entity_slot_acquire(UCEntityEventRegistry*r,uint32_t*next,
        UCEntityEventTarget*target,int32_t*out){
    if(!r||!next||!out||*next>=0x7ffffbffu)return 1;
    for(uint32_t i=0;i<1024;i++){
        uint32_t id=*next+i;UCEntityEventSlot*s=r->slots+(id&1023);
        if(s->id!=-1)continue;
        s->id=(int32_t)id;s->target=target;
        *next=id+1;if(*next>=0x7ffffbffu)*next=0;
        *out=(int32_t)id;return 0;
    }
    *out=-1;return 0;
}

#include "level_entity_slots.h"
int uc_level_entity_lookup(const UCLevelEntitySlots*v,uint32_t index,int32_t*id){
    if(!v||!id||!v->count)return 1;
    if(index&0xf0000u){*id=(int32_t)index;return 0;}
    if((int32_t)index<0||(int32_t)index>=*v->count){*id=-1;return 0;}
    if(!v->slots||index>=v->capacity||!v->slots[index]||!v->slots[index]->flags||!v->slots[index]->id)return 1;
    const UCLevelEntitySlot*s=v->slots[index];*id=(*s->flags&0x10)?-2:(*s->flags&8)?*s->id:-1;return 0;
}
int uc_level_entity_ensure(const UCLevelEntitySlots *v,uint32_t index,int32_t *id){
    if(!v||!id||!v->count)return 1;
    if((int32_t)index<0||(int32_t)index>=*v->count){*id=-1;return 0;}
    if(!v->slots||index>=v->capacity)return 1;
    UCLevelEntitySlot *slot=v->slots[index];
    if(!slot){*id=-1;return 0;}
    if(!slot->flags||!slot->id||(!(*slot->flags&8)&&!v->create))return 1;
    if(!(*slot->flags&8)){
        *slot->flags=(*slot->flags&~13u)|2u;
        if(v->create(v->owner,slot,index))return 2;
    }
    *id=(*slot->flags&8)?*slot->id:-1;return 0;
}

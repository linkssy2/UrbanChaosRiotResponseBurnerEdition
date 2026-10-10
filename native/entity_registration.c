#include "entity_registration.h"
int uc_entity_register(const UCEntityRegistration*r,const UCEntityRegistrationTarget*t,UCEntityRegistrationResult*out){
    if(!r||!t||!out||!r->registry||!r->next_id||!r->live||!r->creation||
       !r->flags||!r->insert_record||!t->identity||!t->flags||!t->parent_id)return 1;
    int32_t id;if(uc_entity_slot_acquire(r->registry,r->next_id,t->identity,&id))return 1;
    *t->parent_id=r->creation->parent_id;
    int live=uc_entity_list_prepend(r->live,t->identity);
    if(live){
        if(r->insert_record(r->owner,t,id,0))return 2;
        UCEntityEventTarget*parent=uc_entity_event_lookup(r->registry,r->creation->parent_id);
        if(r->creation->relative&&parent&&(r->flags(r->owner,parent)&8)){
            if(id!=-1&&(*t->flags&8)){
                r->creation->relative=0;r->creation->priority=1;
                r->creation->parent_id=*t->parent_id;
                if(r->insert_record(r->owner,t,id,1))return 2;
                *t->flags&=~8u;
            }
        }else if(r->insert_record(r->owner,t,id,1))return 2;
    }
    *t->flags&=~8u;*out=(UCEntityRegistrationResult){id,(uint8_t)live};return 0;
}

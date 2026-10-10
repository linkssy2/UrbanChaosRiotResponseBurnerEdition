#include "entity_retirement.h"
int uc_entity_retire(const UCEntityRetirement*r,int32_t id,uint8_t*queued){
    if(!r||!r->registry||!r->live||!r->pending||r->live==r->pending||
       !r->remove_update_record||!r->remove_render_record||!queued)return 1;
    if(r->remove_update_record(r->owner,id))return 2;
    if(r->remove_render_record(r->owner,id))return 2;
    UCEntityEventTarget*t=uc_entity_event_lookup(r->registry,id);
    *queued=0;if(!t)return 0;
    uc_entity_list_remove_first(r->live,t);
    uc_entity_slot_release(r->registry,id);
    *queued=(uint8_t)uc_entity_list_prepend(r->pending,t);
    return 0;
}

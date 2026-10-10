#include "entity_update.h"
#include <math.h>
static UCEntityRecord *next(const UCEntityRecordBucket **b,const UCEntityListNode **n){
    UCEntityRecord*r=(*n)->target;
    if((*n)->next)*n=(*n)->next;
    else{*b=(*b)->next;if(*b)*n=(*b)->head;}
    return r;
}
int uc_entity_update_records(UCEntityRecords*records,UCEntityList*pending,float delta,float other,
        float*current,float*current_other,const UCEntityUpdateServices*h){
    if(!records||!pending||!current||!current_other||!h||!h->fields||!h->remove||
       !h->step||!h->late||!h->destroy||!isfinite(delta)||!isfinite(other)||
       !uc_entity_records_traversable(records,1))return 1;
    const UCEntityRecordBucket*b=uc_entity_records_view(records).first;
    const UCEntityListNode*n=b?b->head:0;
    *current=delta;*current_other=other;
    while(b){
        UCEntityRecord*r=next(&b,&n);UCEntityEventTarget*t=r->target;uint32_t*flags=r->flags;
        UCEntityUpdateFields f=h->fields(h->owner,t);
        if(*f.secondary_flags&0x40000)continue;
        if(*flags&1){
            *flags&=~1u;h->remove(h->owner,t);
            uc_entity_list_remove_first(pending,t);h->destroy(h->owner,t);
        }else{
            h->step(h->owner,t);
            if(*f.helper)h->helper_tick(h->owner,*f.helper,delta);
            if(*flags&0x8000){uc_entity_list_remove_first(pending,t);h->destroy(h->owner,t);}
        }
    }
    if(!uc_entity_records_traversable(records,1))return 2;
    b=uc_entity_records_view(records).first;n=b?b->head:0;
    while(b){UCEntityRecord*r=next(&b,&n);UCEntityEventTarget*t=r->target;h->late(h->owner,t);}
    return 0;
}

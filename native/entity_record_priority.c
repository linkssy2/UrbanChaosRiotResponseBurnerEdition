#include "entity_record_priority.h"
int uc_entity_record_priority(const UCEntityRecordMemory*m,UCEntityRecords*p,
        int32_t id,int32_t priority){
    if(!m||!m->release||!p||!uc_entity_records_traversable(p,1))return 1;
    UCEntityRecordsView v=uc_entity_records_view(p);
    for(const UCEntityRecordBucket*b=v.first;b;b=b->next)
        for(const UCEntityListNode*n=b->head;n;n=n->next){
            UCEntityRecord*r=n->target;
            if(r->id!=id)continue;
            uc_entity_records_remove_first(p,r);
            if(!uc_entity_records_insert_priority(p,r,priority))m->release(m->owner,r);
            return 0;
        }
    return 0;
}
int uc_entity_record_priority_both(const UCEntityRecordMemory*m,
        const UCEntityPriorityLists*const*active,int32_t id,int32_t priority){
    if(!m||!m->release||!active||!*active||!(*active)->render||!(*active)->update||
       !uc_entity_records_traversable((*active)->render,1)||
       !uc_entity_records_traversable((*active)->update,1))return 1;
    if(uc_entity_record_priority(m,(*active)->render,id,priority))return 1;
    if(!*active||uc_entity_record_priority(m,(*active)->update,id,priority))return 2;
    return 0;
}

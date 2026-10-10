#include "entity_record_ops.h"
static UCEntityRecord *find(UCEntityRecords *records,int32_t id){
    UCEntityRecordsView v=uc_entity_records_view(records);
    for(const UCEntityRecordBucket*b=v.first;b;b=b->next)
        for(const UCEntityListNode*n=b->head;n;n=n->next){
            UCEntityRecord*r=n->target;if(r->id==id)return r;
        }
    return 0;
}
int uc_entity_record_insert(const UCEntityRecordMemory*m,UCEntityRecords*records,
        const UCEntityCreation*c,const UCEntityRegistrationTarget*t,int32_t id,
        UCEntityRecordInsertResult*out){
    if(!m||!m->allocate||!records||!c||!t||!t->identity||!t->flags||!t->parent_id||!out)return 1;
    if(!uc_entity_records_traversable(records,1))return 1;
    UCEntityRecord*parent=c->relative?find(records,*t->parent_id):0;
    UCEntityRecord*r=m->allocate(m->owner);
    if(!r)return 2;
    *r=(UCEntityRecord){id,t->identity,t->flags};
    int inserted=c->relative?uc_entity_records_insert_after(records,parent,r):
        uc_entity_records_insert_priority(records,r,c->priority);
    *out=(UCEntityRecordInsertResult){r,(uint8_t)inserted};return 0;
}
static int remove_record(const UCEntityRecordMemory*m,UCEntityRecords*records,
        int32_t id,uint8_t*out,int update){
    if(!m||!m->release||!out||(!records&&!update))return 1;
    if(records&&!uc_entity_records_traversable(records,1))return 1;
    UCEntityRecord*r=find(records,id);*out=0;if(!r)return 0;
    if(update&&r->target){if(*r->flags&8)return 0;*r->flags|=8;}
    uc_entity_records_remove_first(records,r);
    m->release(m->owner,r);*out=1;return 0;
}
int uc_entity_record_remove_update(const UCEntityRecordMemory*m,UCEntityRecords*r,int32_t id,uint8_t*out){return remove_record(m,r,id,out,1);}
int uc_entity_record_remove_render(const UCEntityRecordMemory*m,UCEntityRecords*r,int32_t id,uint8_t*out){return remove_record(m,r,id,out,0);}

#include "entity_records.h"
#include <stdlib.h>
struct UCEntityRecords {
    uint32_t capacity,bucket_count,source_count,source_word,first_index;
    UCEntityListNode *nodes,*free,*source_head,*source_cursor;
    UCEntityRecordBucket *buckets,*first;
};
UCEntityRecords *uc_entity_records_create(uint32_t nodes,uint32_t buckets){
    if(!nodes||nodes>65536||buckets>65536)return NULL;
    if(!buckets)buckets=1;if(buckets>nodes)buckets=nodes;
    UCEntityRecords*p=calloc(1,sizeof *p);if(!p)return NULL;
    p->nodes=malloc((size_t)nodes*sizeof *p->nodes);
    p->buckets=calloc(buckets,sizeof *p->buckets);
    if(!p->nodes||!p->buckets){free(p->buckets);free(p->nodes);free(p);return NULL;}
    p->capacity=nodes;p->bucket_count=buckets;p->first_index=1;
    for(uint32_t i=0;i<nodes;i++){p->nodes[i]=(UCEntityListNode){NULL,p->free,NULL};p->free=p->nodes+i;}
    for(uint32_t i=0;i<buckets;i++)p->buckets[i].index=i;
    return p;
}
void uc_entity_records_destroy(UCEntityRecords*p){if(p){free(p->buckets);free(p->nodes);free(p);}}
UCEntityRecordsView uc_entity_records_view(const UCEntityRecords*p){
    if(!p)return (UCEntityRecordsView){0};
    return (UCEntityRecordsView){p->capacity,p->bucket_count,p->source_count,p->source_word,
        p->first_index,p->nodes,p->free,p->source_head,p->source_cursor,p->buckets,p->first};
}
int uc_entity_records_traversable(const UCEntityRecords*p,int nonempty){
    if(!p)return 0;uint32_t buckets=0,nodes=0;
    for(const UCEntityRecordBucket*b=p->first;b;b=b->next){
        if(++buckets>p->bucket_count||(nonempty&&!b->head))return 0;
        for(const UCEntityListNode*n=b->head;n;n=n->next)if(++nodes>p->capacity)return 0;
    }
    return 1;
}
int uc_entity_records_insert_priority(UCEntityRecords*p,void*value,int32_t priority){
    if(!p)return -1;UCEntityListNode*n=p->free;if(!n)return 0;
    p->free=n->next;n->target=value;
    if(priority>=(int32_t)p->bucket_count)priority=(int32_t)p->bucket_count-1;
    if(priority<0)priority=0;
    UCEntityRecordBucket*b=p->buckets+priority;n->next=b->head;
    if(b->head)b->head->previous=n;
    else{
        for(int32_t i=priority-1;i>=0;i--)if(p->buckets[i].head){
            UCEntityRecordBucket*lower=p->buckets+i;b->previous=lower;b->next=lower->next;
            if(lower->next)lower->next->previous=b;lower->next=b;break;
        }
        if(!p->first){p->first_index=(uint32_t)priority;p->first=b;}
        else if((uint32_t)priority<p->first_index){
            p->buckets[p->first_index].previous=b;b->next=p->buckets+p->first_index;
            p->first_index=(uint32_t)priority;p->first=b;
        }
    }
    b->head=n;if(!priority){if(p->source_head)p->source_head->previous=n;p->source_head=n;}
    return 1;
}
int uc_entity_records_insert_after(UCEntityRecords*p,const void*parent,void*value){
    if(!p)return -1;UCEntityListNode*n=p->free;if(!n)return 0;
    p->free=n->next;n->target=value;
    for(UCEntityRecordBucket*b=p->first;b;b=b->next)for(UCEntityListNode*q=b->head;q;q=q->next)
        if(q->target==parent){n->next=q->next;n->previous=q;if(q->next)q->next->previous=n;q->next=n;return 1;}
    return 0;
}
int uc_entity_records_remove_first(UCEntityRecords*p,const void*value){
    if(!p)return -1;
    for(UCEntityRecordBucket*b=p->first;b;b=b->next)for(UCEntityListNode*n=b->head;n;n=n->next)
        if(n->target==value){
            if(n->next)n->next->previous=n->previous;
            if(n->previous)n->previous->next=n->next;
            if(b->head==n){b->head=n->next;if(!n->next){
                if(p->first==b){if(b->next){p->first_index=b->next->index;p->first=p->buckets+p->first_index;}else p->first=NULL;}
                if(b->previous){b->previous->next=b->next;if(b->next)b->next->previous=b->previous;}
            }}
            if(p->source_head==n)p->source_head=n->next;
            n->previous=NULL;n->next=p->free;p->free=n;return 1;
        }
    return 0;
}

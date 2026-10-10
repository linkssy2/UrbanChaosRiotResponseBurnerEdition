#include "entity_list.h"
#include <stdlib.h>
struct UCEntityList {
    uint32_t count,capacity;
    UCEntityListNode *nodes,*free,*head,*source_cursor;
    uint32_t source_word;
};
UCEntityList *uc_entity_list_create(uint32_t capacity){
    if(capacity>65536)return NULL;
    if(!capacity)capacity=1;
    UCEntityList *p=calloc(1,sizeof *p);if(!p)return NULL;
    p->nodes=malloc((size_t)capacity*sizeof *p->nodes);
    if(!p->nodes){free(p);return NULL;}
    p->capacity=capacity;
    for(uint32_t i=0;i<capacity;i++){
        p->nodes[i]=(UCEntityListNode){NULL,p->free,NULL};p->free=p->nodes+i;
    }
    return p;
}
void uc_entity_list_destroy(UCEntityList*p){if(p){free(p->nodes);free(p);}}
UCEntityListView uc_entity_list_view(const UCEntityList*p){
    if(!p)return (UCEntityListView){0};
    return (UCEntityListView){p->count,p->capacity,p->nodes,p->free,p->head,
        p->source_cursor,p->source_word};
}
int uc_entity_list_prepend(UCEntityList*p,void*target){
    if(!p)return -1;
    UCEntityListNode*n=p->free;if(!n)return 0;
    p->free=n->next;n->target=target;n->next=p->head;
    if(p->head)p->head->previous=n;
    p->count++;p->head=n;return 1;
}
int uc_entity_list_remove_first(UCEntityList*p,const void*target){
    if(!p)return -1;
    UCEntityListNode*n=p->head;while(n&&n->target!=target)n=n->next;
    if(!n)return 0;
    if(n->next)n->next->previous=n->previous;
    if(n->previous)n->previous->next=n->next;
    if(p->head==n)p->head=n->next;
    if(p->source_cursor==n)p->source_cursor=n->previous;
    n->previous=NULL;n->next=p->free;p->free=n;p->count--;return 1;
}

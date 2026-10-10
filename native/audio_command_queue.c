#include "audio_command_queue.h"
#include <stddef.h>
static UCAudioPendingCommand *command(UCAudioLink*p){return (UCAudioPendingCommand*)((char*)p - offsetof(UCAudioPendingCommand,membership));}
static uint16_t *index_of(UCAudioPendingCommand*p,unsigned ready){return ready?&p->ready_index:&p->heap_index;}
static int shape(const UCAudioCommandQueue*q){return q&&q->items&&q->capacity&&q->capacity<=65535&&q->count<=q->capacity;}
static void swap(UCAudioCommandQueue*q,unsigned a,unsigned b,unsigned ready){UCAudioPendingCommand*p=q->items[a-1];q->items[a-1]=q->items[b-1];q->items[b-1]=p;*index_of(q->items[a-1],ready)=(uint16_t)a;*index_of(p,ready)=(uint16_t)b;}
static void pop_heap(UCAudioCommandQueue*q){
    unsigned old=q->count,i=1;UCAudioPendingCommand*removed=q->items[0];
    if(old!=1){q->items[0]=q->items[old-1];q->items[0]->heap_index=1;q->items[old-1]=NULL;}
    while(i*2<old){unsigned child=i*2;
        if(child+1<old&&q->items[child-1]->time>=q->items[child]->time)++child;
        if(q->items[i-1]->time<=q->items[child-1]->time)break;
        swap(q,i,child,0);i=child;
    }
    --q->count;removed->heap_index=0;
}
int uc_audio_commands_push(UCAudioCommandQueue*q,UCAudioPendingCommand*p,unsigned ready,uint8_t*added){
    if(!shape(q)||!p||ready>1||!added||p->heap_index||p->ready_index||p->time>INT64_MAX)return 1;
    if(q->count==q->capacity){*added=0;return 0;}
    unsigned i=++q->count;q->items[i-1]=p;*index_of(p,ready)=(uint16_t)i;
    if(!ready)while(i>1&&q->items[i/2-1]->time>q->items[i-1]->time){swap(q,i,i/2,0);i/=2;}
    *added=1;return 0;
}
int uc_audio_commands_remove(UCAudioCommandQueue*q,UCAudioPendingCommand*p,unsigned ready,uint8_t*removed){
    if(!shape(q)||!p||ready>1||!removed)return 1;
    unsigned i=*index_of(p,ready);if(!i){*removed=0;return 0;}
    if(i>q->count||q->items[i-1]!=p)return 1;
    if(!ready){while(i>1){swap(q,i,i/2,0);i/=2;}pop_heap(q);}
    else{swap(q,1,i,1);if(q->count!=1){q->items[0]=q->items[q->count-1];q->items[0]->ready_index=1;q->items[q->count-1]=NULL;}--q->count;p->ready_index=0;}
    *removed=1;return 0;
}
static int valid(const UCAudioCommandQueue*q,unsigned ready){
    if(!shape(q))return 0;
    for(unsigned i=0;i<q->count;i++){UCAudioPendingCommand*p=q->items[i];if(!p||*index_of(p,ready)!=i+1||*index_of(p,!ready)||p->time>INT64_MAX)return 0;}
    return 1;
}
int uc_audio_commands_cancel(UCAudioCommandQueue*heap,UCAudioCommandQueue*ready,UCAudioInstanceTrack*t,void(*release)(void*,UCAudioPendingCommand*),void*ctx){
    if(!t||!release||heap==ready||!valid(heap,0)||!valid(ready,1)||heap->items==ready->items)return 1;
    UCAudioLink*head=&t->commands,*node=head->next,*previous=head;unsigned count=0;
    if(!node||!head->previous)return 1;
    while(node!=head){
        if(!node||node->previous!=previous||++count>65535)return 1;UCAudioPendingCommand*p=command(node);
        if(p->track!=t||p->references!=1||p->heap_index&&p->ready_index)return 1;
        if(p->heap_index&&(p->heap_index>heap->count||heap->items[p->heap_index-1]!=p))return 1;
        if(p->ready_index&&(p->ready_index>ready->count||ready->items[p->ready_index-1]!=p))return 1;
        previous=node;node=node->next;
    }
    if(head->previous!=previous)return 1;
    node=head->next;
    while(node!=head){UCAudioPendingCommand*p=command(node);node=node->next;uint8_t removed;
        uc_audio_commands_remove(heap,p,0,&removed);if(!removed)uc_audio_commands_remove(ready,p,1,&removed);
        --p->references;uc_audio_link_remove(&p->membership);release(ctx,p);
    }
    return 0;
}

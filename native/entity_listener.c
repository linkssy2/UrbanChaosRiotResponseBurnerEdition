#include "entity_listener.h"
#include <stdlib.h>
#include <string.h>
typedef struct Node {struct Node *next,*previous;int32_t id;} Node;
struct UCEntityListenerList {Node head;uint32_t count;Node *active;};
struct UCEntitySubscriptions {UCEntityEventListener listener;uint32_t capacity;};
int uc_entity_listener_list_create(UCEntityListenerList **out){
    if(!out)return 1;UCEntityListenerList *s=calloc(1,sizeof *s);if(!s)return 1;
    s->head.next=s->head.previous=&s->head;*out=s;return 0;
}
void uc_entity_listener_list_close(UCEntityListenerList *s){
    if(!s)return;Node*n=s->head.next;while(n!=&s->head){Node*next=n->next;free(n);n=next;}free(s);
}
uint32_t uc_entity_listener_list_count(const UCEntityListenerList*s){return s?s->count:0;}
int uc_entity_listener_list_copy(const UCEntityListenerList*s,int32_t*out,uint32_t capacity){
    if(!s||capacity<s->count||(s->count&&!out))return 1;
    uint32_t i=0;for(const Node*n=s->head.next;n!=&s->head;n=n->next)out[i++]=n->id;return 0;
}
int uc_entity_listener_list_remove_first(UCEntityListenerList*s,int32_t id){
    if(!s)return 1;
    for(Node*n=s->head.next;n!=&s->head;n=n->next)if(n->id==id){
        if(n==s->active)return 1;
        n->previous->next=n->next;n->next->previous=n->previous;free(n);--s->count;break;
    }
    return 0;
}
int uc_entity_listener_broadcast(UCEntityListenerList*s,const UCEntityEventRegistry*r,
 uint32_t event,const void*payload,uint32_t size,uint32_t*sender){
 if(!s||!r||s->active)return 1;unsigned visits=0;
 for(Node*n=s->head.next;n!=&s->head;){
  if(++visits>65536){s->active=NULL;return 2;}
  s->active=n;uint8_t handled;
  if(uc_entity_event_send(r,n->id,event,payload,size,sender,&handled)){s->active=NULL;return 2;}
  n=n->next;
 }
 s->active=NULL;return 0;
}
int uc_entity_listener_register(const UCEntityEventRegistry*r,int32_t request,
        int32_t target_id,UCEntityEventListener*listener,UCEntityListenerList*s,int*inserted){
    if(!r||!listener||!inserted)return 1;
    UCEntityEventTarget*t=uc_entity_event_lookup(r,request);if(!t){*inserted=0;return 0;}
    if(!t->listeners[0]){
        if(!s||s->count>=65536)return 1;Node*n=malloc(sizeof *n);if(!n)return 1;
        n->next=&s->head;n->previous=s->head.previous;n->id=target_id;++s->count;
        s->head.previous=n;n->previous->next=n;
    }
    for(unsigned i=0;i<3;i++)if(!t->listeners[i]){t->listeners[i]=listener;*inserted=1;return 0;}
    *inserted=0;return 0;
}
int uc_entity_subscriptions_create(void*owner,UCEntitySubscriptions**out){
    if(!out)return 1;UCEntitySubscriptions*s=calloc(1,sizeof *s);if(!s)return 1;
    s->listener.owner=owner;*out=s;return 0;
}
UCEntityEventListener *uc_entity_subscriptions_listener(UCEntitySubscriptions*s){return s?&s->listener:NULL;}
uint32_t uc_entity_subscriptions_capacity(const UCEntitySubscriptions*s){return s?s->capacity:0;}
int uc_entity_subscriptions_append(UCEntitySubscriptions*s,const UCEntityEventSubscription*entry){
    if(!s||!entry||!entry->receive||s->listener.count>=256)return 1;
    UCEntityEventSubscription value=*entry,*data=(UCEntityEventSubscription*)s->listener.subscriptions;
    uint32_t count=s->listener.count;
    if(count==s->capacity){
        uint32_t capacity=s->capacity+s->capacity/2;if(capacity<count+1)capacity=count+1;
        UCEntityEventSubscription*next=malloc((size_t)capacity*sizeof *next);if(!next)return 1;
        if(count)memcpy(next,data,(size_t)count*sizeof *next);next[count]=value;if(data)free(data);
        s->capacity=capacity;s->listener.count=count+1;s->listener.subscriptions=next;
    }else{data[count]=value;s->listener.count=count+1;}
    return 0;
}
void uc_entity_subscriptions_clear(UCEntitySubscriptions*s){if(s){free((void*)s->listener.subscriptions);s->listener.subscriptions=NULL;s->listener.count=0;s->capacity=0;}}
void uc_entity_subscriptions_close(UCEntitySubscriptions*s){if(s){uc_entity_subscriptions_clear(s);free(s);}}

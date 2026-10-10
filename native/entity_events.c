#include "entity_events.h"
UCEntityEventTarget *uc_entity_event_lookup(const UCEntityEventRegistry *r,int32_t id){
    if(!r||id==-1)return 0;
    const UCEntityEventSlot *s=&r->slots[(uint32_t)id&1023];
    return s->id==id?s->target:0;
}
int uc_entity_event_send(const UCEntityEventRegistry *r,int32_t id,uint32_t event,
    const void *payload,uint32_t size,uint32_t *sender,uint8_t *handled){
    if(!r||!handled)return 1;
    (void)size; /* This actual subscription receiver does not inspect arg3. */
    UCEntityEventTarget *t=uc_entity_event_lookup(r,id);
    if(t)for(unsigned i=0;i<3&&t->listeners[i];++i){
        const UCEntityEventListener *l=t->listeners[i];
        if(l->count>256||(l->count&&!l->subscriptions))return 1;
        for(uint32_t j=0;j<l->count;++j)if(!l->subscriptions[j].receive)return 1;
    }
    if(t)for(unsigned i=0;i<3&&t->listeners[i];++i){
        const UCEntityEventListener *l=t->listeners[i];
        for(uint32_t j=0;j<l->count;++j)if(l->subscriptions[j].event==event){
            if(l->subscriptions[j].receive(l->owner,payload,sender))return 2;
            *handled=1;return 0;
        }
    }
    *handled=0;return 0;
}

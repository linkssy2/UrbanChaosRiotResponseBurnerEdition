#include "collision_phantom_overlaps.h"
#include <stdlib.h>
struct UCCollisionPhantomOverlaps {
    UCCollisionPhantomOverlapsView view;
    UCCollisionPhantomOverlapHooks hooks;
    void *context;
    uint8_t active;
    const void **items;
};
static uint32_t find(const UCCollisionPhantomOverlaps *p, const void *collidable) {
    uint32_t i=0;while(i<p->view.count&&p->items[i]!=collidable)++i;return i;
}
static int listeners(UCCollisionPhantomOverlaps *p,UCCollisionPhantomOverlapListeners *list) {
    *list=(UCCollisionPhantomOverlapListeners){0};p->hooks.listeners(p->context,list);
    return list->count<=list->capacity&&list->capacity<=65536&&(!list->capacity||list->data);
}
UCCollisionStatus uc_collision_phantom_overlaps_create(const void *phantom,uint32_t capacity,
    const UCCollisionPhantomOverlapHooks *hooks,void *context,UCCollisionPhantomOverlaps **out) {
    if(!phantom||capacity>65536||!hooks||!hooks->listeners||!hooks->invoke||!out||*out)return UC_COLLISION_ARGUMENT;
    UCCollisionPhantomOverlaps *p=calloc(1,sizeof(*p)+(size_t)capacity*sizeof(*p->items));
    if(!p)return UC_COLLISION_MEMORY;
    p->items=(const void **)(p+1);p->view=(UCCollisionPhantomOverlapsView){phantom,p->items,0,capacity};p->hooks=*hooks;p->context=context;*out=p;
    return UC_COLLISION_OK;
}
UCCollisionStatus uc_collision_phantom_overlaps_dispose(UCCollisionPhantomOverlaps *p) {
    if(!p||p->active||p->view.count)return UC_COLLISION_ARGUMENT;free(p);return UC_COLLISION_OK;
}
const UCCollisionPhantomOverlapsView *uc_collision_phantom_overlaps_view(const UCCollisionPhantomOverlaps *p) {
    return p?&p->view:NULL;
}
UCCollisionStatus uc_collision_phantom_overlaps_contains(const UCCollisionPhantomOverlaps *p,const void *other,int *present) {
    if(!p||!other||!present)return UC_COLLISION_ARGUMENT;*present=find(p,other)<p->view.count;return UC_COLLISION_OK;
}
UCCollisionStatus uc_collision_phantom_overlaps_change(UCCollisionPhantomOverlaps *p,const void *other,int added,uint32_t budget) {
    if(!p||!other||(added!=0&&added!=1)||p->active)return UC_COLLISION_ARGUMENT;
    if(added&&p->view.count==p->view.capacity)return UC_COLLISION_LIMIT;
    UCCollisionPhantomOverlapListeners list;
    if(!listeners(p,&list))return UC_COLLISION_ARGUMENT;
    uint32_t index=added?p->view.count:find(p,other),remaining=list.count;
    UCCollisionPhantomOverlapEvent event={p->view.phantom,other,0,(uint8_t)(!added&&index<p->view.count)};
    UCCollisionStatus status=UC_COLLISION_OK;p->active=1;
    while(remaining) {
        --remaining;
        if(!listeners(p,&list)||remaining>=list.capacity){status=UC_COLLISION_ARGUMENT;break;}
        const void *listener=list.data[remaining];if(!listener)continue;
        if(!budget){status=UC_COLLISION_LIMIT;break;}--budget;
        p->hooks.invoke(p->context,listener,&event,added);
        if(event.phantom!=p->view.phantom||event.collidable!=other||!listeners(p,&list)) {status=UC_COLLISION_ARGUMENT;break;}
    }
    if(status==UC_COLLISION_OK) {
        if(added){if(!event.acceptance)p->items[p->view.count++]=other;}
        else if(index<p->view.count){--p->view.count;p->items[index]=p->items[p->view.count];}
    }
    p->active=0;return status;
}

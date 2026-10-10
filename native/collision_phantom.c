#include "collision_phantom.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
struct UCCollisionPhantom {
    UCCollisionPhantomView view;
    UCCollisionPhantomMember member;
    uint16_t references;
    const void **listeners;
};
UCCollisionStatus uc_collision_phantom_create(const float lo[4],const float hi[4],
    uint32_t filter,uint32_t capacity,UCCollisionPhantom **out) {
    if(!lo||!hi||!out||*out||capacity>65536)return UC_COLLISION_ARGUMENT;
    for(unsigned i=0;i<3;i++)if(!isfinite(lo[i])||!isfinite(hi[i])||lo[i]>hi[i])return UC_COLLISION_ARGUMENT;
    UCCollisionPhantom *p=calloc(1,sizeof(*p)+(size_t)capacity*sizeof(void *));
    if(!p)return UC_COLLISION_MEMORY;
    memcpy(p->view.lower,lo,16);memcpy(p->view.upper,hi,16);
    p->view.collidable=(UCCollisionPhantomCollidable){&p->view.collidable,p,NULL,2,UINT32_MAX,filter};
    p->listeners=(const void **)(p+1);p->view.listeners=p->listeners;p->view.listener_capacity=capacity;
    p->member.broadphase=(UCCollisionSAPHandle){&p->view.collidable,UC_COLLISION_SAP_DETACHED};
    p->references=1;*out=p;return UC_COLLISION_OK;
}
UCCollisionStatus uc_collision_phantom_retain(UCCollisionPhantom *p) {
    if(!p||!p->references)return UC_COLLISION_ARGUMENT;
    if(p->references==UINT16_MAX)return UC_COLLISION_LIMIT;
    ++p->references;return UC_COLLISION_OK;
}
UCCollisionStatus uc_collision_phantom_release(UCCollisionPhantom *p) {
    if(!p||!p->references)return UC_COLLISION_ARGUMENT;
    if(p->references==1&&(p->member.world||p->member.broadphase.index!=UC_COLLISION_SAP_DETACHED))return UC_COLLISION_UNSUPPORTED;
    if(!--p->references)free(p);
    return UC_COLLISION_OK;
}
UCCollisionStatus uc_collision_phantom_add_listener(UCCollisionPhantom *p,const void *listener) {
    if(!p)return UC_COLLISION_ARGUMENT;
    if(p->view.listener_count==p->view.listener_capacity)return UC_COLLISION_LIMIT;
    p->listeners[p->view.listener_count++]=listener;return UC_COLLISION_OK;
}
UCCollisionStatus uc_collision_phantom_remove_listener(UCCollisionPhantom *p,const void *listener) {
    if(!p)return UC_COLLISION_ARGUMENT;
    uint32_t i=0,n=p->view.listener_count;
    while(i<n&&p->listeners[i]!=listener)++i;
    if(i==n)return UC_COLLISION_ARGUMENT;
    memmove(p->listeners+i,p->listeners+i+1,(n-i-1)*sizeof(void *));
    --p->view.listener_count;return UC_COLLISION_OK;
}
const UCCollisionPhantomView *uc_collision_phantom_view(const UCCollisionPhantom *p){return p?&p->view:NULL;}
UCCollisionPhantomMember *uc_collision_phantom_member(UCCollisionPhantom *p){return p?&p->member:NULL;}
uint16_t uc_collision_phantom_references(const UCCollisionPhantom *p){return p?p->references:0;}

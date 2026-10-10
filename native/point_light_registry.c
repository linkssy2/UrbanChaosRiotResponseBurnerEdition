#include "point_light_registry.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
struct UCPointLightRegistry {uint32_t capacity,count,owned;const UCModelMembershipLight **items;};
UCPointLightRegistry *uc_point_light_registry_create(uint32_t capacity){
    if(!capacity||capacity>65536)return NULL;
    UCPointLightRegistry *r=calloc(1,sizeof(*r)+(size_t)capacity*sizeof(void*));
    if(r){r->capacity=capacity;r->items=(const UCModelMembershipLight**)(r+1);}return r;
}
int uc_point_light_registry_free(UCPointLightRegistry*r){if(!r||r->count||r->owned)return 0;free(r);return 1;}
int uc_point_light_registry_add(UCPointLightRegistry*r,const UCModelMembershipLight*l){
    if(!r||!l||r->count==r->capacity)return 0;r->items[r->count++]=l;return 1;
}
uint32_t uc_point_light_registry_remove(UCPointLightRegistry*r,const UCModelMembershipLight*l){
    if(!r||!l)return 0;uint32_t out=0,removed=0;
    for(uint32_t i=0;i<r->count;i++)if(r->items[i]==l)removed++;else r->items[out++]=r->items[i];
    r->count=out;return removed;
}
const UCModelMembershipLight *const *uc_point_light_registry_view(const UCPointLightRegistry*r,uint32_t*n){if(!r||!n)return NULL;*n=r->count;return r->items;}
UCPointLightInstance *uc_point_light_registry_point(UCPointLightRegistry*r,const float p[3],float attenuation,const void*device){
    if(!r||!p||r->count==r->capacity||r->owned==r->capacity||!isfinite(attenuation))return NULL;
    for(unsigned i=0;i<3;i++)if(!isfinite(p[i]))return NULL;
    UCPointLightInstance value={0};
    for(unsigned i=0;i<4;i++){value.world[i*5]=1;value.state.color[i]=1;}
    memcpy(value.local_center,p,12);value.local_center[3]=1;
    value.state.enabled=1;value.state.multiplier=1;value.state.attenuation=attenuation;value.state.dirty=1;
    if(!uc_point_light_geometry_update(&value.state,value.world,value.local_center,&value.geometry))return NULL;
    UCPointLightInstance *l=malloc(sizeof(*l));if(!l)return NULL;*l=value;
    l->membership.state=&l->state;l->membership.geometry=&l->geometry;l->membership.local_center=l->local_center;l->membership.index=-1;
    l->device=device;l->registry=r;r->items[r->count++]=&l->membership;r->owned++;return l;
}
int uc_point_light_registry_destroy(UCPointLightRegistry*r,UCPointLightInstance*l){
    if(!r||!l||l->registry!=r||!r->owned)return 0;
    uc_point_light_registry_remove(r,&l->membership);r->owned--;free(l);return 1;
}

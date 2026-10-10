#include "bullet_observers.h"
#include <stdlib.h>
#include <string.h>
struct UCBulletObservers {uint32_t count,capacity;uint8_t busy;const UCBulletObserver **items;};
int uc_bullet_observers_create(uint32_t capacity,UCBulletObservers **out){
    if(!out||!capacity||capacity>65536)return 1;
    UCBulletObservers*p=malloc(sizeof *p+capacity*sizeof(*p->items));if(!p)return 1;
    p->count=0;p->capacity=capacity;p->busy=0;p->items=(const UCBulletObserver**)(p+1);*out=p;return 0;
}
static uint32_t lower(const UCBulletObservers*p,const UCBulletObserver*t){
    uint32_t lo=0,hi=p->count;
    while(lo<hi){uint32_t m=lo+(hi-lo)/2;if((uintptr_t)p->items[m]<(uintptr_t)t)lo=m+1;else hi=m;}
    return lo;
}
int uc_bullet_observers_add(UCBulletObservers*p,const UCBulletObserver*t,uint8_t*inserted){
    if(!p||!t||!t->segment||!inserted||p->busy)return 1;
    uint32_t i=lower(p,t);if(i<p->count&&p->items[i]==t){*inserted=0;return 0;}
    if(p->count==p->capacity)return 1;
    memmove(p->items+i+1,p->items+i,(p->count-i)*sizeof(*p->items));p->items[i]=t;p->count++;*inserted=1;return 0;
}
int uc_bullet_observers_remove(UCBulletObservers*p,const UCBulletObserver*t,uint8_t*removed){
    if(!p||!t||!removed||p->busy)return 1;
    uint32_t i=lower(p,t);*removed=(uint8_t)(i<p->count&&p->items[i]==t);
    if(*removed){p->count--;memmove(p->items+i,p->items+i+1,(p->count-i)*sizeof(*p->items));}return 0;
}
int uc_bullet_observers_clear(UCBulletObservers*p){if(!p||p->busy)return 1;p->count=0;return 0;}
int uc_bullet_observers_dispatch(UCBulletObservers*p,const UCBulletSegment*s){
    if(!p||p->busy)return 1;if(!p->count)return 0;if(!s)return 1;
    for(uint32_t i=0;i<p->count;i++)if(!p->items[i]->segment)return 1;
    p->busy=1;
    for(uint32_t i=0;i<p->count;i++)if(p->items[i]->segment(p->items[i]->owner,s)){p->busy=0;return 2;}
    p->busy=0;return 0;
}
int uc_bullet_observers_close(UCBulletObservers*p){if(p&&p->busy)return 1;free(p);return 0;}
const UCBulletObserver *const *uc_bullet_observers_view(const UCBulletObservers*p,uint32_t*count){
    if(!p||!count)return NULL;*count=p->count;return p->items;
}

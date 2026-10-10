#include "collision_pair_reconcile.h"
#include <stdlib.h>
typedef struct Entry {uintptr_t lo,hi;uint32_t index,count;} Entry;
static Entry key(UCCollisionSAPPair p,uint32_t index) {
    uintptr_t a=(uintptr_t)p.first,b=(uintptr_t)p.second;
    return (Entry){a<b?a:b,a<b?b:a,index,1};
}
static int compare_key(const Entry *a,const Entry *b) {
    if(a->lo!=b->lo)return a->lo<b->lo?-1:1;
    return a->hi==b->hi?0:a->hi<b->hi?-1:1;
}
static int compare(const void *a,const void *b) {
    const Entry *x=a,*y=b;int c=compare_key(x,y);
    return c?c:x->index==y->index?0:x->index<y->index?-1:1;
}
static int overlap(const void *a,size_t an,const void *b,size_t bn) {
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return an&&bn&&(x<=y?y-x<an:x-y<bn);
}
UCCollisionStatus uc_collision_pair_reconcile(UCCollisionSAPPairs *a,UCCollisionSAPPairs *b) {
    if(!a||!b||overlap(a,sizeof(*a),b,sizeof(*b)))return UC_COLLISION_ARGUMENT;
    if(a->count>UC_COLLISION_PAIR_RECONCILE_MAX||b->count>UC_COLLISION_PAIR_RECONCILE_MAX)return UC_COLLISION_LIMIT;
    UCCollisionSAPPairs *lists[2]={a,b};size_t sizes[2];
    for(unsigned i=0;i<2;i++) {
        UCCollisionSAPPairs *p=lists[i];
        if(p->count>p->capacity||(p->capacity&&!p->data)||p->capacity>SIZE_MAX/sizeof(*p->data))return UC_COLLISION_ARGUMENT;
        sizes[i]=(size_t)p->capacity*sizeof(*p->data);
        if(sizes[i]>UINTPTR_MAX-(uintptr_t)p->data||overlap(p->data,sizes[i],a,sizeof(*a))||overlap(p->data,sizes[i],b,sizeof(*b)))return UC_COLLISION_ARGUMENT;
    }
    if(overlap(a->data,sizes[0],b->data,sizes[1]))return UC_COLLISION_ARGUMENT;
    for(unsigned i=0;i<2;i++)for(uint32_t j=0;j<lists[i]->count;j++)
        if(!lists[i]->data[j].first||!lists[i]->data[j].second)return UC_COLLISION_ARGUMENT;
    if(a->count<32||b->count<32) {
        for(uint32_t i=0;i<b->count;) {
            uint32_t j=0;Entry k=key(b->data[i],0);
            while(j<a->count){Entry other=key(a->data[j],0);if(!compare_key(&k,&other))break;++j;}
            if(j==a->count){++i;continue;}
            a->data[j]=a->data[--a->count];b->data[i]=b->data[--b->count];
        }
        return UC_COLLISION_OK;
    }
    Entry *map=malloc((size_t)a->count*sizeof(*map));if(!map)return UC_COLLISION_MEMORY;
    for(uint32_t i=0;i<a->count;i++)map[i]=key(a->data[i],i);
    qsort(map,a->count,sizeof(*map),compare);uint32_t unique=0;
    for(uint32_t i=0;i<a->count;i++) {
        if(unique&&!compare_key(map+unique-1,map+i)){++map[unique-1].count;a->data[map[i].index].first=NULL;}
        else map[unique++]=map[i];
    }
    for(uint32_t i=0;i<b->count;) {
        Entry k=key(b->data[i],0);uint32_t lo=0,hi=unique;
        while(lo<hi){uint32_t mid=lo+(hi-lo)/2;if(compare_key(map+mid,&k)<0)lo=mid+1;else hi=mid;}
        if(lo==unique||compare_key(map+lo,&k)||!map[lo].count){++i;continue;}
        if(!--map[lo].count)a->data[map[lo].index].first=NULL;
        b->data[i]=b->data[--b->count];
    }
    uint32_t count=0;for(uint32_t i=0;i<a->count;i++)if(a->data[i].first)a->data[count++]=a->data[i];
    a->count=count;free(map);return UC_COLLISION_OK;
}

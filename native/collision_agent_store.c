#include "collision_agent_store.h"
#include <string.h>

#define CAP(x) ((x)&0x3fffffffu)
static UCCollisionAgent *entry(const UCCollisionAgentStore *s,uint32_t block,uint32_t offset) {
    return (UCCollisionAgent *)(s->blocks[block]+offset);
}
static int resident(const UCCollisionAgentStore *s,const UCCollisionAgent *a) {
    uintptr_t p=(uintptr_t)a;
    for(uint32_t b=0;b<s->count;++b) {
        uintptr_t base=(uintptr_t)s->blocks[b];
        uint32_t used=b+1==s->count?s->used_bytes:s->block_size;
        if(p>=base&&p-base<used&&(p-base)%s->entry_size==0)return 1;
    }
    return 0;
}
UCCollisionStatus uc_collision_agent_store_validate(const UCCollisionAgentStore *s) {
    if(!s||s->active||!s->blocks||!CAP(s->capacity_flags)||s->count>CAP(s->capacity_flags)||
       CAP(s->capacity_flags)>65535||s->entry_size<sizeof(UCCollisionAgent)+16||
       s->entry_size%sizeof(void *)||!s->block_size||s->block_size%s->entry_size||
       !s->used_bytes||s->used_bytes>s->block_size||s->used_bytes%s->entry_size||
       (!s->count&&s->used_bytes!=s->block_size))return UC_COLLISION_ARGUMENT;
    if(s->blocks==&s->inline_block&&CAP(s->capacity_flags)!=1)return UC_COLLISION_ARGUMENT;
    if(s->count&&((uint64_t)(s->count-1)*s->block_size+s->used_bytes)/s->entry_size>65535)
        return UC_COLLISION_LIMIT;
    for(uint32_t b=0;b<s->count;++b) {
        if(!s->blocks[b]||(uintptr_t)s->blocks[b]%sizeof(void *))return UC_COLLISION_ARGUMENT;
        for(uint32_t j=0;j<b;++j)if(s->blocks[b]==s->blocks[j])return UC_COLLISION_ARGUMENT;
        uint32_t used=b+1==s->count?s->used_bytes:s->block_size;
        for(uint32_t p=0;p<used;p+=s->entry_size) {
            UCCollisionAgent *a=entry(s,b,p);
            if(!a->side[0]||!a->side[1]||a->side[0]==a->side[1])return UC_COLLISION_ARGUMENT;
            for(unsigned side=0;side<2;++side) {
                UCCollisionAgentBody *body=a->side[side];uint32_t i=a->index[side];
                if(!body->identity||!body->data||body->count>CAP(body->capacity_flags)||
                   CAP(body->capacity_flags)>65535||i>=body->count||
                   body->data[i].agent!=a||body->data[i].other!=a->side[1-side])
                    return UC_COLLISION_ARGUMENT;
                UCCollisionAgentLink tail=body->data[body->count-1];
                if(!resident(s,tail.agent))return UC_COLLISION_ARGUMENT;
                unsigned which=tail.agent->side[0]!=body;
                if(tail.agent->side[which]!=body||tail.agent->side[1-which]!=tail.other||
                   tail.agent->index[which]!=body->count-1)return UC_COLLISION_ARGUMENT;
            }
        }
    }
    return UC_COLLISION_OK;
}
static UCCollisionStatus shrink(void **data,uint32_t count,uint32_t *flags,size_t stride,
    void *embedded,const UCCollisionAgentStoreHooks *h,void *ctx) {
    uint32_t cap=CAP(*flags),n=cap/2,borrowed=0;void *next;
    if(embedded&&!count){next=embedded;n=1;borrowed=0x80000000u;}
    else {next=h->allocate(ctx,(size_t)n*stride,0x14);if(!next)return UC_COLLISION_MEMORY;}
    memcpy(next,*data,(size_t)count*stride);
    h->release(ctx,*data,(size_t)cap*stride,0x14);
    *data=next;*flags=(*flags&0x40000000u)|borrowed|n;return UC_COLLISION_OK;
}
UCCollisionStatus uc_collision_agent_store_remove(UCCollisionAgentStore *s,
    UCCollisionAgent *a,const UCCollisionAgentStoreHooks *h,void *ctx,int *applied) {
    if(!a||!h||!h->allocate||!h->release||!h->destroy||!applied)return UC_COLLISION_ARGUMENT;
    UCCollisionStatus status=uc_collision_agent_store_validate(s);if(status)return status;
    if(!resident(s,a))return UC_COLLISION_ARGUMENT;
    s->active=1;*applied=1;
    unsigned char header[sizeof(*a)];memcpy(header,a,sizeof(header));
    if(a->tag==2||a->tag==4||a->tag==6) {
        h->destroy(ctx,a,(unsigned char *)a+sizeof(*a)+(a->tag==4?16:0),a->manager);
        if(memcmp(header,a,sizeof(header))){status=UC_COLLISION_ARGUMENT;goto done;}
    }
    for(unsigned side=0;side<2;++side) {
        UCCollisionAgentBody *body=a->side[side];uint32_t i=a->index[side];
        body->data[i]=body->data[--body->count];
        if(i<body->count) {
            UCCollisionAgent *moved=body->data[i].agent;
            moved->index[moved->side[0]!=body]=(uint16_t)i;
        }
        if(body->count*2+8<=CAP(body->capacity_flags)) {
            status=shrink((void **)&body->data,body->count,&body->capacity_flags,
                sizeof(*body->data),NULL,h,ctx);if(status)goto done;
        }
    }
    UCCollisionAgent *last=entry(s,s->count-1,s->used_bytes-s->entry_size);
    if(a!=last) {
        memcpy(a,last,s->entry_size);
        for(unsigned side=0;side<2;++side)a->side[side]->data[a->index[side]].agent=a;
    }
    s->used_bytes-=s->entry_size;
    if(!s->used_bytes) {
        unsigned char *block=s->blocks[--s->count];
        if(!(s->capacity_flags&0x80000000u)&&(s->count+1)*2<CAP(s->capacity_flags)) {
            status=shrink((void **)&s->blocks,s->count,&s->capacity_flags,
                sizeof(*s->blocks),&s->inline_block,h,ctx);if(status)goto done;
        }
        h->release(ctx,block,s->block_size,0x24);s->used_bytes=s->block_size;
    }
done:
    s->active=0;return status;
}

#include "particle_registry.h"
#include <stdlib.h>
typedef struct Entry {const UCParticleLifecycleView *view;const UCGunFlareSpriteSource *sprite;uint32_t next,previous,free_next;unsigned active;} Entry;
struct UCParticleRegistry {Entry *items;uint32_t capacity,count,first,last,free_first;};
static uint32_t index_of(const UCParticleRegistry *r,uintptr_t token){
    if(!r||token<(uintptr_t)r->items)return UINT32_MAX;
    uintptr_t offset=token-(uintptr_t)r->items;
    if(offset%sizeof(Entry)||offset/sizeof(Entry)>=r->capacity)return UINT32_MAX;
    uint32_t i=(uint32_t)(offset/sizeof(Entry));return r->items[i].active?i:UINT32_MAX;
}
static uintptr_t token_of(const UCParticleRegistry *r,uint32_t i){return i==UINT32_MAX?0:(uintptr_t)(r->items+i);}
int uc_particle_registry_create(uint32_t capacity,UCParticleRegistry **out){
    if(!out||!capacity||capacity>65536)return 1;
    UCParticleRegistry *r=calloc(1,sizeof(*r));if(!r)return 2;
    r->items=calloc(capacity,sizeof(*r->items));if(!r->items){free(r);return 2;}
    r->capacity=capacity;r->first=r->last=UINT32_MAX;
    for(uint32_t i=0;i<capacity;i++)r->items[i].free_next=i+1<capacity?i+1:UINT32_MAX;
    *out=r;return 0;
}
static int add(UCParticleRegistry *r,const UCParticleLifecycleView *v,const UCGunFlareSpriteSource *s,uintptr_t *out){
    if(r->free_first==UINT32_MAX){*out=0;return 0;}
    uint32_t i=r->free_first;Entry *e=r->items+i;r->free_first=e->free_next;
    e->view=v;e->sprite=s;e->active=1;e->previous=r->last;e->next=UINT32_MAX;
    if(r->last==UINT32_MAX)r->first=i;else r->items[r->last].next=i;
    r->last=i;++r->count;*out=token_of(r,i);return 0;
}
int uc_particle_registry_add(UCParticleRegistry *r,const UCParticleLifecycleView *v,uintptr_t *out){
    if(!r||!v||!v->asset||!v->system||!out||(v->asset->particle_type!=0&&v->asset->particle_type!=1&&v->asset->particle_type!=2&&v->asset->particle_type!=3&&v->asset->particle_type!=5))return 1;
    return add(r,v,NULL,out);
}
int uc_particle_registry_add_sprite(UCParticleRegistry *r,const UCGunFlareSpriteSource *s,uintptr_t *out){
    if(!r||!s||!s->visible||!s->enabled||!s->texture||!s->position||!s->radius||!out)return 1;
    return add(r,NULL,s,out);
}
int uc_particle_registry_remove(UCParticleRegistry *r,uintptr_t token){
    if(!r)return 1;if(!token)return 0;uint32_t i=index_of(r,token);if(i==UINT32_MAX)return 1;
    Entry *e=r->items+i;
    if(e->previous==UINT32_MAX)r->first=e->next;else r->items[e->previous].next=e->next;
    if(e->next==UINT32_MAX)r->last=e->previous;else r->items[e->next].previous=e->previous;
    --r->count;e->active=0;e->view=NULL;e->sprite=NULL;e->free_next=r->free_first;r->free_first=i;return 0;
}
uint32_t uc_particle_registry_count(const UCParticleRegistry *r){return r?r->count:0;}
uintptr_t uc_particle_registry_first(const UCParticleRegistry *r){return r?token_of(r,r->first):0;}
uintptr_t uc_particle_registry_next(const UCParticleRegistry *r,uintptr_t token){uint32_t i=index_of(r,token);return i==UINT32_MAX?0:token_of(r,r->items[i].next);}
const UCParticleLifecycleView *uc_particle_registry_view(const UCParticleRegistry *r,uintptr_t token){uint32_t i=index_of(r,token);return i==UINT32_MAX?NULL:r->items[i].view;}
const UCGunFlareSpriteSource *uc_particle_registry_sprite(const UCParticleRegistry *r,uintptr_t token){uint32_t i=index_of(r,token);return i==UINT32_MAX?NULL:r->items[i].sprite;}
int uc_particle_registry_close(UCParticleRegistry *r){if(!r)return 0;if(r->count)return 1;free(r->items);free(r);return 0;}

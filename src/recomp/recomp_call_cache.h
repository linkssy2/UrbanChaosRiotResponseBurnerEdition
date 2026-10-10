#ifndef UCRR_CALL_CACHE_H
#define UCRR_CALL_CACHE_H
/* Only immutable manual/generated mappings. Never store kernel or unresolved
 * targets. Full guest VA is checked; callers still load each vtable target. */
typedef struct {uint32_t va[2];recomp_func_t fn[2];unsigned replace;} RecompCallSlot;
typedef struct {RecompCallSlot slot[64];} RecompCallCache;
static __forceinline recomp_func_t recomp_call_cache_get(RecompCallCache*c,uint32_t va){
 RecompCallSlot*s=&c->slot[((va>>2)^(va>>10))&63];
 if(s->fn[0]&&s->va[0]==va)return s->fn[0];
 if(s->fn[1]&&s->va[1]==va)return s->fn[1];return NULL;
}
static __forceinline void recomp_call_cache_put(RecompCallCache*c,uint32_t va,recomp_func_t fn){
 if(!fn)return;RecompCallSlot*s=&c->slot[((va>>2)^(va>>10))&63];unsigned i=s->replace++&1;
 s->va[i]=va;s->fn[i]=fn;
}
#endif

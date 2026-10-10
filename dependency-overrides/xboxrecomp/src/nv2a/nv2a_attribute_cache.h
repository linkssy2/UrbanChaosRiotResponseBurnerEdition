#ifndef NV2A_ATTRIBUTE_CACHE_H
#define NV2A_ATTRIBUTE_CACHE_H
#include <stdint.h>
#include <string.h>
/* Exact decoded attributes, not transformed outputs. Owner is the synchronous
 * decoder. Invalidate on guest reentry, object mapping changes, or ANY method
 * other than consecutive element submissions. Collisions are misses. */
#define NV_ATTRIBUTE_CACHE_SLOTS 1024u
typedef struct NvAttributeCache {
 uint32_t epoch, tags[NV_ATTRIBUTE_CACHE_SLOTS], keys[NV_ATTRIBUTE_CACHE_SLOTS];
 float values[NV_ATTRIBUTE_CACHE_SLOTS][16][4];
 uint64_t hits,misses;
} NvAttributeCache;
static void nv_attribute_cache_invalidate(NvAttributeCache *c){
 if(++c->epoch==0){memset(c->tags,0,sizeof(c->tags));c->epoch=1;}
}
/* Equivalent to count consecutive invalidations without intervening accesses.
 * Callers bound count to2047, so at most one rollover is possible. */
static void nv_attribute_cache_advance(NvAttributeCache*c,unsigned count){
 uint64_t sum=(uint64_t)c->epoch+count;
 if(sum>UINT32_MAX){memset(c->tags,0,sizeof(c->tags));sum-=UINT32_MAX;}
 c->epoch=(uint32_t)sum;
}
/* Borrow only until the next put/invalidation; never retain across guest entry. */
static const float (*nv_attribute_cache_lookup(NvAttributeCache*c,uint32_t key))[4]{
 unsigned slot=key&(NV_ATTRIBUTE_CACHE_SLOTS-1);
 if(c->epoch && c->tags[slot]==c->epoch && c->keys[slot]==key){
  ++c->hits;return c->values[slot];
 }
 ++c->misses;return 0;
}
static int nv_attribute_cache_get(NvAttributeCache*c,uint32_t key,float dst[16][4]){
 const float (*value)[4]=nv_attribute_cache_lookup(c,key);
 if(!value)return 0;
 memcpy(dst,value,sizeof(c->values[0]));return 1;
}
static void nv_attribute_cache_put(NvAttributeCache*c,uint32_t key,const float src[16][4]){
 unsigned slot=key&(NV_ATTRIBUTE_CACHE_SLOTS-1);
 if(!c->epoch)nv_attribute_cache_invalidate(c);
 memcpy(c->values[slot],src,sizeof(c->values[slot]));
 c->keys[slot]=key;c->tags[slot]=c->epoch;
}
#endif

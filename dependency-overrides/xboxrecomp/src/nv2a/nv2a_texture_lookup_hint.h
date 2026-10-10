#ifndef NV2A_TEXTURE_LOOKUP_HINT_H
#define NV2A_TEXTURE_LOOKUP_HINT_H
/* Host-owned identity hints only. No guest content validity is cached here.
 * The cache producer must maintain unique valid identities, as the original
 * first-match lookup followed by miss-only insertion does. Revalidate a slot
 * after every reuse; stale/colliding hints fall through to the original scan.
 * Call after existing format/layout/palette/range checks, before unchanged
 * surface feedback and source comparison/upload handling. */
typedef struct NvTextureLookupHint { unsigned slots[256]; } NvTextureLookupHint;
static int nv_texture_identity_equal(const LiveTextureCacheEntry*e,uint32_t offset,
 uint32_t format,uint32_t width,uint32_t height,unsigned levels){
 return e->valid&&e->offset==offset&&e->format==format&&e->width==width&&e->height==height&&e->levels==levels;
}
static unsigned nv_texture_hint_bucket(uint32_t offset,uint32_t format,uint32_t width,uint32_t height,unsigned levels){
 uint32_t h=(offset>>6)^(offset>>14)^(format*0x9e3779b9u)^(width*17u)^(height*131u)^(levels*8191u);
 return h&255u;
}
static LiveTextureCacheEntry*nv_texture_lookup_hint(NvTextureLookupHint*h,
 LiveTextureCacheEntry*entries,unsigned capacity,uint32_t offset,uint32_t format,
 uint32_t width,uint32_t height,unsigned levels){
 unsigned bucket=nv_texture_hint_bucket(offset,format,width,height,levels),slot=h->slots[bucket];
 if(slot&&slot<=capacity&&nv_texture_identity_equal(entries+slot-1,offset,format,width,height,levels))return entries+slot-1;
 for(unsigned i=0;i<capacity;++i)if(nv_texture_identity_equal(entries+i,offset,format,width,height,levels)){
  h->slots[bucket]=i+1;return entries+i;
 }
 h->slots[bucket]=0;return NULL;
}
/* Cheap front-of-cache hits retain direct scan. Only later entries pay hashing.
 * Full identity checks and original content/upload validation remain unchanged. */
static LiveTextureCacheEntry*nv_texture_lookup_adaptive(NvTextureLookupHint*h,
 LiveTextureCacheEntry*entries,unsigned capacity,uint32_t offset,uint32_t format,
 uint32_t width,uint32_t height,unsigned levels){
 unsigned front=capacity<8?capacity:8;
 for(unsigned i=0;i<front;++i)if(nv_texture_identity_equal(entries+i,offset,format,width,height,levels))return entries+i;
 unsigned bucket=nv_texture_hint_bucket(offset,format,width,height,levels),slot=h->slots[bucket];
 if(slot>front&&slot<=capacity&&nv_texture_identity_equal(entries+slot-1,offset,format,width,height,levels))return entries+slot-1;
 for(unsigned i=front;i<capacity;++i)if(nv_texture_identity_equal(entries+i,offset,format,width,height,levels)){
  h->slots[bucket]=i+1;return entries+i;
 }
 h->slots[bucket]=0;return NULL;
}
#endif

#ifndef NV_GEOMETRY_RESIDENT_H
#define NV_GEOMETRY_RESIDENT_H
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
/* Cache byte-identical uploads in the CURRENT dynamic-buffer allocation.
 * No guest pointers survive a call. A hash is only a lookup hint; every hit
 * compares all bytes with owned CPU shadow storage, never mapped GPU memory.
 * Owner must clear before DISCARD or any writer outside upload_core. */
#define NV_GEOMETRY_SETS 64
typedef struct NvGeometryEntry {uint32_t key,bytes,offset;unsigned valid;} NvGeometryEntry;
typedef struct NvGeometryResident {
    uint8_t *shadow;unsigned capacity,enabled;
    NvGeometryEntry entry[NV_GEOMETRY_SETS][2];
    uint8_t victim[NV_GEOMETRY_SETS];
    unsigned miss_streak,cooldown,skip_store;
    unsigned byte_budget_enabled,byte_credit;
    uint64_t hits,misses,saved_bytes,uploads;
} NvGeometryResident;
#define NV_GEOMETRY_BYTE_CREDIT (256u*1024u)
static void nv_geometry_byte_budget(NvGeometryResident*c,int enabled){
    c->byte_budget_enabled=!!enabled;c->byte_credit=enabled?NV_GEOMETRY_BYTE_CREDIT:0;
}
/* Admission only: exhausted credit never suppresses an otherwise exact lookup.
 * Existing cooldown expiry replenishes bounded probation, while hits pay back
 * the actual saved bytes. Failed Maps/stores spend no shadow-copy credit. */
static int nv_geometry_can_store(const NvGeometryResident*c,unsigned bytes){
    return !c->byte_budget_enabled||bytes<=c->byte_credit;
}
static void nv_geometry_store_credit(NvGeometryResident*c,unsigned bytes){
    if(c->byte_budget_enabled)c->byte_credit-=bytes;
}
static void nv_geometry_hit_credit(NvGeometryResident*c,unsigned bytes){
    if(c->byte_budget_enabled){unsigned room=NV_GEOMETRY_BYTE_CREDIT-c->byte_credit;c->byte_credit+=bytes<room?bytes:room;}
}
static uint32_t nv_geometry_key(const void *data,unsigned bytes){
    const uint8_t *p=data;uint32_t h=bytes*0x9e3779b9u;
    unsigned at[3]={0,(bytes/2)&~3u,bytes-16};
    for(unsigned a=0;a<3;++a)for(unsigned j=0;j<4;++j){
        uint32_t w;memcpy(&w,p+at[a]+4*j,4);h=(h^w)*16777619u;
    }
    return h;
}
static void nv_geometry_expire(NvGeometryResident *c){
    /* Entries are published only after owned shadow allocation succeeds.
     * Keep clearing allocated caches even while disabled: owners may reenable. */
    if(!c->shadow)return;
    memset(c->entry,0,sizeof(c->entry));
}
static int nv_geometry_eligible(const NvGeometryResident*c,unsigned bytes){
    return c->enabled&&bytes>=64&&bytes<=262144&&bytes<=c->capacity;
}
/* The split representation has the exact same byte identity as its contiguous
 * concatenation. Hash samples may straddle the directory/payload boundary. */
static uint32_t nv_geometry_key_parts(const void*head,unsigned head_bytes,const void*tail,unsigned tail_bytes){
    unsigned bytes=head_bytes+tail_bytes;uint32_t h=bytes*0x9e3779b9u;
    unsigned at[3]={0,(bytes/2)&~3u,bytes-16};
    for(unsigned a=0;a<3;++a){
        uint8_t sample[16];unsigned n=at[a]<head_bytes?head_bytes-at[a]:0;if(n>16)n=16;
        if(n)memcpy(sample,(const uint8_t*)head+at[a],n);
        if(n<16)memcpy(sample+n,(const uint8_t*)tail+(at[a]+n-head_bytes),16-n);
        for(unsigned j=0;j<4;++j){uint32_t w;memcpy(&w,sample+4*j,4);h=(h^w)*16777619u;}
    }
    return h;
}
static int nv_geometry_find_ready(NvGeometryResident*c,const void*data,unsigned bytes){
    c->skip_store=0;
    if(!nv_geometry_eligible(c,bytes)||!data)return 0;
    /* Dynamic streams must not pay an unconditional extra copy forever.
     * Bounded probation admits new content again after the quiet interval. */
    if(c->cooldown){--c->cooldown;c->skip_store=1;
        if(!c->cooldown&&c->byte_budget_enabled)c->byte_credit=NV_GEOMETRY_BYTE_CREDIT;
        return 0;}
    return 1;
}
static int nv_geometry_lookup(NvGeometryResident*c,const void*head,unsigned head_bytes,
    const void*tail,unsigned tail_bytes,uint32_t key,unsigned*out){
    unsigned bytes=head_bytes+tail_bytes,set=key&(NV_GEOMETRY_SETS-1);
    for(unsigned i=0;i<2;++i){NvGeometryEntry*e=&c->entry[set][i];
        if(e->valid&&e->key==key&&e->bytes==bytes&&c->shadow&&
           e->offset<=c->capacity-bytes&&!memcmp(c->shadow+e->offset,head,head_bytes)&&
           (!tail_bytes||!memcmp(c->shadow+e->offset+head_bytes,tail,tail_bytes))){
            *out=e->offset;c->victim[set]=(uint8_t)(i^1);c->miss_streak=0;++c->hits;c->saved_bytes+=bytes;
            nv_geometry_hit_credit(c,bytes);return 1;
        }
    }
    ++c->misses;if(++c->miss_streak>=128){c->miss_streak=0;c->cooldown=2048;}return 0;
}
static int nv_geometry_find(NvGeometryResident*c,const void*data,unsigned bytes,unsigned*out){
    if(!nv_geometry_find_ready(c,data,bytes))return 0;
    return nv_geometry_lookup(c,data,bytes,NULL,0,nv_geometry_key(data,bytes),out);
}
static int nv_geometry_find_parts(NvGeometryResident*c,const void*head,unsigned head_bytes,
    const void*tail,unsigned tail_bytes,unsigned*out){
    c->skip_store=0;
    if(!head||!head_bytes||(!tail&&tail_bytes)||head_bytes>UINT32_MAX-tail_bytes)return 0;
    if(!nv_geometry_find_ready(c,head,head_bytes+tail_bytes))return 0;
    return nv_geometry_lookup(c,head,head_bytes,tail,tail_bytes,nv_geometry_key_parts(head,head_bytes,tail,tail_bytes),out);
}
/* Called only after successful Map/copy/Unmap. NO_OVERWRITE makes prior entries
 * immutable. A failed allocation merely disables admission until next attempt. */
static void nv_geometry_store(NvGeometryResident*c,const void*data,unsigned bytes,unsigned offset){
    if(c->skip_store||!nv_geometry_eligible(c,bytes)||!data||offset>c->capacity-bytes||!nv_geometry_can_store(c,bytes))return;
    if(!c->shadow){c->shadow=malloc(c->capacity);if(!c->shadow){c->enabled=0;return;}}
    uint32_t key=nv_geometry_key(data,bytes);unsigned set=key&(NV_GEOMETRY_SETS-1),way=c->victim[set]&1;
    memcpy(c->shadow+offset,data,bytes);
    c->entry[set][way]=(NvGeometryEntry){key,bytes,offset,1};c->victim[set]=(uint8_t)(way^1);++c->uploads;nv_geometry_store_credit(c,bytes);
}
static void nv_geometry_store_parts(NvGeometryResident*c,const void*head,unsigned head_bytes,
    const void*tail,unsigned tail_bytes,unsigned offset){
    if(!head||!head_bytes||(!tail&&tail_bytes)||head_bytes>UINT32_MAX-tail_bytes)return;
    unsigned bytes=head_bytes+tail_bytes;
    if(c->skip_store||!nv_geometry_eligible(c,bytes)||offset>c->capacity-bytes||!nv_geometry_can_store(c,bytes))return;
    if(!c->shadow){c->shadow=malloc(c->capacity);if(!c->shadow){c->enabled=0;return;}}
    uint32_t key=nv_geometry_key_parts(head,head_bytes,tail,tail_bytes);
    unsigned set=key&(NV_GEOMETRY_SETS-1),way=c->victim[set]&1;
    memcpy(c->shadow+offset,head,head_bytes);
    if(tail_bytes)memcpy(c->shadow+offset+head_bytes,tail,tail_bytes);
    c->entry[set][way]=(NvGeometryEntry){key,bytes,offset,1};c->victim[set]=(uint8_t)(way^1);++c->uploads;nv_geometry_store_credit(c,bytes);
}
static void nv_geometry_shutdown(NvGeometryResident*c){free(c->shadow);memset(c,0,sizeof(*c));}
#endif

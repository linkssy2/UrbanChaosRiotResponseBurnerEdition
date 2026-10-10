#ifndef NV2A_PACKED_REUSE_H
#define NV2A_PACKED_REUSE_H
#include <stdint.h>
#include <string.h>
/* Draw-owned index map. A hit requires the full index and fetch-plan revision.
 * Collisions miss. Rows remain owned for fallback even after revision changes.
 * Caller checks current capture validity and bounds BEFORE lookup. */
typedef struct NvPackedReuse {
    uint32_t generation,count;
    uint64_t lean_revision;
    struct {uint32_t generation,index,row;} lean[1024];
    struct {uint64_t revision;uint32_t generation,index,row;} entries[1024];
} NvPackedReuse;
static void nv_packed_reuse_begin(NvPackedReuse*m){
    if(!++m->generation){memset(m->entries,0,sizeof(m->entries));memset(m->lean,0,sizeof(m->lean));m->generation=1;}
    m->count=0;
}
static unsigned nv_packed_reuse_slot(uint32_t index){return (index^(index>>10)^(index>>20))&1023u;}
/* Compare the two adjacent 32-bit key fields with one unaligned-safe load.
 * Build the expected representation by memcpy too; no endian assumption. */
static int nv_packed_reuse_key(const NvPackedReuse*m,unsigned slot,uint32_t index){
    uint32_t fields[2]={m->generation,index};uint64_t actual,expected;
    memcpy(&actual,&m->lean[slot].generation,sizeof(actual));
    memcpy(&expected,fields,sizeof(expected));return actual==expected;
}
/* A revision change invalidates lookup entries, not owned rows needed by fallback.
 * Compact hot entries halve the lookup footprint. Start/commit remain draw-owned. */
static int nv_packed_reuse_find_lean(NvPackedReuse*m,uint32_t index,uint64_t revision,uint32_t*row,unsigned*slot){
    if(m->lean_revision!=revision){
        if(!++m->generation){memset(m->lean,0,sizeof(m->lean));memset(m->entries,0,sizeof(m->entries));m->generation=1;}
        m->lean_revision=revision;
    }
    unsigned s=nv_packed_reuse_slot(index);*slot=s;
    if(!nv_packed_reuse_key(m,s,index))return 0;
    *row=m->lean[s].row;return 1;
}
static void nv_packed_reuse_commit_lean(NvPackedReuse*m,uint32_t index,unsigned slot){
    m->lean[slot].generation=m->generation;m->lean[slot].index=index;m->lean[slot].row=m->count++;
}
static int nv_packed_reuse_find(const NvPackedReuse*m,uint32_t index,uint64_t revision,uint32_t*row){
    unsigned s=nv_packed_reuse_slot(index);
    if(m->entries[s].generation!=m->generation||m->entries[s].index!=index||m->entries[s].revision!=revision)return 0;
    *row=m->entries[s].row;return 1;
}
/* Publish only AFTER successful capture into row=count. */
static void nv_packed_reuse_commit(NvPackedReuse*m,uint32_t index,uint64_t revision){
    unsigned s=nv_packed_reuse_slot(index);
    m->entries[s].generation=m->generation;m->entries[s].index=index;
    m->entries[s].revision=revision;m->entries[s].row=m->count++;
}
#endif

#ifndef NV2A_VERTEX_FETCH_PLAN_H
#define NV2A_VERTEX_FETCH_PLAN_H
#include <stdint.h>
/* Only descriptor identity is retained, never attribute contents. The caller
 * must still invalidate at every producer/reentry/completion-write boundary. */
static void nv_vertex_dma_rebind(uint32_t *handle,unsigned *cached,uint32_t value,int owned){
    if(!owned||*handle!=value)*cached=0;
    *handle=value;
}
#include "nv2a_vertex_fetch.h"
#include "nv2a_dma_resolve.h"
#include "nv2a_dma_certificate.h"
/* Decoder-owned immutable descriptors, never immutable guest data. The owner
 * must invalidate at every non-index method and guest/register reentry. */
typedef struct NvVertexFetchEntry {
    uint32_t format, address, stride, max_index, attribute;
} NvVertexFetchEntry;
typedef struct NvVertexFetchPlan {
    unsigned valid, count;
    uint64_t revision;
    NvVertexFetchEntry entry[16];
    float defaults[16][4];
    const uint8_t *ram;
} NvVertexFetchPlan;
static int nv_vertex_fetch_entry(NvVertexFetchEntry *e,const NvDMARange *range,
    uint32_t format,uint32_t offset,size_t ram_size,unsigned attribute){
    unsigned bytes=nv2a_vertex_attribute_bytes(format);
    uint32_t address;
    if(range->base>UINT32_MAX || !nv2a_dma_range_resolve(range,offset,bytes,ram_size,&address))return 0;
    uint64_t room=(uint64_t)range->limit+1-offset-bytes;
    uint64_t ram_room=(uint64_t)ram_size-address-bytes;
    if(ram_room<room)room=ram_room;
    if((uint64_t)UINT32_MAX-address<room)room=(uint64_t)UINT32_MAX-address;
    if((uint64_t)UINT32_MAX-offset<room)room=(uint64_t)UINT32_MAX-offset;
    e->format=format;e->address=address;e->stride=format>>8;e->attribute=attribute;
    e->max_index=e->stride?(uint32_t)(room/e->stride):UINT32_MAX;
    return 1;
}
static int nv_vertex_fetch_plan_build_cached_certified(NvVertexFetchPlan *p,const uint32_t formats[16],
    const uint32_t offsets[16],const uint32_t dma[2],const float defaults[16][4],
    const uint8_t *regs,size_t regs_size,const uint8_t *ram,size_t ram_size,
    NvDMARange ranges[2],uint32_t resolved[2],NvDMACertificate certificates[2],int certify){
    if(!++p->revision)++p->revision;
    p->valid=0;p->count=0;
    if(!ram)return 0;
    for(unsigned a=0;a<16;++a){
        if(!((formats[a]>>4)&15))continue;
        unsigned bank=offsets[a]>>31;
        if(!resolved[bank]){
            if(!nv2a_dma_lookup_certified(certificates?&certificates[bank]:NULL,
                regs,regs_size,dma[bank],&ranges[bank],0,certify))return 0;
            resolved[bank]=1;
        }
        if(!nv_vertex_fetch_entry(&p->entry[p->count],&ranges[bank],formats[a],offsets[a]&0x7FFFFFFFu,ram_size,a))return 0;
        ++p->count;
    }
    memcpy(p->defaults,defaults,sizeof(p->defaults));p->ram=ram;p->valid=1;return 1;
}
static int nv_vertex_fetch_plan_build_cached(NvVertexFetchPlan *p,const uint32_t formats[16],
    const uint32_t offsets[16],const uint32_t dma[2],const float defaults[16][4],
    const uint8_t *regs,size_t regs_size,const uint8_t *ram,size_t ram_size,
    NvDMARange ranges[2],uint32_t resolved[2]){
    return nv_vertex_fetch_plan_build_cached_certified(p,formats,offsets,dma,defaults,
        regs,regs_size,ram,ram_size,ranges,resolved,NULL,0);
}
static int nv_vertex_fetch_plan_build(NvVertexFetchPlan *p,const uint32_t formats[16],
    const uint32_t offsets[16],const uint32_t dma[2],const float defaults[16][4],
    const uint8_t *regs,size_t regs_size,const uint8_t *ram,size_t ram_size){
    NvDMARange ranges[2];uint32_t resolved[2]={0};
    return nv_vertex_fetch_plan_build_cached(p,formats,offsets,dma,defaults,regs,regs_size,ram,ram_size,ranges,resolved);
}
static int nv_vertex_fetch_plan_read(const NvVertexFetchPlan *p,uint32_t index,float out[16][4]){
    if(!p->valid)return 0;
    /* Validate every source before touching output or guest memory. */
    for(unsigned i=0;i<p->count;++i)if(index>p->entry[i].max_index)return 0;
    memcpy(out,p->defaults,sizeof(p->defaults));
    for(unsigned i=0;i<p->count;++i){
        const NvVertexFetchEntry *e=&p->entry[i];
        uint32_t address=e->address+index*e->stride;
        if(!nv2a_vertex_fetch(out[e->attribute],e->format,p->ram+address))return 0;
    }
    return 1;
}
#endif

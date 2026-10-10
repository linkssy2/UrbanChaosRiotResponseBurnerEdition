#ifndef NV2A_DMA_RESOLVE_H
#define NV2A_DMA_RESOLVE_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
static uint32_t nv2a_dma_u32(const uint8_t *p) { uint32_t v; memcpy(&v,p,4); return v; }
/* Resolve a channel-zero RAMHT handle, not a guessed physical pointer. The
 * current live decoder is single-channel. Reject duplicate/foreign objects. */
typedef struct NvDMARange { uint64_t base; uint32_t limit; } NvDMARange;
static int nv2a_dma_lookup_scalar(const uint8_t *regs, size_t regs_size,
    uint32_t handle, NvDMARange *range, int write)
{
    if (!regs || regs_size < 0x800000) return 0;
    uint32_t cfg=nv2a_dma_u32(regs+0x2210);
    uint32_t base=0x700000+((cfg&0x1F0)<<8);
    uint32_t count=4096u<<((cfg>>16)&3);
    int found=0; NvDMARange result={0};
    if ((uint64_t)base+count>regs_size) return 0;
    for(uint32_t i=0;i<count;i+=8) {
        uint32_t key=nv2a_dma_u32(regs+base+i);
        uint32_t ctx=nv2a_dma_u32(regs+base+i+4);
        if(key!=handle || !(ctx&0x80000000u) || (ctx&0x1F030000u)) continue;
        uint32_t object=0x700000+((ctx&0xFFFF)<<4);
        if((uint64_t)object+12>regs_size || found) return 0;
        uint32_t flags=nv2a_dma_u32(regs+object);
        uint32_t limit=nv2a_dma_u32(regs+object+4);
        uint32_t frame=nv2a_dma_u32(regs+object+8);
        unsigned klass=flags&0xFFF, target=(flags>>16)&3;
        if((klass!=(write?3:2) && klass!=0x3D) || (target!=0 && target!=2) || !(flags&0x2000)) return 0;
        result.base=(frame&0xFFFFF000u)+(uint64_t)(flags>>20);
        result.limit=limit; found=1;
    }
    if(found) *range=result;
    return found;
}
#include "nv2a_dma_scan.h"
#include "nv2a_dma_scan_avx2.h"
static int nv2a_dma_avx2_enabled(void){
    static int enabled=-1;
    if(enabled<0){
        const char*v=getenv("UCRR_DMA_SCAN_AVX2");enabled=0;
#if defined(_MSC_VER) && defined(_M_X64)
        if(v&&!strcmp(v,"1"))enabled=nv2a_dma_avx2_supported();
#endif
    }
    return enabled;
}
/* The original scalar entry stays intact as the baseline and independent
 * resolver contract. This cold-certificate variant additionally returns the
 * object already validated by the same full duplicate-rejecting search. */
static int nv2a_dma_lookup_scalar_object(const uint8_t *regs,size_t regs_size,
    uint32_t handle,NvDMARange *range,int write,uint32_t *selected_object)
{
    if(!regs||regs_size<0x800000)return 0;
    uint32_t cfg=nv2a_dma_u32(regs+0x2210);
    uint32_t base=0x700000+((cfg&0x1F0)<<8),count=4096u<<((cfg>>16)&3);
    int found=0;NvDMARange result={0};uint32_t chosen=0;
    if((uint64_t)base+count>regs_size)return 0;
    for(uint32_t i=0;i<count;i+=8){
        uint32_t key=nv2a_dma_u32(regs+base+i),ctx=nv2a_dma_u32(regs+base+i+4);
        if(key!=handle||!(ctx&0x80000000u)||(ctx&0x1F030000u))continue;
        uint32_t object=0x700000+((ctx&0xFFFF)<<4);
        if((uint64_t)object+12>regs_size||found)return 0;
        uint32_t flags=nv2a_dma_u32(regs+object),limit=nv2a_dma_u32(regs+object+4);
        uint32_t frame=nv2a_dma_u32(regs+object+8);
        unsigned klass=flags&0xFFF,target=(flags>>16)&3;
        if((klass!=(write?3:2)&&klass!=0x3D)||(target!=0&&target!=2)||!(flags&0x2000))return 0;
        result.base=(frame&0xFFFFF000u)+(uint64_t)(flags>>20);result.limit=limit;
        chosen=object;found=1;
    }
    if(found){*range=result;*selected_object=chosen;}
    return found;
}
static int nv2a_dma_simd_enabled(void){
    static int enabled=-1;
    if(enabled<0){const char*v=getenv("UCRR_DMA_SCAN_SIMD");enabled=v&&!strcmp(v,"1");}
    return enabled;
}
#if defined(UCRR_DMA_DIAGNOSTIC) && (defined(_M_X64) || defined(__SSE2__))
#include <stdio.h>
#include "nv2a_dma_probe.h"
#endif
static int nv2a_dma_lookup(const uint8_t *regs,size_t regs_size,
    uint32_t handle,NvDMARange *range,int write)
{
#if defined(_M_X64) || defined(__SSE2__)
    int enabled=nv2a_dma_simd_enabled();
#if defined(UCRR_DMA_DIAGNOSTIC)
    static NvDmaProbe probe;
    int result=nv2a_dma_probe_lookup(&probe,regs,regs_size,handle,range,write,enabled);
    if(probe.reports<64 && (probe.calls%65536)==0){
        ++probe.reports;
        fprintf(stderr,"[DMA-PROBE] instance=%p calls=%llu tables=%llu,%llu,%llu,%llu invalid=%llu reads=%llu writes=%llu checks=%llu mismatches=%llu failures=%llu report=%llu/64\n",
            (void*)&probe,(unsigned long long)probe.calls,
            (unsigned long long)probe.tables[0],(unsigned long long)probe.tables[1],
            (unsigned long long)probe.tables[2],(unsigned long long)probe.tables[3],
            (unsigned long long)probe.invalid_registers,(unsigned long long)probe.reads,
            (unsigned long long)probe.writes,(unsigned long long)probe.checks,
            (unsigned long long)probe.mismatches,(unsigned long long)probe.failures,
            (unsigned long long)probe.reports);
    }
    return result;
#endif
    if(enabled){
#if defined(_MSC_VER) && defined(_M_X64)
        if(nv2a_dma_avx2_enabled())return nv2a_dma_lookup_avx2(regs,regs_size,handle,range,write);
#endif
        return nv2a_dma_lookup_simd(regs,regs_size,handle,range,write);
    }
#endif
    return nv2a_dma_lookup_scalar(regs,regs_size,handle,range,write);
}
static int nv2a_dma_range_resolve(const NvDMARange *range,uint32_t offset,
    uint32_t bytes,size_t ram_size,uint32_t *address)
{
    uint64_t start=range->base+offset;
    if(!bytes || (uint64_t)offset+bytes>(uint64_t)range->limit+1 ||
       start+bytes>ram_size || start>UINT32_MAX)return 0;
    *address=(uint32_t)start; return 1;
}
static int nv2a_dma_resolve_access(const uint8_t *regs,size_t regs_size,
    uint32_t handle,uint32_t offset,uint32_t bytes,size_t ram_size,
    uint32_t *address,int write)
{
    NvDMARange range;
    return nv2a_dma_lookup(regs,regs_size,handle,&range,write)&&
        nv2a_dma_range_resolve(&range,offset,bytes,ram_size,address);
}
static int nv2a_dma_resolve(const uint8_t *regs, size_t regs_size,
    uint32_t handle, uint32_t offset, uint32_t bytes, size_t ram_size,
    uint32_t *address)
{return nv2a_dma_resolve_access(regs,regs_size,handle,offset,bytes,ram_size,address,1);}
#endif

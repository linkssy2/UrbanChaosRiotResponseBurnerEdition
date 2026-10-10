#ifndef NV2A_DMA_CERTIFICATE_H
#define NV2A_DMA_CERTIFICATE_H
#include "nv2a_dma_resolve.h"
#include "nv2a_cpu_compact2.h"

/* These are owned dependency bytes, not a claim that guest RAM is immutable.
 * Acquire only when the existing DMA bank needs resolving in a newly owned
 * decoder interval. All old bank/plan/content invalidations remain necessary.
 * The caller must reset on renderer/device/register mapping replacement, even
 * when the replacement happens to use the same address. No concurrent writer
 * may modify the register mapping during acquire (the resolver's old contract).
 */
typedef struct NvDMACertificate {
    const uint8_t *regs;
    size_t regs_size;
    uint32_t cfg, table_base, table_bytes, object, handle;
    unsigned valid, write, retry_skip, miss_streak;
    NvDMARange range;
    uint8_t object_bytes[12];
    uint8_t table[32768];
} NvDMACertificate;

#ifndef NV_DMA_CERT_EVENT
#define NV_DMA_CERT_EVENT(event, bytes) ((void)0)
#endif
enum { NV_DMA_CERT_HIT, NV_DMA_CERT_MISS, NV_DMA_CERT_COPY,
       NV_DMA_CERT_FALLBACK, NV_DMA_CERT_EVENTS };

static int nv_dma_certificate_enabled(void) {
    static int enabled=-1;
    if(enabled<0){const char *v=getenv("UCRR_CPU_BUNDLE_DMA_CERT");enabled=v&&!strcmp(v,"1");}
    return enabled;
}
static void nv_dma_certificate_reset(NvDMACertificate *c) {
    c->valid=0;c->retry_skip=0;c->miss_streak=0;
}

static int nv2a_dma_lookup_certified(NvDMACertificate *c,
    const uint8_t *regs,size_t regs_size,uint32_t handle,NvDMARange *range,
    int write,int enabled)
{
    if(!enabled||!c){
        NV_DMA_CERT_EVENT(NV_DMA_CERT_FALLBACK,0);
        return nv2a_dma_lookup(regs,regs_size,handle,range,write);
    }
    if(c->retry_skip){
        --c->retry_skip;
        NV_DMA_CERT_EVENT(NV_DMA_CERT_FALLBACK,0);
        return nv2a_dma_lookup(regs,regs_size,handle,range,write);
    }
    unsigned direction=write!=0;
    uint32_t cfg=0,base=0,bytes=0;
    if(regs&&regs_size>=0x800000){
        cfg=nv2a_dma_u32(regs+0x2210);
        base=0x700000+((cfg&0x1F0)<<8);
        bytes=4096u<<((cfg>>16)&3);
    }
    if(bytes&&(uint64_t)base+bytes<=regs_size&&c->valid&&
       c->regs==regs&&c->regs_size==regs_size&&c->cfg==cfg&&
       c->table_base==base&&c->table_bytes==bytes&&c->handle==handle&&
       c->write==direction&&(uint64_t)c->object+12<=regs_size&&
       (NV_CPU_COMPACT2_ON(NV_C2_DMA_REJECT)?
        (!memcmp(c->object_bytes,regs+c->object,12)&&!memcmp(c->table,regs+base,bytes)):
        (!memcmp(c->table,regs+base,bytes)&&!memcmp(c->object_bytes,regs+c->object,12)))){
        *range=c->range;
        c->miss_streak=0;
        NV_DMA_CERT_EVENT(NV_DMA_CERT_HIT,bytes+12);
        return 1;
    }
    c->valid=0;
    /* Two misses without an exact hit imply churn. Skip the expensive cold
     * snapshot refresh on this and the next 15 acquires, then retry. An
     * isolated change following hits still refreshes immediately. This only
     * selects the original resolver; it never reuses an uncertified range. */
    int refresh=1;
    if(++c->miss_streak>=2){c->miss_streak=0;c->retry_skip=15;refresh=0;}
    NV_DMA_CERT_EVENT(NV_DMA_CERT_MISS,bytes);
    /* Preserve the selected production resolver, failure sentinels, and its
     * complete duplicate/context/class/access validation. No negative cache. */
    uint32_t selected_object=UINT32_MAX;
    int found;
#if !defined(UCRR_DMA_DIAGNOSTIC)
    if(refresh&&NV_CPU_COMPACT2_ON(NV_C2_DMA_OBJECT)&&!nv2a_dma_simd_enabled())
        found=nv2a_dma_lookup_scalar_object(regs,regs_size,handle,range,write,&selected_object);
    else
#endif
        found=nv2a_dma_lookup(regs,regs_size,handle,range,write);
    if(!found)return 0;
    if(!refresh)return 1;
    /* A successful full lookup proves a unique eligible entry. Finding that
     * entry again is cold-path overhead included in the complete benchmarks. */
    if(!bytes||(uint64_t)base+bytes>regs_size)return 1;
    for(uint32_t i=0;i<bytes;i+=8){
        uint32_t object=selected_object;
        if(object==UINT32_MAX){
        uint32_t key=nv2a_dma_u32(regs+base+i);
        uint32_t context=nv2a_dma_u32(regs+base+i+4);
        if(key!=handle||!(context&0x80000000u)||(context&0x1F030000u))continue;
        object=0x700000+((context&0xFFFF)<<4);
        }
        if((uint64_t)object+12>regs_size)return 1;
        memcpy(c->table,regs+base,bytes);
        memcpy(c->object_bytes,regs+object,12);
        c->regs=regs;c->regs_size=regs_size;c->cfg=cfg;
        c->table_base=base;c->table_bytes=bytes;c->object=object;
        c->handle=handle;c->write=direction;c->range=*range;
        c->valid=1;
        NV_DMA_CERT_EVENT(NV_DMA_CERT_COPY,bytes+12);
        break;
    }
    return 1;
}
#endif

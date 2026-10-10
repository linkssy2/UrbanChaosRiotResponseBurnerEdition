#ifndef NV2A_DMA_PROBE_H
#define NV2A_DMA_PROBE_H
/* Diagnostic-build only. Per translation-unit state; caller supplies its name.
 * No timers, retained guest pointers, mutation, or per-call logging. The decoder
 * owns register memory during lookup, just as for the ordinary implementation.
 * This is not a thread-safe probe for concurrent register writers. */
typedef struct NvDmaProbe {
    uint64_t calls, tables[4], invalid_registers, reads, writes;
    uint64_t checks, mismatches, failures, reports;
} NvDmaProbe;
static int nv2a_dma_probe_lookup(NvDmaProbe *p, const uint8_t *regs,
    size_t size, uint32_t handle, NvDMARange *out, int write, int simd)
{
    ++p->calls;
    if(write)++p->writes;else ++p->reads;
    if(regs && size>=0x800000)
        ++p->tables[(nv2a_dma_u32(regs+0x2210)>>16)&3];
    else ++p->invalid_registers;
    NvDMARange selected={0};
    int ok=simd?nv2a_dma_lookup_simd(regs,size,handle,&selected,write):
        nv2a_dma_lookup_scalar(regs,size,handle,&selected,write);
    if(!ok)++p->failures;
    /* Reserve samples for later gameplay, rather than consuming all at boot. */
    if(p->checks<1024 && (p->calls<=16 || (p->calls%4096)==0)){
        NvDMARange a={UINT64_C(0xA5A5A5A5A5A5A5A5),0xA5A5A5A5u}, b=a;
        int x=nv2a_dma_lookup_scalar(regs,size,handle,&a,write);
        int y=nv2a_dma_lookup_simd(regs,size,handle,&b,write);
        ++p->checks;
        if(x!=y || a.base!=b.base || a.limit!=b.limit || ok!=x ||
           (ok && (selected.base!=a.base || selected.limit!=a.limit)))++p->mismatches;
    }
    if(ok)*out=selected;
    return ok;
}
#endif

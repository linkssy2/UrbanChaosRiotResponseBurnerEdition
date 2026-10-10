#ifndef NV2A_DMA_SCAN_H
#define NV2A_DMA_SCAN_H
/* Included after NvDMARange/scalar lookup. Read fresh register bytes each call;
 * no cached handles, object descriptors, or expanded ownership intervals. */
#if defined(_M_X64) || defined(__SSE2__)
#include <emmintrin.h>
static int nv2a_dma_lookup_simd(const uint8_t *regs,size_t regs_size,
    uint32_t handle,NvDMARange *range,int write)
{
    if(!regs || regs_size<0x800000)return 0;
    uint32_t cfg=nv2a_dma_u32(regs+0x2210);
    uint32_t base=0x700000+((cfg&0x1F0)<<8);
    uint32_t count=4096u<<((cfg>>16)&3);
    int found=0;NvDMARange result={0};
    if((uint64_t)base+count>regs_size)return 0;
    __m128i wanted=_mm_set1_epi32((int)handle);
    /* Every allowed table size is a multiple of32. Both unaligned loads remain
     * inside the checked table. Match lanes are processed in original order. */
    for(uint32_t i=0;i<count;i+=32){
        __m128i pair=_mm_loadu_si128((const __m128i*)(regs+base+i));
        __m128i second=_mm_loadu_si128((const __m128i*)(regs+base+i+16));
        __m128i keys=_mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(pair),_mm_castsi128_ps(second),_MM_SHUFFLE(2,0,2,0)));
        unsigned matches=(unsigned)_mm_movemask_ps(_mm_castsi128_ps(_mm_cmpeq_epi32(keys,wanted)));
        if(!matches)continue;
        __m128i contexts=_mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(pair),_mm_castsi128_ps(second),_MM_SHUFFLE(3,1,3,1)));
        __m128i eligible=_mm_cmpeq_epi32(_mm_and_si128(contexts,_mm_set1_epi32((int)0x9F030000u)),_mm_set1_epi32((int)0x80000000u));
        matches&=(unsigned)_mm_movemask_ps(_mm_castsi128_ps(eligible));
        if(!matches)continue;
        for(unsigned lane=0;lane<4;++lane){
            if(!(matches&(1u<<lane)))continue;
            uint32_t at=i+lane*8;
            uint32_t ctx=nv2a_dma_u32(regs+base+at+4);
            uint32_t object=0x700000+((ctx&0xFFFF)<<4);
            if((uint64_t)object+12>regs_size || found)return 0;
            uint32_t flags=nv2a_dma_u32(regs+object);
            uint32_t limit=nv2a_dma_u32(regs+object+4);
            uint32_t frame=nv2a_dma_u32(regs+object+8);
            unsigned klass=flags&0xFFF,target=(flags>>16)&3;
            if((klass!=(write?3:2)&&klass!=0x3D)||(target!=0&&target!=2)||!(flags&0x2000))return 0;
            result.base=(frame&0xFFFFF000u)+(uint64_t)(flags>>20);
            result.limit=limit;found=1;
        }
    }
    if(found)*range=result;
    return found;
}
#endif
#endif

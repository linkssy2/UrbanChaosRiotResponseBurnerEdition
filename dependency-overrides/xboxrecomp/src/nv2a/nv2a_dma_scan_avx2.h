#ifndef NV2A_DMA_SCAN_AVX2_H
#define NV2A_DMA_SCAN_AVX2_H
/* Fresh RAMHT scan. Include after NvDMARange and nv2a_dma_u32. */
#if defined(_MSC_VER) && defined(_M_X64)
#include <immintrin.h>
#include <intrin.h>
static int nv2a_dma_avx2_supported(void)
{
    int cpu[4];__cpuid(cpu,0);if(cpu[0]<7)return 0;
    __cpuid(cpu,1);
    if((cpu[2]&0x18000000)!=0x18000000 || (_xgetbv(0)&6)!=6)return 0;
    __cpuidex(cpu,7,0);return !!(cpu[1]&32);
}
__declspec(noinline) static int nv2a_dma_lookup_avx2(const uint8_t *regs,
    size_t regs_size,uint32_t handle,NvDMARange *range,int write)
{
    if(!regs || regs_size<0x800000)return 0;
    uint32_t cfg=nv2a_dma_u32(regs+0x2210);
    uint32_t base=0x700000+((cfg&0x1F0)<<8);
    uint32_t count=4096u<<((cfg>>16)&3);
    if((uint64_t)base+count>regs_size)return 0;
    int found=0;NvDMARange result={0};
    __m256i wanted=_mm256_set1_epi32((int)handle);
    /* Eight interleaved key/context pairs per two bounded unaligned loads.
     * Key matches occupy even lanes. Contexts are checked in the same bytes;
     * no pointer or object survives this call and all duplicate checks remain. */
    for(uint32_t i=0;i<count;i+=64){
        __m256i entries=_mm256_loadu_si256((const __m256i*)(regs+base+i));
        __m256i second=_mm256_loadu_si256((const __m256i*)(regs+base+i+32));
        unsigned matches=(unsigned)_mm256_movemask_ps(_mm256_castsi256_ps(
            _mm256_cmpeq_epi32(entries,wanted)));
        matches|=(unsigned)_mm256_movemask_ps(_mm256_castsi256_ps(
            _mm256_cmpeq_epi32(second,wanted)))<<8;
        matches&=0x5555u;
        if(!matches)continue;
        __m256i eligible=_mm256_cmpeq_epi32(_mm256_and_si256(entries,
            _mm256_set1_epi32((int)0x9F030000u)),_mm256_set1_epi32((int)0x80000000u));
        __m256i eligible_second=_mm256_cmpeq_epi32(_mm256_and_si256(second,
            _mm256_set1_epi32((int)0x9F030000u)),_mm256_set1_epi32((int)0x80000000u));
        unsigned contexts=(unsigned)_mm256_movemask_ps(_mm256_castsi256_ps(eligible));
        contexts|=(unsigned)_mm256_movemask_ps(_mm256_castsi256_ps(eligible_second))<<8;
        matches&=contexts>>1;
        if(!matches)continue;
        for(unsigned lane=0;lane<8;++lane){
            if(!(matches&(1u<<(lane*2))))continue;
            uint32_t ctx=nv2a_dma_u32(regs+base+i+lane*8+4);
            uint32_t object=0x700000+((ctx&0xFFFF)<<4);
            if((uint64_t)object+12>regs_size || found){_mm256_zeroupper();return 0;}
            uint32_t flags=nv2a_dma_u32(regs+object);
            uint32_t limit=nv2a_dma_u32(regs+object+4);
            uint32_t frame=nv2a_dma_u32(regs+object+8);
            unsigned klass=flags&0xFFF,target=(flags>>16)&3;
            if((klass!=(write?3:2)&&klass!=0x3D)||(target!=0&&target!=2)||!(flags&0x2000)){
                _mm256_zeroupper();return 0;
            }
            result.base=(frame&0xFFFFF000u)+(uint64_t)(flags>>20);
            result.limit=limit;found=1;
        }
    }
    _mm256_zeroupper();if(found)*range=result;return found;
}
#endif
#endif

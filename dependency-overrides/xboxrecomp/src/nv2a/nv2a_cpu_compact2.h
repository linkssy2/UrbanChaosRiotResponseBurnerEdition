#ifndef NV2A_CPU_COMPACT2_H
#define NV2A_CPU_COMPACT2_H
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
static int nv_cpu_compact2_disjoint(const void*a,size_t an,const void*b,size_t bn){
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return x>=y?x-y>=bn:y-x>=an;
}
/* Independent correctness/leave-one-out switches. No option changes ownership,
 * authored values, diagnostic accounting or the selected DMA SIMD policy. */
enum { NV_C2_DMA_OBJECT=1, NV_C2_DMA_REJECT=2, NV_C2_STATE_GUARDS=4,
       NV_C2_STATE_RUNS=8, NV_C2_ARRAY_PACKET=16, NV_C2_ZERO_FIRST=32 };
#ifndef NV_CPU_COMPACT2_MASK
static unsigned nv_cpu_compact2_mask(void){
    static int cached=-1;
    if(cached<0){
        const char *v=getenv("UCRR_CPU_COMPACT2");
        unsigned mask=v&&!strcmp(v,"1")?63u:0u;
        const char *bits=getenv("UCRR_CPU_COMPACT2_MASK");
        if(mask&&bits){
            unsigned parsed=0,valid=*bits!=0;
            for(const char *p=bits;*p;++p){
                if(*p<'0'||*p>'9'||parsed>63){valid=0;break;}
                parsed=parsed*10u+(unsigned)(*p-'0');
            }
            mask=valid&&parsed<=63?parsed:0;
        }
        cached=(int)mask;
    }
    return (unsigned)cached;
}
#define NV_CPU_COMPACT2_MASK() nv_cpu_compact2_mask()
#endif
#define NV_CPU_COMPACT2_ON(bit) ((NV_CPU_COMPACT2_MASK()&(bit))!=0)
#endif

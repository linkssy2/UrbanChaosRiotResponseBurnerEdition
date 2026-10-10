#ifndef UC_HYBRID_MODELS_SAMPLER_H
#define UC_HYBRID_MODELS_SAMPLER_H
#include <stdint.h>
#include <math.h>
#include <string.h>
/* Original 3ED480 stage0 deferred sampler publication. Snapshot DWORDs0..12
 * at3F3DA0, device+968 global float bias and resolved3F4128 wrap bits. The
 * masked905B0 material forces ALPHAKILL=4 before its draw. Qualify ordinary
 * point/linear spatial filtering and wrap/mirror/edge-clamp only; other source
 * modes need their own contract. No guest or host state is mutated. */
static int uc_hybrid_models_sampler(const uint32_t state[13],float global_bias,
    uint32_t wrap_bits,uint32_t out[3]){
    if(!state||!out||wrap_bits)return 0;
    for(unsigned i=0;i<3;i++)if(state[i]<1||state[i]>3)return 0;
    if(state[3]<1||state[3]>2||state[4]<1||state[4]>2||state[5]>2||
       state[7]>15||state[9]||state[10]||state[12]>=25)return 0;
    float bias;memcpy(&bias,state+6,4);
    if(!isfinite(bias)||!isfinite(global_bias))return 0;
    /* Source x87 computes double intermediates, stores float, then CVTTSS2SI.
     * Keep conversion before the signed13-bit clamp, including negative bias. */
    float scaled=(float)(((double)bias+(double)global_bias)*256.0+0.5);
    if(!isfinite(scaled)||scaled< -2147483648.0f||scaled>=2147483648.0f)return 0;
    int32_t lod=(int32_t)scaled;if(lod< -4096)lod=-4096;if(lod>4095)lod=4095;
    uint32_t address=state[0]|(state[1]<<8)|(state[2]<<16);
    uint32_t min_code=state[4]+2*state[5];
    out[0]=address;out[1]=(state[3]<<24)|(min_code<<16)|0x2000u|((uint32_t)lod&0x1FFFu);
    out[2]=(state[7]<<26)|0x4003FFC4u;return 1;
}
#endif

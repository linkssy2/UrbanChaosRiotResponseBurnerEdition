#ifndef NV2A_DEPTH_OFFSET_H
#define NV2A_DEPTH_OFFSET_H
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "nv2a_regs.h"
/* Supported bridge: filled triangles, affine Z, 24-bit UNORM depth and
 * integral Xbox offset units. Do not reinterpret fractional/W-buffer offsets
 * as a host integer bias. Outputs always reset, including disabled draws. */
static int nv_depth_offset(const uint32_t *m,unsigned primitive,int depth,
                           float *slope,int *units){
    *slope=0;*units=0;
    if(!depth||primitive<5||primitive>10||!m[NV097_SET_POLY_OFFSET_FILL_ENABLE/4])return 1;
    if(m[NV097_SET_FRONT_POLYGON_MODE/4]!=0x1B02||m[NV097_SET_BACK_POLYGON_MODE/4]!=0x1B02||
       (m[NV097_SET_CONTROL0/4]&(NV097_SET_CONTROL0_Z_PERSPECTIVE_ENABLE|NV097_SET_CONTROL0_Z_FORMAT))||
       m[NV097_SET_CLIP_MAX/4]!=0x4B7FFFFF)return 0;
    float factor,bias;
    memcpy(&factor,&m[NV097_SET_POLYGON_OFFSET_SCALE_FACTOR/4],4);
    memcpy(&bias,&m[NV097_SET_POLYGON_OFFSET_BIAS/4],4);
    if(!isfinite(factor)||!isfinite(bias)||bias<-2147483648.0||bias>=2147483648.0||floor((double)bias)!=(double)bias)return 0;
    *slope=factor;*units=(int)bias;return 1;
}
#endif

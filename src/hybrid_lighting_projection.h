#ifndef UC_HYBRID_LIGHTING_PROJECTION_H
#define UC_HYBRID_LIGHTING_PROJECTION_H
#include <math.h>
#include <string.h>
/* Row-vector composition of source clip -> NV viewport -> host D3D NDC.
 * Source oPos.xyz=(clip.xyz/clip.w)*c58.xyz+c59.xyz; oPos.w=clip.w.
 * The actual GPU vertex adapter uses logical draw width/height and clip max,
 * not physical scaled target size. This math does not certify guest camera or
 * program provenance; the insertion owner must establish those separately. */
static int uc_hybrid_compose_projection(float out[16],const float guest[16],
    const float scale[3],const float bias[3],float width,float height,float depth){
    if(!out||!guest||!scale||!bias||!isfinite(width)||!isfinite(height)||!isfinite(depth)||width<=0||height<=0||depth<=0)return 0;
    float n[]={2*scale[0]/width,-2*scale[1]/height,scale[2]/depth};
    float t[]={2*bias[0]/width-1,1-2*bias[1]/height,bias[2]/depth};
    for(unsigned c=0;c<3;c++)if(!isfinite(n[c])||!isfinite(t[c])||n[c]==0)return 0;
    float result[16];
    for(unsigned r=0;r<4;r++){
        for(unsigned c=0;c<3;c++)result[4*r+c]=guest[4*r+c]*n[c]+guest[4*r+3]*t[c];
        result[4*r+3]=guest[4*r+3];
    }
    for(unsigned i=0;i<16;i++)if(!isfinite(result[i]))return 0;
    memcpy(out,result,sizeof result);return 1;
}
#endif

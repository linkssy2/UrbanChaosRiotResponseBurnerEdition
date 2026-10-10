#include "environment_glow_outer.h"
#include <math.h>
#include <xmmintrin.h>
int uc_environment_glow_outer(const UCEnvironmentGlowOuterInput *in,UCEnvironmentGlowOuterPlan *out){
    if(!in||!out||(_mm_getcsr()&0xE040u))return 1;
    const uint32_t sizes[]={in->scene_width,in->scene_height,in->scratch_width,in->scratch_height,in->display_width,in->display_height};
    for(unsigned i=0;i<6;i++)if(!sizes[i]||sizes[i]>4096)return 1;
    const float values[]={in->view[0],in->view[1],in->view[2],in->view[3],in->threshold,in->gain};
    for(unsigned i=0;i<6;i++)if(!isfinite(values[i])||fabsf(values[i])>4096)return 1;
    UCEnvironmentGlowOuterPlan p={0};
    for(unsigned i=0;i<4;i++){
        float x=(float)(i&1),y=(float)(i>>1);
        p.vertices[0][i]=(UCEnvironmentGlowVertex){{x*in->scratch_width,y*in->scratch_height,1},{x*in->scene_width,y*in->scene_height}};
        p.vertices[1][i]=(UCEnvironmentGlowVertex){{x*in->scene_width-2,y*in->scene_height-2,1},{x*in->scratch_width,y*in->scratch_height}};
    }
    for(unsigned k=0;k<3;k++)p.constants[0][k]=in->threshold;
    for(unsigned k=0;k<4;k++)p.constants[1][k]=in->gain*.25f;
    float ratios[4]={(float)((double)in->view[0]/in->display_width),(float)((double)in->view[1]/in->display_height),
        (float)(((double)in->view[2]+in->view[0])/in->display_width),(float)(((double)in->view[3]+in->view[1])/in->display_height)};
    p.scissor[0]=(int32_t)((double)ratios[0]*in->scene_width);p.scissor[1]=(int32_t)((double)ratios[1]*in->scene_height);
    p.scissor[2]=(int32_t)((double)ratios[2]*in->scene_width);p.scissor[3]=(int32_t)((double)ratios[3]*in->scene_height);
    *out=p;return 0;
}

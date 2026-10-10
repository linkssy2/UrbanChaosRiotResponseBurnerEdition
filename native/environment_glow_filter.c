#include "environment_glow_filter.h"
int uc_environment_glow_filter(UCEnvironmentGlowKernel *kernel,float sigma,
    uint32_t width,uint32_t height,UCEnvironmentGlowFilterPlan *out){
    if(!kernel||!out||!width||!height||width>4096||height>4096)return 1;
    UCEnvironmentGlowKernel next=*kernel;UCEnvironmentGlowTaps taps;
    if(uc_environment_glow_prepare(&next,sigma,&taps))return 1;
    UCEnvironmentGlowFilterPlan plan;
    const float w=(float)width,h=(float)height;
    const float corners[4][2]={{0,0},{w,0},{0,h},{w,h}};
    const float offsets[4]={0-taps.pair_offset[0],taps.pair_offset[0],0-taps.pair_offset[1],taps.pair_offset[1]};
    for(unsigned pass=0;pass<2;pass++)for(unsigned corner=0;corner<4;corner++){
        UCEnvironmentGlowFilterVertex *v=&plan.vertices[pass][corner];
        v->position[0]=corners[corner][0];v->position[1]=corners[corner][1];v->position[2]=1;
        for(unsigned sample=0;sample<4;sample++){
            v->uv[sample][0]=(pass?0:offsets[sample])+corners[corner][0];
            v->uv[sample][1]=(pass?offsets[sample]:0)+corners[corner][1];
        }
    }
    for(unsigned i=0;i<2;i++){
        plan.constants[i][0]=plan.constants[i][1]=plan.constants[i][2]=taps.pair_weight[i];
        plan.constants[i][3]=taps.center*.5f;
    }
    *kernel=next;*out=plan;return 0;
}

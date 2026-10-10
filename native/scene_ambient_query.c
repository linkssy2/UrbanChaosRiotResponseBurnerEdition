#include "scene_ambient_query.h"
#include <fenv.h>
#include <math.h>
#include <xmmintrin.h>
static int bounded(const float *v,unsigned n,float limit){if(!v)return 0;for(unsigned i=0;i<n;i++)if(!isfinite(v[i])||fabsf(v[i])>limit)return 0;return 1;}
static void contribution(const UCSceneAmbientLight *l,const float p[3],float v[4]){
    for(unsigned j=0;j<4;j++)v[j]=0;
    if(l->kind!=3)return;
    float d[4];for(unsigned j=0;j<4;j++){d[j]=(j==3?1.f:p[j])-l->local_center[j];d[j]=d[j]*d[j];}
    float squared=(d[0]+d[2])+(d[1]+d[3]);float reciprocal=_mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(squared)));
    float correction=3.f-(reciprocal*squared)*reciprocal;
    float distance=squared*((.5f*reciprocal)*correction);
    float factor=(l->state->attenuation-distance)/l->state->attenuation;
    if(!(factor<1))factor=1;else if(!(factor>0))factor=0;
    float scale=l->state->multiplier*factor;for(unsigned j=0;j<4;j++)v[j]=l->state->color[j]*scale;
}
int uc_scene_ambient_query(uint32_t base,const UCSceneAmbientLight *lights,uint32_t count,const float p[3],uint32_t *out){
    if(!out||count>65536||(count&&!lights)||!bounded(p,3,1e6f)||(_mm_getcsr()&0xE040u)||fegetround()!=FE_TONEAREST)return 1;
    for(uint32_t i=0;i<count;i++){
        const UCSceneAmbientLight *l=lights+i;
        if(l->kind!=0&&l->kind!=1&&l->kind!=3)return 1;
        if(l->kind==3&&(!l->state||!bounded(l->local_center,4,1e6f)||!bounded(l->state->color,4,32)||
           !isfinite(l->state->multiplier)||fabsf(l->state->multiplier)>32||!isfinite(l->state->attenuation)||fabsf(l->state->attenuation)>1e6f))return 1;
    }
    const unsigned shift[4]={16,8,0,24};float sum[4];
    for(unsigned j=0;j<4;j++)sum[j]=(float)((base>>shift[j])&255)*(1.f/255.f);
    for(uint32_t i=0;i<count;i++){
        float v[4];contribution(lights+i,p,v);
        for(unsigned j=0;j<4;j++)sum[j]=sum[j]+v[j];
    }
    uint32_t packed=0;
    for(unsigned j=0;j<4;j++){
        float f=sum[j]*255.f;if(!isfinite(f)||f< -2147483648.f||f>=2147483648.f)return 1;
        int32_t n=(int32_t)f;if(n>255)n=255;else if(n<0)n=0;packed|=(uint32_t)n<<shift[j];
    }
    *out=packed;return 0;
}

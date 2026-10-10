#include "point_light_vertex.h"
#include "model_asset.h"
#include <math.h>
#include <string.h>
#include <fenv.h>
#include <xmmintrin.h>
static int finite_values(const float *p,unsigned n){for(unsigned i=0;i<n;i++)if(!isfinite(p[i]))return 0;return 1;}
static float normal_component(uint32_t bits,unsigned width){
    int value=(int)(bits&((1u<<width)-1));int sign=1<<(width-1);
    if(value&sign)value-=sign*2;
    return value== -sign?-1.f:(float)value/(float)(sign-1);
}
static float position_row(const float p[3],const float row[4]){
    return p[0]*row[0]+p[1]*row[1]+p[2]*row[2]+row[3];
}
int uc_point_light_vertex(const UCPointLightVertexInput *v,
    const UCPointLightVertexConstants *c,UCPointLightVertexOutput *out){
    if(!v||!c||!out||!finite_values(v->position,3)||!finite_values(&c->clip_rows[0][0],16)||
       !finite_values(&c->attenuation_rows[0][0],12)||!finite_values(c->center,3)||
       !finite_values(c->viewport_scale,3)||!finite_values(c->viewport_offset,3)||
       (v->skin_rows&&!finite_values(&v->skin_rows[0][0],36))||
       (_mm_getcsr()&0xE040u)||fegetround()!=FE_TONEAREST)return 0;
    UCPointLightVertexOutput r;float position[3],normal[3]={normal_component(v->packed_normal,11),
        normal_component(v->packed_normal>>11,11),normal_component(v->packed_normal>>22,10)};
    if(v->skin_rows){
        uc_model_skin_position(v->position,v->weights,v->skin_rows,position);
        float weights[3]={v->weights[0]/255.f,v->weights[1]/255.f,0};weights[2]=1.f-(weights[0]+weights[1]);
        for(unsigned row=0;row<3;row++){
            float blended[3];
            for(unsigned k=0;k<3;k++){
                unsigned i=row*4+k;float first=weights[0]*v->skin_rows[0][i];
                float second=weights[1]*v->skin_rows[1][i]+first;
                blended[k]=weights[2]*v->skin_rows[2][i]+second;
            }
            r.normal[row]=normal[0]*blended[0]+normal[1]*blended[1]+normal[2]*blended[2];
        }
    }else{memcpy(position,v->position,12);memcpy(r.normal,normal,12);}
    if(!finite_values(position,3))return 0;
    for(unsigned i=0;i<4;i++){r.clip[i]=position_row(position,c->clip_rows[i]);r.color[i]=1;}
    if(!finite_values(r.clip,4)||r.clip[3]==0)return 0;
    float reciprocal=1.f/r.clip[3],magnitude=fabsf(reciprocal);
    if(magnitude<0x1p-64f)magnitude=0x1p-64f;
    if(magnitude>0x1p64f)magnitude=0x1p64f;
    reciprocal=copysignf(magnitude,reciprocal);
    for(unsigned i=0;i<3;i++){
        r.attenuation[i]=position_row(position,c->attenuation_rows[i]);
        r.direction[i]=c->center[i]+ -position[i];
        float scaled=r.clip[i]*c->viewport_scale[i];
        r.screen[i]=scaled*reciprocal+c->viewport_offset[i];
    }
    if(!finite_values(r.screen,3)||!finite_values(r.attenuation,3)||
       !finite_values(r.direction,3)||!finite_values(r.normal,3))return 0;
    *out=r;return 1;
}

#include "model_visibility.h"
#include <math.h>

static int finite_values(const float *p,unsigned n){
    for(unsigned i=0;i<n;++i)if(!isfinite(p[i]))return 0;
    return 1;
}
static int distance(const UCModelBounds *b,const float p[4],int maximum,float *out){
    float v[3];
    for(unsigned i=0;i<3;++i){
        int high=maximum?(p[i]>0):!(p[i]>0);
        v[i]=(high?b->maximum[i]:b->minimum[i])*p[i];
    }
    *out=(v[2]+v[1])+v[0];
    return finite_values(v,3)&&isfinite(v[2]+v[1])&&isfinite(*out);
}
UCModelStatus uc_model_visibility_plane(const float m[16],const UCModelBounds *b,
    const float near_plane[4],uint32_t *out){
    unsigned inside=1;
    if(!m||!b||!near_plane||!out)return UC_MODEL_ARGUMENT;
    if(!finite_values(m,16)||!finite_values(b->minimum,3)||
       !finite_values(b->maximum,3)||!finite_values(near_plane,4))return UC_MODEL_UNSUPPORTED;
    for(unsigned plane=0;plane<6;++plane){
        float p[4],limit,high,low;
        for(unsigned row=0;row<4;++row){
            unsigned n=row*4;
            switch(plane){
            case 0:p[row]=near_plane[row];break;
            case 1:p[row]=m[n+3]-m[n+2];break;
            case 2:p[row]=row==1||row==2?m[n]+m[n+3]:m[n+3]+m[n];break;
            case 3:p[row]=m[n+3]-m[n];break;
            case 4:p[row]=row==1||row==2?m[n+1]+m[n+3]:m[n+3]+m[n+1];break;
            default:p[row]=m[n+3]-m[n+1];break;
            }
        }
        limit=0.0f-p[3];
        if(!finite_values(p,4)||!distance(b,p,1,&high))return UC_MODEL_UNSUPPORTED;
        if(limit>high){*out=0;return UC_MODEL_OK;}
        if(inside){
            if(!distance(b,p,0,&low))return UC_MODEL_UNSUPPORTED;
            if(limit>low)inside=0;
        }
    }
    *out=1+inside;return UC_MODEL_OK;
}
UCModelStatus uc_model_visibility(const float m[16],const UCModelBounds *b,uint32_t *out){
    if(!m)return UC_MODEL_ARGUMENT;
    const float p[4]={m[2],m[6],m[10],m[14]};
    return uc_model_visibility_plane(m,b,p,out);
}

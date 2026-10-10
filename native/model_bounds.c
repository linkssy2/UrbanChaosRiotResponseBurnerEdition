#include "model_bounds.h"
#include <math.h>
#include <string.h>

static int finite_values(const float *p,unsigned n){for(unsigned i=0;i<n;++i)if(!isfinite(p[i]))return 0;return 1;}
static int finite_bounds(const UCModelBounds *b){return finite_values(b->minimum,3)&&finite_values(b->maximum,3)&&finite_values(b->center,3)&&finite_values(b->extent,3);}
static void include(UCModelBounds *b,const float point[3]){
    for(unsigned i=0;i<3;++i){
        if(b->minimum[i]>point[i])b->minimum[i]=point[i];
        else if(point[i]>b->maximum[i])b->maximum[i]=point[i];
        b->center[i]=(b->maximum[i]+b->minimum[i])*.5f;
        b->extent[i]=(b->maximum[i]-b->minimum[i])*.5f;
    }
}
UCModelStatus uc_model_bounds(const UCModel *m,const float (*world)[16],size_t count,
    const uint8_t *valid,UCModelBounds *out){
    UCModelBounds result;
    if(!m||!out||count!=m->bone_count||!m->bones||(!world&&m->bound_count)||
       (m->bound_count&&!m->bounds))return UC_MODEL_ARGUMENT;
    memcpy(&result,m->source_group+4,sizeof(result));
    if(!m->bound_count){
        if(!finite_bounds(&result))return UC_MODEL_UNSUPPORTED;
        *out=result;return UC_MODEL_OK;
    }
    for(uint32_t i=0;i<m->bound_count;++i){
        const UCModelBound *bound=m->bounds+i;float matrix[16],center[3],extent[3];UCModelBounds transformed;
        if(bound->bone>=count||(valid&&!valid[bound->bone]))return UC_MODEL_ARGUMENT;
        const float *a=m->bones[bound->bone].bind_correction,*b=world[bound->bone];
        if(!finite_values(a,16)||!finite_values(b,16)||!finite_values(bound->limits,6))return UC_MODEL_UNSUPPORTED;
        for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)
            matrix[r*4+c]=(a[r*4]*b[c]+a[r*4+1]*b[4+c])+(a[r*4+2]*b[8+c]+a[r*4+3]*b[12+c]);
        if(!finite_values(matrix,16))return UC_MODEL_UNSUPPORTED;
        for(unsigned c=0;c<3;++c){center[c]=(bound->limits[c]+bound->limits[3+c])*.5f;extent[c]=(bound->limits[3+c]-bound->limits[c])*.5f;}
        for(unsigned c=0;c<3;++c){
            float midpoint=(matrix[c]*center[0]+matrix[12+c])+(matrix[4+c]*center[1]+matrix[8+c]*center[2]);
            float radius=fabsf(matrix[c]*extent[0])+(fabsf(matrix[4+c]*extent[1])+fabsf(matrix[8+c]*extent[2]));
            float low=midpoint-radius,high=midpoint+radius;
            transformed.minimum[c]=high<low?high:low;
            transformed.maximum[c]=high>low?high:low;
            transformed.center[c]=(transformed.maximum[c]+transformed.minimum[c])*.5f;
            transformed.extent[c]=(transformed.maximum[c]-transformed.minimum[c])*.5f;
        }
        if(!finite_bounds(&transformed))return UC_MODEL_UNSUPPORTED;
        if(!i)result=transformed;
        else{include(&result,transformed.minimum);include(&result,transformed.maximum);}
        if(!finite_bounds(&result))return UC_MODEL_UNSUPPORTED;
    }
    *out=result;return UC_MODEL_OK;
}

#include "firstperson_projection.h"
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int finite_matrix(const float p[16]) {
    for (unsigned i=0;i<16;++i) if (!isfinite(p[i])) return 0;
    return 1;
}
static int depth_valid(float low,float high) {return isfinite(low)&&isfinite(high)&&low>=0&&high>=low&&high<=1;}
int uc_firstperson_projection(float fov,float aspect,float near_z,float far_z,float low,float high,float out[16]) {
    float p[16]={0};
    if (!out || !isfinite(fov) || fov<=0 || fov>=180 || !isfinite(aspect) || aspect<=0
        || !isfinite(near_z) || !isfinite(far_z) || near_z<=0 || far_z<=near_z || !depth_valid(low,high)) return -1;
    if ((_mm_getcsr()&0xE040u)!=0) return -2;
    p[5]=(float)(1.0/tan(((double)fov*(double)0.01745329238474369f)*.5));
    p[0]=p[5]/aspect;
    p[14]=(((low-high)*far_z)*near_z)/(far_z-near_z);
    p[10]=high-p[14]/far_z; p[11]=1;
    if (!finite_matrix(p)) return -2;
    memcpy(out,p,sizeof(p));return 0;
}
int uc_firstperson_depth_slice(const float projection[16],float low,float high,float out[16]) {
    float p[16],span;
    if (!projection || !out || !finite_matrix(projection) || !depth_valid(low,high)) return -1;
    if ((_mm_getcsr()&0xE040u)!=0) return -2;
    memcpy(p,projection,sizeof(p));span=high-low;
    for (unsigned r=0;r<4;++r) p[r*4+2]=projection[r*4+3]*low+projection[r*4+2]*span;
    if (!finite_matrix(p)) return -2;
    memcpy(out,p,sizeof(p));return 0;
}

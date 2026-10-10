#include "point_light_geometry.h"
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int finite_values(const float *p,unsigned n){for(unsigned i=0;i<n;i++)if(!isfinite(p[i]))return 0;return 1;}
/* ACD00 arithmetic, with no synthetic model/duplicate model owner. The
 * existing model_bounds implementation only exposes whole-model aggregation. */
static void transform_bounds(const UCModelBounds *source,const float m[16],UCModelBounds *out){
    float center[3],extent[3];
    for(unsigned i=0;i<3;i++){center[i]=(source->minimum[i]+source->maximum[i])*.5f;extent[i]=(source->maximum[i]-source->minimum[i])*.5f;}
    for(unsigned i=0;i<3;i++){
        float middle=(m[i]*center[0]+m[12+i])+(m[4+i]*center[1]+m[8+i]*center[2]);
        float radius=fabsf(m[i]*extent[0])+(fabsf(m[4+i]*extent[1])+fabsf(m[8+i]*extent[2]));
        float low=middle-radius,high=middle+radius;
        out->minimum[i]=high<low?high:low;out->maximum[i]=high>low?high:low;
        out->center[i]=(out->maximum[i]+out->minimum[i])*.5f;
        out->extent[i]=(out->maximum[i]-out->minimum[i])*.5f;
    }
}
int uc_point_light_geometry_update(UCPointLightState *state,const float world[16],
    const float center[4],UCPointLightGeometry *out){
    if(!state||!out)return 0;
    if(!state->dirty)return 1;
    if(!world||!center||(_mm_getcsr()&0xE040u)||!finite_values(world,16)||
       !finite_values(center,4)||!isfinite(state->attenuation))return 0;
    UCPointLightGeometry next={0};float radius=state->attenuation;
    for(unsigned i=0;i<3;i++){
        next.local_bounds.minimum[i]=center[i]-radius;
        next.local_bounds.maximum[i]=i?center[i]+radius:radius+center[i];
        next.local_bounds.center[i]=center[i];next.local_bounds.extent[i]=radius;
    }
    transform_bounds(&next.local_bounds,world,&next.world_bounds);
    for(unsigned i=0;i<4;i++)next.world_center[i]=(world[i]*center[0]+world[4+i]*center[1])+(world[8+i]*center[2]+world[12+i]*center[3]);
    float scale=radius>0?1.0f/(radius*2.0f):1000000000.0f;
    for(unsigned i=0;i<3;i++){
        next.world_to_volume[i*5]=scale;next.scale[i]=scale;
        next.world_to_volume[12+i]=-next.world_center[i]*scale;
        next.offset[i]=next.world_to_volume[12+i];
    }
    next.world_to_volume[15]=-next.world_center[3]*scale;
    if(!finite_values(next.local_bounds.minimum,3)||!finite_values(next.local_bounds.maximum,3)||
       !finite_values(next.world_bounds.minimum,3)||!finite_values(next.world_bounds.maximum,3)||
       !finite_values(next.world_bounds.center,3)||!finite_values(next.world_bounds.extent,3)||
       !finite_values(next.world_center,4)||!finite_values(next.world_to_volume,16))return 0;
    state->dirty=0;*out=next;return 1;
}

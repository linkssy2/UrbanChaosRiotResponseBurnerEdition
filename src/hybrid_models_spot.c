#include "hybrid_models_guest.h"
#include "../native/point_light_receiver.h"
#include "../native/point_light_pixel.h"
#include "../native/model_visibility.h"
#include <math.h>
#include <string.h>
static int finite_values(const float*v,unsigned n){for(unsigned i=0;i<n;i++)if(!isfinite(v[i]))return 0;return 1;}
static void transform(const float v[4],const float m[16],float out[4]){
    for(unsigned i=0;i<4;i++)out[i]=(m[i]*v[0]+m[4+i]*v[1])+(m[8+i]*v[2]+m[12+i]*v[3]);
}
int uc_hybrid_models_prepare_spot(const UcHybridLightingScene*s,const UcHybridLightingRecord*l,const UcHybridModelBatch*b,UcHybridModelLightView*out){
    if(!s||!l||!b||!out||!l->spot_valid||l->vtable!=0x457270u||!finite_values(b->bounds,6)||
       !finite_values(l->local,4)||!finite_values(l->direction,4))return 0;
    float inverse[16];if(!uc_point_light_inverse(b->world,inverse))return 0;
    /* B02D0 -> ACD00 world AABB, then AF4B0's explicit near plane. */
    UCModelBounds bounds;float center[3],extent[3];
    for(unsigned i=0;i<3;i++){center[i]=(b->bounds[i]+b->bounds[3+i])*.5f;extent[i]=(b->bounds[3+i]-b->bounds[i])*.5f;}
    for(unsigned i=0;i<3;i++){
        float middle=(b->world[i]*center[0]+b->world[12+i])+(b->world[4+i]*center[1]+b->world[8+i]*center[2]);
        float radius=fabsf(b->world[i]*extent[0])+(fabsf(b->world[4+i]*extent[1])+fabsf(b->world[8+i]*extent[2]));
        float low=middle-radius,high=middle+radius;bounds.minimum[i]=high<low?high:low;bounds.maximum[i]=high>low?high:low;
    }
    float plane[4]={l->direction[0],l->direction[1],l->direction[2],
        0.f-((l->local[2]*l->direction[2]+l->local[1]*l->direction[1])+l->local[0]*l->direction[0])};
    uint32_t visible;if(uc_model_visibility_plane(l->spot.projection,&bounds,plane,&visible)!=UC_MODEL_OK||!visible)return 0;
    UCNativeControlSpot receiver=l->spot;
    for(unsigned i=0;i<4;i++)transform(b->world+4*i,l->spot.projection,receiver.projection+4*i);
    UcHybridModelLightView next=*out;
    if(!uc_spot_light_rows(&receiver,next.attenuation_rows))return 0;
    transform(l->spot.center,inverse,next.center);
    float scalar[4]={b->material_scalar,b->material_scalar,b->material_scalar,b->material_scalar};uint32_t packed;
    if(!finite_values(next.center,4)||!uc_point_light_constant(scalar,&packed))return 0;
    next.scalar[0]=(float)((packed>>16)&255)/255.f;next.scalar[1]=(float)((packed>>8)&255)/255.f;
    next.scalar[2]=(float)(packed&255)/255.f;next.scalar[3]=(float)(packed>>24)/255.f;
    next.kind=2;
    if(l->specular&&b->specular_strength>=.05f){
        transform(s->camera+12,inverse,next.eye);
        float spec[4];for(unsigned i=0;i<4;i++)spec[i]=l->intensity*b->specular_strength;
        if(!finite_values(next.eye,4)||!uc_point_light_constant(spec,&packed))return 0;
        next.specular[0]=(float)((packed>>16)&255)/255.f;next.specular[1]=(float)((packed>>8)&255)/255.f;
        next.specular[2]=(float)(packed&255)/255.f;next.specular[3]=(float)(packed>>24)/255.f;next.kind=5;
    }
    *out=next;return 1;
}

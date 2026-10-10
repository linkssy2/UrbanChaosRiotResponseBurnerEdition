#include "hybrid_models_guest.h"
#include "../native/point_light_geometry.h"
#include "../native/point_light_receiver.h"
#include "../native/point_light_pixel.h"
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int finite_values(const float*v,unsigned n){for(unsigned i=0;i<n;i++)if(!isfinite(v[i]))return 0;return 1;}
static void transform(const float v[4],const float m[16],float out[4]){
    for(unsigned i=0;i<4;i++)out[i]=(m[i]*v[0]+m[4+i]*v[1])+(m[8+i]*v[2]+m[12+i]*v[3]);
}
static void multiply(const float a[16],const float b[16],float out[16]){for(unsigned i=0;i<4;i++)transform(a+4*i,b,out+4*i);}
static int packed(const float v[4],float out[4]){
    uint32_t p;if(!uc_point_light_constant(v,&p))return 0;
    out[0]=(float)((p>>16)&255)/255.f;out[1]=(float)((p>>8)&255)/255.f;out[2]=(float)(p&255)/255.f;out[3]=(float)(p>>24)/255.f;return 1;
}
static int directional(const UcHybridLightingRecord*l,const float world[16],UcHybridModelLightView*out){
    /* 2D47C0 is uniform-scale affine inversion, NOT the general cofactor path. */
    float norm=(world[8]*world[8]+world[4]*world[4])+world[0]*world[0];
    if(!isfinite(norm)||norm==0)return 0;float inv[16]={0},scale=1.f/norm;
    for(unsigned i=0;i<3;i++)for(unsigned j=0;j<3;j++)inv[i*4+j]=world[j*4+i]*scale;
    for(unsigned j=0;j<3;j++)inv[12+j]=0.f-((inv[8+j]*world[14]+inv[4+j]*world[13])+inv[j]*world[12]);
    inv[15]=1;float world_direction[4],local[4];transform(l->local,l->world,world_direction);transform(world_direction,inv,local);
    __m128 v=_mm_loadu_ps(local),n=_mm_mul_ps(v,v);n=_mm_add_ps(n,_mm_movehl_ps(n,n));n=_mm_add_ss(n,_mm_shuffle_ps(n,n,1));
    float length=_mm_cvtss_f32(n);if(!(length>0)||!isfinite(length))return 0;
    __m128 r=_mm_rsqrt_ss(n);__m128 correction=_mm_sub_ss(_mm_set_ss(3.f),_mm_mul_ss(_mm_mul_ss(r,n),r));
    r=_mm_mul_ss(_mm_mul_ss(_mm_set_ss(.5f),r),correction);r=_mm_shuffle_ps(r,r,0);
    _mm_storeu_ps(local,_mm_xor_ps(_mm_mul_ps(v,r),_mm_set1_ps(-0.f)));
    float positive[4],negative[4],a[4],b[4];for(unsigned i=0;i<4;i++){positive[i]=local[i];negative[i]=-local[i];}
    if(!packed(positive,a)||!packed(negative,b))return 0;
    for(unsigned i=0;i<4;i++)out->direction[i]=a[i]-b[i];return 1;
}
static void upload(void*p,unsigned first,const float*v,unsigned count){
    UcHybridModelLightView*out=p;if(first==113&&count==16)memcpy(out->attenuation_rows,v,64);
    else if(first==112&&count==4)memcpy(out->center,v,16);
}
static void draw(void*p,const void*r,const void*pass){(void)p;(void)r;(void)pass;}
static int two_sided(const UcHybridModelBatch*b){
    /* Exact 8EC20 recipe: cull=0; draw once; cull=0x900. No interpreter. */
    const unsigned char*p=b->setup;
    if(b->setup_bytes!=69||p[0]!=0xC1||p[17]!=0x82||p[34]!=2||p[35]!=0xC0||p[52]!=0x82)return 0;
    for(unsigned i=0;i<4;i++){
        uint32_t a,c,d,e;memcpy(&a,p+1+4*i,4);memcpy(&c,p+18+4*i,4);
        memcpy(&d,p+36+4*i,4);memcpy(&e,p+53+4*i,4);
        if(a||c!=0x93||d!=0x900||e!=0x93)return 0;
    }return 1;
}
int uc_hybrid_models_prepare(const UcHybridLightingScene*s,const UcHybridLightingRecord*l,const UcHybridModelBatch*b,UcHybridModelLightView*out){
    if(!s||!l||!b||!out||(_mm_getcsr()&0xE040u)||
       !finite_values(b->world,16)||!finite_values(b->projection,16)||
       !finite_values(s->view,16)||!isfinite(s->exposure)||s->exposure<=0)return 0;
    UcHybridModelLightView v={0};v.two_sided=(uint32_t)two_sided(b);
    if(b->setup_bytes&&!v.two_sided)return 0;
    if(b->material_mode||b->material_flags&0x222u){
        if(b->material_mode!=1||b->material_flags!=0x25u||!b->skinned||!v.two_sided||
           !b->texture_header_va||l->vtable!=0x457270u)return 0;
        v.material_kind=1;
    }
    float matrix[16],clip[16],color[4];multiply(b->world,s->view,matrix);multiply(matrix,b->projection,clip);
    for(unsigned i=0;i<4;i++)for(unsigned j=0;j<4;j++)v.clip_rows[i][j]=clip[j*4+i];
    for(unsigned i=0;i<4;i++)color[i]=(l->color[i]*l->intensity)/s->exposure;
    if(!packed(color,v.color))return 0;
    if(l->vtable==0x457338u){
        if(!directional(l,b->world,&v))return 0;
        if(b->specular_strength>=.05f){
            v.kind=3;float inverse[16],spec[4];if(!uc_point_light_inverse(b->world,inverse))return 0;
            transform(s->camera+12,inverse,v.eye);
            for(unsigned i=0;i<4;i++)spec[i]=l->intensity*b->specular_strength;
            if(!packed(spec,v.specular))return 0;
        }
    }else if(l->vtable==0x4573F0u){
        v.kind=1;UCPointLightState state={0};state.dirty=1;state.attenuation=l->radius;UCPointLightGeometry geometry;
        if(!uc_point_light_geometry_update(&state,l->world,l->local,&geometry))return 0;
        float provider[16],center[4];uint8_t dirty=0;UCPointLightReceiverTarget t={provider,&dirty,center,&v,upload,draw};
        if(uc_point_light_receiver(&geometry,b->world,(const UCModelBounds*)b->bounds,b,NULL,&t)!=2)return 0;
        float scalar[4]={0,0,0,b->material_scalar};if(!packed(scalar,v.scalar))return 0;
        if(l->specular&&b->specular_strength>=.05f){
            v.kind=4;float inverse[16],spec[4];if(!uc_point_light_inverse(b->world,inverse))return 0;
            transform(s->camera+12,inverse,v.eye);
            for(unsigned i=0;i<4;i++)spec[i]=l->intensity*b->specular_strength;
            if(!packed(spec,v.specular))return 0;
        }
    }else if(l->vtable==0x457270u){
        if(!uc_hybrid_models_prepare_spot(s,l,b,&v))return 0;
        if(v.material_kind&&v.kind!=2)return 0; /* Alternate specular is a distinct source program. */
    }else return 0;
    if(!finite_values(&v.clip_rows[0][0],56))return 0;*out=v;return 1;
}

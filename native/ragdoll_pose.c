#include "ragdoll_pose.h"
#include "animation_binding.h"
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int finite_values(const float*p,unsigned n){for(unsigned i=0;i<n;i++)if(!isfinite(p[i]))return 0;return 1;}
void uc_ragdoll_multiply(const float*a,const float*b,float*out){
    float m[16];for(unsigned i=0;i<4;i++)for(unsigned j=0;j<4;j++)
        m[i*4+j]=(a[i*4]*b[j]+a[i*4+1]*b[4+j])+(a[i*4+2]*b[8+j]+a[i*4+3]*b[12+j]);
    memcpy(out,m,sizeof m);
}
int uc_ragdoll_pose_key(const float*bone,const float*actor,UCAnimationKey*out){
    if(!bone||!actor||!out||!finite_values(bone,16)||!finite_values(actor,16))return 0;
    const float swap[16]={1,0,0,0,0,0,1,0,0,1,0,0,0,0,0,1};float world[16],m[16];UCAnimationKey key;
    uc_ragdoll_multiply(actor,swap,world);uc_ragdoll_multiply(bone,world,m);
    if(uc_animation_binding_key_from_matrix(m,&key))return 0;
    double n=(double)key.rotation[0]*key.rotation[0]+(double)key.rotation[1]*key.rotation[1];
    n+=(double)key.rotation[2]*key.rotation[2];n+=(double)key.rotation[3]*key.rotation[3];
    if(!(n>0)||!isfinite(n))return 0;float inverse=(float)(1.0/sqrt(n));
    for(unsigned i=0;i<4;i++)key.rotation[i]*=inverse;
    for(unsigned i=0;i<3;i++)key.translation[i]*=.01f;
    if(!finite_values(key.rotation,4)||!finite_values(key.translation,3))return 0;*out=key;return 1;
}
int uc_ragdoll_pose_world(const float*physics,const float*inverse_actor,float*out){
    if(!physics||!inverse_actor||!out||!finite_values(physics,16)||!finite_values(inverse_actor,16))return 0;
    const float swap[16]={1,0,0,0,0,0,1,0,0,1,0,0,0,0,0,1};float p[16],a[16],m[16];memcpy(p,physics,64);
    for(unsigned i=0;i<3;i++)p[12+i]*=100.f;
    uc_ragdoll_multiply(swap,inverse_actor,a);uc_ragdoll_multiply(p,a,m);
    if(!finite_values(m,16))return 0;memcpy(out,m,64);return 1;
}
int uc_ragdoll_forces(const float*p,float multiplier,UCRagdollForce*out){
    if(!p||!out||!finite_values(p,9)||!finite_values(p+12,8)||!isfinite(p[24])||
       !isfinite(multiplier)||multiplier<0||!(p[8]>0))return 0;
    for(unsigned i=4;i<8;i++)if(p[i]<0&&p[i]!=-1)return 0;
    float f=(p[8]-p[24])/p[8];if(f>1)f=1;else if(f<0)f=0;
    float direction[4];memcpy(direction,p+16,16);
    float square=(direction[0]*direction[0]+direction[1]*direction[1])+direction[2]*direction[2];
    if(!isfinite(square))return 0;
    if(square==0){direction[0]=1;direction[1]=direction[2]=direction[3]=0;square=1;}
    __m128 x=_mm_set_ss(square),r=_mm_rsqrt_ss(x);r=_mm_mul_ss(_mm_mul_ss(_mm_set_ss(.5f),r),_mm_sub_ss(_mm_set_ss(3),_mm_mul_ss(_mm_mul_ss(x,r),r)));
    float inverse=_mm_cvtss_f32(r);for(unsigned i=0;i<4;i++)direction[i]*=inverse;
    UCRagdollForce result[2]={0};
    for(unsigned k=0;k<2;k++){
        UCRagdollForce*v=result+k;float magnitude=((1-f)*p[k*2+1]+p[k*2]*f)*multiplier;
        v->remaining=v->duration=p[4+k];v->radius=p[6+k];memcpy(v->center,p+12,16);
        for(unsigned i=0;i<4;i++)v->initial[i]=v->value[i]=magnitude*(k?(i==2?1.f:0.f):direction[i]);
        if(!finite_values(v->value,4))return 0;
    }memcpy(out,result,sizeof result);return 1;
}
int uc_ragdoll_force_step(UCRagdollForce*s,float dt){
    if(!s||!isfinite(dt)||dt<0||dt>1||!isfinite(s->remaining)||!isfinite(s->duration)||!finite_values(s->initial,4))return 0;
    if(s->remaining!=0&&s->remaining!=-1){if(!(s->duration>0))return 0;float next=s->remaining-dt;if(next<0)next=0;
        float ratio=next/s->duration,value[4];for(unsigned i=0;i<4;i++)value[i]=s->initial[i]*ratio;
        if(!finite_values(value,4))return 0;s->remaining=next;memcpy(s->value,value,16);
    }return 1;
}
int uc_ragdoll_force_sample(const UCRagdollForce*s,const float*p,float*out){
    if(!s||!p||!out||!isfinite(s->remaining)||!isfinite(s->radius)||!finite_values(p,4)||!finite_values(s->center,4)||!finite_values(s->value,4))return 0;
    float result[4]={0};if(s->remaining!=0){
        for(unsigned i=0;i<4;i++)result[i]=p[i]-s->center[i];
        float distance=sqrtf((result[0]*result[0]+result[1]*result[1])+result[2]*result[2]);if(!isfinite(distance))return 0;
        if(distance<.001f){result[0]=result[1]=result[3]=0;result[2]=1;}
        if(s->radius>distance||s->radius==-1){float scale=s->radius!=0?1-distance/s->radius:1;
            for(unsigned i=0;i<4;i++)result[i]=s->value[i]*scale;}
    }if(!finite_values(result,4))return 0;memcpy(out,result,16);return 1;
}

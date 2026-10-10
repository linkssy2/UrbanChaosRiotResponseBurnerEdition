#include "bullet_step.h"
#include <fenv.h>
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int finite_values(const float *v,unsigned n){for(unsigned i=0;i<n;i++)if(!isfinite(v[i]))return 0;return 1;}
static __m128 cross(__m128 a,__m128 b){
    return _mm_sub_ps(_mm_mul_ps(_mm_shuffle_ps(a,a,0xC9),_mm_shuffle_ps(b,b,0xD2)),
        _mm_mul_ps(_mm_shuffle_ps(a,a,0xD2),_mm_shuffle_ps(b,b,0xC9)));
}
static float squared(__m128 v){
    __m128 a=_mm_mul_ps(v,v);a=_mm_add_ps(a,_mm_movehl_ps(a,a));
    return _mm_cvtss_f32(_mm_add_ss(a,_mm_shuffle_ps(a,a,1)));
}
static float inverse(float n){
    float r=_mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(n)));
    float product=(r*n)*r;return (.5f*r)*(3.f-product);
}
int uc_bullet_aim(float *m,float *q,const float *target){
    if(!m||!q||!target||(_mm_getcsr()&0xE040u)||fegetround()!=FE_TONEAREST||
       !finite_values(m,16)||!finite_values(q,4)||!finite_values(target,3))return 1;
    float d[3];for(unsigned i=0;i<3;i++)d[i]=(float)((double)target[i]-m[12+i]);
    double z=(double)target[2]-m[14];
    double length=sqrt(((z*d[2]+(double)d[0]*d[0])+(double)d[1]*d[1]));
    if(length<(double).0001f)return 0;
    float inv=1.f/(float)length,y=inv*d[1];
    if(y<-.99f)y=-1;else if(y>.99f)y=1;
    __m128 forward=_mm_set_ps(0,d[2]*inv,y,d[0]*inv),up=_mm_set_ps(0,0,1,0);
    __m128 right=cross(up,forward),vertical=cross(forward,right);
    float n=squared(right),root=n*inverse(n);
    if(root<.0001f){n=squared(vertical);root=n*inverse(n);if(root<.0001f)return 0;}
    right=_mm_mul_ps(right,_mm_set1_ps(inverse(squared(right))));
    vertical=_mm_mul_ps(vertical,_mm_set1_ps(inverse(squared(vertical))));
    forward=cross(right,vertical);vertical=cross(forward,right);
    float next[16],rotation[4];memcpy(next,m,64);
    _mm_storeu_ps(next,right);_mm_storeu_ps(next+4,vertical);_mm_storeu_ps(next+8,forward);
    if(uc_bullet_frame_quaternion(next,rotation))return 1;
    memcpy(m,next,48);memcpy(q,rotation,16);return 0;
}
int uc_bullet_tracer(const UCBulletTracerServices*h,const UCEntityFreshView*v,UCBulletState*s){
    if(!h||!v||!v->transform||!s||!h->multiplayer||!h->player_actor||!h->append||
       !finite_values(v->transform+12,3)||!finite_values(s->previous218,3))return 1;
    s->tracer_alpha=s->item==4?.15f:.45f;s->tracer_width=1;
    for(unsigned i=0;i<3;i++)s->tracer_rgb[i]=1;
    float from[3],to[3],rgba[4];memcpy(from,s->previous218,12);memcpy(to,v->transform+12,12);
    memcpy(rgba,s->tracer_rgb,12);rgba[3]=s->tracer_alpha;
    unsigned online;if(h->multiplayer(h->owner,&online))return 2;
    if(online){unsigned exists,player;if(h->player_actor(h->owner,s->actor,&exists,&player))return 2;
        if(!exists||!player)return 0;
        float d[3];for(unsigned i=0;i<3;i++)d[i]=(float)((double)to[i]-from[i]);
        float length=(float)sqrt((((double)to[2]-from[2])*d[2]+(double)d[1]*d[1])+(double)d[0]*d[0]);
        if(length<=200)return 0;
        float scale=200.f/length;for(unsigned i=0;i<3;i++)from[i]=d[i]*scale+from[i];
    }
    return h->append(h->owner,from,to,rgba,s->tracer_width)?2:0;
}
int uc_bullet_step(const UCBulletStep *h,const UCEntityFreshView *v,UCBulletState *s){
    if(!s)return 1;if(s->dead1F5)return 0;
    if(!h||!v||!v->transform||!v->actor||!h->delta||!h->preflight||!h->displacement||
       s->sentinel!=-1||(s->tracer1E8&&!h->tracer)||!finite_values(v->transform,16)||
       !finite_values(s->direction,3)||!isfinite(*h->delta)||
       (_mm_getcsr()&0xE040u)||fegetround()!=FE_TONEAREST)return 1;
    float frame[16],q[4];memcpy(frame,v->transform,64);memcpy(q,s->motion.quaternion,16);
    if(!s->started1F4){float target[3];for(unsigned i=0;i<3;i++)target[i]=frame[12+i]+s->direction[i];if(uc_bullet_aim(frame,q,target))return 1;}
    UCBulletMotion admitted=s->motion;float position[4];memcpy(position,frame+12,16);
    if(uc_bullet_motion_finish(&admitted,position,*h->delta)||!h->preflight(h->context,v,s))return 1;
    if(!s->started1F4){memcpy(v->transform,frame,48);memcpy(s->motion.quaternion,q,16);s->started1F4=1;}
    if(!(v->actor->word90&1)){
        float correction[3];if(h->displacement(h->context,v,s,correction))return 2;
        if(!finite_values(correction,3))return 2;
        for(unsigned i=0;i<3;i++)v->transform[12+i]+=correction[i];v->transform[15]=1;
    }
    if(uc_bullet_motion_finish(&s->motion,v->transform+12,*h->delta))return 2;
    if(s->sentinel!=-1)return 2;
    if(s->tracer1E8&&(!h->tracer||h->tracer(h->context,v,s)))return 2;
    memcpy(s->previous218,v->transform+12,12);return 0;
}

#include "bullet_motion.h"
#include <fenv.h>
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int finite_values(const float *p,unsigned n){if(!p)return 0;for(unsigned i=0;i<n;++i)if(!isfinite(p[i]))return 0;return 1;}
static int rounding_ok(void){return !(_mm_getcsr()&0xE040u)&&fegetround()==FE_TONEAREST;}
UCMainplayerStatus uc_bullet_motion_reset(const float position[3],const float quaternion[4],UCBulletMotion *out){
    UCBulletMotion result={0};
    if(!out||!finite_values(position,3)||!finite_values(quaternion,4))return UC_MAINPLAYER_ARGUMENT;
    memcpy(result.previous_position,position,12);memcpy(result.quaternion,quaternion,16);
    result.drag[0]=result.drag[2]=.013f;result.drag[1]=.3f;result.mass=1;
    *out=result;return UC_MAINPLAYER_OK;
}
UCMainplayerStatus uc_bullet_motion_begin(UCBulletMotion *state){
    UCBulletMotion s;float twice[3],cross[3],rotated[3],inverse;
    if(!state||!finite_values(state->local_impulse,3)||!finite_values(state->world_impulse,3)
       ||!finite_values(state->velocity,3)||!finite_values(state->quaternion,4)
       ||!isfinite(state->mass)||!(state->mass>0))return UC_MAINPLAYER_ARGUMENT;
    if((state->flags&1)||!rounding_ok())return UC_MAINPLAYER_UNSUPPORTED;
    s=*state;memset(s.angular_delta,0,sizeof(s.angular_delta));inverse=1.0f/s.mass;
    for(unsigned i=0;i<3;++i)twice[i]=s.local_impulse[i]*2.0f;
    cross[0]=s.quaternion[1]*twice[2]-s.quaternion[2]*twice[1];
    cross[1]=s.quaternion[2]*twice[0]-s.quaternion[0]*twice[2];
    cross[2]=s.quaternion[0]*twice[1]-s.quaternion[1]*twice[0];
    for(unsigned i=0;i<3;++i){
        unsigned j=(i+1)%3,k=(i+2)%3;
        rotated[i]=(s.local_impulse[i]+s.quaternion[3]*cross[i])
            +(s.quaternion[j]*cross[k]-s.quaternion[k]*cross[j]);
    }
    s.velocity[0]=s.velocity[0]+inverse*(s.world_impulse[0]+rotated[0]);
    for(unsigned i=1;i<3;++i)s.velocity[i]=(s.world_impulse[i]+rotated[i])*inverse+s.velocity[i];
    if(!finite_values(s.velocity,3))return UC_MAINPLAYER_UNSUPPORTED;
    *state=s;return UC_MAINPLAYER_OK;
}
UCMainplayerStatus uc_bullet_motion_finish(UCBulletMotion *state,float position[4],float delta){
    UCBulletMotion s;float result[4];
    if(!state||!finite_values(position,3)||!finite_values(state->velocity,3)
       ||!isfinite(delta)||!isfinite(state->mass)||!(state->mass>0)
       ||!isfinite(state->drag[0])||!isfinite(state->drag[2]))return UC_MAINPLAYER_ARGUMENT;
    if(!rounding_ok())return UC_MAINPLAYER_UNSUPPORTED;
    s=*state;memcpy(s.previous_position,position,12);
    for(unsigned i=0;i<3;++i)result[i]=position[i]+s.velocity[i];result[3]=1;
    memset(s.local_impulse,0,sizeof(s.local_impulse));memset(s.world_impulse,0,sizeof(s.world_impulse));
    for(unsigned i=0;i<3;i+=2){
        if(!(s.flags&(i?0x80000u:0x20000u)))s.velocity[i]=0;
        else {
            double amount=(double)delta*s.drag[i]/s.mass;
            double value=s.velocity[i]<0?amount+s.velocity[i]:(double)s.velocity[i]-amount;
            if(s.velocity[i]<0?value>0:value<0)s.velocity[i]=0;
            else s.velocity[i]=(float)value;
        }
    }
    if(!(s.flags&0x40000u))s.velocity[1]=0;
    if(!finite_values(result,3)||!finite_values(s.velocity,3))return UC_MAINPLAYER_UNSUPPORTED;
    *state=s;memcpy(position,result,sizeof(result));return UC_MAINPLAYER_OK;
}
static void rotate_axis(float q[4],float angle,unsigned axis){
    float a=(angle*.01745329238474369f)*.5f,s=(float)sin((double)a),c=(float)cos((double)a);
    float x=q[0],y=q[1],z=q[2],w=q[3];
    if(axis==0){
        q[0]=(s*w+c*x)+(y*0.f-z*0.f);
        q[1]=((s*z-x*0.f)+c*y)+w*0.f;
        q[2]=((x*0.f-s*y)+c*z)+w*0.f;
        q[3]=((c*w-s*x)-y*0.f)-z*0.f;
    }else if(axis==1){
        q[0]=((y*0.f-s*z)+c*x)+w*0.f;
        q[1]=(s*w+c*y)+(z*0.f-x*0.f);
        q[2]=((s*x-y*0.f)+c*z)+w*0.f;
        q[3]=((c*w-x*0.f)-s*y)-z*0.f;
    }else{
        q[0]=((s*y-z*0.f)+c*x)+w*0.f;
        q[1]=((z*0.f-s*x)+c*y)+w*0.f;
        q[2]=(s*w+c*z)+(x*0.f-y*0.f);
        q[3]=((c*w-x*0.f)-y*0.f)-s*z;
    }
}
UCMainplayerStatus uc_bullet_motion_begin_frame(UCBulletMotion *state,float frame[16]){
    if(!state||!frame||!finite_values(frame,16)||!finite_values(state->angular_delta,3)||
       !finite_values(state->quaternion,4)||!rounding_ok())return UC_MAINPLAYER_ARGUMENT;
    UCBulletMotion next=*state;float basis[12];memcpy(basis,frame,48);
    if(next.flags&1){
        if(next.flags&0x10000)rotate_axis(next.quaternion,next.angular_delta[2],2);
        if(next.flags&0x4000)rotate_axis(next.quaternion,next.angular_delta[0],0);
        if(next.flags&0x8000)rotate_axis(next.quaternion,next.angular_delta[1],1);
        float*q=next.quaternion;double length=sqrt((((double)q[0]*q[0]+(double)q[1]*q[1])+(double)q[2]*q[2])+(double)q[3]*q[3]);
        if(!(length>0)||!isfinite(length))return UC_MAINPLAYER_UNSUPPORTED;
        double inv=1./length;for(unsigned i=0;i<4;i++)q[i]=(float)(inv*q[i]);
        float xx=q[0]*(q[0]*2.f),yy=q[1]*(q[1]*2.f),zz=q[2]*(q[2]*2.f);
        float xy=q[1]*(q[0]*2.f),xz=q[2]*(q[0]*2.f),yz=q[2]*(q[1]*2.f);
        float wx=q[3]*(q[0]*2.f),wy=q[3]*(q[1]*2.f),wz=q[3]*(q[2]*2.f);
        basis[0]=1.f-(zz+yy);basis[1]=wz+xy;basis[2]=xz-wy;basis[3]=0;
        basis[4]=xy-wz;basis[5]=1.f-(zz+xx);basis[6]=yz+wx;basis[7]=0;
        basis[8]=wy+xz;basis[9]=yz-wx;basis[10]=1.f-(yy+xx);basis[11]=0;
        next.flags&=0xFFFE3FFEu;
    }
    UCMainplayerStatus status=uc_bullet_motion_begin(&next);if(status)return status;
    *state=next;memcpy(frame,basis,48);return UC_MAINPLAYER_OK;
}
UCMainplayerStatus uc_bullet_motion_accumulate(UCBulletMotion*state,const float*v,const float*a,float delta){
    if(!state||!finite_values(v,3)||!finite_values(a,3)||!finite_values(state->velocity,3)||
       !finite_values(state->angular_delta,3)||!isfinite(delta)||!rounding_ok())return UC_MAINPLAYER_ARGUMENT;
    UCBulletMotion next=*state;for(unsigned i=0;i<3;i++){
        next.velocity[i]+=v[i]*delta;next.angular_delta[i]=(float)((double)delta*a[i]+next.angular_delta[i]);
    }
    next.flags|=0x1C001;if(!finite_values(next.velocity,3)||!finite_values(next.angular_delta,3))return UC_MAINPLAYER_UNSUPPORTED;
    *state=next;return UC_MAINPLAYER_OK;
}

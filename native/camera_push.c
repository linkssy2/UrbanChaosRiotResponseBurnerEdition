#include "camera_push.h"
#include "euler_rotation.h"
#include <math.h>
#include <string.h>
#include <fenv.h>
#include <xmmintrin.h>
static int finite_values(const float *p,unsigned n){for(unsigned i=0;i<n;i++)if(!isfinite(p[i]))return 0;return 1;}
static int rounding(void){return (_mm_getcsr()&0xE040u)==0&&fegetround()==FE_TONEAREST;}
void uc_camera_push_initialize(UCCameraPush *p){if(p){memset(p,0,sizeof(*p));for(unsigned i=0;i<4;i++)p->matrix[i*5]=1;}}
static UCCameraPushStatus add(UCCameraPush *p,const UCCameraPushInput *in,unsigned translation){
    if(!p||!in||!finite_values(in->position,3)||!isfinite(in->force)||!isfinite(in->lifetime)||
       in->lifetime<0||!isfinite(in->fade_in)||!isfinite(in->fade_out)||
       !isfinite(in->minimum_range)||!isfinite(in->maximum_range))return UC_CAMERA_PUSH_ARGUMENT;
    if(!rounding())return UC_CAMERA_PUSH_UNSUPPORTED;
    if(p->count>=UC_CAMERA_PUSH_CAPACITY)return UC_CAMERA_PUSH_FULL;
    UCCameraPushPulse v={0};v.force=in->force*(translation?10.f:.25f);v.lifetime=in->lifetime;v.translation=translation;
    v.fade_in=in->fade_in;v.fade_out=in->fade_out;memcpy(v.position,in->position,12);
    v.minimum_squared=in->minimum_range*in->minimum_range;
    v.maximum_squared=in->maximum_range*in->maximum_range;
    if(v.minimum_squared>=v.maximum_squared)v.maximum_squared=v.minimum_squared+1;
    if(!isfinite(v.minimum_squared)||!isfinite(v.maximum_squared)||v.maximum_squared<=v.minimum_squared)
        return UC_CAMERA_PUSH_SINGULAR;
    memmove(p->pulses+1,p->pulses,p->count*sizeof(*p->pulses));p->pulses[0]=v;++p->count;
    return UC_CAMERA_PUSH_OK;
}
UCCameraPushStatus uc_camera_push_add(UCCameraPush*p,const UCCameraPushInput*in){return add(p,in,0);}
UCCameraPushStatus uc_camera_push_point(UCCameraPush*p,unsigned slot,const float point[3],float force,float minimum,float maximum){
    if(!point||(slot!=0xC&&slot!=0x14))return UC_CAMERA_PUSH_ARGUMENT;
    UCCameraPushInput in={{point[0],point[1],point[2]},force,0,0,1,minimum,maximum};return add(p,&in,slot==0x14);
}
static float soften(float x,float limit){
    if(x>limit)x=(x-limit)*.2f+limit;
    if(x< -limit)x=(x+limit)*.2f-limit;
    return x;
}
static float bend(float x){
    if(x< -.628f)x=-.628f-1.0f/((1.0f-(x+.628f))*20.0f);
    if(x> .628f)x=1.0f/(((x-.628f)+1.0f)*20.0f)+.628f;
    return x;
}
static void integrate(UCCameraPush *p,float dt){
    for(unsigned i=0;i<3;i++){
        p->angular_velocity[i]-=(p->angle[i]*6.0f)*dt;
        p->velocity[i]-=(p->position[i]*6.0f)*dt;
        p->angular_velocity[i]=soften(p->angular_velocity[i],.8f);
        p->velocity[i]=soften(p->velocity[i],5);
        p->angle[i]=soften(p->angle[i]+p->angular_velocity[i],1);
        p->position[i]=soften(p->position[i]+p->velocity[i],20);
        p->angular_velocity[i]*=1.0f/(dt*20.0f+1.0f);
        p->velocity[i]*=1.0f/(dt*15.0f+1.0f);
    }
    uc_euler_rotation(bend(p->angle[0]),bend(p->angle[1]),bend(p->angle[2]),p->matrix);
    memcpy(p->matrix+12,p->position,12);
}
static int push_vector(const UCCameraPushPulse *p,const float frame[16],float out[3]){
    out[0]=out[1]=out[2]=0;if(!frame)return 1;
    float x=p->position[0]-frame[12],y=p->position[1]-frame[13],z=p->position[2]-frame[14],local[3];
    for(unsigned i=0;i<3;i++)local[i]=p->translation?((frame[i*4+1]*y+frame[i*4]*x)+frame[i*4+2]*z)+frame[i*4+3]:((frame[i*4+1]*y+frame[i*4+2]*z)+frame[i*4]*x)+frame[i*4+3];
    float distance=p->translation?(local[1]*local[1]+local[0]*local[0])+local[2]*local[2]:(local[1]*local[1]+local[2]*local[2])+local[0]*local[0];
    if(!isfinite(distance)||distance<=0)return 0;
    float falloff=distance-p->minimum_squared;if(falloff<0)falloff=0;
    falloff/=p->maximum_squared-p->minimum_squared;if(falloff>1)falloff=1;
    falloff=1-falloff;
    float scale=(float)(1.0/sqrt((double)distance)),force=p->weight*p->force;
    out[0]=(force*(scale*local[p->translation?0:2]))*falloff;
    out[2]=p->translation?(force*(scale*local[2]))*falloff:0.0f-(force*(scale*local[0]))*falloff;
    return finite_values(out,3);
}
UCCameraPushStatus uc_camera_push_update(UCCameraPush *p,float dt,const float frame[16]){
    if(!p||!isfinite(dt)||dt<=0||dt>1||p->count>UC_CAMERA_PUSH_CAPACITY||
       !finite_values(p->angle,3)||!finite_values(p->angular_velocity,3)||
       !finite_values(p->position,3)||!finite_values(p->velocity,3)||
       !finite_values(p->matrix,16)||(frame&&!finite_values(frame,16)))return UC_CAMERA_PUSH_ARGUMENT;
    if(!rounding())return UC_CAMERA_PUSH_UNSUPPORTED;
    UCCameraPush next=*p;unsigned count=0;
    for(unsigned i=0;i<p->count;i++){
        const UCCameraPushPulse *q=p->pulses+i;
        if(!isfinite(q->progress)||q->progress<0||q->progress>1||!isfinite(q->lifetime)||q->lifetime<0||
           !isfinite(q->force)||!isfinite(q->fade_in)||!isfinite(q->fade_out)||
           !finite_values(q->position,3)||!isfinite(q->minimum_squared)||!isfinite(q->maximum_squared)||
           q->minimum_squared<0||q->maximum_squared<=q->minimum_squared||q->translation>1)return UC_CAMERA_PUSH_ARGUMENT;
        if(q->progress<1)next.pulses[count++]=*q;
    }
    next.count=count;
    for(unsigned i=0;i<count;i++){
        UCCameraPushPulse *q=next.pulses+i;
        q->progress=q->lifetime==0?1:q->progress+dt/q->lifetime;if(q->progress>1)q->progress=1;
        q->weight=1;
        if(q->fade_in>q->progress&&q->fade_in>0)q->weight=q->progress/q->fade_in;
        else if(q->progress>q->fade_out&&q->fade_out<1)q->weight=(1-q->progress)/(1-q->fade_out);
        if(q->weight<0)q->weight=0;else if(q->weight>1)q->weight=1;
        float force[3];if(!push_vector(q,frame,force))return UC_CAMERA_PUSH_SINGULAR;
        for(unsigned j=0;j<3;j++){
            next.angle[j]=0.0f+next.angle[j];next.angular_velocity[j]=(q->translation?0.f:force[j])+next.angular_velocity[j];
            next.position[j]=0.0f+next.position[j];next.velocity[j]=(q->translation?force[j]:0.f)+next.velocity[j];
        }
    }
    integrate(&next,dt);
    if(!finite_values(next.angle,3)||!finite_values(next.angular_velocity,3)||!finite_values(next.position,3)||
       !finite_values(next.velocity,3)||!finite_values(next.matrix,16))return UC_CAMERA_PUSH_SINGULAR;
    *p=next;return UC_CAMERA_PUSH_OK;
}
UCCameraPushStatus uc_camera_push_tick(UCCameraPush *p,float dt,const float frame[16],float extra[16]){
    if(!p)return UC_CAMERA_PUSH_OK;
    UCCameraPushStatus status=uc_camera_push_update(p,dt,frame);
    if(!status&&extra)memcpy(extra,p->matrix,64);
    return status;
}

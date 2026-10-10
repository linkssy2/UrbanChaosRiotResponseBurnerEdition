#include "firstperson_look.h"
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int finite_values(const float *v,unsigned n) {
    for(unsigned i=0;i<n;++i)if(!isfinite(v[i]))return 0;
    return 1;
}
static int rounding_ok(void) { return (_mm_getcsr()&0xE040u)==0; }
UCMotionStatus uc_firstperson_look_reset(float angles[3],float limit) {
    if(!angles||!isfinite(limit)||limit<0)return UC_MOTION_ARGUMENT;
    if(!rounding_ok())return UC_MOTION_UNSUPPORTED;
    angles[0]=0.f;return UC_MOTION_OK;
}
/* Exact 2D3DC0 Euler order; same bounded leaf used by handgun placement. */
static void rotation(float x,float y,float z,float m[16]) {
    float sx=(float)sin(x),sy=(float)sin(y),sz=(float)sin(z);
    float cx=(float)cos(x),cy=(float)cos(y);
    double dz=cos(z);float cz=(float)dz;
    memset(m,0,16*sizeof(*m));
    m[0]=(float)(dz*cy);m[1]=(float)(-((double)cy*sz));m[2]=sy;
    m[4]=(cz*sy)*sx+cx*sz;m[5]=cz*cx-(sz*sy)*sx;
    m[6]=(float)(-((double)cy*sx));
    m[8]=sz*sx-(cz*cx)*sy;m[9]=(cx*sz)*sy+cz*sx;m[10]=cy*cx;m[15]=1;
}
UCMotionStatus uc_firstperson_look_angles(float angles[3],const float increment[3],float limit) {
    float result[3],low;
    if(!angles || !increment || !finite_values(angles,3) || !finite_values(increment,3)
        || !isfinite(limit) || limit<0)return UC_MOTION_ARGUMENT;
    if(!rounding_ok())return UC_MOTION_UNSUPPORTED;
    result[0]=increment[0]+angles[0];low=0.0f-limit;
    if(!isfinite(result[0]))return UC_MOTION_UNSUPPORTED;
    if(low>result[0])result[0]=low;
    if(result[0]>limit)result[0]=limit;
    result[1]=increment[1]+angles[1];result[2]=increment[2]+angles[2];
    if(!finite_values(result,3))return UC_MOTION_UNSUPPORTED;
    memcpy(angles,result,sizeof(result));return UC_MOTION_OK;
}
UCMotionStatus uc_firstperson_look_base(const UCFirstpersonLookBase *in,float body[16],float look[16]) {
    float angles[3],frame[16],result_body[16];
    if(!in || !body || !look || !finite_values(in->body,16) || !finite_values(in->angles,3)
        || !finite_values(in->additive_angles,3) || !finite_values(in->eye_offset,3))return UC_MOTION_ARGUMENT;
    if(!rounding_ok())return UC_MOTION_UNSUPPORTED;
    memcpy(angles,in->angles,sizeof(angles));
    if(in->player_state!=11)for(unsigned i=0;i<3;++i)angles[i]=angles[i]+in->additive_angles[i];
    if(!finite_values(angles,3))return UC_MOTION_UNSUPPORTED;
    rotation(angles[0],angles[1],angles[2],frame);
    frame[12]=frame[12]+(in->eye_offset[0]+in->body[12]);
    frame[13]=frame[13]+(in->body[13]+in->eye_offset[1]);
    frame[14]=frame[14]+(in->body[14]+in->eye_offset[2]);
    frame[15]=frame[15]+0.0f;
    if(!finite_values(frame,16))return UC_MOTION_UNSUPPORTED;
    memcpy(result_body,in->body,sizeof(result_body));memcpy(result_body,frame,12*sizeof(float));
    memcpy(body,result_body,sizeof(result_body));memcpy(look,frame,sizeof(frame));return UC_MOTION_OK;
}
UCMotionStatus uc_firstperson_look_sway(UCFirstpersonLookMotion *state,const UCFirstpersonLookInput *in,float look[16]) {
    UCFirstpersonLookMotion s;
    UCMotionStatus status;
    float base[16],turn[16],result[16],horizontal=15,vertical=-25,scale,t,amount,target,pitch;
    if(!state || !in || !look || !finite_values(in->base,16) || !isfinite(in->delta) || in->delta<0
        || !isfinite(in->bob_phase) || in->bob_phase<0 || in->bob_phase>1 || !isfinite(in->bob_amount)
        || !isfinite(in->movement) || !isfinite(in->vertical_motion))return UC_MOTION_ARGUMENT;
    if(!rounding_ok())return UC_MOTION_UNSUPPORTED;
    s=*state;memcpy(base,in->base,sizeof(base));
    scale=(1.0f-in->movement)*0.002f;
    if(in->crouched){vertical=-150;scale=scale*10;horizontal=90;}
    if(in->mirrored){horizontal=0.0f-horizontal;scale=0.0f-scale;}
    t=in->bob_phase*2;if(t>1)t=2-t;
    amount=in->bob_amount*in->bob_amount;target=(amount*t)*horizontal;
    if(!isfinite(scale) || !isfinite(amount) || !isfinite(target))return UC_MOTION_UNSUPPORTED;
    status=uc_motion_smoother_step(&s.translation[0],target,in->delta);if(status)return status;
    base[12]=(float)((double)s.translation[0].current+base[12]);
    target=(amount*t)*vertical;
    if(!isfinite(target))return UC_MOTION_UNSUPPORTED;
    status=uc_motion_smoother_step(&s.translation[1],target,in->delta);if(status)return status;
    base[13]=(float)((double)s.translation[1].current+base[13]);
    s.angle[0].damping=.5f;s.angle[1].damping=.5f;
    s.angle[0].target=0;s.angle[1].target=0;
    if(in->offset_active){s.angle[0].target=.05f;s.angle[1].target=-.2f;}
    else if(!(in->vertical_motion>0)){s.angle[0].damping=.1f;s.angle[1].damping=.1f;}
    status=uc_motion_spring_step(&s.angle[0],in->delta);if(status)return status;
    status=uc_motion_spring_step(&s.angle[1],in->delta);if(status)return status;
    pitch=(in->bob_amount*t)*scale+s.angle[0].position;
    if(!isfinite(pitch) || !finite_values(base,16))return UC_MOTION_UNSUPPORTED;
    rotation(s.angle[1].position,pitch,0,turn);
    for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)
        result[4*r+c]=(turn[4*r]*base[c]+turn[4*r+1]*base[4+c])
            +(turn[4*r+2]*base[8+c]+turn[4*r+3]*base[12+c]);
    if(!finite_values(result,16))return UC_MOTION_UNSUPPORTED;
    *state=s;memcpy(look,result,sizeof(result));return UC_MOTION_OK;
}

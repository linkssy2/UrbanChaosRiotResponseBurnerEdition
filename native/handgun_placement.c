#include "handgun_placement.h"
#include <math.h>
#include <string.h>
#include <xmmintrin.h>

static int finite_values(const float *p, unsigned n) {
    for (unsigned i=0;i<n;++i) if (!isfinite(p[i])) return 0;
    return 1;
}
/* 2D3DC0: keep its mixed single/double stores and source Euler convention. */
static void rotation(float x,float y,float z,float m[16]) {
    float sx=(float)sin(x),sy=(float)sin(y),sz=(float)sin(z);
    float cx=(float)cos(x),cy=(float)cos(y);
    double dz=cos(z); float cz=(float)dz;
    memset(m,0,16*sizeof(*m));
    m[0]=(float)(dz*cy); m[1]=(float)(-((double)cy*sz)); m[2]=sy;
    m[4]=(cz*sy)*sx+cx*sz; m[5]=cz*cx-(sz*sy)*sx;
    m[6]=(float)(-((double)cy*sx));
    m[8]=sz*sx-(cz*cx)*sy; m[9]=(cx*sz)*sy+cz*sx; m[10]=cy*cx; m[15]=1;
}
static void multiply(const float a[16],const float b[16],float out[16]) {
    for (unsigned r=0;r<4;++r) for (unsigned c=0;c<4;++c)
        out[4*r+c]=(a[4*r]*b[c]+a[4*r+1]*b[4+c])+(a[4*r+2]*b[8+c]+a[4*r+3]*b[12+c]);
}
UCMotionStatus uc_handgun_placement_step(UCHandgunPlacement *s,const UCHandgunPlacementInput *in,float out[16]) {
    static const float frequency[8]={.21f,.37f,.51f,.69f,.91f,1.17f,1.43f,1.73f};
    const float tau=6.2831854820251465f;
    UCHandgunPlacement v;
    float horizontal,vertical,scale,t,q,a,b,c,bob_roll,bob_angle,recoil;
    float x,y,z,angle[16],ambient[16],look[16],result[16],offset[4],aim_x=0,aim_y=0;
    if (!s || !in || !out) return UC_MOTION_ARGUMENT;
    if ((in->item!=0 && in->item!=1 && in->item!=2 && in->item!=11 && in->item!=17) || (in->player_state==11 && in->player_substate>=2)
        || (in->player_state==3 && in->player_substate==2)) return UC_MOTION_UNSUPPORTED;
    if (!finite_values(in->look,16) || !finite_values(in->base_offset,3) || !finite_values(in->aimed_offset,3)
        || !isfinite(in->delta) || in->delta<0 || in->delta>1 || !isfinite(in->bob_phase)
        || in->bob_phase<0 || in->bob_phase>1 || !isfinite(in->bob_amount) || !isfinite(in->movement)
        || !isfinite(in->recoil_pitch) || !isfinite(in->recoil_yaw) || !isfinite(in->angular_input)
        || (in->recoil_clip_gate && (!isfinite(in->recoil_clip_time) || !isfinite(in->recoil_clip_duration)
                                  || in->recoil_clip_duration<=0))) return UC_MOTION_ARGUMENT;
    if ((_mm_getcsr()&0xE040u)!=0) return UC_MOTION_UNSUPPORTED;
    v=*s;
    for (unsigned i=0;i<8;++i) {
        if (!isfinite(v.phase[i]) || v.phase[i]<0 || v.phase[i]>tau) return UC_MOTION_ARGUMENT;
        double phase=(double)in->delta*frequency[i]+v.phase[i];
        v.phase[i]=(float)phase;
        /* FST retains the unrounded value for the first original comparison. */
        if (phase>tau) do {v.phase[i]=v.phase[i]-tau;} while (v.phase[i]>tau);
    }
    horizontal=3.5f-in->movement*1.5f; vertical=2-in->movement*.5f; scale=.025f;
    if (in->crouched) {scale=.0375f; vertical=vertical*2; horizontal=horizontal*1.5f;}
    if (in->mirrored) {scale=0.0f-scale; horizontal=0.0f-horizontal;}
    t=(in->bob_phase*in->bob_phase)*2; if (t>1) t=2-t;
    q=1-t; a=1-q*q;
    t=in->bob_phase*2; if (t>1) t=2-t;
    q=1-t; b=1-q*q; q=1-in->bob_amount; c=1-q*q;
    t=in->bob_amount*a; bob_roll=t*.04f; bob_angle=t*scale;
    recoil=in->recoil_pitch*.075f;
    if (in->recoil_clip_gate) {
        t=(float)((double)in->recoil_clip_time/in->recoil_clip_duration-.75f);
        if (!(t>0)) t=0;
        recoil=(t*recoil)*4;
    }
    if (!isfinite(recoil)) return UC_MOTION_UNSUPPORTED;
    if (recoil>0 || in->block_recoil) recoil=0;
    recoil=0.0f-recoil;
    x=(float)(sin(v.phase[6])*(double).3f+sin(v.phase[0])*(double).2f+(double).5f);
    y=(float)((cos(v.phase[5])*sin(v.phase[1])+1)*(double).3f
              +sin(v.phase[2])*cos(v.phase[4])*(double).4f);
    z=(float)(sin(v.phase[7])*sin(v.phase[3])*sin(v.phase[4])*(double).8f+(double).5f);
    if (y<0) y=0; if (z<0) z=0.0f-z; if (z>1) z=1;
    x=((x*x)*x-.5f)*.05f; y=(y*y-.5f)*.04f; z=((z*z)*z-.5f)*.05f;
    for (unsigned i=0;i<2;++i) {
        UCMotionStatus status=uc_motion_spring_step(&v.angular[i],in->delta);
        if (status) return status;
    }
    if (uc_motion_smoother_step(&v.lowering,0,in->delta)) return UC_MOTION_UNSUPPORTED;
    if(in->item==0){memset(angle,0,sizeof(angle));angle[0]=angle[5]=angle[10]=angle[15]=1;}
    else rotation(v.angular[1].position,v.angular[0].position,0,angle);
    if (in->aiming && in->item!=0) {
        aim_x=in->aimed_offset[0]-in->base_offset[0]; aim_y=in->aimed_offset[1]-in->base_offset[1];
        x=x*.1f; y=y*.1f; z=z*.1f;
        if (in->item==2) bob_angle=bob_angle-.05340707674622536f;
    }
    x=((in->angular_input*-.75f+v.lowering.current)+x)+recoil;
    y=(v.lowering.current*4+y)+bob_angle; z=(z+in->recoil_yaw*.075f)+bob_roll;
    if (!isfinite(x) || !isfinite(y) || !isfinite(z)) return UC_MOTION_UNSUPPORTED;
    rotation(x,y,z,ambient); multiply(ambient,in->look,look);
    angle[13]=-150; multiply(angle,look,result);
    if (uc_motion_smoother_step(&v.translation[0],(c*a)*horizontal+aim_x,in->delta)
        || uc_motion_smoother_step(&v.translation[1],(c*b)*vertical+aim_y,in->delta)) return UC_MOTION_UNSUPPORTED;
    offset[0]=(float)((double)v.translation[0].current+in->base_offset[0]);
    offset[1]=(float)((double)in->base_offset[1]-v.translation[1].current);
    offset[2]=in->base_offset[2]; offset[3]=1;
    for (unsigned i=0;i<3;++i)
        result[12+i]=(offset[0]*look[i]+offset[1]*look[4+i])+(offset[2]*look[8+i]+offset[3]*look[12+i]);
    if (!finite_values(result,16)) return UC_MOTION_UNSUPPORTED;
    *s=v; memcpy(out,result,sizeof(result)); return UC_MOTION_OK;
}

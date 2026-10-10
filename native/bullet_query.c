#include "bullet_query.h"
#include <fenv.h>
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int finite3(const float *v) {
    return v&&isfinite(v[0])&&isfinite(v[1])&&isfinite(v[2]);
}
static int rounding_ok(void) {
    return !(_mm_getcsr()&0xE040u)&&fegetround()==FE_TONEAREST;
}
UCMainplayerStatus uc_bullet_prepare(const UCWeaponBulletRequest *r,
    const UCBulletPrepareInput *in,UCBulletPrepared *out) {
    UCBulletPrepared next={0};
    const float defaults[9]={1200,900,500,350,.6f,.4f,1,1.5f,20};
    double length,inverse;float half;
    if(!r||!in||!out||(r->item!=2&&r->item!=17)||!finite3(r->origin)
       ||!finite3(r->direction)||!isfinite(r->value20)||!isfinite(r->scale24)
       ||!isfinite(r->value28))return UC_MAINPLAYER_ARGUMENT;
    if(!rounding_ok())return UC_MAINPLAYER_UNSUPPORTED;
    length=((double)r->direction[0]*r->direction[0]+(double)r->direction[1]*r->direction[1])
        +(double)r->direction[2]*r->direction[2];
    if(!(length>0))return UC_MAINPLAYER_UNSUPPORTED;
    inverse=1.0/sqrt(length);
    next.item=r->item;next.definition_kind=r->definition_kind;next.team=r->team;
    next.actor=r->actor;next.owner=in->owner;next.frame=in->frame;
    next.sentinel=r->sentinel;next.group=in->group;next.flag2c=r->flag2c;
    next.query_mask=0x4000;next.impact_impulse=r->item==2?100.0f:150.0f;
    if(in->progression181)next.impact_impulse*=3.0f;
    next.mass=r->item==2?.002f:.0004f;
    next.value20=(float)((double)r->value20*r->value28);
    memcpy(next.parameters,defaults,sizeof(defaults));
    if(r->item==2)next.parameters[1]*=.5f;
    else {
        for(unsigned i=0;i<5;++i)next.parameters[i]*=1.5f;
        next.parameters[5]*=2.0f;
    }
    next.parameters[8]=30;
    half=r->scale24-1.0f;half*=.5f;half+=1.0f;
    next.parameters[2]*=half;next.parameters[3]*=half;
    next.parameters[0]=r->scale24*next.parameters[0];
    next.parameters[1]=r->scale24*next.parameters[1];
    next.parameters[4]=r->scale24*next.parameters[4];
    for(unsigned i=0;i<3;++i) {
        next.position[i]=next.previous[i]=r->origin[i];
        next.direction[i]=(float)(inverse*r->direction[i]);
        next.velocity[i]=next.direction[i]*40000.0f;
    }
    if(!isfinite(next.value20)||!finite3(next.direction)||!finite3(next.velocity))
        return UC_MAINPLAYER_UNSUPPORTED;
    for(unsigned i=0;i<9;++i)if(!isfinite(next.parameters[i]))return UC_MAINPLAYER_UNSUPPORTED;
    *out=next;return UC_MAINPLAYER_OK;
}
UCMainplayerStatus uc_bullet_query_prepare(const UCBulletPrepared *p,
    const float position[3],const float velocity[3],UCBulletQuery *out) {
    UCBulletQuery next;
    if(!p||!out||!finite3(position)||!finite3(velocity)||!isfinite(p->impact_impulse))
        return UC_MAINPLAYER_ARGUMENT;
    if(!rounding_ok())return UC_MAINPLAYER_UNSUPPORTED;
    for(unsigned i=0;i<3;++i){next.start[i]=position[i];next.end[i]=position[i]+velocity[i];}
    if(!finite3(next.end))return UC_MAINPLAYER_UNSUPPORTED;
    next.type=9;next.group=p->group;next.mask=p->query_mask;next.impact_impulse=p->impact_impulse;
    *out=next;return UC_MAINPLAYER_OK;
}

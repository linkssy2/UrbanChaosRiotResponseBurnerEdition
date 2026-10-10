#include "particle_trigger.h"
#include <math.h>
#include <fenv.h>
#include <xmmintrin.h>
static int range(float x,float lo,float hi){return isfinite(x)&&x>=lo&&x<=hi;}
static int valid(const UCParticleTrigger *s,float dt){
    if(!s||s->kind>UC_PARTICLE_FINITE||!range(dt,0,1)||fegetround()!=FE_TONEAREST||(_mm_getcsr()&0xE040u))return 0;
    if(s->kind==UC_PARTICLE_BURST)return s->burst<=1048576;
    return range(s->rate,0,65536)&&range(s->carry,0,1)&&
        (s->kind==UC_PARTICLE_CONTINUOUS||(range(s->lifetime,0,65536)&&s->lifetime>0&&range(s->remaining,-1,s->lifetime)));
}
int uc_particle_trigger_burst(UCParticleTrigger *s,uint32_t count){
    if(!s||count>1048576)return 1;
    UCParticleTrigger next={0};next.kind=UC_PARTICLE_BURST;next.burst=count;*s=next;return 0;
}
int uc_particle_trigger_constant(UCParticleTrigger *s,float rate,float life){
    if(!s||!range(rate,0,65536)||!range(life,0,65536))return 1;
    UCParticleTrigger next={0};next.kind=life==0?UC_PARTICLE_CONTINUOUS:UC_PARTICLE_FINITE;
    next.rate=rate;if(life!=0)next.remaining=next.lifetime=life;*s=next;return 0;
}
int uc_particle_trigger_query(const UCParticleTrigger *s,float dt,UCParticleTriggerInfo *out){
    if(!out||!valid(s,dt))return 1;
    UCParticleTriggerInfo next={0};
    if(s->kind==UC_PARTICLE_BURST){next.estimate=s->burst;next.finished=s->burst==0;next.transient=1;}
    else {
        float span=s->kind==UC_PARTICLE_FINITE&&s->lifetime<dt?s->lifetime:dt;
        next.estimate=(uint32_t)ceil((double)span*s->rate);
        next.finished=s->kind==UC_PARTICLE_FINITE&&s->remaining<=0;
        next.transient=s->kind==UC_PARTICLE_FINITE&&s->lifetime<2;
    }
    *out=next;return 0;
}
int uc_particle_trigger_emit(UCParticleTrigger *s,float dt,uint32_t *births){
    if(!births||!valid(s,dt))return 1;
    if(s->kind==UC_PARTICLE_BURST){*births=s->burst;s->burst=0;return 0;}
    if(s->kind==UC_PARTICLE_FINITE&&s->remaining<=0){*births=0;return 0;}
    float span=s->kind==UC_PARTICLE_FINITE&&s->remaining<=dt?s->remaining:dt;
    double integer;const double fraction=modf((double)span*s->rate+s->carry,&integer);
    s->carry=(float)fraction;if(s->kind==UC_PARTICLE_FINITE)s->remaining-=dt;
    *births=(uint32_t)integer;return 0;
}

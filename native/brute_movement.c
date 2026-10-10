#include "brute_movement.h"
#include <math.h>
int uc_brute_movement_blend(UCBruteMovementBlend*s,int32_t mode,float dt){
    if(!s||mode<0||mode>2||!isfinite(dt)||dt<=0||!isfinite(s->current)||!isfinite(s->velocity))return 1;
    float target=(float)mode*.5f,factor=2.f*dt;
    factor=1.f/(factor+1.f);float error=target-s->current;
    float speed=((1.f-factor)*error)/dt,change=speed-s->velocity;
    if(fabs((double)speed)>fabs((double)s->velocity)){if(change>=2.f)change=2.f;if(change<=-2.f)change=-2.f;}
    speed=change+s->velocity;float value=dt*speed;value+=s->current;
    if(fabs((double)value-target)<.0001f){value=target;speed=0;}
    if(!isfinite(value)||!isfinite(speed))return 1;
    s->current=value;s->velocity=speed;return 0;
}
int uc_brute_movement_slider(float value,uint32_t*first,float*weight){
    if(!first||!weight||!isfinite(value))return 1;
    uint32_t index=value>.5f?1:0;float offset=(float)index*.5f;
    float fraction=(value-offset)/.5f,w=1.f-fraction;
    *first=index;*weight=w>=1?1:w<=0?0:w;return 0;
}

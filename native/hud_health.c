#include "hud_health.h"
#include <math.h>
#include <string.h>

float uc_hud_health_maximum(uint32_t mode,uint32_t tier) {
    if(mode)return 500;
    switch(tier){case 1:return 1200;case 2:return 1400;case 3:return 1600;default:return 1000;}
}
int uc_hud_health_initialize(UCHudHealth *out,uint32_t mode,uint32_t tier,float scale) {
    UCHudHealth s={0};
    if(!out||!isfinite(scale)||scale<=0||scale>16)return 0;
    s.delayed_fraction=s.fraction=1;
    s.extent=(float)((double)uc_hud_health_maximum(mode,tier)*(double)scale*(double).1f+20.0);
    *out=s;return 1;
}
int uc_hud_health_step(UCHudHealth *state,float health,uint32_t mode,uint32_t tier,float delta) {
    static const float low[4]={236,30,8,255},high[4]={157,219,64,255},pulse[4]={1,1,0,1};
    const double tau=6.2831854820251465;const float maximum=uc_hud_health_maximum(mode,tier);
    UCHudHealth s;float fraction,t;unsigned i;
    if(!state||!isfinite(health)||health<0||health>4*maximum||!isfinite(delta)||delta<0||delta>1)return 0;
    if(!isfinite(state->fraction)||state->fraction<0||state->fraction>4||!isfinite(state->delayed_fraction)||state->delayed_fraction<0||state->delayed_fraction>4||
       !isfinite(state->delay)||state->delay<0||state->delay>1||!isfinite(state->phase)||state->phase<0||state->phase>tau||
       !isfinite(state->extent)||state->extent<=0)return 0;
    for(i=0;i<4;++i)if(!isfinite(state->color[i]))return 0;
    s=*state;fraction=(float)((double)health/(double)maximum);
    s.delay-=delta;if(s.delay<0)s.delay=0;
    if(s.fraction>s.delayed_fraction)s.delayed_fraction=s.fraction;
    if(fraction!=s.fraction){if(s.delay==0)s.delayed_fraction=s.fraction;s.delay=1;}
    s.fraction=fraction;t=(fraction-.1f)*1.1111111640930176f;
    if(t>=1)t=1;else if(t<=0)t=0;
    for(i=0;i<4;++i){const float a=low[i]/255.f,b=high[i]/255.f;s.color[i]=a+(b-a)*t;}
    if(fraction<=.3f) {
        const double intensity=(double)fraction*(double)3.3333332538604736f;
        const float stored=(float)intensity;
        const double phase=fmod(((intensity+.25)*(double)delta*20.0)+(double)s.phase,tau);
        const double weight=((cos(phase)+1.0)*.5)*(((double)stored+1.0)*.5);
        s.phase=(float)phase;
        for(i=0;i<4;++i)s.color[i]=(float)(((double)pulse[i]-(double)s.color[i])*weight+(double)s.color[i]);
    }
    *state=s;return 1;
}

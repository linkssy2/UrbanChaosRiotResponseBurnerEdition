#include "hud_heartbeat.h"
#include <math.h>
#include <string.h>
static float clamp(float x){return x>=1?1:x<=0?0:x;}
static void color(float fraction,float out[4]){
    const float lo[4]={236,30,8,255},hi[4]={157,219,64,255};
    const float t=clamp((fraction-.1f)*1.1111111640930176f);
    for(unsigned i=0;i<4;i++){const float a=lo[i]/255.f,b=hi[i]/255.f;out[i]=a+(b-a)*t;}
}
int uc_hud_heartbeat_initialize(UCHudHeartbeat *out){
    UCHudHeartbeat h={0};if(!out)return 0;
    h.damage_color[0]=h.damage_color[1]=h.damage_color[2]=1;
    for(unsigned i=0;i<4;i++)h.alert_color[i]=1;
    h.pulse=999;*out=h;return 1;
}
int uc_hud_heartbeat_step(UCHudHeartbeat *s,const UCHudHeartbeatActor *a,float dt,
    UCGameRandom *rng,void (*sound)(void *),void *ctx){
    unsigned first,last;float fraction,value=0;
    if(!s||!a||!rng||!sound||rng->first>=250||rng->second>=250||!isfinite(dt)||dt<0||dt>1)return 0;
    /*71960 resolves the actor after history fading; absent actors never read health.*/
    if(a->present&&(!isfinite(a->health)||a->health<0||!isfinite(a->maximum)||a->maximum<=0||a->maximum>65536||a->health>4*a->maximum))return 0;
    if(!isfinite(s->previous_health)||s->previous_health<0||s->previous_health>262144||
       !isfinite(s->damage)||s->damage<0||s->damage>1||!isfinite(s->alert)||s->alert<0||s->alert>1||
       !isfinite(s->sweep)||s->sweep<0||s->sweep>=1||!isfinite(s->pulse)||s->pulse<0||s->pulse>999||
       !isfinite(s->elapsed)||s->elapsed<0||s->elapsed>0x1p24f||s->alarms<0||s->alarms>3)return 0;
    for(unsigned i=0;i<4;i++)if(!isfinite(s->damage_color[i])||!isfinite(s->alert_color[i]))return 0;
    for(unsigned i=0;i<53;i++){
        if(!isfinite(s->samples[i]))return 0;
        for(unsigned j=0;j<4;j++)if(!isfinite(s->colors[i][j]))return 0;
    }
    s->elapsed+=dt;
    for(unsigned i=0;i<53;i++){s->colors[i][3]-=dt;if(s->colors[i][3]<0)s->colors[i][3]=0;}
    if(!a->present)return 1;
    if(s->previous_health>a->health){s->damage=1;s->alert=1;}
    float threshold=a->maximum*.6666666865348816f;
    if(a->health<=threshold&&s->previous_health>threshold)s->alarms=3;
    threshold=a->maximum*.3333333432674408f;
    if(a->health<=threshold&&s->previous_health>threshold)s->alarms=3;
    s->previous_health=a->health;s->damage=clamp(s->damage-dt*2);
    s->damage_color[0]=s->damage_color[1]=s->damage_color[2]=1;s->damage_color[3]=s->damage;
    s->alert=clamp(s->alert-dt);
    s->alert_color[0]=1;s->alert_color[1]=s->alert_color[2]=1+(0-1)*s->alert;s->alert_color[3]=1;
    fraction=a->health/a->maximum;
    const float prior=s->sweep;s->sweep+=dt*.5f;if(s->sweep>=1)s->sweep=0;
    first=(unsigned)((double)prior*53);last=(unsigned)((double)s->sweep*53);
    if(a->health>0){
        const float range=fraction*.15000000596046448f+.05000000074505806f;
        int32_t word;uc_game_random_next(rng,&word);
        const double random=((double)word*((double)range-(-range)))*0x1p-31+(-range);
        s->pulse+=dt;
        const float period=(float)(random+((double)fraction*.699999988079071+(double).30000001192092896f));
        if(s->pulse>=period){s->pulse=0;if(s->alarms){sound(ctx);if(s->alarms>0)--s->alarms;}}
        if(s->pulse<.30000001192092896f){const double t=(double)s->pulse*3.3333332538604736;
            value=(float)(-sin(t*6.2831854820251465)*cos(t*1.5707963705062866)*14.0);}
    }
    for(unsigned i=first;i!=last;i=(i+1)%53){
        s->samples[i]=value;
        if(a->external_color)for(unsigned j=0;j<4;j++)s->colors[i][j]=1;
        else color(fraction,s->colors[i]);
    }
    return 1;
}

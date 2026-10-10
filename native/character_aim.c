#include "character_aim.h"
#include <math.h>
#include <string.h>
static float wrapped(float a){
    const double pi=3.1415927410125732,tau=6.2831854820251465;
    return a>0?(float)(fmod((double)a+pi,tau)-pi):(float)-(fmod(pi-(double)a,tau)-pi);
}
int uc_character_aim_solve(const UCCharacterAim*s,const float*w,const float*h,float heading,UCCharacterAimAngles out[2]){
    if(!s||!w||!h||!out||!isfinite(heading))return 1;
    for(unsigned i=0;i<16;i++)if(!isfinite(w[i])||!isfinite(h[i]))return 1;
    UCCharacterAimAngles result[2]={{0}};
    for(unsigned side=0;side<2;side++){
        if(!(side?s->look_enabled:s->enabled))continue;
        const float*p=side?s->look_point:s->point;float d[3];
        for(unsigned i=0;i<3;i++){
            if(!isfinite(p[i]))return 1;
            float x=w[i]*0.f,y=w[4+i]*h[13],z=w[8+i]*h[14];
            float origin=(x+y)+(z+w[12+i]);d[i]=p[i]-origin;
        }
        float x2=d[0]*d[0],z2=d[2]*d[2];double horizontal=sqrt((double)x2+z2);
        if(!isfinite(horizontal))return 1;if(horizontal<=10)continue;
        float yaw=wrapped((float)-atan2((double)d[0],(double)d[2]));
        float pitch=wrapped((float)(1.5707963705062866-atan2((double)(float)horizontal,(double)d[1])));
        result[side].enabled=1;result[side].yaw=wrapped(yaw-heading);
        if(side||!(s->mode&1))result[side].pitch=pitch;
    }
    memcpy(out,result,sizeof result);return 0;
}
int uc_character_aim_layer(const UCCharacterAim*s,const UCCharacterAimAngles a[2],const float*w,UCCharacterAimRequest*out,uint32_t*count,float*used){
    if(!s||!a||!w||!out||!count||!used)return 1;
    for(unsigned i=0;i<5;i++)if(!isfinite(w[i]))return 1;
    float total[5]={0};uint32_t n=0;
    for(unsigned side=0;side<2;side++)if(!side||a[side].enabled){
        unsigned channel=1-side,lane=channel+3;float remaining=w[lane]-total[lane];
        if(!isfinite(a[side].yaw)||!isfinite(a[side].pitch))return 1;
        if(remaining>0)out[n++]=(UCCharacterAimRequest){channel,remaining,a[side].yaw,a[side].pitch};
        total[lane]+=remaining;
    }
    *count=n;memcpy(used,total,sizeof total);return 0;
}

int uc_character_turn_plan(float yaw,uint8_t narrow,uint32_t*action,float*scale){
    if(!action||!scale||!isfinite(yaw))return 1;
    float threshold=(narrow?.10000000149011612f:.30000001192092896f)*3.1415927410125732f;
    uint32_t selected=0;float degrees=0;
    if(yaw>2.356194496154785f){selected=1;degrees=180;}
    else if(yaw>threshold){selected=2;degrees=90;}
    else if(yaw< -2.356194496154785f){selected=3;degrees=-180;}
    else if(yaw<0.f-threshold){selected=4;degrees=-90;}
    float factor=0;if(selected){float radians=degrees*.0027777778450399637f;radians*=6.2831854820251465f;factor=yaw/radians;if(!isfinite(factor))return 1;}
    *action=selected;*scale=factor;return 0;
}
int uc_character_action_clip(UCCharacterClipLoop*s,uint8_t root,float frequency,float fade,float scale,float delta,
    const float*w,uint8_t hold,float*used,UCCharacterClipRequest*request,uint8_t*emitted,uint8_t*finished){
    if(!s||!w||!used||!request||!emitted||!finished||!isfinite(s->phase)||s->phase<0||s->phase>1||
       !isfinite(frequency)||frequency<=0||!isfinite(fade)||fade<0||!isfinite(delta)||delta<=0||!isfinite(scale))return 1;
    double unbounded=(double)frequency*delta+s->phase;
    float phase=unbounded>=1?1:(float)unbounded;if(phase<=0)phase=0;
    UCCharacterClipLoop next=*s;next.phase=phase;float out[5]={0};UCCharacterClipRequest node={0};uint8_t emit=0;
    if((hold||phase<1)&&uc_character_clip_request(&next.leaf_phase,root,frequency,delta,phase,w,scale,out,&node,&emit))return 1;
    for(unsigned i=0;i<5;i++)if(!isfinite(w[i])||w[i]<0||w[i]>1)return 1;
    *finished=phase>=1.-(double)frequency*fade;*s=next;memcpy(used,out,sizeof out);if(emit)*request=node;*emitted=emit;return 0;
}
int uc_character_turn_clip(UCCharacterClipLoop*s,uint8_t root,float frequency,float fade,float scale,float delta,
    const float*w,float*used,UCCharacterClipRequest*request,uint8_t*emitted,uint8_t*finished){
    return uc_character_action_clip(s,root,frequency,fade,scale,delta,w,0,used,request,emitted,finished);
}


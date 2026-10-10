#include "player_blast.h"
#include <math.h>
#include <string.h>
int uc_player_blast_receive(UCPlayerConcussion*s,const float*p,int32_t sender,const UCExplosionBlast*b,
 const void*network,uint8_t multiplayer,const UCPlayerBlastServices*h){
 if(!s||!p||!b||!h)return 1;
 if(!b->blast&&b->type!=2)return 0;
 if(network||multiplayer||!h->target||!h->obstruction||!h->pain||!h->damage||!h->sound||!h->parameter||!h->bus_effect||!h->delta)return 1;
 uint8_t eligible;float amount;
 if(uc_explosion_damage_eligible(b->point,b->radius,b->type,b->ignore_collision,p,80.f,140.f,h->target,-1,h->obstruction,h->owner,&eligible)||
    uc_explosion_damage_amount(b->point,b->radius,b->damage,b->type,b->blast,p,80.f,140.f,&amount))return 1;
 float v[3];for(unsigned i=0;i<3;i++)v[i]=p[i]-b->point[i];
 double distance=sqrt(((double)v[2]*v[2]+(double)v[1]*v[1])+(double)v[0]*v[0]);
 if(eligible){
  if(b->type==2&&h->pain(h->owner))return 2;
  if(!isfinite(distance)||distance<=0)return 2;float inverse=(float)(1./distance);
  UCPlayerFireDamage d={0};d.source=-1;d.kind=5;d.zero=b->team;for(unsigned i=0;i<3;i++)d.direction[i]=v[i]*inverse;
  d.owner=b->owner;memcpy(d.center,b->point,12);d.other=sender;d.session=d.unique=-1;
  return h->damage(h->owner,amount,&d)?2:0;
 }
 if(!(b->radius>0))return 0;
 double ratio=distance/b->radius;if(!(1.75>ratio))return 0;float fraction=(float)ratio;
 s->seconds=2.f;s->strength=10.f-fraction*2.f;
 double dt=h->delta(h->owner);if(!isfinite(dt)||dt<0)return 2;s->strength_step=(float)(dt*((double)s->strength/s->seconds));
 s->attenuation=fraction*.5f;if(!(1.f>s->attenuation))s->attenuation=1.f;else if(!(s->attenuation>.2f))s->attenuation=.2f;
 dt=h->delta(h->owner);if(!isfinite(dt)||dt<0)return 2;s->attenuation_step=(float)(dt*((double)s->attenuation/((double)s->seconds*.5f)));
 if(h->sound(h->owner,"Tinnitus")||h->parameter(h->owner,"Tinnitus","Distance",fraction*.6666666865348816f))return 2;
 static const char*names[]={"Game3D","Game 3D","Speaker","Music"};
 for(unsigned i=0;i<4;i++)if(h->bus_effect(h->owner,names[i],2000.f,.5f,2.f,4.f))return 2;
 s->active=1;return 0;
}

static float approach(float target,float value,float step){if(value>target){value-=step;return value>target?value:target;}value+=step;return value>target?target:value;}
int uc_player_concussion_step(UCPlayerConcussion*s,const UCPlayerConcussionServices*h){
 if(!s||!h||!h->shared_attenuation||!h->feedback||!h->group_pitch||!h->unscaled_delta||!h->clock)return 1;
 if(!s->active||s->active>3)return 0;
 const char*names[]={"Game3D","Game 3D","Speaker","Music"};float pitch;
 if(s->active==3){
  if(*h->shared_attenuation!=1.f){*h->shared_attenuation=approach(1,*h->shared_attenuation,s->attenuation_step);pitch=(float)(1.-(1.-*h->shared_attenuation)*.4f);}
  else{s->active=0;*h->feedback=0;pitch=1;}
  for(unsigned i=0;i<4;i++)if(h->group_pitch(h->owner,names[i],pitch))return 2;
  if(!s->active)h->clock(h->owner);return 0;
 }
 if(s->active==1&&s->attenuation>=*h->shared_attenuation){s->active=2;s->attenuation=1;}
 s->strength=s->strength>0?s->strength-s->strength_step:0.f;*h->feedback=s->strength;
 *h->shared_attenuation=approach(s->attenuation,*h->shared_attenuation,s->attenuation_step);
 pitch=(float)(1.-(1.-*h->shared_attenuation)*.4f);
 for(unsigned i=0;i<4;i++)if(h->group_pitch(h->owner,names[i],pitch))return 2;
 if(s->seconds>0)s->seconds=(float)((double)s->seconds-h->unscaled_delta(h->owner));else s->active=3;return 0;
}

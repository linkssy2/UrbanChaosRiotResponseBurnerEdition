#include "npc_flight.h"
#include <fenv.h>
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int finite_vector(const float*v,unsigned n){if(!v)return 0;for(unsigned i=0;i<n;i++)if(!isfinite(v[i]))return 0;return 1;}
static int environment(void){return !(_mm_getcsr()&0xE040u)&&fegetround()==FE_TONEAREST;}
int uc_npc_flight_intercept(const UCBulletHistory*h,int32_t id,const float*p,const uint32_t*player,float radius,unsigned only,float*out){
 if(!h||h->count>200||!isfinite(radius)||!environment())return -1;if(!p)return 0;
 if(!finite_vector(p,3)||only>1)return -1;
 for(unsigned n=0;n<h->count;n++){
  const UCBulletHistoryEntry*e=h->entries+n;if(e->owner_id==(uint32_t)id||e->bullet_id==(uint32_t)id)continue;
  if(only&&(!player||e->owner_id!=*player))continue;
  if(!finite_vector(e->from,3)||!finite_vector(e->to,3))return -1;
  float d[3],v[3],at[3];for(unsigned i=0;i<3;i++){d[i]=e->to[i]-e->from[i];v[i]=p[i]-e->from[i];}
  float t=((d[2]*v[2]+d[1]*v[1])+d[0]*v[0])/((d[2]*d[2]+d[1]*d[1])+d[0]*d[0]);
  if(t<0)memcpy(at,e->from,12);else if(t>1)memcpy(at,e->to,12);else for(unsigned i=0;i<3;i++)at[i]=d[i]*t+e->from[i];
  double x=(double)at[0]-p[0],y=(double)at[1]-p[1],z=(double)at[2]-p[2];
  if((double)radius>sqrt((z*z+y*y)+x*x)){if(out)memcpy(out,d,12);return 1;}
 }
 return 0;
}
int uc_npc_flight_late(UCNpcThrownWeapon*s,UCBulletState*b,const UCEntityFreshView*v,const UCNpcFlightServices*h){
 if(!s||!b||!v||!v->transform||!v->actor||!v->target.flags||!v->id||!h||!h->delta||!h->time||
    !isfinite(*h->delta)||!isfinite(*h->time)||!finite_vector(v->transform,16)||!environment())return 1;
 if(s->launched1F8&&!s->settled202&&!s->exploded200){
  if(!h->displacement||!finite_vector(s->velocity21C,3)||!finite_vector(s->angular228,3)||!isfinite(s->delay1FC))return 2;
  if(s->delay1FC>0)s->delay1FC=(float)((double)s->delay1FC-*h->delta);
  s->velocity21C[1]=(float)((double)*h->delta*-980.f+s->velocity21C[1]);
  if(uc_bullet_motion_accumulate(&b->motion,s->velocity21C,s->angular228,*h->delta))return 2;
  if(s->weapon1EC==8)s->angular228[1]=s->angular228[2]=0;
  if(s->weapon1EC==33||s->weapon1EC==31){float to[3];for(unsigned i=0;i<3;i++)to[i]=v->transform[12+i]+s->velocity21C[i];
   if(uc_bullet_aim(v->transform,b->motion.quaternion,to))return 2;}
  if(!(v->actor->word90&1)){float correction[3];if(h->displacement(h->owner,v,b,correction)||!finite_vector(correction,3))return 2;
   for(unsigned i=0;i<3;i++)v->transform[12+i]+=correction[i];v->transform[15]=1;}
  if(uc_bullet_motion_finish(&b->motion,v->transform+12,*h->delta))return 2;
 }
 if(!s->hidden203&&s->settled202&&!s->armed1F0){
  float tint[4];if(!h->tint||h->tint(h->owner,tint,0))return 2;tint[3]-=*h->delta*1.5f;
  if(tint[3]<=.1f)s->hidden203=1;else if(h->tint(h->owner,tint,1))return 2;
 }
 if(s->dropped210&&!s->exploded200){const float basis[12]={1,-0.f,0,0,0,1,-0.f,0,0,0,1,0};memcpy(v->transform,basis,48);}
 if(s->effect248){
  float m[16],camera[16];memcpy(m,v->transform,64);if(!h->camera||!h->light||h->camera(h->owner,camera)||!finite_vector(camera,16))return 2;
  float amount=(3.f-(*h->time-s->light_time24C))*.3333333432674408f;amount=amount>0?amount*150.f:0;
  __m128 direction=_mm_sub_ps(_mm_loadu_ps(camera+12),_mm_loadu_ps(m+12)),square=_mm_mul_ps(direction,direction);
  square=_mm_add_ps(square,_mm_movehl_ps(square,square));float n=_mm_cvtss_f32(_mm_add_ss(square,_mm_shuffle_ps(square,square,1)));
  if(!(n>0)||!isfinite(n))return 2;float r=_mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(n)));r=(.5f*r)*(3.f-(r*n)*r);
  direction=_mm_mul_ps(direction,_mm_set1_ps(r));_mm_storeu_ps(m+12,_mm_add_ps(_mm_loadu_ps(m+12),_mm_mul_ps(direction,_mm_set1_ps(amount))));
  if(h->light(h->owner,s->effect248,m,amount+100.f))return 2;
 }
 unsigned multiplayer;if(!h->multiplayer||h->multiplayer(h->owner,&multiplayer)||multiplayer>1)return 2;
 if(!multiplayer&&(s->weapon1EC==11||s->weapon1EC==8)&&s->launched1F8&&!s->settled202&&!s->exploded200){
  float d[3];unsigned hit;if(!h->intercept||h->intercept(h->owner,*v->id,50.f,1,d,&hit)||hit>1)return 2;
  if(hit){s->hit201=1;if(s->weapon1EC==11){s->exploded200=1;*v->target.flags|=0x100;}
   else{if(!h->random||!h->deflect)return 2;double length=sqrt(((double)d[2]*d[2]+(double)d[1]*d[1])+(double)d[0]*d[0]);if(!(length>0))return 2;
    double inverse=1./length;for(unsigned i=0;i<3;i++)d[i]=(float)(inverse*d[i]);
    d[0]=(float)(h->random(h->owner,-1,1)+d[0]);d[1]=(float)(h->random(h->owner,0,.5f)+d[1]);d[2]=(float)(h->random(h->owner,-1,1)+d[2]);
    double speed=h->random(h->owner,800,1000);for(unsigned i=0;i<3;i++)d[i]=(float)((double)d[i]*speed);
    float angular[3];for(unsigned i=0;i<3;i++)angular[i]=(float)h->random(h->owner,-15,15);
    if(h->deflect(h->owner,v,s,d,angular))return 2;
   }
  }
 }
 return 0;
}

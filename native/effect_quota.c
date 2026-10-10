#include "effect_quota.h"
#include <fenv.h>
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int mode(void){return !(_mm_getcsr()&0xE040u)&&fegetround()==FE_TONEAREST;}
static int point(const float*p){if(!p)return 0;for(unsigned i=0;i<3;i++)if(!isfinite(p[i])||fabsf(p[i])>1e6f)return 0;return 1;}
int uc_effect_quota_init(UCEffectQuota*s){if(!s)return 1;s->count=0;return 0;}
int uc_effect_quota_add(UCEffectQuota*s,const float*p,float radius,float duration,uint8_t*added){
 if(!s||!added||s->count>64||!point(p)||!isfinite(radius)||fabsf(radius)>1e6f||!isfinite(duration)||!mode())return 1;
 if(s->count==64){*added=0;return 0;}
 UCEffectQuotaEntry*e=s->entries+s->count;memcpy(e->point,p,12);e->radius_squared=radius*radius;e->remaining=duration;s->count++;*added=1;return 0;
}
int uc_effect_quota_test(const UCEffectQuota*s,const float*p,uint8_t*blocked){
 if(!s||!blocked||s->count>64||!point(p)||!mode())return 1;
 for(unsigned i=0;i<s->count;i++)if(!point(s->entries[i].point)||!isfinite(s->entries[i].radius_squared)||s->entries[i].radius_squared<0)return 1;
 for(unsigned i=0;i<s->count;i++){
  const UCEffectQuotaEntry*e=s->entries+i;float x=p[0]-e->point[0],z=p[2]-e->point[2],y=p[1]-e->point[1];
  float squared=x*x+z*z;squared=squared+y*y;
  if(e->radius_squared>squared){*blocked=1;return 0;}
 }*blocked=0;return 0;
}
int uc_effect_quota_tick(UCEffectQuota*s,float delta){
 if(!s||s->count>64||!isfinite(delta)||!mode())return 1;
 for(unsigned i=0;i<s->count;i++)if(!isfinite(s->entries[i].remaining)||!isfinite(s->entries[i].remaining-delta))return 1;
 for(unsigned i=0;i<s->count;){
  s->entries[i].remaining-=delta;
  if(s->entries[i].remaining>0){i++;continue;}
  if(s->count>1)s->entries[i]=s->entries[s->count-1];
  s->count--;
 }return 0;
}

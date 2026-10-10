#include "npc_thrown_effects.h"
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
int uc_npc_thrown_light_position(const float*w,const float*c,float*out){
 if(!w||!c||!out)return 1;float v[4];for(unsigned i=0;i<4;i++)v[i]=c[i]-w[12+i];
 float sum=(v[0]*v[0]+v[2]*v[2])+(v[1]*v[1]+v[3]*v[3]);
 if(!(sum>0)||!isfinite(sum))return 1;
 float r=_mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(sum)));float q=3.f-(r*sum)*r;float n=(.5f*r)*q;
 for(unsigned i=0;i<3;i++)out[i]=w[12+i]+(v[i]*n)*150.f;return 0;
}
static int admitted(const UCNpcThrownWeapon*s){return s&&(s->weapon1EC==11||s->weapon1EC==12);}
int uc_npc_thrown_ignition_effects(UCNpcThrownWeapon*s,int32_t id,const float*w,const UCNpcThrownEffectsServices*h){
 if(!admitted(s)||!w||!h)return 1;const char*assets[3]={0,0,0};
 if(s->template244){if(!h->asset)return 2;static const char*names[]={"particle sys 1","particle sys 2","particle sys 3"};
  for(unsigned i=0;i<3;i++){unsigned found;if(h->asset(h->owner,s->template244,names[i],assets+i,&found))return 2;}}
 if(!h->particles)return 2;
 if(s->weapon1EC==11){for(unsigned i=0;i<2;i++)if(h->particles(h->owner,assets[i],id,1))return 2;
  if(s->effect248&&(!h->light_destroy||h->light_destroy(h->owner,s->effect248)))return 2;
  float camera[4],point[3];if(!h->time||h->time(h->owner,&s->light_time24C)||!h->camera_position||h->camera_position(h->owner,camera)||uc_npc_thrown_light_position(w,camera,point)||!h->light_create||h->light_create(h->owner,point,&s->effect248))return 2;
 }else for(unsigned i=0;i<2;i++)if(h->particles(h->owner,assets[i],id,1))return 2;
 return !h->sound||h->sound(h->owner,"Hiss",0)?2:0;
}
int uc_npc_thrown_release_effects(UCNpcThrownWeapon*s,int32_t id,const UCNpcThrownEffectsServices*h){
 if(!admitted(s)||!h)return 1;if(!h->random)return 2;
 for(unsigned i=0;i<3;i++){double value;if(h->random(h->owner,-500,500,&value))return 2;s->angular228[i]=(float)value;}
 if(s->template244&&s->weapon1EC==11){const float*v=s->velocity21C;float n=(v[0]*v[0]+v[1]*v[1])+v[2]*v[2];
  if(n>.1f){const char*asset=0;unsigned found;if(!h->asset||h->asset(h->owner,s->template244,"particle sys 3",&asset,&found))return 2;
   if(found&&(!h->particles||h->particles(h->owner,asset,id,1)))return 2;}}
 return !h->sound||h->sound(h->owner,"Thrown",0)?2:0;
}
int uc_npc_thrown_explosion_effects(UCNpcThrownWeapon*s,const UCBulletState*b,const UCNpcThrownEffectsServices*h){
 if(!admitted(s)||!b||!h)return 1;if(!h->flags||h->flags(h->owner,0x100)||!h->sound||h->sound(h->owner,"Hiss",1)||h->sound(h->owner,"Explode",0))return 2;
 s->flag219=0;
 if(s->weapon1EC==11){UCNpcMolotovExplosion p={0};const float*n=s->vector258,*v=s->velocity21C;
  p.surface=(uint8_t)(s->settled202&&((n[2]*n[2]+n[0]*n[0])+n[1]*n[1])>0);p.hit=s->hit201;
  memcpy(p.point,s->vector264,12);memcpy(p.normal,n,12);float sum=(v[2]*v[2]+v[1]*v[1])+v[0]*v[0];
  if(sum>0.00000011920928955078125f){double inverse=1./sqrt((double)sum);for(unsigned i=0;i<3;i++)p.direction[i]=(float)(v[i]*inverse);}else p.direction[0]=1;
  p.airborne=(uint8_t)!s->settled202;p.owner=b->actor;p.team=b->team;p.group1=s->group280;p.group2=s->group284;
  if(!s->template244||!h->molotov_event||h->molotov_event(h->owner,s->template244,&p))return 2;
 }else if(!s->template244||!h->pipebomb_event||h->pipebomb_event(h->owner,s->template244,b->actor))return 2;
 if(s->effect248){if(!h->light_destroy||h->light_destroy(h->owner,s->effect248))return 2;s->effect248=0;}
 return !h->physics_close||h->physics_close(h->owner)?2:0;
}
int uc_npc_thrown_molotov_impact(UCNpcThrownWeapon*s,int32_t id,unsigned has_actor,int32_t target,const float*point,const float*normal,const UCNpcThrownEffectsServices*h){
 if(!s||s->weapon1EC!=11||!point||!normal||!h||has_actor>1)return 1;
 memcpy(s->vector264,point,12);memcpy(s->vector258,normal,12);
 if(!s->exploded200&&has_actor){if(!h->hit_actor||h->hit_actor(h->owner,target,id))return 2;s->exploded200=1;return !h->flags||h->flags(h->owner,0x100)?2:0;}
 if(s->delay1FC>0||s->settled202)return 0;
 if(!h->sound||h->sound(h->owner,"Thrown",1)||!h->flags||h->flags(h->owner,0x20))return 2;
 s->settled202=1;if(!s->armed1F0)s->hidden203=1;s->fuse1F4=0;return 0;
}

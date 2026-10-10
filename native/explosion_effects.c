#include "explosion_effects.h"
#include <math.h>
#include <string.h>
const char *uc_explosion_particle_asset(int32_t type,unsigned index){
 static const char *const names[2][4]={
  {"generic_fx\\StdExplosion01.simpleparticlesystem","generic_fx\\StdExplosion02.simpleparticlesystem",
   "generic_fx\\StdExplosion03.simpleparticlesystem","generic_fx\\StdExplosion04.simpleparticlesystem"},
  {"generic_fx\\GasExplosion01.simpleparticlesystem","generic_fx\\GasExplosion02.simpleparticlesystem",
   "generic_fx\\GasExplosion03.simpleparticlesystem","generic_fx\\GasExplosion04.simpleparticlesystem"}};
 return index<4?names[type==2][index]:0;
}
int uc_explosion_effect_particles(const uint32_t *id,const int32_t *type,const UCExplosionEffectServices *s){
 float world[16],frame[16]={0,1,0,0,1,0,0,0,0,0,1,0};unsigned i;int32_t selected;
 UCExplosionVisualChild c={0};
 if(!id||!type||!s||!s->world||!s->particle)return 1;
 if(s->world(s->owner,world))return 2;
 for(i=0;i<16;i++)if(!isfinite(world[i]))return 2;
 memcpy(frame+12,world+12,16);selected=*type;
 for(i=0;i<4;i++)if(s->particle(s->owner,uc_explosion_particle_asset(selected,i),frame))return 2;
 if(*type==1)return 0;
 if(!s->child)return 2;
 c.kind=UC_EXPLOSION_WARP_RING;c.parent_id=*id;c.mode=1;
 c.start_size=100;c.end_size=5000;c.lifetime=1;c.strength=1;
 if(s->child(s->owner,&c))return 2;
 memset(&c,0,sizeof c);c.kind=UC_EXPLOSION_CAMERA_PUSH;c.parent_id=*id;c.mode=1;
 c.force=.6f;c.fade_out=1;c.minimum_range=1000;c.maximum_range=3000;
 if(s->child(s->owner,&c))return 2;
 c.kind=UC_EXPLOSION_CAMERA_KICK;c.parent_id=*id;c.force=.3f;
 return s->child(s->owner,&c)?2:0;
}
int uc_explosion_effect_flash(const uint32_t *id,const int32_t *type,const float *scale,const UCExplosionEffectServices *s){
 UCShotLightFlashRequest r={0};UCLightFlashParameters *p=&r.parameters;
 if(!id||!type||!scale||!s||!isfinite(*scale))return 1;
 p->attenuation_start=*scale*2048.f;p->attenuation_end=*scale*128.f;
 if(*type==1)return 0;
 if(!s->flash||!isfinite(p->attenuation_start)||!isfinite(p->attenuation_end))return 2;
 r.position[1]=40;r.parent_id=*id;r.mode=2;
 p->lifespan=.2f;p->multiplier_start=2;p->multiplier_end=-1;
 p->color_start[0]=2;p->color_start[1]=1.5f;p->color_start[2]=1;
 p->color_end[0]=.5f;p->color_end[1]=.25f;p->curve_power=.5f;
 return s->flash(s->owner,&r)?2:0;
}

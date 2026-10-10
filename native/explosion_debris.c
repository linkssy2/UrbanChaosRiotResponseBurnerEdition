#include "explosion_debris.h"
#include <math.h>
#include <string.h>
static int finite3(const float*p){return p&&isfinite(p[0])&&isfinite(p[1])&&isfinite(p[2]);}
static float draw(UCGameRandom*r,float low,float high){int32_t word=0;uc_game_random_next(r,&word);return(float)(((double)word*((double)high-low))*0x1p-31+low);}
int uc_explosion_debris_initialize(UCExplosionDebris*s,UCExplosionDebrisParticle*p,uint32_t count,
 const float*position,float size,float duration,float power,UCGameRandom*r){
 if(!s||(!p&&count)||!r||r->first>=250||r->second>=250||!finite3(position)||!isfinite(size)||
    !isfinite(duration)||duration<=0||!isfinite(power))return 1;
 s->duration=s->remaining=duration;s->particles=p;s->count=count;
 const float negative=0.f-power,vertical=power*-.05f;
 for(uint32_t i=0;i<count;i++){
  memcpy(p[i].position,position,12);float z=draw(r,negative,power),y=draw(r,vertical,power),x=draw(r,negative,power);
  p[i].velocity[0]=x;p[i].velocity[1]=y;p[i].velocity[2]=z;p[i].size=size;p[i].stopped=0;
 }
 return 0;
}
void uc_explosion_debris_resolve(UCExplosionDebrisParticle*p,float delta,
 const float input_velocity[3],const float input_to[3],const float *point,const float *normal){
 float velocity[3],to[3];memcpy(velocity,input_velocity,12);memcpy(to,input_to,12);
 if(point){
  float square=(velocity[2]*velocity[2]+velocity[1]*velocity[1])+velocity[0]*velocity[0];
  if(10000.f>square){p->stopped=1;memcpy(to,point,12);}
  else{
   float projection=(((0.f-velocity[2])*normal[2]+(0.f-velocity[1])*normal[1])+(0.f-velocity[0])*normal[0])*2.f;
   for(unsigned j=0;j<3;j++){velocity[j]=(normal[j]*projection+velocity[j])*.1f;to[j]=velocity[j]*delta+point[j];}
  }
 }
 memcpy(p->position,to,12);memcpy(p->velocity,velocity,12);
}
int uc_explosion_debris_step(UCExplosionDebris*s,float delta,UCExplosionDebrisQuery query,void*owner){
 if(!s||(!s->particles&&s->count)||!query||!isfinite(delta)||delta<0||!isfinite(s->remaining))return 1;
 s->remaining-=delta;if(s->remaining<=0)return 0;
 const float gravity=delta*-980.5999755859375f;
 for(uint32_t i=0;i<s->count;i++){
  UCExplosionDebrisParticle*p=s->particles+i;if(p->stopped)continue;
  float velocity[3]={p->velocity[0],p->velocity[1]+gravity,p->velocity[2]},to[3],point[3],normal[3];uint8_t hit=0;
  for(unsigned j=0;j<3;j++)to[j]=velocity[j]*delta+p->position[j];
  if(!finite3(to)||!finite3(velocity)||query(owner,p->position,to,1,65535,65535,0,point,normal,&hit)||hit>1)return 2;
  if(hit&&(!finite3(point)||!finite3(normal)))return 2;
  uc_explosion_debris_resolve(p,delta,velocity,to,hit?point:NULL,normal);
 }
 return 0;
}
int uc_explosion_debris_vertices(const UCExplosionDebris*s,uint32_t first,uint32_t count,float(*p)[4],float(*color)[4]){
 if(!s||(!s->particles&&s->count)||!p||!color||first>s->count||count>s->count-first||
    !isfinite(s->duration)||s->duration<=0||!isfinite(s->remaining))return 1;
 for(uint32_t i=0;i<count;i++){const UCExplosionDebrisParticle*v=s->particles+first+i;
  if(!finite3(v->position)||!isfinite(v->size))return 1;
  memcpy(p[i],v->position,12);p[i][3]=v->size;color[i][0]=color[i][1]=color[i][2]=1;color[i][3]=s->remaining/s->duration;
 }
 return 0;
}

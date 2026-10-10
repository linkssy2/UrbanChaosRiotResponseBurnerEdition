#include "explosion.h"
#include <math.h>
#include <string.h>
static float distance2(const float*a,const float*b){float x=a[0]-b[0],y=a[1]-b[1],z=a[2]-b[2];return(y*y+x*x)+z*z;}
static int position(const float*p){if(!p)return 0;for(unsigned i=0;i<3;i++)if(!isfinite(p[i])||fabsf(p[i])>1e6f)return 0;return 1;}
int uc_explosion_scorch(const float*p,const UCExplosionScorchServices*h){
 if(!position(p)||!h||!h->query||!h->decal)return 1;
 float from[3]={p[0],p[1]+20.f,p[2]},to[3]={p[0],p[1]-50.f,p[2]},point[3],normal[3];uint8_t hit=0;
 if(h->query(h->owner,from,to,1,0,0x4000,0,point,normal,&hit)||hit>1)return 2;
 if(!hit)return 0;if(!position(point)||!position(normal))return 2;
 const float size[2]={128.f,128.f};
 return h->decal(h->owner,"FX\\FX_Molotov_Scorch\\FX_Molotov_Scorch.shader",point,normal,size,0x4000)?2:0;
}
int uc_explosion_parameters(const UCLevelEntity*e,UCExplosionParameters*out){
 if(!e||!out||!e->class_name||strcmp(e->class_name,"Explosion")||!e->properties.data||e->properties.size!=52)return 1;
 UCExplosionParameters p;memcpy(&p,e->properties.data,52);
 if(!isfinite(p.scale)||p.scale<0||!isfinite(p.radius)||p.radius<0||!isfinite(p.power)||p.power<0||p.affects_doors>1||p.ignore_collision>1||p.force_kill>1||p.show_particles>1||p.effects_only>1)return 1;
 *out=p;return 0;
}
int uc_explosion_construct(UCExplosionConstructor*s,const UCExplosionConstructorServices*h){
 if(!s||!h||!h->base||!h->coroutine_initialize||!h->property||!h->sound||!h->coroutine_bind)return 1;
 if(h->base(h->owner)||h->coroutine_initialize(h->owner))return 2;
 s->child_b4=s->child_b8=-1;
 UCExplosionProperty value;unsigned found;
 #define READ(t,n) do{found=0;if(h->property(h->owner,t,n,&value,&found)||found>1)return 2;}while(0)
 value.string=NULL;READ(6,"Sound");s->sound=value.string;
 if(h->sound(h->owner,s->sound,&s->audio))return 2;
 READ(2,"Scale");s->parameters.scale=found?value.number:1.f;
 READ(1,"DamageType");s->parameters.damage_type=found?value.integer:0;
 READ(2,"Radius");s->parameters.radius=found?value.number:500.f;
 READ(2,"Power");s->parameters.power=found?value.number:25000.f;
 READ(1,"OwnerID");s->parameters.owner_id=found?value.integer:-1;
 READ(3,"AffectsDoors");s->parameters.affects_doors=found?(uint8_t)value.integer!=0:0;
 READ(3,"IgnoreCollision");s->parameters.ignore_collision=found?(uint8_t)value.integer!=0:0;
 READ(1,"ParticleType");s->parameters.particle_type=found?value.integer:0;
 READ(3,"Force kill");s->parameters.force_kill=found?(uint8_t)value.integer!=0:1;
 READ(1,"Damage Team");s->parameters.damage_team=found?value.integer:-1;
 READ(3,"Show particles");s->parameters.show_particles=found?(uint8_t)value.integer!=0:1;
 READ(3,"Show effects only");s->parameters.effects_only=found?(uint8_t)value.integer!=0:0;
 #undef READ
 s->parameters.power*=1.5f;s->field=NULL;
 return h->coroutine_bind(h->owner)?2:0;
}
int uc_explosion_damage_amount(const float*c,float radius,float damage,int32_t type,uint8_t blast,const float*t,float width,float height,float*out){
 if(!out||!position(c)||!position(t)||!isfinite(radius)||radius<0||!isfinite(damage)||damage<0||!isfinite(width)||width<0||!isfinite(height)||height<0)return 1;
 float result=0;
 if(blast||type==2){
  result=damage;
  if(radius>0){double x=(double)t[0]-c[0],z=(double)t[2]-c[2];float y=t[1]-c[1];
   float horizontal=(float)(sqrt(x*x+z*z)-width),vertical=(float)(fabs((double)y)-height);
   if(horizontal<=0)horizontal=0;if(vertical<=0)vertical=0;
   float ratio=(horizontal+vertical)/radius;
   if(ratio>.2f){float factor=1.f-ratio;if(factor>1)factor=1;if(factor<.1f)factor=.1f;result=factor*damage;}
  }
 }
 *out=result;return 0;
}
int uc_explosion_obstructed(const float*from,const float*to,int32_t target,int32_t group,const UCExplosionObstructionServices*h){
 if(!position(from)||!position(to)||!h||!h->query||!h->multiplayer||group< -1||group>65535)return -1;
 int32_t id=0;uint8_t actor=0;int hit=h->query(h->owner,from,to,0x21,(uint32_t)group,0x4000,0,&id,&actor);
 if(hit<0||hit>1)return -1;if(!hit)return 0;int multi=h->multiplayer(h->owner);if(multi<0)return -1;
 return multi||!actor||id!=target;
}
int uc_explosion_damage_eligible(const float*c,float radius,int32_t type,uint8_t ignore,const float*t,float width,float height,const void*target,int32_t group,UCExplosionObstruction blocked,void*context,uint8_t*out){
 if(!out||!position(c)||!position(t)||!isfinite(radius)||radius<0||!isfinite(width)||width<0||width>1e6f||!isfinite(height)||height<0||height>1e6f||ignore>1||group< -1||group>65535)return 1;
 double x=(double)t[0]-c[0],z=(double)t[2]-c[2];float y=t[1]-c[1];
 float horizontal=(float)(sqrt(x*x+z*z)-width),vertical=(float)(fabs((double)y)-height);
 if(horizontal<=0)horizontal=0;if(vertical<=0)vertical=0;
 if(horizontal+vertical>=radius){*out=0;return 0;}
 if(ignore&&type!=2){*out=1;return 0;}
 if(!blocked||!target)return 1;
 float from[3]={c[0],c[1]+10.f,c[2]},middle=t[1]+height*.5f,offset=height*.35f;
 float to[3]={t[0],middle-offset,t[2]};int hit=blocked(context,from,to,target,group);
 if(hit<0||hit>1)return 1;if(!hit){*out=1;return 0;}
 to[1]=middle+offset;hit=blocked(context,from,to,target,group);
 if(hit<0||hit>1)return 1;*out=(uint8_t)!hit;return 0;
}
int uc_explosion_force(const float*c,float radius,float power,float minimum,float maximum,float remaining,const float*t,float*out){
 if(!out||!position(c)||!position(t)||!isfinite(radius)||radius<=0||!isfinite(power)||power<0||!isfinite(minimum)||!isfinite(maximum)||minimum<0||minimum>maximum||maximum>1||!isfinite(remaining)||(remaining<0&&remaining!= -1))return 1;
 float v[3]={0,0,0};
 if(remaining!=0){for(unsigned k=0;k<3;k++)v[k]=t[k]-c[k];float distance=sqrtf((v[1]*v[1]+v[0]*v[0])+v[2]*v[2]);
  if(distance<=0||distance>=radius)return 1;
  float ratio=distance/radius;if(ratio>maximum)ratio=maximum;if(ratio<minimum)ratio=minimum;
  float magnitude=((1.f-ratio)*power)/distance;for(unsigned k=0;k<3;k++){v[k]*=magnitude;if(!isfinite(v[k]))return 1;}
 }
 memcpy(out,v,sizeof v);return 0;
}
int uc_explosion_force_tick(float*remaining,float delta){
 if(!remaining||!isfinite(*remaining)||(*remaining<0&&*remaining!= -1)||!isfinite(delta)||delta<0)return 1;
 if(*remaining!= -1&&*remaining!=0){float value=*remaining-delta;*remaining=value<0?0:value;}return 0;
}
int uc_explosion_field_step(UCExplosionField*f,float delta,UCExplosionBody*b,size_t count){
 if(!f||(!b&&count)||count>4096||!position(f->center)||!isfinite(f->power)||f->power<0||f->power>1e6f||f->count>80||f->ticks>30||f->done>1)return 1;
 float remaining=f->remaining;if(uc_explosion_force_tick(&remaining,delta))return 1;
 for(size_t i=0;i<count;i++){
  const UCExplosionBody*p=b+i;
  if(!p->identity||p->dynamic>1||p->excluded>1||p->disabled>1||!position(p->position)||!position(p->force_position)||!isfinite(p->property16))return 1;
  if(!f->done&&f->ticks<29&&f->count<80&&remaining!=0&&p->dynamic&&!p->excluded&&!p->disabled&&distance2(p->position,f->center)<25){
   int seen=0;for(unsigned j=0;j<f->count;j++)seen|=f->visited[j]==p->identity;
   float d=distance2(p->force_position,f->center);if(!seen&&(d<=0||d>=25))return 1;
  }
 }
 for(size_t i=0;i<count;i++){b[i].affected=b[i].impulsed=0;memset(b[i].impulse,0,sizeof b[i].impulse);}
 if(f->done)return 0;
 if(++f->ticks>=30||f->count==80){f->done=1;return 0;}
 f->remaining=remaining;
 for(size_t i=0;i<count&&f->count<80;i++){
  UCExplosionBody*p=b+i;float d=distance2(p->position,f->center);
  if(!p->dynamic||p->excluded||d>=25)continue;
  int seen=0;for(unsigned j=0;j<f->count;j++)seen|=f->visited[j]==p->identity;if(seen)continue;
  f->visited[f->count++]=p->identity;p->affected=1;
  if(p->type==1)p->property16+=(25.f-d);
  if(p->disabled)continue;
  if(uc_explosion_force(f->center,5,f->power,.2f,.7f,remaining,p->force_position,p->impulse))return 1;
  for(unsigned k=0;k<3;k++){p->impulse[k]*=.03f;if(p->type==4)p->impulse[k]*=.4f;}
  float length=sqrtf((p->impulse[1]*p->impulse[1]+p->impulse[0]*p->impulse[0])+p->impulse[2]*p->impulse[2]);
  p->impulse[2]+=length*.2f;p->impulsed=1;
 }
 return 0;
}

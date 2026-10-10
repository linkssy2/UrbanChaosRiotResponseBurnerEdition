#include "explosion_sequence.h"
#include <math.h>
#include <string.h>
int uc_explosion_sequence(UCExplosionSequence*s,const UCExplosionSequenceServices*h){
 if(!s||!h||!s->position||!s->parameters||!h->field_create||!h->shake||!h->online||
 !h->broadcast||!h->area_effect||!h->sound||!h->effect||!h->debris||!h->field_done||
 !h->field_step||!h->delta||!h->sound_active||!h->yield||!h->remove)return 1;
 const UCExplosionParameters*p=s->parameters;
 if(!isfinite(p->scale)||p->scale<0||p->scale>1000||!isfinite(p->radius)||p->radius<0||p->radius>1e6f||
 !isfinite(p->power)||p->power<=0||p->power>1e6f||p->affects_doors>1||p->ignore_collision>1||
 p->force_kill>1||p->effects_only>1||p->show_particles>1)return 1;
 for(unsigned k=0;k<3;k++)if(!isfinite(s->position[k])||fabsf(s->position[k])>1e6f)return 1;
 void*o=h->owner;float strength=(p->scale*p->scale)*.3f;
 if(p->power!=0&&h->field_create(o,s->position,p->power))return 2;
 if(h->shake(o,s->position,5000,strength,2,3))return 2;
 float damage=1000;
 if(p->damage_type==0||(p->damage_type>=1&&p->damage_type<=3)){
  int online=h->online(o);if(online<0||online>1)return 2;
  damage=p->damage_type==0?(online?490.f:150.f):(online?510.f:200.f);
 }else if(p->damage_type==6)damage=200;
 s->damage=p->force_kill?2000.f:damage;
 if(!p->effects_only){
  UCExplosionBlast b={0};memcpy(b.point,s->position,12);b.radius=p->radius;b.owner=p->owner_id;
  b.affects_doors=(uint8_t)p->affects_doors;b.ignore_collision=(uint8_t)p->ignore_collision;
  b.damage=s->damage;b.type=p->damage_type;b.blast=p->damage_type==0||p->damage_type==3||p->damage_type==6;
  b.team=p->damage_team;b.parent=s->parent;if(h->broadcast(o,&b,s->id))return 2;
 }
 int online=h->online(o);if(online<0||online>1)return 2;
 if(!online){float box[12];for(unsigned k=0;k<3;k++){
   box[k]=s->position[k]-p->radius;box[k+3]=s->position[k]+p->radius;box[k+6]=s->position[k];box[k+9]=p->radius;}
  if(h->area_effect(o,box,2))return 2;
 }
 if(h->sound(o,"Explode"))return 2;
 if(p->show_particles){
  if((p->damage_type==0||p->damage_type==3||p->damage_type==6)&&h->effect(o,0xE9130))return 2;
  if(h->effect(o,0xE9650))return 2;
  if(h->effect(o,p->damage_type==3?0xEA170:p->damage_type==2?0xE9D90:0xE98E0))return 2;
  if(h->debris(o,p->damage_type==2?128:32,p->damage_type==2?2.f:1.f,
   p->damage_type==2?10000.f:p->damage_type==3?2000.f:200.f,
   p->damage_type==2?"Particles\\riot_grenade_ball.tga":"FX\\FX_Debris\\FX_Debris_Rock.tga"))return 2;
 }
 for(;;){int done=h->field_done(o);if(done<0||done>1)return 2;if(done)break;
  double dt=h->delta(o);if(!isfinite(dt)||dt<0||dt>1)return 2;
  if(h->field_step(o,(float)dt)||h->yield(o))return 2;
 }
 for(;;){int active=h->sound_active(o);if(active<0||active>1)return 2;if(!active)break;if(h->yield(o))return 2;}
 if(h->remove(o)||h->yield(o))return 2;return 0;
}

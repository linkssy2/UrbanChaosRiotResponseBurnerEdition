#include "player_damage.h"
#include "player_shield.h"
#include "player_health.h"
#include <math.h>
#include <string.h>
void uc_player_damage_fresh(UCPlayerDamageState*s){if(s){memset(s,0,sizeof*s);s->fire_timer=.75f;s->hit_elapsed=.333f;}}
int uc_player_damage_age(UCPlayerDamageState*s,double delta,uint8_t elapsed_enabled){
 if(!s||!isfinite(delta)||delta<0||!isfinite(s->fire_timer)||!isfinite(s->hit_elapsed))return 1;
 float fire=(float)((double)s->fire_timer+delta),elapsed=elapsed_enabled?(float)((double)s->hit_elapsed+delta):s->hit_elapsed;if(!isfinite(fire)||!isfinite(elapsed))return 1;
 s->fire_timer=fire>.75f?.75f:fire;s->hit_elapsed=elapsed;return 0;
}
static int impact_effects(UCPlayerImpactState*s,const UCPlayerDamageInput*i,float damage,const UCPlayerFireDamage*d,int blocked,int notify,const UCPlayerDamageServices*h,const UCPlayerImpactServices*x){
 if(x->statistic(x->owner,blocked?0x38u:0x28u))return 2;
 if(blocked){
  if(x->shield_sound(x->owner,d->source==8?"ShieldDeflectMelee":"ShieldDeflectBullet")||x->shield_recoil(x->owner,d->source!=25,damage,d->direction))return 2;
  const float*look=i->frame+8;float heading;
  if(fabs((double)look[1])>.9999989867210388f)heading=0;
  else if(fabs((double)look[2])<9.999999974752427e-7f)heading=look[0]>0?1.5707963705062866f:4.71238899230957f;
  else{double angle=atan2((double)look[0]/look[2],1.);if(look[2]<0)angle+=3.1415927410125732f;heading=(float)angle;if(heading<0)heading+=6.2831854820251465f;}
  float degrees=6.2829999923706055f-heading;degrees+=1.5709999799728394f;degrees*=57.2957763671875f;
  float point[3],rotation[3]={0,degrees,0};if(x->shield_point(x->owner,point)||x->shield_spark(x->owner,point,rotation))return 2;
 }else{
  *i->health-=damage;if(*i->health<0)*i->health=0;
  if(notify){
   if(h->sound(h->owner,1))return 2;
   s->yaw=(float)x->random(x->owner,15,30);if(x->random(x->owner,0,1)>.5f)s->yaw=0.f-s->yaw;
   s->pitch=(float)x->random(x->owner,-30,-30);s->intensity_x=s->intensity_y=1;
   const float*v=d->direction;if(v[0]!=0||v[1]!=0||v[2]!=0){
    double length=(double)v[2]*v[2]+(double)v[1]*v[1];length+=(double)v[0]*v[0];double inv=1./sqrt(length);
    float vector[3]={(float)((double)v[0]*inv),(float)((double)v[1]*inv),(float)((double)v[2]*inv)},point[3];
    for(unsigned n=0;n<3;n++){vector[n]=(float)(x->random(x->owner,.95f,1.05f)*vector[n]);point[n]=x->position[n]-vector[n]*100.f;}
    if(x->has_point_push&&(x->point_push(x->owner,0x14,point,.5f,1000,1100)||x->point_push(x->owner,0xC,point,.3f,1000,1100)))return 2;
   }
  }
 }
 return 0;
}
static int damage_apply(UCPlayerDamageState*s,UCPlayerImpactState*impact,const UCPlayerDamageInput*i,float raw,const UCPlayerFireDamage*d,const UCPlayerDamageServices*h,const UCPlayerImpactServices*x){
 if(!s||!i||!d||!h||!i->health||!i->maximum||!i->frame||!h->difficulty||!h->multiplayer||!h->sound||!h->pain||!h->feedback||!h->enemy_type||!h->adaptive_hit||!h->delta||
    (d->kind!=12&&d->kind!=13&&d->kind!=15&&d->kind!=5)||!isfinite(raw)||raw<0||!isfinite(i->scale)||i->scale<0||!isfinite(*i->health)||!isfinite(*i->maximum)||!isfinite(s->fire_timer)||!isfinite(s->hit_elapsed)||!isfinite(s->raw_damage))return 1;
 if(d->kind==5&&(!impact||!x||!x->position||!x->random||!x->statistic||!x->shield_sound||!x->shield_recoil||!x->shield_point||!x->shield_spark||(x->has_point_push&&!x->point_push)))return 1;
 uint8_t cone=0;UCMainplayerEquipControl no_selection={0};
 /* Both stable virtual10 reads are the same published player frame. Preflight
  * the finite cone domain; its pure value is consumed at sourceC9028 below. */
 if(uc_player_shield_blocks(0,&no_selection,0,i->frame,d->direction,&cone))return 1;
 int notify=i->selected_level74||s->hit_elapsed>=.333f;
 if((i->disabled||i->locked||i->action==12||i->action==13||(i->world_flags&0x8600))&&!d->flag[0])return 0;
 s->raw_damage+=raw;s->debug_timer=2.f;
 double value;if(uc_game_difficulty_value(h->difficulty,&value))return 2;
 float multiplier=(float)(((double)2.f-.4f)*value+.4f);if(i->selected_level74)multiplier=1;
 float damage=i->scale*multiplier;damage*=raw;
 if(300.+(0.-300.)*value>*i->health){int multi=h->multiplayer(h->owner);if(multi<0)return 2;if(!multi)damage*=.7f;}
 if(damage>0&&damage<=4)damage=4;
 int blocked=((i->item==0||i->shield.select_empty)&&!i->shield.select_previous&&i->shield_impact<100&&cone);
 int multi=h->multiplayer(h->owner);if(multi<0)return 2;if(multi&&(d->source==11||d->flag[0]))blocked=0;
 if(h->sound(h->owner,0))return 2;
 if(damage>0&&!blocked&&h->pain(h->owner))return 2;
 if(i->action!=12){
  float strength=1.f-raw*.001f;if(strength>1)strength=1;
  if(strength>0){float duration=strength;if(blocked){duration=(float)((double)strength*.1f);if(h->feedback(h->owner,duration,strength*.5f,2))return 2;}
   if(h->feedback(h->owner,strength,duration,2))return 2;}
  if(d->kind==5){if(impact_effects(impact,i,damage,d,blocked,notify,h,x))return 2;}
  else{
  if(d->kind!=15){if(s->fire_timer>0){damage=0;double dt=h->delta(h->owner);if(!isfinite(dt)||dt<0)return 2;s->fire_timer=(float)((double)s->fire_timer-(dt+dt));}
   else{s->fire_timer=.75f;if(notify&&h->sound(h->owner,1))return 2;}}
  *i->health-=damage;if(*i->health<0)*i->health=0;
  }
  if(!blocked&&damage>0){int32_t type=-1;if(h->enemy_type(h->owner,d->owner,&type))return 2;
   if(type!=-1){if(h->adaptive_hit(h->owner,type))return 2;if(!cone&&h->adaptive_hit(h->owner,type))return 2;}}
 }
 if((i->world_flags&8)&&*i->health<.001f&&!uc_player_health_set(i->health,i->maximum,900))return 2;
 if(notify&&damage>0)s->hit_elapsed=0;
 memcpy(&s->last,d,54);s->last_present=1;return 0;
}
int uc_player_damage_environment(UCPlayerDamageState*s,const UCPlayerDamageInput*i,float raw,const UCPlayerFireDamage*d,const UCPlayerDamageServices*h){
 if(d&&d->kind==5)return 1;return damage_apply(s,0,i,raw,d,h,0);
}
int uc_player_damage_impact(UCPlayerDamageState*s,UCPlayerImpactState*impact,const UCPlayerDamageInput*i,float raw,const UCPlayerFireDamage*d,const UCPlayerDamageServices*h,const UCPlayerImpactServices*x){
 if(!d||d->kind!=5)return 1;return damage_apply(s,impact,i,raw,d,h,x);
}

int uc_player_damage_blazer_hit(uint32_t mode,uint8_t disabled,float*channel){
 static const float rate[]={1.2f,1.f,1.3f,2.f};
 if(!channel||mode>3||!isfinite(*channel))return 1;if(disabled)return 0;
 float amount=1.f/12.f;amount*=rate[mode];float value=*channel-amount;*channel=value>0?value:0.f;return 0;
}

int uc_player_blast_quiet(const UCExplosionBlast*b){return !b?-1:(!b->blast&&b->type!=2);}

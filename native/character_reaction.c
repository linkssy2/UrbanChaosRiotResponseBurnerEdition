#include "character_reaction.h"
#include <math.h>
#include <string.h>
static int prop(const UCCharacterReactionServices*h,unsigned scope,const char*n,uint8_t*out){return !h->property||h->property(h->owner,scope,n,0,out);}
static int spawn(UCCharacterReaction*s,const UCCharacterReactionServices*h,unsigned kind,const UCCharacterReactionDescriptor*d,int32_t mode,int secondary){
 int32_t id;if(!h->spawn||h->spawn(h->owner,kind,d,mode,&id))return 2;
 if(secondary>=0&&(!h->queue||h->queue(h->owner,id,(uint8_t)secondary)))return 2;(void)s;return 0;
}
int uc_character_hit_response(UCCharacterReaction*s,int32_t location,uint8_t knockback,uint8_t shield,const UCCharacterReactionServices*h){
 if(!s||!h)return 1;uint8_t yes;
 if(s->status&0x40)return !h->event||h->event(h->owner,0x37B52,s->actor,s->root,0,0,0)?2:0;
 if(prop(h,1,"procedural hit reactions",&yes))return 2;
 if(yes){if(!h->random||h->random(h->owner,&yes)||!h->procedural_hit||h->procedural_hit(h->owner,yes?1.f:-1.f,.3f))return 2;return 0;}
 if(location==0||location==2){if(prop(h,1,"no knockbacks",&yes))return 2;if(yes)location=4;}
 UCCharacterReactionDescriptor d={0};d.location=location;d.knockback=knockback;d.shield=shield;d.attacker=s->damage_owner;
 return spawn(s,h,UC_REACTION_HIT,&d,1,1);
}
int uc_character_reaction_update(UCCharacterReaction*s,uint32_t flags,float damage,const UCCharacterReactionServices*h){
 if(!s||!h||!isfinite(damage))return 1;if(flags&0x80000)return 0;
 uint32_t kind=flags&0x1F0FE;uint8_t yes,can;
 if(kind==0x2000){s->status|=0x40000;
  if((s->status&5)||(s->flags&4)){
   int32_t status=1;if(s->stun_task&&(!h->task_status||h->task_status(h->owner,s->stun_task,&status)))return 2;
   if(status){if(s->stun_task&&(!h->task_release||h->task_release(h->owner,s->stun_task)))return 2;
    if(!h->task_create||h->task_create(h->owner,&s->stun_task)||!h->clip||h->clip(h->owner,"TaserStunOverlay",s->stun_task))return 2;}
  }
 }
 if(s->status&5)return 0;
 if(!h->dead||h->dead(h->owner,&yes))return 2;can=!yes;if(s->status&0x20807)can=0;
 if(kind==0x10000&&!(s->status&4)&&!(s->flags&4)){
  if(prop(h,0,"enemy of player",&yes))return 2;
  if(!yes){float health;if(!h->health||h->health(h->owner,&health))return 2;
   if(health>damage){if(h->health(h->owner,&health))return 2;if(!h->set_health||h->set_health(h->owner,health-damage)||!h->unrestrain||h->unrestrain(h->owner))return 2;goto ordinary;}}
  if(prop(h,1,"is hostage",&yes))return 2;
  if(yes){UCCharacterReactionDescriptor d={0};d.world=h->world;d.entity=s->actor;d.incapacitated=1;
   if(!h->killer||h->killer(h->owner,&d.killer)||prop(h,0,"has obituary",&d.obituary))return 2;
   if(spawn(s,h,UC_REACTION_FRIENDLY_DEATH,&d,5,-1)||!h->session_death||h->session_death(h->owner))return 2;}
  {UCCharacterReactionDescriptor d={0};d.world=h->world;if(spawn(s,h,UC_REACTION_RESTRAINED,&d,5,0))return 2;}
  s->status|=4;if(!h->player||h->player(h->owner,&s->damage_owner))return 2;
 }else if(kind==0x4000){
  if(!can){if(s->foreground==-1)return 0;if(prop(h,2,"allow electricity response",&yes))return 2;if(!yes)return 0;}
  if(!(s->status&0x200)){UCCharacterReactionDescriptor d={0};if(spawn(s,h,UC_REACTION_ELECTROCUTE,&d,1,1))return 2;}
  else if(!h->event||h->event(h->owner,0x38171,s->actor,s->root,0,0,0))return 2;
 }else if(kind==0x2000){
  uint8_t floor=0,lightning=0;
  if(!can){if(s->foreground==-1)return 0;if(prop(h,2,"allow tazer on floor response",&yes))return 2;
   if(yes)floor=1;else{if(prop(h,2,"allow tazer response",&yes))return 2;
    if(!yes){if(prop(h,2,"straight to ragdoll on tazer",&yes))return 2;if(!yes){if(s->flags&4)return 0;lightning=1;}}}}
  if(s->foreground!=-1){if(prop(h,2,"straight to ragdoll on tazer",&yes))return 2;
   if(yes){if(!h->event||h->event(h->owner,0x38261,s->actor,s->root,floor,(uint8_t)((flags&0x40000)&&s->enemy),0))return 2;goto ordinary;}}
  if(!(s->status&0x10040)){UCCharacterReactionDescriptor d={0};double time;
   d.on_floor=floor;d.player_tasering=(flags&0x40000)!=0;d.enemy=s->enemy;d.lightning_only=lightning;d.speed=damage;
   if(!h->taser_time||h->taser_time(h->owner,&time))return 2;d.required_time=(float)(time*(double).002f);
   if(spawn(s,h,UC_REACTION_TASER,&d,1,1))return 2;
  }else if(!h->event||h->event(h->owner,0x383E9,s->actor,s->root,0,0,(flags&0x40000)?damage:1.f))return 2;
 }
ordinary:
 if(!can)return 0;
 if((flags&0x1FFFE)&&(flags&0x40000)&&!(flags&0x12000)&&(s->status&4)){if(!h->unrestrain||h->unrestrain(h->owner))return 2;}
 switch(kind){
 case 2:return uc_character_hit_response(s,4,(flags&0x100000)!=0,1,h);
 case 4:return uc_character_hit_response(s,1,0,0,h);
 case 8:return uc_character_hit_response(s,0,0,0,h);
 case 0x10:return uc_character_hit_response(s,3,0,0,h);
 case 0x20:return uc_character_hit_response(s,2,0,0,h);
 case 0x40:case 0x80:return uc_character_hit_response(s,4,0,0,h);
 case 0x1000:if(s->enemy){if(!h->random||h->random(h->owner,&yes))return 2;return uc_character_hit_response(s,yes?0:2,0,0,h);}break;
 default:break;
 }
 return 0;
}

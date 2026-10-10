#include "brute_combat.h"
#include <string.h>
static uint8_t pending(const UCBruteCombatServices*h,uintptr_t task){return task&&!h->task_status(h->owner,task);}
static void release(const UCBruteCombatServices*h,uintptr_t*task){if(*task){h->task_release(h->owner,*task);*task=0;}}
static void transition(UCBruteCombat*s,const UCBruteCombatServices*h,int32_t stage){
 s->entered=0;s->elapsed=s->since_action=0;h->transition(h->owner,stage);
}
uint8_t uc_brute_throw_predicate(const UCBruteCombat*s,const UCBruteCombatServices*h){
 if(!s||!h||!h->attention||h->attention(h->owner)||!s->in_cone||!s->can_throw||s->throw_cooldown>0||
    1000.f>s->distance||s->distance>2200.f||.8999999761581421f>s->facing_dot||s->throws_left<=0)return 0;
 return s->route<=0;
}
uint8_t uc_brute_charge_predicate(const UCBruteCombat*s){
 if(!s||!s->can_charge||500.f>s->distance||s->distance>600.f||1.5f>s->elapsed||1.5f>s->since_attack)return 0;
 float height=s->position[1]-s->target_y;return !(height>25.f||-25.f>height);
}
int uc_brute_combat_step(UCBruteCombat*s,const UCBruteCombatServices*h,uint32_t stage){
 if(!s||!h||stage<3||stage>7||!h->transition||!h->reset_navigation||!h->attention||!h->clear_attention||
 !h->look||!h->stop_aim||!h->stop_look||!h->aim||!h->pressure||!h->delta||!h->task_release||!h->task_status||
 !h->task_create||!h->melee_reset||!h->melee_facing||!h->melee_distance||!h->melee_prepare||!h->melee_action||
 !h->melee_move||!h->sound||!h->dialogue||!h->random_int||!h->random_bool||!h->random_float||
 !h->difficulty_delay||!h->target_is_player||!h->throw_clip||!h->action)return 1;
 if(stage==3){
  if(!s->entered){s->entered=1;h->reset_navigation(h->owner);memcpy(s->saved_position,s->position,12);s->attack_cooldown=0;s->attack_count=0;}
  if(!h->attention(h->owner))h->look(h->owner,s->aim);
  if(s->retreat){transition(s,h,5);return 0;}
  if(pending(h,s->tasks[0])){
   h->stop_aim(h->owner);if(!h->attention(h->owner))h->stop_look(h->owner);
   h->pressure(h->owner,(float)((double)h->delta(h->owner)*20.f));
   if(!(s->actor_flags&0x10))return 0;
   release(h,s->tasks);h->melee_reset(h->owner);
  }
  h->aim(h->owner,s->aim);if(!h->attention(h->owner))h->look(h->owner,s->aim);
  if(s->distance>400.f){transition(s,h,2);return 0;}
  if(s->attack_cooldown<=0&&h->melee_facing(h->owner)<=(double)s->facing_dot){
   int32_t choice=s->attack_count>0?h->random_int(h->owner,0,3):0;
   uintptr_t callback=h->task_create(h->owner,s->tasks);h->melee_action(h->owner,choice==1?1:choice==2?2:0,callback);
   h->sound(h->owner,"Swipe");h->dialogue(h->owner,0x31,0x19);h->melee_prepare(h->owner,s->target);
   s->attack_count=(int32_t)((uint32_t)s->attack_count+1);s->attack_cooldown=(float)h->difficulty_delay(h->owner,3.5f,1.5f,.2f);
   h->clear_attention(h->owner);return 0;
  }
  s->attack_cooldown=(float)((double)s->attack_cooldown-h->delta(h->owner));
  float low=(float)(h->melee_distance(h->owner)-45.f),high=(float)(h->melee_distance(h->owner)+45.f);
  h->melee_move(h->owner,s->facing_dot>.7300000190734863f?(low>s->distance?2:s->distance>high?0:1):1);return 0;
 }
 if(stage==4){
  if(!s->entered){uintptr_t cb=h->task_create(h->owner,s->tasks+1);h->throw_clip(h->owner,cb);
   h->sound(h->owner,"Throw");h->dialogue(h->owner,0x24,0x32);s->throw_cooldown=(float)h->random_float(h->owner,3,7);s->entered=1;}
  if(!pending(h,s->tasks[1]))transition(s,h,2);return 0;
 }
 if(stage==5){
  if(!s->entered){release(h,s->tasks+2);release(h,s->tasks+3);s->taunt_delay=(float)h->random_float(h->owner,2,5);h->stop_aim(h->owner);s->entered=1;}
  h->pressure(h->owner,(float)((double)h->delta(h->owner)*20.f));if(!h->attention(h->owner))h->look(h->owner,s->aim);
  if(!pending(h,s->tasks[3])){
   s->taunt_delay=(float)((double)s->taunt_delay-h->delta(h->owner));
   if(350.f>s->distance)h->melee_move(h->owner,2);
   else if(s->distance>450.f)h->melee_move(h->owner,0);
   else if(s->taunt_delay<=0){uint8_t first=h->random_bool(h->owner);uint32_t dialogue=first?0x2E:0x2D;
    if(h->target_is_player(h->owner)&&h->random_bool(h->owner))dialogue=0x14;
    h->dialogue(h->owner,dialogue,0x32);uintptr_t cb=h->task_create(h->owner,s->tasks+3);
    h->action(h->owner,first?"Taunt1":"Taunt2",cb);s->taunt_delay=(float)(h->random_float(h->owner,2,5)*2.);
   }else h->melee_move(h->owner,1);
  }
  if(!s->retreat||s->distance>600.f)transition(s,h,2);return 0;
 }
 if(stage==6){
  if(!s->entered){h->stop_aim(h->owner);h->stop_look(h->owner);h->clear_attention(h->owner);h->melee_prepare(h->owner,s->target);
   uintptr_t cb=h->task_create(h->owner,s->tasks);h->action(h->owner,"RunningJumpAttack",cb);h->dialogue(h->owner,0x30,0x32);s->entered=1;}
  h->pressure(h->owner,(float)((double)h->delta(h->owner)*30.f));if(!pending(h,s->tasks[0]))transition(s,h,3);return 0;
 }
 if(!s->entered){uint8_t first=h->random_bool(h->owner);uint32_t dialogue=first?0x2E:0x2D;
  if(s->shield_time>=3.f&&h->random_bool(h->owner))dialogue=0x14;
  h->dialogue(h->owner,dialogue,0x32);uintptr_t cb=h->task_create(h->owner,s->tasks+3);
  h->action(h->owner,first?"Taunt1":"Taunt2",cb);s->entered=1;}
 h->aim(h->owner,s->aim);if(!h->attention(h->owner))h->look(h->owner,s->aim);
 h->pressure(h->owner,(float)((double)h->delta(h->owner)*20.f));if(!pending(h,s->tasks[3]))transition(s,h,2);return 0;
}

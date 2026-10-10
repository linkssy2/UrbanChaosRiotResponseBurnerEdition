#include "blazer_combat.h"
#include <math.h>
uint8_t uc_blazer_combat_sidestep(const UCBlazerCombat*s,const UCBlazerCombatServices*h){
 if(!h->throw_active(h->owner)||!h->throw_ready(h->owner)||1.f>s->near_time||600.f>s->distance)return 0;
 return 1;
}
uint8_t uc_blazer_combat_melee(const UCBlazerCombat*s,const UCBlazerCombatServices*h){
 if(.4f>s->facing_dot||s->distance>600.f||h->throw_active(h->owner)||!s->melee_enabled)return 0;
 return fabs((double)s->target_y-s->actor_y)<=100.;
}
static uint8_t pending(const UCBlazerCombatServices*h,uintptr_t task){return task&&!h->task_status(h->owner,task);}
int uc_blazer_combat_step(UCBlazerCombat*s,const UCBlazerCombatServices*h){
 uint8_t imminent,close;
 if(!s||!h||!h->stop||!h->stop_aim||!h->reset_navigation||!h->task_release||!h->task_status||!h->aim_task_create||
 !h->transition||!h->attention||!h->look||!h->cancel_position_search||!h->throw_active||!h->throw_ready||!h->throw_elapsed||
 !h->throw_reset||!h->throw_target||!h->publish_shield||!h->aim||!h->throw_predicate||!h->throw_request||!h->reset_cooldown)return 1;
 if(!s->entered){h->stop(h->owner);h->stop_aim(h->owner);h->reset_navigation(h->owner);
  if(s->aim_task){h->task_release(h->owner,s->aim_task);s->aim_task=0;}s->threw=0;s->entered=1;}
 if(s->target==-1){h->transition(h->owner,1);return 0;}
 if(!h->attention(h->owner))h->look(h->owner,s->aim);
 if(!s->dont_move&&!s->defend&&(!s->visible||500.f>s->distance||s->distance>3600.000244140625f))h->cancel_position_search(h->owner,1);
 if(pending(h,s->wait_task)||pending(h,s->side_task))return 0;
 if(s->defend&&!s->defend_near){h->transition(h->owner,3);return 0;}
 if(s->threw||!s->visible){
  if(!s->dont_move){if(s->position_changed&&s->position_ready){h->transition(h->owner,3);return 0;}
   if(uc_blazer_combat_sidestep(s,h)){h->transition(h->owner,6);return 0;}}
  else if(uc_blazer_combat_sidestep(s,h)){h->transition(h->owner,7);return 0;}
 }
 if(uc_blazer_combat_melee(s,h)){h->transition(h->owner,5);return 0;}
 h->publish_shield(h->owner,s->shield_time>=3.f);
 imminent=h->throw_elapsed(h->owner)>0.;if(imminent)imminent=h->throw_elapsed(h->owner)<3.;
 if(pending(h,s->aim_task)||!s->visible)h->throw_reset(h->owner);
 if(!s->facing&&s->visible&&(!s->defend||s->defend_near)){
  uintptr_t callback=h->aim_task_create(h->owner,&s->aim_task);h->aim(h->owner,s->aim,callback,2);}
 if(s->in_cone){
  if(s->cooldown>0)return 0;
  if(h->throw_active(h->owner)&&h->attention(h->owner))return 0;
  if(h->throw_predicate(h->owner))return 0;
  if(h->throw_ready(h->owner))h->throw_target(h->owner,s->target,2000.f,5.f,0);
  if(pending(h,s->aim_task))return 0;
  h->stop(h->owner);h->aim(h->owner,s->aim,0,0);close=10.f>s->distance;
  if(!h->throw_request(h->owner,close,0))return 0;
 }else{
  if(!imminent)return 0;close=10.f>s->distance;if(!h->throw_request(h->owner,close,1))return 0;
 }
 s->cooldown=h->reset_cooldown(h->owner);s->threw=1;return 0;
}

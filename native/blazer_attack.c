#include "blazer_attack.h"
#include <math.h>
int uc_blazer_attack_idle(UCBlazerAttackIdle*s,const UCBlazerAttackIdleServices*h){
 float point[3];uint8_t near;
 if(!s||!h||!h->target||!h->transition||!h->reset_navigation||!h->stop||
    !h->nav_status||!h->waypoint||!h->near_goal||!h->move||!h->stop_aim)return 1;
 if(!s->entered){s->entered=1;h->reset_navigation(h->owner);h->stop(h->owner);s->route=0;s->route_entity=-1;}
 if(h->target(h->owner)!=-1){h->transition(h->owner,2);return 0;}
 if(h->nav_status(h->owner)==2){h->waypoint(h->owner,point);near=h->near_goal(h->owner);h->move(h->owner,point,0,near);}
 else h->stop(h->owner);
 h->stop_aim(h->owner);return 0;
}
int uc_blazer_attack_new_target(UCBlazerAttackIdle*s,uint8_t*first,int completion,const UCBlazerAttackIdleServices*h){
 if(!s||!first||!h||!h->reset_navigation||!h->stop||!h->stop_aim||!h->target||!h->transition)return 1;
 if(!s->entered){h->reset_navigation(h->owner);h->stop(h->owner);h->stop_aim(h->owner);*first=0;s->entered=1;}
 if(completion==0)return 0;h->transition(h->owner,h->target(h->owner)==-1?1:4);return 0;
}
int uc_blazer_attack_move(UCBlazerAttackMove*s,const UCBlazerAttackMoveServices*h){
 float x,y,z,square;
 if(!s||!h||!h->player||!h->dialogue||!h->paired_dialogue||!h->retarget||!h->run||!h->transition)return 1;
 if(!s->entered){
  if(s->defend&&!s->dont_move)h->retarget(h->owner,s->defend_position);
  else if(!s->in_cone){if(s->target==h->player(h->owner))h->paired_dialogue(h->owner,33,111,.5f,50);else h->dialogue(h->owner,33,50);}
  else if(s->distance<2000.f){if(s->target==h->player(h->owner))h->paired_dialogue(h->owner,34,110,.5f,50);else h->dialogue(h->owner,32,50);}
  s->entered=1;
 }
 if(s->position_changed){s->position_changed=0;if(s->position_ready)h->retarget(h->owner,s->position);}
 if(!s->defend){x=s->position[0]-s->target_position[0];y=s->position[1]-s->target_position[1];z=s->position[2]-s->target_position[2];square=(z*z+x*x)+y*y;
  if(square<10000.f){s->position_ready=0;h->transition(h->owner,4);return 0;}
  if(s->elapsed>5.f){h->transition(h->owner,4);return 0;}
 }
 {int32_t status=h->run(h->owner);if(status<=0||status>2)h->transition(h->owner,4);}
 return 0;
}
int uc_blazer_target_cue(UCBlazerTargetCue*s,const UCBlazerTargetCueServices*h,uint8_t shield){
 uint8_t spoke;float health,high;
 if(!s||!h||!h->weapon_ready||!h->valid||!h->player||!h->say||!h->pair||!h->health||!h->maximum||!h->random)return 1;
 if(s->remaining>0||isnan(s->remaining)||!h->weapon_ready(h->owner))return 0;
 if(shield)spoke=h->say(h->owner,20,25);
 else {if(!h->valid(h->owner,s->target))s->target=-1;
  if(s->target==h->player(h->owner))spoke=h->pair(h->owner,19,109,25.f,50);
  else spoke=h->say(h->owner,19,25);}
 if(spoke){health=(float)h->health(h->owner);high=(float)(((double)health/h->maximum(h->owner))*15.+5.);s->remaining=(float)h->random(h->owner,5,high);}
 return 0;
}
int uc_blazer_evade(UCBlazerEvade*s,const UCBlazerEvadeServices*h){
 if(!s||!h||!h->random_side||!h->right||!h->clear||!h->inside_defend||!h->player||!h->pair||
    !h->task_create||!h->approach||!h->aim||!h->attention||!h->look||!h->task_status||!h->transition)return 1;
 if(!s->entered){float displacement[3],point[3],scale=h->random_side(h->owner)?-1.f:1.f;
  h->right(h->owner,displacement);scale*=100.f;displacement[0]*=scale;displacement[1]=scale*0.f;displacement[2]*=scale;
  unsigned attempt;for(attempt=0;attempt<2;attempt++){
   for(unsigned j=0;j<3;j++)point[j]=s->position[j]+displacement[j];
   int clear=h->clear(h->owner,point);if(clear<0)return 2;
   if(clear&&h->inside_defend(h->owner,point))break;
   if(!attempt)for(unsigned j=0;j<3;j++)displacement[j]*=-1.f;
  }
  if(attempt==2){h->transition(h->owner,7);return 0;}
  if(s->target==h->player(h->owner)&&s->near_line)h->pair(h->owner,38,113,.5f,50);
  for(unsigned j=0;j<3;j++)displacement[j]=(float)((double)displacement[j]*.6);
  uintptr_t callback=h->task_create(h->owner,&s->task);
  for(unsigned j=0;j<3;j++)point[j]=s->position[j]+displacement[j];
  h->approach(h->owner,point,s->direction,callback,.6f);s->entered=1;
 }
 h->aim(h->owner,s->aim);if(!h->attention(h->owner))h->look(h->owner,s->aim);
 if(!s->task||h->task_status(h->owner,s->task)!=0)h->transition(h->owner,4);
 return 0;
}
int uc_blazer_duck(UCBlazerDuck*s,const UCBlazerDuckServices*h){
 if(!s||!h||!h->status||!h->release||!h->create||!h->duck||!h->stop||!h->player||!h->pair||!h->health_cue||!h->bullet_by||!h->difficulty||!h->transition)return 1;
 if(s->throw_task&&h->status(h->owner,s->throw_task)==0){
  if(!s->thrown)return 0;h->release(h->owner,s->throw_task);s->throw_task=0;s->delay=.2f;
 }
 if(s->delay>0){s->delay=(float)((double)s->delay-h->delta);return 0;}
 if(!s->entered){s->entered=1;
  if(!s->duck_task||h->status(h->owner,s->duck_task)!=1){
   uintptr_t callback=h->create(h->owner,1,&s->duck_task);h->duck(h->owner,callback);
   if(s->target==h->player(h->owner)&&s->near_line)h->pair(h->owner,38,113,.5f,50);
  }
  s->quiet=0;
 }
 if(s->duck_task&&h->status(h->owner,s->duck_task)==0)return 0;
 if(s->stop_task&&h->status(h->owner,s->stop_task)==0)return 0;
 h->health_cue(h->owner);if(h->bullet_by(h->owner))s->quiet=0;else s->quiet=(float)((double)s->quiet+h->delta);
 if(!((double)s->quiet>h->difficulty(h->owner,1.5f,.5f)))return 0;
 if(s->duck_task&&h->status(h->owner,s->duck_task)==1){h->release(h->owner,s->duck_task);s->duck_task=0;
  uintptr_t callback=h->create(h->owner,2,&s->stop_task);h->stop(h->owner,callback);}
 h->transition(h->owner,4);return 0;
}

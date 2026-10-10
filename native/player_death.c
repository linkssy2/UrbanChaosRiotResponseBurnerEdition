#include "player_death.h"
#include <math.h>
int uc_player_death_begin(UCPlayerDeath*s,int32_t killer,const UCPlayerFireDamage*d,const UCPlayerDeathServices*h){
 if(!s||!d||!h||!h->flashlight_off||!h->cleanup||!h->health_zero||!h->stop_input||!h->lower||!h->input_velocity||
 !h->sequence_count||!h->sequence||!h->create_sequence||!h->character_type||!h->npc_count||!h->npc||!h->position||
 !h->sequence_event||!h->broadcast||!h->notify||!h->profile_died||!h->adaptive_death||!h->transition||!h->notice)return 1;
 if(h->flashlight_off(h->owner))return 2;
 if(s->action==12){s->remaining=3;return 0;}
 if(h->cleanup(h->owner)||h->health_zero(h->owner))return 2;
 s->input_timer=0;s->move_dirty=s->look_dirty=1;s->input_range=53;s->interaction=0;
 if(s->input_present&&h->stop_input(h->owner))return 2;
 if(!s->lowered&&s->animation!=-1&&h->lower(h->owner))return 2;
 if(s->input_present){float velocity[3];if(h->input_velocity(h->owner,velocity))return 2;}
 s->heartbeat=s->heartbeat_interval=.3f;s->second_heartbeat=.5f;s->killer=killer;s->remaining=3;
 uint32_t count;if(h->sequence_count(h->owner,&count)||count>4096)return 2;
 if(!count&&h->create_sequence(h->owner))return 2;
 if(h->sequence_count(h->owner,&count)||count>4096)return 2;
 if(count){uint32_t selected=0;
  if(h->sequence_count(h->owner,&count)||count>4096)return 2;
  if(count>1){
   for(uint32_t i=0;i<4096;i++){
    if(h->sequence_count(h->owner,&count)||count>4096)return 2;if(i>=count)break;
    int32_t id,type;if(h->sequence(h->owner,i,&id)||h->character_type(h->owner,id,&type))return 2;
    if(type==1){selected=i;continue;} /*fallback only; a later nearby matching actor wins.*/
    uint32_t n;if(h->npc_count(h->owner,&n)||n>4096)return 2;uint8_t found=0;
    for(uint32_t k=0;k<n;k++){
     int32_t npc,nt;if(h->npc(h->owner,k,&npc)||h->character_type(h->owner,npc,&nt))return 2;
     if(nt==type){float p[3];uint8_t live;if(h->position(h->owner,npc,p,&live))return 2;
      if(live){double x=(double)s->position[0]-p[0],y=(double)s->position[1]-p[1],z=(double)s->position[2]-p[2];
       if(2000.>sqrt((z*z+y*y)+x*x)){selected=i;found=1;break;}}}
     if(h->npc_count(h->owner,&n)||n>4096)return 2;
    }
    if(found)break;
   }
  }
  int32_t id;if(h->sequence(h->owner,selected,&id)||h->sequence_event(h->owner,id,s->view,s->id))return 2;
 }
 if(h->broadcast(h->owner,s->view,d->owner,s->id)||h->notify(h->owner,s->id,killer,d)||h->profile_died(h->owner)||
 h->adaptive_death(h->owner)||h->transition(h->owner,12,0))return 2;
 s->action=12;s->stage=0;return h->notice(h->owner,"Died",1.f)?2:0;
}
int uc_player_death_step(UCPlayerDeath*s,const UCPlayerDeathStepServices*h){
 if(!s||!h||!h->motion||!h->delta||!h->heartbeat||!h->feedback)return 1;
 if(h->motion(h->owner))return 2;s->move_dirty=s->look_dirty=0;
 if(!(2.5f>s->heartbeat_interval))return 0;
 double delta=h->delta(h->owner);if(!isfinite(delta)||delta<0)return 2;
 double next=(double)s->heartbeat-delta;s->heartbeat=(float)next;
 if(0.>next){s->heartbeat_interval+=.2f;s->heartbeat=s->heartbeat_interval;s->second_heartbeat=.2f;
  if(h->heartbeat(h->owner)||h->feedback(h->owner,s->view,.1f,.1f,2))return 2;}
 delta=h->delta(h->owner);if(!isfinite(delta)||delta<0)return 2;next=(double)s->second_heartbeat-delta;s->second_heartbeat=(float)next;
 if(0.>next){s->second_heartbeat=s->heartbeat+.2f;if(h->feedback(h->owner,s->view,.1f,.1f,2))return 2;}
 return 0;
}

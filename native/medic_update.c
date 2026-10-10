#include "medic_update.h"
#include <math.h>
#include <stdio.h>
#define CALL(f,...) do{if(!h->f||h->f(h->owner,__VA_ARGS__))return 2;}while(0)
#define CALL0(f) do{if(!h->f||h->f(h->owner))return 2;}while(0)
static int medic_prefix(UCMedicUpdate*s,UCMedicRootPolicy*p,const UCMedicUpdateServices*h){
 if((s->reactions&0x1fffe)&&(s->reactions&0x40000)&&!(*s->status&1))CALL0(unrestrain);
 if(s->state==0||s->state==1||s->state==2){
  uint8_t ready;CALL(children_ready,&ready);if(ready)s->finished=1;
  if(s->finished){int32_t foreground;CALL(foreground,&foreground);if(foreground==-1){
   CALL(complete,s->order,s->state,s->root);s->state=0;s->finished=0;
  }}
 }else s->finished=0;
 CALL(reactions,1,1);
 if(!(*s->status&0x20)&&(*s->status&5))return 1;
 if(*s->status&0x20007)return 3;
 int32_t heals;CALL(integer,"heals left",0,&heals);
 char label[80];snprintf(label,sizeof label,s->can_heal?"Heals: %d":"Heals: %d (cant heal)",heals);
 CALL0(clear_debug);CALL(debug,label);
 if(p->put_down){uint8_t ready;CALL(body_ready,&ready);if(ready){int32_t foreground;CALL(foreground,&foreground);CALL(drop_allowed,foreground,&ready);}if(!ready)p->put_down=0;}
 if(p->put_down||p->pick_up){CALL(box_event,p->put_down,s->root);p->pick_up=p->put_down=0;}
 return 0;
}
int uc_medic_unassigned_update(UCMedicUpdate*s,UCMedicRootPolicy*p,float delta,const UCMedicUpdateServices*h){
 if(!s||!p||!h||!s->status||s->order!=-1||!isfinite(delta)||delta<0)return 1;
 int result=medic_prefix(s,p,h);if(result)return result==3?0:result;
 CALL(secondary,p->secondary,delta);CALL(base,delta);return 0;
}
int uc_medic_plain_update(UCMedicUpdate*s,UCMedicRootPolicy*p,int32_t speech,float delta,const UCMedicUpdateServices*h){
 if(!s||!p||!h||!s->status||!isfinite(delta)||delta<0)return 1;
 if((s->reactions&0x1fffe)&&(s->reactions&0x40000)&&!(*s->status&1))CALL0(unrestrain);
 CALL(reactions,1,1);
 if(!(*s->status&0x20)&&(*s->status&5))return 1;
 if(!(*s->status&0x20007)){
  int32_t heals;CALL(integer,"heals left",0,&heals);char label[80];snprintf(label,sizeof label,s->can_heal?"Heals: %d":"Heals: %d (cant heal)",heals);
  CALL0(clear_debug);CALL(debug,label);
  if(p->put_down){uint8_t ready;CALL(body_ready,&ready);if(ready){int32_t foreground;CALL(foreground,&foreground);CALL(drop_allowed,foreground,&ready);}if(!ready)p->put_down=0;}
  if(p->put_down||p->pick_up){CALL(box_event,p->put_down,s->root);p->pick_up=p->put_down=0;}
  uint8_t busy;CALL(foreground_busy,&busy);if(!busy){uint8_t run=0;if(speech!=-1){CALL(should_run,speech,&run);}CALL(secondary,run?speech:p->secondary,delta);}
 }
 CALL(base,delta);return 0;
}
#define TEAM(f,...) do{if(!t->f||t->f(t->owner,__VA_ARGS__))return 2;}while(0)
#define TEAM0(f) do{if(!t->f||t->f(t->owner))return 2;}while(0)
int uc_medic_team_update(UCMedicUpdate*s,UCMedicRootPolicy*p,UCMedicTeamUpdate*q,float delta,
 const UCMedicUpdateServices*h,const UCMedicTeamUpdateServices*t){
 if(!s||!p||!q||!h||!t||!s->status||!q->actor_position||q->quit_count<0||
    (q->quit_count&&!q->quit_flags)||!isfinite(delta)||delta<0)return 1;
 int result=medic_prefix(s,p,h);if(result)return result==3?0:result;
 if(s->order!=-1&&q->quit_count){uint8_t ready;TEAM(flags_ready,q->quit_count,q->quit_flags,&ready);
  if(ready){TEAM0(heartbeat_remove);TEAM(client_remove,s->order);
   if(q->quit_behaviour!=-1)TEAM(replace_authored,q->quit_behaviour);
   s->order=q->quit_behaviour=-1;q->quit_count=0;TEAM0(clear_flags);q->quit_flags=0;s->state=0;TEAM0(clear_children);
  }
 }
 if(s->order==-1){CALL(secondary,p->secondary,delta);CALL(base,delta);return 0;}
 if(s->state==0||s->state==1||s->state==2){
  if(*s->status&0x10)q->pending_attack=1;
  uint8_t danger=q->bullet_by!=0;
  if(!danger)TEAM(weapon_threat,600.f,&danger);
  if(!danger)TEAM(bullet_near,300.f,&danger);
  if(danger||q->pending_attack){uint8_t allowed=1;
   if(s->state==1){float player[3];uint8_t found;TEAM(player_position,player,&found);
    if(found){float x=q->actor_position[0]-player[0],y=q->actor_position[1]-player[1],z=q->actor_position[2]-player[2];
     float squared=(x*x+y*y)+z*z;if(squared>250000.f)allowed=0;}}
   if(s->state==1||s->state==2){uint8_t busy;TEAM(movement_busy,&busy);if(busy)allowed=0;}
   if(allowed){if(*s->status&0x80){TEAM(attack_event,q->under_attack);}
    else{uint8_t busy;CALL(foreground_busy,&busy);if(!busy){TEAM(attack_create,&q->under_attack);q->pending_attack=0;}}
   }
  }
 }
 if(!(*s->status&2)){
  if(s->state==3||s->state==4){int32_t foreground;CALL(foreground,&foreground);
   if(foreground==-1){TEAM(emote,12,50);TEAM(choose,s->order,1);}}
  int32_t command=0,value;uint8_t found;TEAM(order_integer,s->order,"current order",&value,&found);if(found)command=value;
  if((*s->status&0x80)&&command)s->state=0;
  if(command==1){if(s->state)TEAM(emote,17,50);
   if(s->state==command){int32_t foreground;CALL(foreground,&foreground);TEAM(resume_follow,foreground);}
   else{TEAM(replace_order,command,-1);s->state=command;}}
  else if(command==2&&s->state!=command){TEAM(emote,16,50);TEAM(replace_order,command,-1);s->state=command;}
  else if(command==3&&s->state!=command){TEAM(order_integer,s->order,"object id",&value,&found);
   if(found){TEAM(emote,11,50);TEAM(replace_order,command,value);s->state=command;}}
  else if(command==4&&s->state!=command){TEAM(emote,11,50);TEAM(replace_order,command,-1);s->state=command;}
  else if(command==7){TEAM(emote,13,50);TEAM(order_integer,s->order,"object id",&value,&found);TEAM(replace_order,command,found?value:-1);s->state=command;}
  TEAM(order_complete,s->order);
 }
 CALL(base,delta);return 0;
}

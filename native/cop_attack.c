#include "cop_attack.h"
#include <math.h>
#include <string.h>
static void crouch(UCCopAttack*s,const UCCopAttackServices*h){h->crouch(h->owner);s->crouching=1;s->crouch_timer=h->random(h->owner,3,6);}
static void stop(UCCopAttack*s,const UCCopAttackServices*h){h->stop(h->owner);s->crouching=0;}
int uc_cop_attack_update(UCCopAttack*s,const UCCopAttackServices*h){
 if(!s||!h||!s->world||!s->actor_world||!isfinite(h->delta)||h->delta<0||!h->stance_busy||!h->target||
 !h->player_position||!h->dialogue_mode||!h->dialogue||!h->search_cover||!h->nav_status||!h->waypoint||
 !h->near_goal||!h->stop_aim||!h->move||!h->crouch||!h->stop||!h->random||!h->gun_target||
 !h->stand_if_crouching||!h->clear_target||!h->stance_mode||!h->idle_look||!h->conditions)return 1;
 if(!isfinite(s->crouch_timer)||!isfinite(s->unseen)||!isfinite(s->no_target))return 1;
 for(unsigned i=0;i<16;i++)if(!isfinite(s->actor_world[i]))return 1;
 uint8_t busy=h->stance_busy(h->owner);
 if(h->target(h->owner)==-1)s->no_target=(float)(h->delta+s->no_target);else s->no_target=0;
 if(s->no_target>10){const float*p=h->player_position(h->owner);if(p){const float*a=s->actor_world+12;
  float y=(float)((double)p[1]-a[1]),z=p[2]-a[2],x=p[0]-a[0];
  if(fabs((double)y)<100&&(x*x+y*y)+z*z<250000){int32_t kind=h->dialogue_mode(h->owner);if(kind>=0&&kind<=2)h->dialogue(h->owner,kind+2,25);s->no_target=0;}}}
 switch(s->state){
 case 0:h->search_cover(h->owner);break;
 case 1:
  if(s->cover==-1)s->state=2;
  if(h->nav_status(h->owner)==4){crouch(s,h);s->state=2;}
  else{float p[3];h->waypoint(h->owner,p);h->stop_aim(h->owner);uint8_t near=h->near_goal(h->owner);h->move(h->owner,p,0,near);}break;
 case 2:
  if(s->cover!=-1){uint8_t visible=0;
   if(h->target(h->owner)!=-1)visible=h->gun_target(h->owner,h->target(h->owner));
   if(h->target(h->owner)==-1&&s->unseen>4){if(s->unseen>12){if(s->crouching)stop(s,h);}else if(!s->crouching)crouch(s,h);}
   else{
    if(!s->crouching){if(busy)crouch(s,h);}
    else if(s->crouch_timer<=0&&!busy&&h->target(h->owner)!=-1&&!visible)stop(s,h);
   }
   if(s->crouching&&h->target(h->owner)!=-1&&s->unseen>3)s->crouch_timer=0;
   if(h->stand_if_crouching(h->owner))s->crouch_timer=0;
   s->crouch_timer=(float)((double)s->crouch_timer-h->delta);
   if(!visible)s->unseen=(float)(h->delta+s->unseen);else s->unseen=0;
   if(!s->crouching&&s->unseen>=2){if(h->target(h->owner)!=-1)h->clear_target(h->owner);s->unseen=0;}
   if(s->unseen>5&&h->target(h->owner)!=-1)s->state=0;
  }
  if(h->stance_mode(h->owner)==0||h->stance_mode(h->owner)==4)h->idle_look(h->owner);
  break;
 default:break;
 }
 memcpy(s->world,s->actor_world,64);h->conditions(h->owner);return 0;
}

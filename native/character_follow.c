#include "character_follow.h"
#include <math.h>
#include <string.h>
int uc_character_follow_update(UCCharacterFollow*s,const UCCharacterFollowServices*h){
 if(!s||!h||!s->world||!s->actor_world||!h->position||!h->transition||!h->retarget||!h->status||
 !h->waypoint||!h->near_goal||!h->stop_aim||!h->move||!h->stop||!h->aim||!h->property||
 !h->side_step||!h->current_target||!h->idle_look||!isfinite(s->stop_range))return 1;
 for(unsigned i=0;i<16;i++)if(!isfinite(s->actor_world[i]))return 1;
 for(unsigned i=0;i<3;i++)if(!isfinite(s->last_target[i]))return 1;
 const float*t=h->position(h->owner,s->target);if(!t){h->transition(h->owner,s->next);return 0;}
 const float*p=s->actor_world+12;float d[3]={t[0]-p[0],t[1]-p[1],t[2]-p[2]};
 if(s->stop_range>0){float squared=(d[0]*d[0]+d[1]*d[1])+d[2]*d[2];
  if(s->stop_range>sqrt((double)squared)){h->transition(h->owner,s->next);return 0;}}
 double length=sqrt(((double)d[0]*d[0]+(double)d[1]*d[1])+(double)d[2]*d[2]);
 if(length>=(s->moving?200.:300.)||fabs((double)d[1])>150.){
  float drift[3];for(unsigned i=0;i<3;i++)drift[i]=d[i]-(s->last_target[i]-p[i]);
  double distance=sqrt(((double)drift[0]*drift[0]+(double)drift[2]*drift[2])+(double)drift[1]*drift[1]);
  if(distance>200.){h->retarget(h->owner,t);memcpy(s->last_target,t,12);}
  int32_t status=h->status(h->owner);
  if(status==1||status==2){float point[3];h->waypoint(h->owner,point);s->moving=1;h->stop_aim(h->owner);
   uint8_t near=h->near_goal(h->owner);h->move(h->owner,point,s->run?0:1,near);
  }else if(status==3||status==4){float point[3]={t[0],t[1]+140.f,t[2]};h->stop(h->owner);h->aim(h->owner,point);}
 }else{
  if(s->moving){h->stop(h->owner);s->moving=0;}
  if(s->side_step&&!h->property(h->owner,"should duck",0)&&h->property(h->owner,"should sidestep",1))h->side_step(h->owner);
  if(h->current_target(h->owner)==-1)h->idle_look(h->owner);
 }
 memcpy(s->world,s->actor_world,64);return 0;
}
/* Float subtractions are retained before source x87 double length arithmetic. */
static double player_length(float x,float y,float z){return sqrt((double)x*x+(double)z*z+(double)y*y);}
int uc_character_player_follow_update(UCCharacterPlayerFollow*s,const UCCharacterPlayerFollowServices*h){
 if(!s||!h||!h->delta||!h->player||!h->actor||!h->side_step_tick||!h->retarget||!h->status||!h->waypoint||!h->near_goal||!h->stop_aim||!h->move||!h->stop||!h->look||!h->property||!h->side_step_run||!h->body_aiming||!h->target||!h->idle_look||!h->target_head||!h->random_pair||!h->target_kind||!h->player_shield||!h->character_type||!h->chance||!h->emote||!h->base)return 1;
 if(s->delay>0){s->delay=(float)((double)s->delay-h->delta(h->owner));if(s->delay<0)s->delay=0;h->base(h->owner);return 0;}
 const float*p=h->player(h->owner);if(!p)return 0;
 h->side_step_tick(h->owner);const float*a=h->actor(h->owner);if(!a)return 1;
 float x=p[0]-a[0],y=p[1]-a[1],z=p[2]-a[2];
 const double distance=sqrt(((double)x*x+(double)y*y)+(double)z*z);
 if(distance>=(s->moving?300.:400.)||fabs((double)y)>150.){
  float dx=x-(s->last_target[0]-a[0]),dy=y-(s->last_target[1]-a[1]),dz=z-(s->last_target[2]-a[2]);
  if(player_length(dx,dy,dz)>200.||s->forced){h->retarget(h->owner,p);for(unsigned i=0;i<3;i++)s->last_target[i]=p[i];}
  int status=h->status(h->owner);if(status==1||status==2){float waypoint[3];h->waypoint(h->owner,waypoint);s->moving=1;h->stop_aim(h->owner);uint8_t near=h->near_goal(h->owner);h->move(h->owner,waypoint,0,near);}
  else if(status==3||status==4){h->stop(h->owner);h->look(h->owner,p,1);}
 }else{
  if(s->moving){s->idle_time=0;s->turned=0;h->stop(h->owner);s->moving=0;}
  if(s->forced)h->side_step_run(h->owner,1);
  else if(s->side_step&&!h->property(h->owner,"should duck",0)&&(!s->check_side_step||h->property(h->owner,"should sidestep",1)))h->side_step_run(h->owner,0);
  if(!h->body_aiming(h->owner)&&h->target(h->owner)==-1)h->idle_look(h->owner);
  double elapsed=h->delta(h->owner)+(double)s->idle_time;s->idle_time=(float)elapsed;
  if(elapsed>5.&&!s->turned){s->turned=1;const float*t=h->target_head(h->owner);if(t)h->look(h->owner,t,0);}
  if(s->idle_time>20.f){int code=-1;if(h->random_pair(h->owner)==0)code=6;
   else switch(h->target_kind(h->owner)){case 1:code=3;break;case 2:code=4;break;case 0:code=h->player_shield(h->owner)&&h->character_type(h->owner)==2&&h->chance(h->owner,.1f)?0x15:2;break;}
   if(code!=-1)h->emote(h->owner,code,50);s->idle_time=0;
  }
 }
 s->forced=0;h->base(h->owner);return 0;
}

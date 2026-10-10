#include "brute_chase.h"
#include <math.h>
#include <string.h>
static void point_copy(float*a,const float*b){memcpy(a,b,3*sizeof(float));}
int uc_brute_side_reset(UCBruteSide*s,const UCBruteChaseServices*h){
 if(!s||!h||!h->difficulty_range)return 1;
 float value=(float)h->difficulty_range(h->owner,0,.2f,.25f,1);
 s->remaining=s->duration=value>0?value:0;return 0;
}
int uc_brute_side_step(UCBruteChase*s,const UCBruteChaseServices*h,float distance,uint8_t*active){
 if(!s||!h||!active||!h->nav_final||!h->difficulty||!h->target_line||!h->player||!h->pair||!h->target_world)return 1;
 *active=0;if(1.f>s->elapsed)return 0;
 int value=h->nav_final(h->owner);if(value<0)return 2;if(value||s->charge)return 0;
 if(.25>h->difficulty(h->owner,0,1))return 0;
 value=h->target_line(h->owner,s->target,50);if(value<0)return 2;
 UCBruteSide*t=&s->side;
 if(value){t->near_time=(float)(h->delta+t->near_time);t->away_time=0;}
 else {t->near_time=0;t->away_time=(float)(h->delta+t->away_time);}
 if(t->near_line){if(t->away_time>.15f){t->near_line=0;t->cooldown=1;}}
 else if(t->cooldown<=0&&t->near_time>.35f){
  if(s->target==h->player(h->owner)&&s->near_line)h->pair(h->owner,38,113,.5f,50);
  t->near_line=1;
 }
 if(t->cooldown>0)t->cooldown=(float)((double)t->cooldown-h->delta);
 if(distance>350.f&&t->near_line){int result=uc_brute_side_reset(t,h);if(result)return result;}
 else t->remaining=(float)((double)t->remaining-h->delta);
 if(!(t->remaining>0)||!(distance>350.f))return 0;
 float world[16];if(h->target_world(h->owner,s->target,world))return 2;
 float dx=world[12]-s->position[0],dz=world[14]-s->position[2];
 float dot=(dz*world[8]+dx*(0.f-world[10]))+0.f;
 t->left=dot>0;*active=1;return 0;
}
int uc_brute_chase_step(UCBruteChase*s,const UCBruteChaseServices*h){
 if(!s||!h||!h->retarget||!h->random_boolean||!h->movement_mode||!h->say||!h->transition||
    !h->visible||!h->charge_allowed||!h->aim||!h->stop_aim||!h->attention||!h->look||!h->stop_look||
    !h->nav_status||!h->navigation_busy||!h->nav_stop||!h->acquire||!h->evade_allowed||!h->waypoint||
    !h->near_goal||!h->move||!h->route_clear||!h->difficulty_range)return 1;
 if(!s->entered){h->retarget(h->owner,s->target_position);point_copy(s->destination,s->target_position);
  s->charge=s->charge_enabled?h->random_boolean(h->owner):0;s->entered=1;}
 h->movement_mode(h->owner,s->distance<1500.f?0:1);
 if(s->distance<480.000030517578125f)h->say(h->owner,47,50);
 if(s->special_close){if(s->distance<400.f){h->transition(h->owner,5);return 0;}}
 else if(fabs((double)s->target_position[1]-s->position[1])<100.){
  int visible=h->visible(h->owner,s->target);if(visible<0)return 2;
  if(visible){
   if(s->charge){int allowed=h->charge_allowed(h->owner);if(allowed<0)return 2;
    if(allowed){h->transition(h->owner,6);return 0;}}
   if(s->distance<250.f){h->transition(h->owner,3);return 0;}
  }
 }
 if(s->facing_dot<.8f)h->stop_aim(h->owner);else h->aim(h->owner,s->aim);
 if(!h->attention(h->owner)){if(s->seen_time>1.f)h->look(h->owner,s->aim);else h->stop_look(h->owner);}
 if(h->nav_status(h->owner)==0||s->target_moved){point_copy(s->destination,s->target_position);h->retarget(h->owner,s->destination);}
 h->navigation_busy(h->owner,s->destination);
 if(h->nav_status(h->owner)==3){point_copy(s->destination,s->position);h->nav_stop(h->owner);h->acquire(h->owner,1,1);h->transition(h->owner,7);return 0;}
 int evade=h->evade_allowed(h->owner);if(evade<0)return 2;if(evade){h->transition(h->owner,4);return 0;}
 float point[3];h->waypoint(h->owner,point);
 float x=point[0]-s->position[0],z=point[2]-s->position[2],square=z*z+x*x;
 float distance=(float)sqrt((double)square);
 float tx=point[0]-s->target_position[0],ty=point[1]-s->target_position[1],tz=point[2]-s->target_position[2];
 if((tx*tx+ty*ty)+tz*tz<100.f){uint8_t side=0;int result=uc_brute_side_step(s,h,distance,&side);if(result)return result;
  if(side){double inv=1./sqrt((double)square);
   float direction[3]={(float)(x*inv),(float)(0.f*inv),(float)(z*inv)};
   float perpendicular[3]={s->side.left?0.f-direction[2]:direction[2],0,s->side.left?direction[0]:0.f-direction[0]};
   float ratio=s->side.remaining/s->side.duration;
   /*3A8320/3A8330 initialize the angle endpoints:sin(0),sin(.4363323152065277).*/
   float scale=(float)h->difficulty_range(h->owner,0,.422618269920349121f,.25f,.6f);if(!(scale>0))scale=0;
   scale=((scale*ratio)*distance)*2.f;
   float v[3];for(unsigned i=0;i<3;i++)v[i]=perpendicular[i]*scale+direction[i]*distance;
   float inv_length=(float)(1./sqrt((double)((v[2]*v[2]+v[1]*v[1])+v[0]*v[0])));
   float length=ratio*400.f;if(!(length>120.f))length=120.f;
   for(unsigned i=0;i<3;i++)v[i]=s->position[i]+(v[i]*inv_length)*length;
   int clear=h->route_clear(h->owner,v);if(clear<0)return 2;if(clear)point_copy(point,v);
  }
 }
 h->move(h->owner,point,0,h->near_goal(h->owner));return 0;
}

#include "blazer_throw.h"
#include <math.h>
int uc_blazer_throw_prepare(UCBlazerThrow*s,const UCBlazerThrowServices*h){
 uintptr_t callback;uint8_t overlay;const char*name;
 if(!s||!h||!h->active||!h->stop||!h->task_status||!h->task_create||!h->play||!h->prime||!h->target||!h->set_target)return -1;
 if(!h->active(h->owner))return 0;h->stop(h->owner);
 overlay=(uint8_t)(s->aim_task&&!h->task_status(h->owner,s->aim_task));
 name=660.f>s->distance?(overlay?"GetIgniteProjectileQuickOverlay":"GetIgniteProjectileQuick"):(overlay?"GetIgniteProjectileOverlay":"GetIgniteProjectile");
 callback=h->task_create(h->owner,&s->wait_task);h->play(h->owner,name,callback,overlay);
 h->prime(h->owner);h->set_target(h->owner,h->target(h->owner),1100.f,5.f,0);s->melee_enabled=1;return 1;
}
int uc_blazer_throw_request(UCBlazerThrow*s,const UCBlazerThrowServices*h,uint8_t close,uint8_t alternate){
 float dx,dy,dz,square,speed;uint32_t flags=0;const char *clip,*sound;uintptr_t callback;
 if(!s||!h||!h->active||!h->ready||!h->random||!h->difficulty||!h->can_throw||!h->task_create||!h->play||!h->sound||
 !h->throw_target||!h->throw_point||!h->speech||!h->speech_pair||!h->primary_target)return -1;
 if(h->active(h->owner)||h->ready(h->owner)||(s->actor_flags&16)||.87f>s->facing_dot)return 0;
 dx=s->target_position[0]-s->position[0];dy=s->target_position[1]-s->position[1];dz=s->target_position[2]-s->position[2];square=dz*dz+dx*dx;
 if(10000.f>square&&fabs((double)dy)>150.)return 0;
 if(s->shield_time>=3.f&&h->random(h->owner,0,1)>.33f&&h->difficulty(h->owner,0,1)>.25f)flags=4;
 if(s->near_line&&h->random(h->owner,0,1)>.9f&&h->difficulty(h->owner,0,1)>.5f)flags|=4;
 if(!s->throw_low&&!(1500.f>=s->distance)){
  if(2500.f<s->distance){speed=1100.f;clip="ThrowFar";sound="ThrowLong";}
  else {speed=1600.f;clip=sound="ThrowMiddle";}
 }else {speed=2000.f;clip=sound="ThrowMiddle";}
 if(!close&&!h->can_throw(h->owner,s->target,speed))return 0;
 callback=h->task_create(h->owner,&s->side_task);h->play(h->owner,clip,callback,0);h->sound(h->owner,sound);
 if(alternate)h->throw_point(h->owner,s->last_clear,speed,flags);else h->throw_target(h->owner,s->target,speed,flags);
 if(flags&4)h->speech(h->owner,30,50);
 else if(h->primary_target(h->owner)==s->target)h->speech_pair(h->owner,37,112,.5f,50);
 else h->speech(h->owner,36,50);
 return 1;
}
int uc_blazer_throw_cooldown(UCBlazerThrow*s,const UCBlazerThrowServices*h){
 float low,high;if(!s||!h||!h->difficulty3||!h->random)return 1;
 low=(float)h->difficulty3(h->owner,4.f,.2f,0.f);high=(float)h->difficulty3(h->owner,6.f,1.5f,.1f);
 s->cooldown=(float)h->random(h->owner,low,high);return 0;
}
int uc_blazer_throw_difficulty(const UCGameDifficulty*s,float channel,double*out){
 static const float low[]={0,.2f,.5f,.8f},high[]={.5f,.8f,1,1};double session;
 if(!s||!out||!isfinite(channel)||uc_game_difficulty_value(s,&session))return 1;
 *out=(session+(((double)high[s->mode]-low[s->mode])*channel+low[s->mode]))*.5;return 0;
}
int uc_blazer_throw_curve(const UCGameDifficulty*s,float channel,float low,float middle,float high,double*out){
 double value,x;float a,b,stored;if(!out||!isfinite(low)||!isfinite(middle)||!isfinite(high)||uc_blazer_throw_difficulty(s,channel,&value))return 1;
 if(channel>.5f){a=middle;b=high;x=(value-.5)/.5;}else{a=low;b=middle;x=value/.5;}
 if(!(1.>x)){*out=b;return 0;}stored=(float)x;*out=stored>0?((double)b-a)*stored+a:a;return 0;
}
int uc_blazer_throw_range(const UCGameDifficulty*s,float channel,float low,float high,float lower,float upper,double*out){
 double value;if(!out||!isfinite(low)||!isfinite(high)||!isfinite(lower)||!isfinite(upper)||lower==upper||uc_blazer_throw_difficulty(s,channel,&value))return 1;
 double fraction=(value-lower)/((double)upper-lower);float stored=(float)fraction;
 if(!(1.>fraction))stored=1;else if(!(stored>0))stored=0;
 *out=((double)high-low)*stored+low;return 0;
}

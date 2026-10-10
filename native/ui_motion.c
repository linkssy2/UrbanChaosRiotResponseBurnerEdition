#include "ui_motion.h"
#include <math.h>
#include <string.h>
static double curve(float t){if(t<=0)return 0;if(t>=1)return 1;return sin((double)t*1.57079637050628662109375);}
int uc_ui_motion_begin(UCUIMotion*s,const float*p,float delay,uint32_t mode,float scale){
 if(!s||!p||!isfinite(p[0])||!isfinite(p[1])||!isfinite(delay)||!isfinite(scale)||mode>7)return 1;
 static const float intro[8]={-1,-1,1,1,0,0,0,0},outro[8]={-1,1,-1,1,-1,1,-1,1},alpha[8]={0,0,0,0,3,3,1,1};
 memset(s,0,sizeof *s);s->base_x=p[0];s->phase=1.f-delay*2.5f;s->in_offset=intro[mode]*640.f;s->out_offset=outro[mode]*640.f;
 s->scale_delta=scale-1.f;s->alpha_factor=alpha[mode];s->offset=s->in_offset;s->x=p[0]+s->offset;s->y=p[1];s->scale=s->scale_delta+1.f;s->alpha=s->alpha_factor!=0?0.f:1.f;
 return 0;
}
int uc_ui_motion_step(UCUIMotion*s,float delta){
 if(!s||!isfinite(delta)||delta<0||!isfinite(s->phase)||s->finishing>1)return 1;
 float distance,scale=0,alpha=0;
 if(s->finishing){s->phase-=delta*3.3333332538604736328125f;distance=s->out_offset;}
 else{s->phase+=delta*2.5f;if(s->phase>1)s->phase=1;distance=s->in_offset;scale=s->scale_delta;alpha=s->alpha_factor;}
 float low=(float)curve(s->phase-.00999999977648258209228515625f);
 float derivative=(float)((curve(s->phase+.00999999977648258209228515625f)-low)*50.);
 float value=(float)(1.-curve(s->phase));s->progress=1.f-value;s->offset=value*distance;
 s->scale=(float)((double)value*scale+1.);s->blur=derivative*640.f;
 if(alpha!=0){s->alpha=s->progress*alpha;if(s->alpha>1)s->alpha=1;}
 s->x=s->base_x+s->offset;return 0;
}
void uc_ui_notification_positions(UCUINotificationPositions*p){if(p)*p=(UCUINotificationPositions){{210,83},{410,83},{256,83},{256,105},{216,89}};}

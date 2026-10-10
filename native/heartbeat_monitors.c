#include "heartbeat_monitors.h"
#include <math.h>
#include <string.h>
void uc_heartbeat_monitors_init(UCHeartbeatMonitors*s){memset(s,0,sizeof *s);for(unsigned i=0;i<4;i++){s->slots[i].widget=i;s->slots[i].actor=-1;}}
int uc_heartbeat_monitors_show(UCHeartbeatMonitors*s,int32_t actor,const char*text,const UCHeartbeatMonitorServices*h){
 return uc_heartbeat_monitors_join(s,actor,text,0,0,h);
}
int uc_heartbeat_monitors_join(UCHeartbeatMonitors*s,int32_t actor,const char*text,const char*joined,const char*left,const UCHeartbeatMonitorServices*h){
 if(!s||!text||!h)return 1;for(unsigned i=0;i<4;i++)if(s->slots[i].actor==actor){s->slots[i].references=(int32_t)((uint32_t)s->slots[i].references+1);return 0;}
 for(unsigned i=0;i<4;i++)if(!s->slots[i].references){UCHeartbeatMonitorSlot*v=s->slots+i;v->actor=actor;
  if(!h->bind||h->bind(h->owner,v->widget,actor,text,joined,left))return 2;v->references=(int32_t)((uint32_t)v->references+1);return 0;}return 0;
}
int uc_heartbeat_monitors_remove(UCHeartbeatMonitors*s,int32_t actor,const UCHeartbeatMonitorServices*h){
 if(!s||!h)return 1;for(unsigned i=0;i<4;i++)if(s->slots[i].actor==actor){UCHeartbeatMonitorSlot*v=s->slots+i;v->references=(int32_t)((uint32_t)v->references-1);
  if(!v->references){if(!h->clear||h->clear(h->owner,v->widget))return 2;v->actor=-1;}break;}return 0;
}
int uc_heartbeat_monitors_step(UCHeartbeatMonitors*s,float dt,const UCHeartbeatMonitorServices*h){
 if(!s||!h||!h->active||!h->exists||!h->step||!isfinite(dt)||dt<0||!isfinite(s->slide))return 1;
 if(s->slide==0.f){for(unsigned i=0;i<4;i++){int active=h->active(h->owner,s->slots[i].widget);if(active<0)return 2;if(active){if(i){UCHeartbeatMonitorSlot copy[4];memcpy(copy,s->slots,sizeof copy);for(unsigned j=0;j<4;j++)s->slots[j]=copy[(i+j)&3];s->slide=1.f;}break;}}}
 else s->slide=s->slide-dt/.2f; //71460 deliberately does not clamp the retained timer.
 for(unsigned i=0;i<4;i++){int32_t actor=s->slots[i].actor;if(actor!=-1){int found=h->exists(h->owner,actor);if(found<0)return 2;if(!found&&uc_heartbeat_monitors_remove(s,actor,h))return 2;}
  if(h->step(h->owner,s->slots[i].widget,dt))return 2;}return 0;
}
double uc_heartbeat_monitors_ease(float value){if(value>=1.f)return 1.;if(value<=0.f)return 0.;return(1.-cos((double)value*(double)3.1415927410125732421875f))*.5;}

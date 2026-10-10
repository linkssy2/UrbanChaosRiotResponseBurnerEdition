#include "environment_ambient.h"
#include <math.h>
#include <string.h>
static int vector(const float*v){if(!v)return 0;for(unsigned k=0;k<3;k++)if(!isfinite(v[k])||fabsf(v[k])>4096)return 0;return 1;}
static int valid(const UCEnvironmentAmbient*s){return s&&s->pending<=1&&vector(s->base)&&vector(s->current)&&vector(s->offset)&&vector(s->target);}
int uc_environment_ambient_set(UCEnvironmentAmbient*s,UCEnvironmentAmbientSlot slot,const float*rgb){
 if(!s||(unsigned)slot>UC_AMBIENT_OFFSET||!vector(rgb))return 0;
 float v[3];memcpy(v,rgb,sizeof v);
 if(slot==UC_AMBIENT_INITIAL){memcpy(s->current,v,sizeof v);memcpy(s->target,v,sizeof v);s->pending=0;}
 else if(slot==UC_AMBIENT_TARGET){memcpy(s->target,v,sizeof v);s->pending=1;}
 else memcpy(slot==UC_AMBIENT_BASE?s->base:s->offset,v,sizeof v);
 return 1;
}
int uc_environment_ambient_init(UCEnvironmentAmbient*s,const float*base){
 if(!s||!vector(base))return 0;float v[3];memcpy(v,base,sizeof v);memcpy(s->base,v,sizeof v);memset(s->offset,0,sizeof s->offset);return uc_environment_ambient_set(s,UC_AMBIENT_INITIAL,v);
}
int uc_environment_ambient_reset(UCEnvironmentAmbient*s){if(!s)return 0;return uc_environment_ambient_set(s,UC_AMBIENT_INITIAL,s->base);}
static uint32_t pack(const float rgb[3]){uint32_t argb=0xff000000;for(unsigned k=0;k<3;k++){float f=rgb[k]*255.f;int32_t n=(int32_t)f;if(n>255)n=255;else if(n<0)n=0;argb|=(uint32_t)n<<((2-k)*8);}return argb;}
int uc_environment_ambient_update(UCEnvironmentAmbient*s,const UCEnvironmentAmbientHooks*h,void*ctx){
 if(!valid(s)||!h||!h->manager||!h->delta||!h->world_mode||!h->publish)return 0;
 void*manager=h->manager(ctx);
 if(s->pending){float delta=h->delta(ctx);if(!isfinite(delta)||delta<0)return 0;float f=(float)pow(0.10000000149011612,(double)delta),other=1.f-f;
  for(unsigned k=0;k<3;k++){float a=s->target[k]*other,b=s->current[k]*f;s->current[k]=b+a;}
  if(s->current[0]==s->target[0]&&s->current[1]==s->target[1]&&s->current[2]==s->target[2])s->pending=0;
 }
 if(manager){float rgb[3];if((uint8_t)h->world_mode(ctx)){for(unsigned k=0;k<3;k++)rgb[k]=0.18000000715255737f;}else for(unsigned k=0;k<3;k++)rgb[k]=s->offset[k]+s->current[k];h->publish(ctx,manager,pack(rgb));}
 return 1;
}

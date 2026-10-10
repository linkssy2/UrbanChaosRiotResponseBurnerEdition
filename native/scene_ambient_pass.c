#include "scene_ambient_pass.h"
#include <math.h>
static void unpack(uint32_t c,float out[4]){const unsigned shift[4]={16,8,0,24};for(unsigned i=0;i<4;i++)out[i]=(float)((c>>shift[i])&255)*0x1.010102p-8f;}
int uc_scene_ambient_color(uint32_t base,const float (*contributions)[4],uint32_t count,uint32_t*out){
 if(!out||count>65536||(count&&!contributions))return 0;
 float sum[4];unpack(base,sum);
 for(uint32_t i=0;i<count;i++)for(unsigned k=0;k<4;k++){if(!isfinite(contributions[i][k]))return 0;sum[k]+=contributions[i][k];}
 uint32_t packed=0;const unsigned shift[4]={16,8,0,24};
 for(unsigned k=0;k<4;k++){float f=sum[k]*255.f;if(!isfinite(f)||f< -2147483648.f||f>=2147483648.f)return 0;int32_t n=(int32_t)f;if(n>255)n=255;else if(n<0)n=0;packed|=(uint32_t)n<<shift[k];}
 *out=packed;return 1;
}
static void advance(UCSceneTextureCursor*c){while(c->current<c->count&&((uint16_t)c->records[c->current].flags&c->mask)!=c->expected)c->current++;}
int uc_scene_ambient_pass(UCSceneTextureCursor*c,UCSceneModelPass*p,uint32_t present,
 uint8_t enabled,uint32_t blend,uint32_t ref,const UCSceneAmbientState*s,const UCSceneAmbientHooks*h,void*ctx){
 if(!c||c->count>65536||c->current>c->count||(c->count&&!c->records))return 0;
 if(c->current==c->count)return 1;
 if(!p||present>1||!s||!s->fog_enabled||!s->dirty||s->fog_enabled==s->dirty||!h||!h->render_state||!h->world||!h->receiver||
    ((uint8_t)blend&&!h->alpha_exempt)||(present&&enabled&&(!h->camera_position||!h->ambient||!h->exposure)))return 0;
 for(uint32_t i=0;i<c->count;i++)if(!c->records[i].model)return 0;
 h->render_state(ctx,0x40304,(uint8_t)blend!=0);*s->dirty|=0x3000;*s->fog_enabled=0;
 if(present&&enabled){float position[3];h->camera_position(ctx,position);for(unsigned k=0;k<3;k++)if(!isfinite(position[k]))return 0;
  unpack(h->ambient(ctx,position),p->color);float exposure=h->exposure(ctx);if(!isfinite(exposure)||exposure==0)return 0;
  for(unsigned k=0;k<4;k++){p->color[k]/=exposure;if(!isfinite(p->color[k]))return 0;}
 }else for(unsigned k=0;k<4;k++)p->color[k]=0;
 p->mode=0;c->current=0;advance(c);
 while(c->current<c->count){const UCSceneModelRecord*r=c->records+c->current;h->world(ctx,r->model);p->partition=r->partition;
  if((uint8_t)blend){uint32_t test=(uint8_t)h->alpha_exempt(ctx,r->model)?0:1;h->render_state(ctx,0x40300,test);h->render_state(ctx,0x40340,test?ref:0);h->render_state(ctx,0x4035C,test);}
  h->receiver(ctx,r->model,p);c->current++;advance(c);
 }
 h->render_state(ctx,0x40304,0);h->render_state(ctx,0x40300,0);return 1;
}

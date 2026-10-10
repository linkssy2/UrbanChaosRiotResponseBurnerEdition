#include "depth_blur.h"
#include <float.h>
#include <math.h>
#include <string.h>
void uc_depth_blur_reset(UCDepthBlurRequest*s){if(s){s->amplitude=0;s->start=s->end=FLT_MAX;}}
int uc_depth_blur_request(UCDepthBlurRequest*s,float a,float b,float v){
 if(!s||!isfinite(a)||!isfinite(b)||!isfinite(v)||v>1)return 1;
 if(v>0){if(s->start>a)s->start=a;if(s->end>b)s->end=b;if(v>s->amplitude)s->amplitude=v;}return 0;
}
int uc_depth_blur_publish(UCDepthBlurRequest*s,UCDepthBlurSettings*r){
 if(!s)return 1;
 if(r){r->enabled=s->amplitude>0;if(r->enabled){r->start=s->start;r->end=s->end;r->amplitude=s->amplitude;}}
 uc_depth_blur_reset(s);return 0;
}
int uc_depth_blur_plan(unsigned w,unsigned h,UCDepthBlurPlan*out){
 if(!out||w<2||h<2||w>4096||h>4096)return 1;
 float x=(float)w,y=(float)h,a=(float)(w/2),b=(float)(h/2);
 const UCDepthBlurPlan p={{{0,0,1,0,0},{a,0,1,x,0},{0,b,1,0,y},{a,b,1,x,y}},
  {{0,0,1,1,0,0,0,0},{x,0,1,1,x,0,a,0},{x,y,1,1,x,y,a,b},{0,y,1,1,0,y,0,b}},1.25f};
 memcpy(out,&p,sizeof p);return 0;
}

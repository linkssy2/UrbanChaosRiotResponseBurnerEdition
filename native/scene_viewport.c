#include "scene_viewport.h"
#include <math.h>
#include <xmmintrin.h>
static int mode(void){return !(_mm_getcsr()&0xE040u);}
static int positive(float x){return isfinite(x)&&x>0;}
static int bounds(float t,float l,float b,float r){return isfinite(t)&&isfinite(l)&&isfinite(b)&&isfinite(r)&&b>t&&r>l;}
int uc_scene_viewport_begin(UCSceneViewport*p,UCSceneViewport*parent,float t,float l,float b,float r,float aspect){
    if(!p||!mode()||!bounds(t,l,b,r)||!positive(aspect))return 1;
    float ratio=((b-t)*aspect)/(r-l);if(!positive(ratio))return 1;
    *p=(UCSceneViewport){parent,t,l,b,r,ratio,1};return 0;
}
int uc_scene_viewport_values(const UCSceneViewport*p,double out[5]){
    if(!p||!p->active||!out||!mode()||!bounds(p->top,p->left,p->bottom,p->right)||!positive(p->ratio))return 1;
    double h=(double)p->bottom-p->top,w=(double)p->right-p->left;
    float numerator=(float)(w*p->ratio);double aspect=(double)numerator/h;
    if(!isfinite(aspect)||aspect<=0)return 1;
    out[0]=p->top;out[1]=p->left;out[2]=h;out[3]=w;out[4]=aspect;return 0;
}
static int derive(UCSceneViewport*p,float a,float b,int fit,const UCSceneViewportMemory*m,UCSceneViewport**out){
    double values[5];float dh=0,dw=0;
    if(!out||!m||!m->allocate||!m->release||!positive(a)||(!fit&&!positive(b))||uc_scene_viewport_values(p,values))return 1;
    float aspect=(float)values[4];
    if(fit){if((double)a>values[4])dh=((1-aspect/a)*(p->bottom-p->top))*.5f;
        else dw=((1-a/aspect)*(p->right-p->left))*.5f;}
    else{dh=((p->bottom-p->top)*(1-a))*.5f;dw=((p->right-p->left)*(1-b))*.5f;}
    if(!isfinite(dh)||!isfinite(dw))return 1;
    UCSceneViewport*q=m->allocate(m->owner);*out=q;if(!q)return 0;
    float top=p->top+dh,left=p->left+dw,bottom=p->bottom-dh,right=p->right-dw;
    float ratio=fit?((bottom-top)*a)/(right-left):((((bottom-top)/a)*aspect)*b)/(right-left);
    if(!bounds(top,left,bottom,right)||!positive(ratio))return 2;
    *q=(UCSceneViewport){p,top,left,bottom,right,ratio,1};return 0;
}
int uc_scene_viewport_fit(UCSceneViewport*p,float aspect,const UCSceneViewportMemory*m,UCSceneViewport**out){return derive(p,aspect,0,1,m,out);}
int uc_scene_viewport_scale(UCSceneViewport*p,float x,float y,const UCSceneViewportMemory*m,UCSceneViewport**out){return derive(p,x,y,0,m,out);}
int uc_scene_viewport_end(UCSceneViewport*p,uint32_t flags,const UCSceneViewportMemory*m){
    if(!p||!p->active||!m||!m->release)return 1;p->active=0;if(flags&1)m->release(m->owner,p);return 0;
}

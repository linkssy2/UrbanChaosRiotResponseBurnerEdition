#include "character_step_aside.h"
#include <math.h>
int uc_character_step_aside_update(UCCharacterStepAside*s,const UCCharacterStepAsideServices*h){
 if(!s||!h||!h->target||!h->busy||!h->actor||!h->clear||!h->side||!h->move||!h->notify||!h->completion||!h->exit)return 1;
 void*p=h->owner;if(!s->first){if(h->completion(p)!=0)h->exit(p);return 0;}
 const float*t=h->target(p,s->target);if(!t||h->busy(p)){h->exit(p);return 0;}
 const float*a=h->actor(p);if(!a)return 1;
 /*3BD10 retains the original 3x3, clears translation. Preserve SIMD grouping.*/
 float f[3];for(unsigned i=0;i<3;i++)f[i]=(a[i]*0.f+a[4+i]*0.f)+(a[8+i]*1.f+0.f);
 float left[3]={a[12]-f[2]*s->distance,a[13],f[0]*s->distance+a[14]};
 float right[3]={f[2]*s->distance+a[12],a[13],a[14]-f[0]*s->distance};
 uint8_t l=h->clear(p,left),r=h->clear(p,right),moved=0;
 if(l&&r){float dx=a[12]-t[12],dy=a[13]-t[13],dz=a[14]-t[14];
  /*Original10AC08 overwrites the temporary direction Y with deltaY.*/
  float dot=(dy*dy+dz*f[0])+dx*(0.f-f[2]);if(dot>0)r=0;else l=0;
 }
 if(l){h->side(p,1);moved=1;}
 else if(r){h->side(p,0);moved=1;}
 else{float x=a[12]-t[12],y=a[13]-t[13],z=a[14]-t[14];
  float square=(x*x+y*y)+z*z,inv=(float)(1./sqrt((double)square));
  float to[3]={a[12]+s->distance*(x*inv),a[13]+(y*inv)*s->distance,a[14]+(z*inv)*s->distance};
  if(h->clear(p,to))h->move(p,to);
 }
 h->notify(p,moved);s->first=0;return 0;
}

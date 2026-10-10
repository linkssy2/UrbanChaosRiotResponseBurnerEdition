#include "character_sight.h"
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int finite3(const float*p){return isfinite(p[0])&&isfinite(p[1])&&isfinite(p[2]);}
static int point(const UCCharacterSightServices*h,const UCCharacterSightActor*a,const char*n,float*p){
    int r=h->point(h->owner,a,n,p);if(r<0)return 1;
    if(!r)memcpy(p,a->position,12);return !finite3(p);
}
int uc_character_sight(const UCCharacterSightActor*a,int32_t target,float cosine,
    uint8_t head,const float offset[3],uint8_t characters,const UCCharacterSightServices*h,uint8_t*out){
    if(!a||!h||!out||!offset||!isfinite(cosine)||!finite3(offset)||(_mm_getcsr()&0xe040))return 1;
    if(target==-1){*out=0;return 0;}
    if(!h->resolve||!h->point||!h->forward||!h->ray)return 1;
    UCCharacterSightActor other={0};int r=h->resolve(h->owner,target,&other);if(r<0)return 1;
    if(!r){*out=0;return 0;}
    float from[3],to[3],forward[3];
    if(point(h,a,head?"head pos":"target position",from)||point(h,&other,"target position",to))return 1;
    for(unsigned i=0;i<3;i++){from[i]+=offset[i];to[i]+=offset[i];}
    if(!finite3(from)||!finite3(to))return 1;
    float dx=to[0]-from[0],dz=to[2]-from[2];
    if(h->forward(h->owner,a,forward)||!finite3(forward))return 1;
    float inverse=(float)(1.0/sqrt((double)dz*dz+(double)dx*dx));
    float nx=dx*inverse,nz=dz*inverse,ny=inverse*0.0f;
    float inv_forward=(float)(1.0/sqrt((double)forward[2]*forward[2]+(double)forward[0]*forward[0]));
    float dot=(forward[2]*inv_forward)*nz+nx*(inv_forward*forward[0]);
    dot+=(inv_forward*0.0f)*ny;
    if(cosine>dot){*out=0;return 0;}
    //20FF40/20FE90 subtract in x87 before the horizontal length test.
    double x=(double)to[0]-from[0],z=(double)to[2]-from[2];
    if(sqrt(x*x+z*z)<(double)0.1f){*out=1;return 0;}
    UCCharacterSightActor hit={0};
    r=h->ray(h->owner,from,to,characters?9:32,a->group,0x4000,&hit);if(r<0)return 1;
    if(!r||hit.identity==other.identity){*out=1;return 0;}
    if(!hit.identity){*out=0;return 0;}
    UCCharacterSightActor parent={0};r=h->resolve(h->owner,hit.parent,&parent);if(r<0)return 1;
    *out=r&&parent.identity==other.identity;return 0;
}

#include "vector_random.h"
#include <fenv.h>
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int arithmetic(void){return !(_mm_getcsr()&0xE040u)&&fegetround()==FE_TONEAREST;}
int uc_vector_random_init(UCVectorRandom *r){
    if(!r||!arithmetic())return 1;
    memset(r,0,sizeof *r);float previous=1605688.f,current=1.f;
    for(unsigned i=1;i<=110;i++){
        r->values[(21*i)%111]=current;
        float next=previous-current;if(next<0)next=4000000.f+next;
        previous=current;current=next;
    }
    for(unsigned pass=0;pass<4;pass++)for(unsigned i=1;i<=110;i++){
        float value=r->values[i]-r->values[(30*i)%111+1];
        r->values[i]=value<0?4000000.f+value:value;
    }
    r->first=27;r->second=14;return 0;
}
int uc_vector_random_signed(UCVectorRandom *r,float scale,float result[4]){
    if(!r||!result||!arithmetic()||r->first>=28||r->second>=28||!isfinite(scale)||fabsf(scale)>65536)return 1;
    unsigned a=r->first?r->first-1:27,b=r->second?r->second-1:27;
    for(unsigned i=0;i<4;i++)if(!isfinite(r->values[4*a+i])||!isfinite(r->values[4*b+i])||
        r->values[4*a+i]<0||r->values[4*a+i]>4000000||r->values[4*b+i]<0||r->values[4*b+i]>4000000)return 1;
    float values[4];const float factor=((1.f/4000000.f)*2.f)*scale;
    for(unsigned i=0;i<4;i++){
        float value=r->values[4*a+i]-r->values[4*b+i];
        values[i]=value+(value<0?4000000.f:0.f);result[i]=(values[i]-2000000.f)*factor;
    }
    memcpy(r->values+4*a,values,sizeof values);r->first=a;r->second=b;return 0;
}

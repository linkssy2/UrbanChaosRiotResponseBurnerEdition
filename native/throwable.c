#include "throwable.h"
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int finite3(const float*p){return isfinite(p[0])&&isfinite(p[1])&&isfinite(p[2]);}
static void cross(const float*a,const float*b,float*out){out[0]=a[1]*b[2]-a[2]*b[1];out[1]=a[2]*b[0]-a[0]*b[2];out[2]=a[0]*b[1]-a[1]*b[0];out[3]=0.f*0.f-0.f*0.f;}
static int normalize(float*v,unsigned n){double sum=(double)v[n-1]*v[n-1];for(unsigned i=n-1;i--;)sum+=(double)v[i]*v[i];if(!(sum>0)||!isfinite(sum))return 0;double k=1./sqrt(sum);for(unsigned i=0;i<n;i++)v[i]=(float)((double)v[i]*k);return 1;}
int uc_molotov_launch(const UCMolotovLaunchInput*in,UCMolotovLaunch*out){
    UCMolotovLaunch result;float direction[4],up[4]={0,1,0,0},right[4],vertical[4],offset[3];
    if(!in||!out||!finite3(in->origin)||!finite3(in->forward)||!finite3(in->target)||
       !isfinite(in->target_distance)||in->target_distance<0||!isfinite(in->pitch)||
       !isfinite(in->charge)||in->charge<0||in->charge>.5f||(_mm_getcsr()&0xE040u))return 1;
    float base=in->shield_selected?.75f:1.f;
    float strength=in->charge*2.f+base;
    float factor=(float)((cos((double)in->pitch)*(double).85f+(double).15f)*(double)strength);
    float range=in->target_distance;
    for(unsigned i=0;i<3;i++)direction[i]=in->target[i]-in->origin[i];
    if(range<200.f){memcpy(direction,in->forward,12);range=200.f;}
    else if(!normalize(direction,3))return 1;
    float distance=factor*800.f;if(distance>range)distance=range;
    for(unsigned i=0;i<3;i++)offset[i]=direction[i]*distance;
    float length=(float)sqrt(((double)offset[2]*offset[2]+(double)offset[1]*offset[1])+(double)offset[0]*offset[0]);
    if(!(length>0)||!(factor>0))return 1;
    float inverse=1.f/length;for(unsigned i=0;i<3;i++)direction[i]=offset[i]*inverse;direction[3]=0;
    cross(up,direction,right);if(!normalize(right,4))return 1;
    cross(direction,right,vertical);if(!normalize(vertical,4))return 1;
    memcpy(result.transform,right,16);memcpy(result.transform+4,vertical,16);memcpy(result.transform+8,direction,16);
    memcpy(result.transform+12,in->origin,12);result.transform[15]=1;
    float time=length/(1500.f*factor),inv_time=1.f/time;
    float zero=((time*0.f)*time)*.5f,gravity=((time*-980.f)*time)*.5f;
    result.velocity[0]=(offset[0]-zero)*inv_time;result.velocity[1]=(offset[1]-gravity)*inv_time;result.velocity[2]=(offset[2]-zero)*inv_time;
    if(!finite3(result.velocity))return 1;
    *out=result;return 0;
}

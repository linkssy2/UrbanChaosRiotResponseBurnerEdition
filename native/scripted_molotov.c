#include "scripted_molotov.h"
#include <math.h>
int uc_scripted_molotov(const float*from,const float*to,float time,float*delay,UCScriptedMolotov*out){
 if(!from||!to||!delay||!out||!isfinite(time)||time<=0||!isfinite(*delay)||*delay<0)return 1;
 for(unsigned i=0;i<3;i++)if(!isfinite(from[i])||!isfinite(to[i]))return 1;
 UCScriptedMolotov r={0};r.fuse=time+20.f;
 float maximum=time*.9f;if(*delay>maximum)*delay=maximum;r.delay=*delay;
 float inverse=1.f/(0.f-time);
 for(unsigned i=0;i<3;i++){float acceleration=time*(i==1?-490.f:0.f);acceleration*=time;r.velocity[i]=(acceleration-(to[i]-from[i]))*inverse;}
 *out=r;return 0;
}

#include "scene_camera_world.h"
#include <math.h>
#include <xmmintrin.h>
int uc_scene_camera_world(UCSceneCamera*c,float*w,float*s,const uint32_t*flags,
 const uint8_t*suppressed,const UCSceneCameraWorldServices*h,const float**out){
 if(!c||!w||!s||!flags||!suppressed||!out||w==s)return 1;
 if((*flags&0x800u)||*suppressed){*out=w;return 0;}
 if(!h||!h->preflight||!h->sample||!h->perturb||(_mm_getcsr()&0xE040u)||!h->preflight(h->owner))return 1;
 float amplitude=(float)h->sample(h->owner,0,w+12,1);
 if(!isfinite(amplitude))return 2;
 h->perturb(h->owner,amplitude,w,s);
 UCSceneCameraAuxiliary*a=(UCSceneCameraAuxiliary*)c->auxiliary;
 float value=(float)((double)amplitude*10.0);
 if(!a||!isfinite(value))return 2;
 a->feedback=value;*out=s;return 0;
}

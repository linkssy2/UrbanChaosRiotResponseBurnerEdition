#include "particle_fog.h"
#include <math.h>
#include <xmmintrin.h>
int uc_particle_linear_fog(unsigned enabled,unsigned range,float start,float end,UCParticleFog *out){
    if(!out||enabled>1||range>1||(_mm_getcsr()&0xE040u))return 1;
    UCParticleFog p={0};
    if(enabled){
        if(!isfinite(start)||!isfinite(end)||fabsf(start)>1e6f||fabsf(end)>1e6f)return 1;
        double reciprocal=start==end?8192.0:1.0/((double)end-(double)start);
        p.enabled=1;p.coordinate_mode=range?1:2;p.mode=0x2601;
        p.bias=(float)((double)end*reciprocal+1.0);p.slope=(float)-reciprocal;
        if(!isfinite(p.bias)||!isfinite(p.slope))return 1;
    }
    *out=p;return 0;
}

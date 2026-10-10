#include "cop_overlay_math.h"
#include <math.h>
double uc_cop_overlay_facing(const float self[3],const float forward[3],const float target[3]){
    /*1BA670 retains both float reciprocals and its final x87 sum. In the
     * source a zero planar vector yields unordered/NaN; do not invent facing.*/
    float x=target[0]-self[0],z=target[2]-self[2];
    float inverse=(float)(1./sqrt((double)z*z+(double)x*x));
    float tx=x*inverse,tz=z*inverse,ty=(float)((double)inverse*0.);
    float fi=(float)(1./sqrt((double)forward[2]*forward[2]+(double)forward[0]*forward[0]));
    float fx=forward[0]*fi,fz=forward[2]*fi;
    return ((double)fz*tz+(double)fx*tx)+((double)fi*0.)*ty;
}
int uc_cop_overlay_player_in_line(const float muzzle[3],const float target[3],const float player[3]){
    /*1BA400: open segment from the real gun27750 muzzle to target.*/
    float a[3],b[3];for(unsigned i=0;i<3;i++){a[i]=player[i]-muzzle[i];b[i]=target[i]-muzzle[i];}
    float t=((b[2]*a[2]+b[1]*a[1])+b[0]*a[0])/((b[2]*b[2]+b[1]*b[1])+b[0]*b[0]);
    if(!(t>0&&t<1))return 0;
    for(unsigned i=0;i<3;i++)a[i]=(b[i]*t+muzzle[i])-player[i];
    return 200.>sqrt(((double)a[2]*a[2]+(double)a[1]*a[1])+(double)a[0]*a[0]);
}

#include "heat_distortion.h"
#include <math.h>
#include <xmmintrin.h>
int uc_heat_distortion_table(float table[512]){
    if(!table||(_mm_getcsr()&0xE040u))return 1;
    for(unsigned i=0;i<512;i++)table[i]=(float)sin(((double)i*.001953125)*(double)6.2831854820251465f);
    return 0;
}
static unsigned index(float v){return (unsigned)_mm_cvttss_si32(_mm_set_ss(v))&511u;}
int uc_heat_distortion_grid(float *previous,float phase,const float table[512],float grid[300][2]){
    if(!previous||!table||!grid||!isfinite(*previous)||!isfinite(phase)||fabsf(phase)>100000||(_mm_getcsr()&0xE040u))return 1;
    for(unsigned i=0;i<512;i++)if(!isfinite(table[i])||fabsf(table[i])>1)return 1;
    if(*previous==phase)return 0;
    *previous=phase;
    float t0=phase*162.9746551513672f,t1=phase*1.2999999523162842f,t2=phase*138.52845764160156f;
    for(unsigned row=0;row<15;row++){
        float y=(float)row*.0714285746216774f;
        float a=y*220.0157928466797f,b=y*252.6107177734375f,c=y*4.210000038146973f,d=y*268.9081726074219f-t2;
        for(unsigned col=0;col<20;col++){
            float x=(float)col*.05263157933950424f;
            float p=table[index(table[index((t1-x*3.700000047683716f)*81.4873275756836f+a)]*334.0980529785156f)];
            float q=table[index(table[index((x*382.99041748046875f+t0)+b)]*431.8828430175781f)];
            x=(p*q)*.029999999329447746f+x;
            p=table[index(table[index(((x*2.700000047683716f+t1)-c)*81.4873275756836f)]*382.99041748046875f)];
            q=table[index(table[index(x*220.0157928466797f+d)]*252.6107177734375f)];
            grid[row*20+col][0]=x;grid[row*20+col][1]=(q*p)*.026000000536441803f+y;
        }
    }
    return 0;
}

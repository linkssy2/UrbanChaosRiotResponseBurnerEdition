#include "fire_visual.h"
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
int uc_fire_visual(const float*d,const float*p,const float*r,float lifetime,UCFireVisual*out){
    if(!d||!p||!r||!out||lifetime!=-1||(_mm_getcsr()&0xE040u))return 1;
    for(unsigned i=0;i<3;i++)if(!isfinite(d[i])||d[i]<0||d[i]>65536||!isfinite(p[i])||fabsf(p[i])>65536)return 1;
    for(unsigned i=0;i<6;i++)if(!isfinite(r[i])||r[i]<0||r[i]>65536)return 1;
    UCFireVisual v={0};v.matrix[1]=v.matrix[4]=v.matrix[10]=v.matrix[15]=1;
    v.matrix[12]=p[0];v.matrix[13]=p[1]-d[1]*.5f;v.matrix[14]=p[2];
    v.size[0]=d[1]*.1f;v.size[1]=d[0];v.size[2]=d[2];
    float area=d[2]*d[0];area=area*0.0001f;
    for(unsigned i=0;i<6;i++){double value=(double)r[i]*area;if(!i)value+=value;v.rate[i]=(float)value;
        if(!isfinite(v.rate[i])||v.rate[i]>65536)return 1;}
    if(fabsf(v.matrix[13])>65536)return 1;
    memcpy(out,&v,sizeof v);return 0;
}

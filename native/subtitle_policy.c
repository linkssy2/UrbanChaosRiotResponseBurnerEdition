#include "subtitle_policy.h"
#include <math.h>
int uc_subtitle_admit(int ui,int named,int always,const float*player,const float*actor){
    if(!ui)return 0;if(named||always)return 1;if(!player||!actor)return 0;
    double x=(double)actor[0]-player[0],y=(double)actor[1]-player[1],z=(double)actor[2]-player[2];
    return sqrt((z*z+y*y)+x*x)<1500;
}
void uc_subtitle_color(int type,int kind,int dispatch,float*out){
    out[0]=out[1]=out[2]=out[3]=1;
    if(type==2){out[1]=out[2]=.5f;return;}
    switch(kind){
    case 2:out[0]=out[1]=.85f;break;
    case 3:out[2]=.7f;break;
    case 4:out[0]=out[2]=.75f;break;
    case 5:out[1]=.8f;break;
    default:if(kind==1||!dispatch)out[1]=out[2]=.8f;break;
    }
}

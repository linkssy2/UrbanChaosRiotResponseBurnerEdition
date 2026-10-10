#include "character_unobserved.h"
#include <math.h>
uint8_t uc_character_tasks_empty(uint32_t count){return count==0;}
int uc_character_unobserved(const float actor[3],const UCCharacterPlayerView*players,size_t count,uint8_t*out){
    if(!actor||!out||count>8||(count&&!players))return 1;
    for(unsigned j=0;j<3;j++)if(!isfinite(actor[j]))return 1;
    for(size_t i=0;i<count;i++)for(unsigned j=0;j<3;j++)if(!isfinite(players[i].position[j])||!isfinite(players[i].head_direction[j]))return 1;
    uint8_t result=1;
    for(size_t i=0;i<count;i++){const float*p=players[i].position,*head=players[i].head_direction;
        float x=(float)((double)actor[0]-p[0]),y=(float)((double)actor[1]-p[1]);double zd=(double)actor[2]-p[2];float z=(float)zd;
        double distance=sqrt((zd*z+(double)y*y)+(double)x*x);if(1000.0>distance)result=0;
        float inverse=1.f/(float)distance,dx=inverse*x,dy=inverse*y,dz=inverse*z;
        float normal=(float)(1.0/sqrt(((double)head[2]*head[2]+(double)head[1]*head[1])+(double)head[0]*head[0]));
        float hx=head[0]*normal,hy=head[1]*normal,hz=head[2]*normal;
        if((hz*dz+hy*dy)+hx*dx>0.f)result=0;
    }
    *out=result;return 0;
}

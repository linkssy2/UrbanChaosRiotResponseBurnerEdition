#include "character_player_range.h"
#include <math.h>
int uc_character_player_range(const float actor[3],const float player[3],float range,float angle,
    void*owner,int(*look)(void*,float[16],uint8_t*),uint8_t*out){
    if(!actor||!player||!out||!look||!isfinite(range)||!isfinite(angle))return 1;
    for(unsigned i=0;i<3;i++)if(!isfinite(actor[i])||!isfinite(player[i]))return 1;
    float x=player[0]-actor[0],y=player[1]-actor[1],z=player[2]-actor[2];
    if(!(range*range>(z*z+y*y)+x*x)){*out=0;return 0;}
    float matrix[16];uint8_t found=0;if(look(owner,matrix,&found)||found>1)return 2;
    if(!found){*out=0;return 0;}for(unsigned i=0;i<16;i++)if(!isfinite(matrix[i]))return 2;
    x=actor[0]-player[0];z=actor[2]-player[2];
    float inverse=(float)(1./sqrt((double)z*z+(double)x*x));float ax=x*inverse,ay=0.f*inverse,az=z*inverse;
    float hx=matrix[8],hz=matrix[10];inverse=(float)(1./sqrt((double)hz*hz+(double)hx*hx));
    hx*=inverse;float hy=0.f*inverse;hz*=inverse;
    float mx=(ax-hx)*.5f+hx,my=(ay-hy)*.5f+hy,mz=(az-hz)*.5f+hz;
    float dx=ax-mx,dy=ay-my,dz=az-mz;
    double difference=sqrt(((double)dz*dz+(double)dy*dy)+(double)dx*dx);
    double middle=sqrt(((double)mz*mz+(double)my*my)+(double)mx*mx);
    *out=(double)angle*.01745329238474369>fabs(atan2(difference,middle));return 0;
}

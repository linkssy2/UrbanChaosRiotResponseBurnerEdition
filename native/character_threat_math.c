#include "character_threat_math.h"
#include <math.h>
void uc_character_threat_direction(const float forward[3],int right,float out[3]){
    const float x=right?0.f-forward[2]:forward[0],z=right?forward[0]:forward[2];
    const float inv=(float)(1./sqrt((double)z*z+(double)x*x));
    out[0]=inv*x;out[1]=inv*0.f;out[2]=inv*z;
}
int uc_character_threat_line_near(const float position[3],const float target[3],const float forward[3],float radius){
    const float inv=(float)(1./sqrt(((double)forward[2]*forward[2]+(double)forward[0]*forward[0])+(double)forward[1]*forward[1]));
    const float x=inv*forward[0],y=inv*forward[1],z=forward[2]*inv;
    const float dx=position[0]-target[0],dy=position[1]-target[1],dz=position[2]-target[2];
    const float projection=(dz*z+dy*y)+dx*x;
    const float px=projection*x-dx,py=projection*y-dy,pz=z*projection-dz;
    return !((pz*pz+py*py)+px*px>radius*radius);
}

#include "character_face_target.h"
#include <math.h>
#include <string.h>
int uc_character_face_target(const float world[16],const float position[3],const float target[3],float limit,float out[16],uint8_t*changed){
 if(!world||!position||!target||!out||!changed)return 1;
 float current[16];memcpy(current,world,sizeof current);memcpy(out,current,sizeof current);*changed=0;
 float x=target[0]-position[0],z=target[2]-position[2],square=x*x+z*z;
 if(square<1.1920928955078125e-7f)return 0;
 double inv=1./sqrt((double)square);
 float heading=(float)atan2((double)x*inv,(double)z*inv);
 inv=1./sqrt((double)current[10]*current[10]+(double)current[8]*current[8]);
 float angle=(float)((double)heading-atan2((double)current[8]*inv,(double)current[10]*inv));
 if(angle>3.141592741012573242f)angle-=6.283185482025146484f;
 else if(angle<-3.141592741012573242f)angle+=6.283185482025146484f;
 float negative=0.f-limit;if(negative>angle)angle=negative;else if(angle>limit)angle=limit;
 float sine=(float)sin((double)angle),cosine=(float)cos((double)angle);
 const float rotation[16]={cosine,0,(float)-sin((double)angle),0,0,1,0,0,sine,0,cosine,0,0,0,0,1};
 for(unsigned row=0;row<3;row++)for(unsigned column=0;column<4;column++){
  float a=rotation[column]*current[row*4],b=rotation[4+column]*current[row*4+1];
  float c=rotation[8+column]*current[row*4+2],d=rotation[12+column]*current[row*4+3];
  out[row*4+column]=(a+b)+(c+d);
 }
 *changed=1;return 0;
}

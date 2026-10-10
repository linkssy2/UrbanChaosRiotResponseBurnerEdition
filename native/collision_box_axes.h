#ifndef UC_COLLISION_BOX_AXES_H
#define UC_COLLISION_BOX_AXES_H
#include <math.h>
/* Shared PC-query convention: normalize an already near-unit quaternion. */
static int uc_collision_box_axes(const float rotation[4],double axes[3][3]){
 double norm=0,q[4];for(unsigned i=0;i<4;i++){q[i]=rotation[i];if(!isfinite(q[i]))return -1;norm+=q[i]*q[i];}
 if(norm<.99||norm>1.01)return -1;
 norm=sqrt(norm);for(unsigned i=0;i<4;i++)q[i]/=norm;
 const double x=q[0],y=q[1],z=q[2],w=q[3];
 axes[0][0]=1-2*(y*y+z*z);axes[0][1]=2*(x*y+z*w);axes[0][2]=2*(x*z-y*w);
 axes[1][0]=2*(x*y-z*w);axes[1][1]=1-2*(x*x+z*z);axes[1][2]=2*(y*z+x*w);
 axes[2][0]=2*(x*z+y*w);axes[2][1]=2*(y*z-x*w);axes[2][2]=1-2*(x*x+y*y);
 return 0;
}
#endif

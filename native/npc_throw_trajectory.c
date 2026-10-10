#include "npc_throw_trajectory.h"
#include <math.h>
static int finite_vector(const float *p,unsigned n){if(!p)return 0;for(unsigned i=0;i<n;i++)if(!isfinite(p[i]))return 0;return 1;}
int uc_npc_throw_trajectory(const float*from,const float*target,int32_t selection,uint32_t options,
    float speed,const UCNpcThrowTrajectoryServices*h,UCNpcThrowTrajectory*out){
    if(!out||!finite_vector(from,3)||!finite_vector(target,3)||selection<0||selection>1||!isfinite(speed)||speed<=0)return -1;
    float d[3];for(unsigned i=0;i<3;i++)d[i]=target[i]-from[i];
    const float square=(d[0]*d[0]+d[1]*d[1])+d[2]*d[2];if(!isfinite(square)||square<=0)return -1;
    const double length=sqrt((double)square);const float distance=length<600.?600.f:(float)length;
    const float inverse=(float)(1./length);for(unsigned i=0;i<3;i++)d[i]=(d[i]*inverse)*distance;
    if(!(options&4)){
        if(!h||!h->random||!h->random_boolean||!h->ray)return -1;
        const float spread=(distance/3500.f)*200.f,low=spread*.25f;
        float a=(float)h->random(h->owner,low,spread);int sign=h->random_boolean(h->owner);if(sign<0||sign>1)return -1;if(sign)a=0.f-a;
        float b=(float)h->random(h->owner,low,spread);sign=h->random_boolean(h->owner);if(sign<0||sign>1)return -1;if(sign)b=0.f-b;
        const double horizontal=(double)d[2]*d[2]+(double)d[0]*d[0];if(!(horizontal>0))return -1;
        const float norm=(float)(1./sqrt(horizontal));const float x=norm*d[0],y=(float)((double)norm*0.),z=d[2]*norm;
        float offset[3]={(z*a)+x*b,(a*0.f)+(b*0.f),((0.f-x)*a)+z*b};
        const double dx=(double)target[0]-from[0],dz=(double)target[2]-from[2];int hit=0;
        if(!(sqrt(dx*dx+dz*dz)<(double).1f)){hit=h->ray(h->owner,from,target,0x20,UINT32_MAX,0x4000,0);if(hit<0||hit>1)return -1;}
        const float shift=(selection?350.f:150.f)*(hit?.25f:1.f);
        d[0]=offset[0]+(d[0]-shift*x);d[1]=offset[1]+(d[1]-shift*y);d[2]=offset[2]+(d[2]-shift*z);
    }
    const float time=distance/speed;UCNpcThrowTrajectory result={{0,0,0},.1f,selection?0.f:1.f};
    if(time>0){const float half_time_square=(time*time)*.5f,vertical=half_time_square*-980.f,horizontal=half_time_square*0.f,inverse_time=1.f/time;
        result.velocity[0]=(d[0]-horizontal)*inverse_time;result.velocity[1]=(d[1]-vertical)*inverse_time;result.velocity[2]=(d[2]-horizontal)*inverse_time;}
    if(!finite_vector(result.velocity,3))return -1;*out=result;return 0;
}
int uc_npc_throw_can(int32_t selection,float speed,const float*local,const float*world,const float*target,const float*forward){
    if(selection==-1||speed<=0)return 0;
    if(selection<0||selection>1||!isfinite(speed)||!finite_vector(local,16)||!finite_vector(world,16)||!finite_vector(target,3)||!finite_vector(forward,3))return -1;
    float position[3];for(unsigned i=0;i<3;i++)position[i]=(world[i]*local[12]+world[4+i]*local[13])+(world[8+i]*local[14]+world[12+i]*local[15]);
    const float x=target[0]-position[0],z=target[2]-position[2],square=z*z+x*x;
    const float a=(float)(1./sqrt((double)forward[2]*forward[2]+(double)forward[0]*forward[0]));
    const float b=(float)(1./sqrt((double)square));
    const float dot=((forward[2]*a)*(z*b)+(b*x)*(forward[0]*a))+(a*0.f)*(b*0.f);
    if(.73f>dot)return 0;
    //The original ordered FUCOMI test rejects both zero distance and NaN.
    return sqrt((double)square)/(double)speed>0.;
}

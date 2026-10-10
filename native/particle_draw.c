#include "particle_draw.h"
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int finite_values(const float *p,unsigned n){for(unsigned i=0;i<n;i++)if(!isfinite(p[i])||fabsf(p[i])>65536)return 0;return 1;}
static int arithmetic(void){return !(_mm_getcsr()&0xE040u);}
int uc_particle_color_prepare(const float colors[3][4],const float hold[2],UCParticleColorCurve *out){
    if(!colors||!hold||!out||!arithmetic()||!finite_values(&colors[0][0],12)||!finite_values(hold,2))return 1;
    UCParticleColorCurve c;memcpy(c.color,colors,sizeof c.color);memcpy(c.hold,hold,sizeof c.hold);
    if(c.hold[0]<=0){c.hold[0]=0;memset(c.slope[0],0,16);}
    else for(unsigned k=0;k<4;k++)c.slope[0][k]=(c.color[1][k]-c.color[0][k])/c.hold[0];
    float remainder=1-c.hold[1];
    if(remainder<=0){c.hold[1]=1;memset(c.slope[1],0,16);}
    else for(unsigned k=0;k<4;k++)c.slope[1][k]=(c.color[2][k]-c.color[1][k])/remainder;
    for(unsigned i=0;i<2;i++)for(unsigned k=0;k<4;k++)if(!isfinite(c.slope[i][k]))return 1;
    *out=c;return 0;
}
int uc_particle_color_sample(const UCParticleColorCurve *c,float age,float rgba[4]){
    if(!c||!rgba||!arithmetic()||!isfinite(age))return 1;
    for(unsigned i=0;i<sizeof(*c)/sizeof(float);i++)if(!isfinite(((const float*)c)[i]))return 1;
    float result[4];
    if(age<=0)memcpy(result,c->color[0],16);
    else if(age>=1)memcpy(result,c->color[2],16);
    else if(age<c->hold[0])for(unsigned k=0;k<4;k++)result[k]=c->color[0][k]+c->slope[0][k]*age;
    else if(age>c->hold[1]){float t=age-c->hold[1];for(unsigned k=0;k<4;k++)result[k]=c->color[1][k]+c->slope[1][k]*t;}
    else memcpy(result,c->color[1],16);
    for(unsigned k=0;k<4;k++)if(!isfinite(result[k]))return 1;
    memcpy(rgba,result,16);return 0;
}
static void transform(const float m[16],const float p[3],float out[3]){
    for(unsigned k=0;k<3;k++)out[k]=(m[k]*p[0]+m[12+k])+(m[4+k]*p[1]+m[8+k]*p[2]);
}
int uc_particle_motion_quads(const UCParticleAsset *a,const UCParticleColorCurve *c,
    const UCParticle *particles,uint32_t count,const float matrix[16],unsigned thermal,UCParticleQuad *out){
    if(!a||!c||!matrix||(!particles&&count)||(!out&&count)||count>1048576||thermal>1||
       a->camera_relative>1||a->use_thermal_color>1||!arithmetic()||
       !finite_values(matrix,16)||!finite_values(&a->blur,1)||!finite_values(a->thermal_color,4))return 1;
    float checked[4];if(uc_particle_color_sample(c,0,checked))return 1;
    for(uint32_t i=0;i<count;i++)if(!finite_values(particles[i].position,3)||!finite_values(particles[i].velocity,3)||!finite_values(&particles[i].radius,1))return 1;
    for(uint32_t i=0;i<count;i++){
        const UCParticle *p=particles+i;UCParticleQuad q;float first[3],last[3],future[3],difference[3];
        for(unsigned k=0;k<3;k++)future[k]=p->position[k]+p->velocity[k]*a->blur;
        if(a->camera_relative){transform(matrix,p->position,first);transform(matrix,future,last);}
        else{memcpy(first,p->position,12);memcpy(last,future,12);}
        for(unsigned k=0;k<3;k++)difference[k]=last[k]-first[k];
        double x=difference[0],y=difference[1],z=difference[2];
        double squared=a->camera_relative?(x*x+z*z)+y*y:(z*z+y*y)+x*x;
        float inverse=(float)(1.0/sqrt(squared));
        for(unsigned k=0;k<3;k++)difference[k]*=inverse;
        float width[3]={difference[1]*matrix[10]-difference[2]*matrix[9],
            difference[2]*matrix[8]-difference[0]*matrix[10],difference[0]*matrix[9]-difference[1]*matrix[8]};
        for(unsigned k=0;k<3;k++){
            width[k]=(width[k]*p->radius)*.5f;
            q.position[0][k]=width[k]+first[k];q.position[1][k]=first[k]-width[k];
            q.position[2][k]=width[k]+last[k];q.position[3][k]=last[k]-width[k];
        }
        if(a->use_thermal_color&&thermal)memcpy(q.color,a->thermal_color,16);
        else if(uc_particle_color_sample(c,(float)p->age*(1.0f/65535.0f),q.color))return 2;
        for(unsigned k=0;k<12;k++)if(!isfinite(((float*)q.position)[k]))return 2;
        out[i]=q;
    }
    return 0;
}

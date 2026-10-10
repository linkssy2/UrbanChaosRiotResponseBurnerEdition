#include "particle_spawn.h"
#include <math.h>
#include <fenv.h>
#include <xmmintrin.h>
static double sample(UCGameRandom *s,float lo,float hi){
    int32_t word;uc_game_random_next(s,&word);
    return ((double)word*((double)hi-lo))*0x1p-31+lo;
}
static int bounded(float x){return isfinite(x)&&fabsf(x)<=65536;}
int uc_particle_spawn(const UCParticleSpawn *in,UCGameRandom *rng,UCParticle *out){
    if(!in||!rng||!out||in->shape>2||rng->first>=250||rng->second>=250||
       !bounded(in->angle_min)||!bounded(in->angle_max)||!bounded(in->force_min)||!bounded(in->force_max)||
       !bounded(in->radius)||fegetround()!=FE_TONEAREST||(_mm_getcsr()&0xE040u))return 1;
    for(unsigned j=0;j<3;j++)if(!bounded(in->size[j]))return 1;
    for(unsigned j=0;j<16;j++)if(!bounded(in->current[j])||!bounded(in->previous[j]))return 1;
    float v[3];
    if(in->shape==0){
        v[2]=(float)sample(rng,-.5f,.5f);v[1]=(float)sample(rng,-.5f,.5f);v[0]=(float)sample(rng,-.5f,.5f);
    }else {
        double length;
        if(in->shape==1){
            v[2]=(float)sample(rng,-1,1);v[1]=(float)sample(rng,-1,1);v[0]=(float)sample(rng,-1,1);
            length=((double)v[2]*v[2]+(double)v[1]*v[1])+(double)v[0]*v[0];
        }else {
            v[0]=(float)sample(rng,0,1);v[1]=(float)sample(rng,-1,1);
            double z=sample(rng,-1,1);v[2]=(float)z;
            length=(z*v[2]+(double)v[1]*v[1])+(double)v[0]*v[0];
        }
        float inverse=(float)(1/sqrt(length));
        for(unsigned j=0;j<3;j++)v[j]*=inverse;
        float radius=(float)sample(rng,-.5f,.5f);
        for(unsigned j=0;j<3;j++)v[j]*=radius;
    }
    for(unsigned j=0;j<3;j++)v[j]*=in->size[j];
    UCParticle p={0};p.radius=in->radius;
    for(unsigned j=0;j<3;j++)p.position[j]=(in->current[j]*v[0]+in->current[4+j]*v[1])+(in->current[8+j]*v[2]+in->current[12+j]);
    float mix=(float)sample(rng,0,1);
    for(unsigned j=0;j<3;j++)p.position[j]-=(in->current[12+j]-in->previous[12+j])*mix;
    double unit=sample(rng,0,1),low=1-cos((double)in->angle_min*0x1.921fb6p+1),high=1-cos((double)in->angle_max*0x1.921fb6p+1);
    float cosine=(float)(1-((high-low)*unit+low));if(cosine<-1)cosine=-1;if(cosine>1)cosine=1;
    float angle=(float)acos((double)cosine);double azimuth=sample(rng,0,0x1.921fb6p+2f);
    v[0]=(float)cos((double)angle);v[1]=(float)(cos(azimuth)*sin((double)angle));v[2]=(float)(sin(azimuth)*sin((double)angle));
    float direction[3];for(unsigned j=0;j<3;j++)direction[j]=(in->current[j]*v[0]+in->current[4+j]*v[1])+in->current[8+j]*v[2];
    mix=(float)sample(rng,0,1);float force=(in->force_max-in->force_min)*mix+in->force_min;
    for(unsigned j=0;j<3;j++)p.velocity[j]=direction[j]*force;
    mix=(float)sample(rng,0,1);
    for(unsigned j=0;j<3;j++)p.position[j]+=(p.velocity[j]*0x1.111112p-5f)*mix;
    int32_t seed;uc_game_random_next(rng,&seed);p.seed=(uint16_t)seed;
    for(unsigned j=0;j<3;j++)if(!isfinite(p.position[j])||!isfinite(p.velocity[j]))return 2;
    *out=p;return 0;
}

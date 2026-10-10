#include "particle_plain.h"
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int arithmetic(void){return !(_mm_getcsr()&0xE040u);}
static int bounded(const float *p,unsigned n){for(unsigned i=0;i<n;i++)if(!isfinite(p[i])||fabsf(p[i])>65536)return 0;return 1;}
int uc_particle_plain_prepare(const UCParticleAsset *a,unsigned blood,unsigned enabled,
    unsigned thermal,uint32_t count,const void *texture,UCParticleColorCurve *curve,UCParticleSpriteAdmission *out){
    if(!a||!curve||!out||blood>1||enabled>1||thermal>1||count>1048576||a->render_in_thermal>1||!arithmetic())return 1;
    if(blood&&!enabled){out->count=0;return 0;}
    UCParticleColorCurve c;if(uc_particle_color_prepare(a->color,a->hold,&c))return 1;
    *curve=c;out->count=(!a->render_in_thermal&&thermal)?0:count;out->blend=a->blend;out->texture=texture;return 0;
}
int uc_particle_plain_draw(const UCParticleAsset *a,const UCParticleColorCurve *c,const UCParticle *p,
    uint32_t count,const float m[16],unsigned near_fade,unsigned blood,unsigned thermal,UCParticlePlainSprite *out){
    if(!a||!c||!m||(!p&&count)||(!out&&count)||count>1048576||near_fade>1||blood>1||thermal>1||
       a->camera_relative>1||a->use_thermal_color>1||!arithmetic()||!bounded(m,16)||!bounded(a->thermal_color,4))return 1;
    float checked[4];if(uc_particle_color_sample(c,0,checked))return 1;
    for(uint32_t i=0;i<count;i++)if(!bounded(p[i].position,3)||!bounded(&p[i].radius,1))return 1;
    float d=(m[8]*-m[12]+m[10]*-m[14])+(m[9]*-m[13]+m[11]*-m[15]);
    for(uint32_t i=0;i<count;i++){
        UCParticlePlainSprite v;float scale=1;v.radius=p[i].radius;
        if(a->camera_relative)for(unsigned k=0;k<3;k++)v.center[k]=(m[k]*p[i].position[0]+m[12+k])+(m[k+4]*p[i].position[1]+m[k+8]*p[i].position[2]);
        else{
            memcpy(v.center,p[i].position,12);
            if(near_fade&&!blood){float depth=(m[8]*p[i].position[0]+m[9]*p[i].position[1])+(m[10]*p[i].position[2]+d);
                if(depth<150){scale=depth*(1.f/150);scale*=scale;}v.radius=scale*v.radius;}
        }
        if(a->use_thermal_color&&thermal)memcpy(v.color,a->thermal_color,16);
        else{if(uc_particle_color_sample(c,(float)p[i].age*(1.f/65535),v.color))return 2;if(!a->camera_relative)v.color[3]*=scale;}
        for(unsigned k=0;k<sizeof v/sizeof(float);k++)if(!isfinite(((const float*)&v)[k]))return 2;out[i]=v;
    }
    return 0;
}

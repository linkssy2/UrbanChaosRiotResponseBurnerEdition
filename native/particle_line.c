#include "particle_line.h"
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int bounded(const float *p,unsigned n){for(unsigned i=0;i<n;i++)if(!isfinite(p[i])||fabsf(p[i])>65536)return 0;return 1;}
int uc_particle_line_prepare(const UCParticleAsset *a,unsigned blood,unsigned enabled,
    uint32_t count,const void *texture,UCParticleSpriteAdmission *out){
    if(!a||!out||blood>1||enabled>1||count>1048576)return 1;
    if(blood&&!enabled){out->count=0;return 0;}
    out->count=count;out->blend=a->blend;out->texture=texture;return 0;
}
int uc_particle_lines(const UCParticleAsset *a,const UCParticle *p,uint32_t count,
    const float camera[16],const float global_wind[3],float delta,UCParticleLine *out){
    if(!a||!camera||!global_wind||(!p&&count)||(!out&&count)||count>1048576||a->camera_relative>1||
       (_mm_getcsr()&0xE040u)||!bounded(camera,16)||!bounded(global_wind,3)||
       !bounded(a->simulation.wind,3)||!bounded(&a->simulation.global_wind,1)||
       !bounded(&a->line_length,1)||!bounded(a->color[0],4)||!bounded(a->color[2],4)||!isfinite(delta)||fabsf(delta)>1)return 1;
    for(uint32_t i=0;i<count;i++)if(!bounded(p[i].position,3)||!bounded(p[i].velocity,3))return 1;
    const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    const float *m=a->camera_relative?camera:identity;
    float wind[3],length=a->line_length*delta;
    for(unsigned k=0;k<3;k++)wind[k]=a->simulation.wind[k]+global_wind[k]*a->simulation.global_wind;
    for(uint32_t i=0;i<count;i++){
        UCParticleLine v;float direction[3];
        for(unsigned k=0;k<3;k++){
            v.position[0][k]=(m[k]*p[i].position[0]+m[12+k])+(m[k+4]*p[i].position[1]+m[k+8]*p[i].position[2]);
            direction[k]=p[i].velocity[k]+wind[k];
        }
        double squared=((double)direction[2]*direction[2]+(double)direction[1]*direction[1])+(double)direction[0]*direction[0];
        if(!(squared>0))return 2;
        float inverse=(float)(1./sqrt(squared));
        for(unsigned k=0;k<3;k++)v.position[1][k]=v.position[0][k]-(direction[k]*inverse)*length;
        memcpy(v.color[0],a->color[0],16);memcpy(v.color[1],a->color[2],16);
        for(unsigned j=0;j<2;j++)for(unsigned k=0;k<3;k++)if(!isfinite(v.position[j][k]))return 2;
        out[i]=v;
    }
    return 0;
}

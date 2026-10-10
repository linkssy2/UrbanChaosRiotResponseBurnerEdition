#include "particle_sprite.h"
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int arithmetic(void){return !(_mm_getcsr()&0xE040u);}
static int bounded(const float *p,unsigned n){for(unsigned i=0;i<n;i++)if(!isfinite(p[i])||fabsf(p[i])>65536)return 0;return 1;}
int uc_particle_sprite_random(uint32_t *seed,float table[16]){
    if(!seed||!table||!arithmetic())return 1;
    for(unsigned i=0;i<16;i++){*seed=*seed*214013u+2531011u;table[i]=(float)((*seed>>16)&32767u)*(1.0f/32767.0f);}
    return 0;
}
int uc_particle_sprite_prepare(const UCParticleAsset *a,unsigned blood,unsigned enabled,
    unsigned thermal,uint32_t count,const void *texture,UCParticleSpriteState *state,UCParticleSpriteAdmission *out){
    if(!a||!state||!out||blood>1||enabled>1||thermal>1||count>1048576||a->render_in_thermal>1||!arithmetic())return 1;
    if(blood&&!enabled){out->count=0;return 0;}
    if(!bounded(a->rotation,4)||!bounded(&a->simulation.lifetime,1))return 1;
    UCParticleSpriteState s;if(uc_particle_color_prepare(a->color,a->hold,&s.curve))return 1;
    s.speed=a->rotation[0];s.speed_delta=a->rotation[1]-a->rotation[0];
    s.offset=a->rotation[2];s.offset_delta=a->rotation[3]-a->rotation[2];
    s.lifetime_radians=a->simulation.lifetime*6.283185482025146484375f;
    *state=s;out->count=(!a->render_in_thermal&&thermal)?0:count;out->blend=a->blend;out->texture=texture;return 0;
}
int uc_particle_sprite_draw(const UCParticleAsset *a,const UCParticleSpriteState *s,
    const UCParticle *p,uint32_t count,const float m[16],const float table[16],
    unsigned near_fade,unsigned blood,unsigned thermal,UCParticleSprite *out){
    if(!a||!s||!m||!table||(!p&&count)||(!out&&count)||count>1048576||near_fade>1||blood>1||thermal>1||
       a->camera_relative>1||a->use_thermal_color>1||!arithmetic()||!bounded(m,16)||!bounded(table,16)||
       !bounded(a->thermal_color,4))return 1;
    for(unsigned k=0;k<sizeof(*s)/sizeof(float);k++)if(!isfinite(((const float*)s)[k]))return 1;
    for(uint32_t i=0;i<count;i++)if(!bounded(p[i].position,3)||!bounded(&p[i].radius,1))return 1;
    float d=(m[8]*-m[12]+m[10]*-m[14])+(m[9]*-m[13]+m[11]*-m[15]);
    for(uint32_t i=0;i<count;i++){
        UCParticleSprite v;float age=(float)p[i].age*(1.0f/65535.0f),random=table[p[i].seed&15u],scale=1;
        v.angle=(s->speed_delta*random+s->speed)*(s->lifetime_radians*age)+
            (s->offset_delta*random+s->offset)*6.283185482025146484375f;
        v.radius=p[i].radius;
        if(a->camera_relative)for(unsigned k=0;k<3;k++)v.center[k]=(m[k]*p[i].position[0]+m[12+k])+(m[k+4]*p[i].position[1]+m[k+8]*p[i].position[2]);
        else{
            memcpy(v.center,p[i].position,12);
            if(near_fade&&!blood){
                float depth=(m[8]*p[i].position[0]+m[9]*p[i].position[1])+(m[10]*p[i].position[2]+d);
                if(depth<150){scale=depth*(1.0f/150.0f);scale*=scale;}
                v.radius=scale*v.radius;
            }
        }
        if(a->use_thermal_color&&thermal)memcpy(v.color,a->thermal_color,16);
        else {if(uc_particle_color_sample(&s->curve,age,v.color))return 2;if(!a->camera_relative)v.color[3]*=scale;}
        for(unsigned k=0;k<sizeof v/sizeof(float);k++)if(!isfinite(((const float*)&v)[k]))return 2;
        out[i]=v;
    }
    return 0;
}

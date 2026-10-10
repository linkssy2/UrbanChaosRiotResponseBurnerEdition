#include "particle_rotating_quad.h"
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int arithmetic(void){return !(_mm_getcsr()&0xE040u);}
static int bounded(const float *p,unsigned n){for(unsigned i=0;i<n;i++)if(!isfinite(p[i])||fabsf(p[i])>65536)return 0;return 1;}
int uc_particle_quad_axes(uint32_t *seed,float sine[16],float cosine[16]){
    if(!seed||!sine||!cosine||!arithmetic())return 1;
    for(unsigned i=0;i<16;i++){
        *seed=*seed*214013u+2531011u;
        double angle=(double)((*seed>>16)&32767u)*(double)(1.f/32767.f)*(double)6.283185482025146484375f;
        sine[i]=(float)sin(angle);cosine[i]=(float)cos(angle);
    }return 0;
}
int uc_particle_rotating_quads(const UCParticleAsset *a,const UCParticleSpriteState *s,
    const UCParticle *p,uint32_t count,const float camera[16],const float random[16],
    const float sine[16],const float cosine[16],unsigned thermal,UCParticleQuad *out){
    if(!a||!s||!camera||!random||!sine||!cosine||(!p&&count)||(!out&&count)||count>1048576||
       thermal>1||a->camera_relative>1||a->use_thermal_color>1||!arithmetic()||
       !bounded(camera,16)||!bounded(random,16)||!bounded(sine,16)||!bounded(cosine,16)||!bounded(a->thermal_color,4))return 1;
    for(unsigned k=0;k<sizeof(*s)/sizeof(float);k++)if(!isfinite(((const float*)s)[k]))return 1;
    for(uint32_t i=0;i<count;i++)if(!bounded(p[i].position,3)||!bounded(&p[i].radius,1))return 1;
    const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    const float *m=a->camera_relative?camera:identity;
    for(uint32_t i=0;i<count;i++){
        UCParticleQuad q;unsigned index=p[i].seed&15u;float age=(float)p[i].age*(1.f/65535.f);
        double t=random[index],angle=((t*s->speed_delta+s->speed)*((double)age*s->lifetime_radians))+
            (t*s->offset_delta+s->offset)*(double)6.283185482025146484375f;
        float sn=(float)sin(angle),cs=(float)cos(angle),ax=sine[index],ay=cosine[index],r=p[i].radius;
        float sx=(sn*r)*ax,sy=(sn*ay)*r,one=1-cs;
        float xx=((one*ax)*ax+cs)*r,yy=((one*ay)*ay+cs)*r,xy=((one*r)*ay)*ax;
        float center[3];for(unsigned k=0;k<3;k++)center[k]=(m[k]*p[i].position[0]+m[12+k])+(m[k+4]*p[i].position[1]+m[k+8]*p[i].position[2]);
        float left=center[0]+xx,right=center[0]-xx,top=center[1]+xy,bottom=center[1]-xy;
        float front=center[2]+(0-sy),back=center[2]-(0-sy);
        q.position[0][0]=left+xy;q.position[0][1]=top+yy;q.position[0][2]=front+sx;
        q.position[1][0]=left-xy;q.position[1][1]=top-yy;q.position[1][2]=front-sx;
        q.position[2][0]=right+xy;q.position[2][1]=bottom+yy;q.position[2][2]=back+sx;
        q.position[3][0]=right-xy;q.position[3][1]=bottom-yy;q.position[3][2]=back-sx;
        if(a->use_thermal_color&&thermal)memcpy(q.color,a->thermal_color,16);
        else if(uc_particle_color_sample(&s->curve,age,q.color))return 2;
        for(unsigned k=0;k<sizeof(q)/sizeof(float);k++)if(!isfinite(((const float*)&q)[k]))return 2;
        out[i]=q;
    }return 0;
}

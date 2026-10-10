#include "particle_stretch.h"
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int bounded(const float *p,unsigned n){for(unsigned i=0;i<n;i++)if(!isfinite(p[i])||fabsf(p[i])>65536)return 0;return 1;}
int uc_particle_stretch_prepare(const UCParticleAsset *a,unsigned blood,unsigned enabled,
    uint32_t count,const void *texture,UCParticleStretchState *out,UCParticleSpriteAdmission *admit){
    if(!a||!out||!admit||blood>1||enabled>1||count>1048576||(_mm_getcsr()&0xE040u))return 1;
    if(blood&&!enabled){admit->count=0;return 0;}
    if(!bounded(&a->stretch_start,5))return 1;
    UCParticleStretchState s;UCParticleSpriteAdmission d;
    if(uc_particle_sprite_prepare(a,blood,enabled,0,count,texture,&s.sprite,&d))return 1;
    s.amount[0]=a->stretch_start;s.amount[1]=a->stretch_hold;s.amount[2]=a->stretch_end;
    s.hold[0]=a->stretch_hold_start;s.hold[1]=a->stretch_hold_end;
    if(s.hold[0]<=0){s.hold[0]=0;s.slope[0]=0;}else s.slope[0]=(s.amount[1]-s.amount[0])/s.hold[0];
    float tail=1-s.hold[1];
    if(tail<=0){s.hold[1]=1;s.slope[1]=0;}else s.slope[1]=(s.amount[2]-s.amount[1])/tail;
    *out=s;*admit=d;return 0;
}
static double sample(const UCParticleStretchState *s,float age){
    if(age<=0)return s->amount[0];if(age>=1)return s->amount[2];
    if(age<s->hold[0])return (double)age*s->slope[0]+s->amount[0];
    if(age>s->hold[1])return ((double)age-s->hold[1])*s->slope[1]+s->amount[1];
    return s->amount[1];
}
static int corner(const float a[3],const float b[3],const float direction[3],const float stretch[3],float out[3]){
    float v[3];for(unsigned k=0;k<3;k++)v[k]=a[k]+b[k];
    double square=((double)v[2]*v[2]+(double)v[0]*v[0])+(double)v[1]*v[1];
    if(!(square>0))return 0;float inverse=(float)(1./sqrt(square));
    float x=v[0]*inverse,y=v[1]*inverse,z=v[2]*inverse;
    float dot=(direction[2]*z+direction[1]*y)+direction[0]*x;
    if(dot<0)dot=0;else if(dot>1)dot=1;
    for(unsigned k=0;k<3;k++)out[k]=stretch[k]*dot+v[k];return 1;
}
int uc_particle_stretched_quads(const UCParticleAsset *a,const UCParticleStretchState *s,
    const UCParticle *p,uint32_t count,const float m[16],const float random[16],unsigned thermal,UCParticleQuad *out){
    if(!a||!s||!m||!random||(!p&&count)||(!out&&count)||count>1048576||thermal>1||a->camera_relative>1||a->use_thermal_color>1||
       (_mm_getcsr()&0xE040u)||!bounded(m,16)||!bounded(random,16)||!bounded(a->stretch_direction,2)||
       !isfinite(a->stretch_power)||fabsf(a->stretch_power)>16||!bounded(a->thermal_color,4))return 1;
    for(unsigned k=0;k<sizeof(*s)/sizeof(float);k++)if(!isfinite(((const float*)s)[k]))return 1;
    for(uint32_t i=0;i<count;i++)if(!bounded(p[i].position,3)||!bounded(&p[i].radius,1))return 1;
    for(uint32_t i=0;i<count;i++){
        UCParticleQuad q;float age=(float)p[i].age*(1.f/65535),r=random[p[i].seed&15];
        float angle=(s->sprite.speed_delta*r+s->sprite.speed)*(s->sprite.lifetime_radians*age)+
            (s->sprite.offset_delta*r+s->sprite.offset)*6.283185482025146484375f;
        float sn=(float)(sin((double)angle)*p[i].radius),cs=(float)(cos((double)angle)*p[i].radius);
        float u[3],v[3],nu[3],nv[3],center[3],stretch[3],direction[3]={0,0,0};
        double amount=sample(s,age);if(!isfinite(amount)||(amount<0&&truncf(a->stretch_power)!=a->stretch_power))return 2;
        float power=(float)pow(amount,a->stretch_power);if(!isfinite(power))return 2;
        for(unsigned k=0;k<3;k++){
            u[k]=m[k]*sn+m[4+k]*cs;v[k]=m[k]*(0.f-cs)+m[4+k]*sn;nu[k]=0.f-u[k];nv[k]=0.f-v[k];
            center[k]=a->camera_relative?(m[k]*p[i].position[0]+m[12+k])+(m[k+4]*p[i].position[1]+m[k+8]*p[i].position[2]):p[i].position[k];
            stretch[k]=(m[4+k]*a->stretch_direction[1]+m[k]*a->stretch_direction[0])*power;
        }
        if(a->camera_relative){
            double square=((double)stretch[0]*stretch[0]+(double)stretch[2]*stretch[2])+(double)stretch[1]*stretch[1];
            if(!(square>0))return 2;float inverse=(float)(1./sqrt(square));for(unsigned k=0;k<3;k++)direction[k]=stretch[k]*inverse;
        }else{
            float square=(stretch[0]*stretch[0]+stretch[2]*stretch[2])+stretch[1]*stretch[1];
            if(square>0.0001f){double inverse=1./sqrt((double)square);for(unsigned k=0;k<3;k++)direction[k]=(float)((double)stretch[k]*inverse);}
        }
        if(!corner(u,v,direction,stretch,q.position[0])||!corner(u,nv,direction,stretch,q.position[1])||
           !corner(nu,nv,direction,stretch,q.position[3])||!corner(nu,v,direction,stretch,q.position[2]))return 2;
        for(unsigned j=0;j<4;j++)for(unsigned k=0;k<3;k++){q.position[j][k]+=center[k];if(!isfinite(q.position[j][k]))return 2;}
        if(a->use_thermal_color&&thermal)memcpy(q.color,a->thermal_color,16);
        else if(uc_particle_color_sample(&s->sprite.curve,age,q.color))return 2;
        out[i]=q;
    }
    return 0;
}

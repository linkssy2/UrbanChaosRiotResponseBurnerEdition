#include "particle_system.h"
#include <math.h>
#include <string.h>
static int bounded(float x){return isfinite(x)&&fabsf(x)<=65536;}
int uc_particle_system_step_emission(UCParticleSystem *s,const UCParticleDefinition *d,float dt,
    const float wind[3],UCGameRandom *rng,const UCParticleSource *source,const UCParticleEmission *emission){
    if(!s||!d||!wind||!rng||!source||!source->dead||!source->sample||
       (!s->particles&&s->capacity)||s->count>s->capacity||s->capacity>1048576||
       !isfinite(dt)||dt>1||d->shape>2||rng->first>=250||rng->second>=250||
       !bounded(d->angle_min)||!bounded(d->angle_max)||!bounded(d->force_min)||!bounded(d->force_max)||
       !bounded(d->drag)||!bounded(d->start_size)||!bounded(d->end_size)||
       !bounded(d->global_wind)||!bounded(d->lifetime)||d->lifetime<=0)return 1;
    for(unsigned j=0;j<3;j++)if(!bounded(d->size[j])||!bounded(d->gravity[j])||!bounded(d->wind[j])||!bounded(wind[j]))return 1;
    for(unsigned j=0;j<16;j++)if(!bounded(s->previous[j]))return 1;
    if(dt<0)dt=0;
    UCParticleTriggerInfo info;if(uc_particle_trigger_query(&s->trigger,dt,&info))return 1;
    UCParticleMotion motion={0};motion.delta=dt;motion.size_start=d->start_size;motion.size_delta=d->end_size-d->start_size;
    float age=(dt/d->lifetime)*65535.f;if(age>=65535)age=65535;
    motion.age_step=(uint16_t)age;if(dt>=.01f&&!motion.age_step)motion.age_step=1;
    motion.drag=(1.f-1.f/(dt+1.f))*d->drag;
    for(unsigned j=0;j<3;j++){
        motion.gravity_step[j]=d->gravity[j]*dt;
        // Preserve source operand order, including the middle component.
        float global=wind[j]*d->global_wind;
        motion.wind_step[j]=(j==1?global+d->wind[j]:d->wind[j]+global)*dt;
    }
    uint32_t empty=0;UCParticleBounds unused;
    if(uc_particle_motion(NULL,0,&empty,&motion,&unused))return 1;
    if(!info.finished){
        uint8_t dead;if(source->dead(source->user,&dead)||dead>1)return 2;
        if(!dead){
            UCParticleSpawn birth={0};birth.shape=d->shape;memcpy(birth.size,d->size,12);
            birth.angle_min=d->angle_min;birth.angle_max=d->angle_max;birth.force_min=d->force_min;birth.force_max=d->force_max;birth.radius=d->start_size;
            memcpy(birth.previous,s->previous,64);
            if(source->sample(source->user,&s->bounds,birth.current))return 2;
            for(unsigned j=0;j<16;j++)if(!bounded(birth.current[j]))return 2;
            uint32_t n=0;if(emission?(!emission->emit||emission->emit(emission->owner,&s->trigger,dt,&n)):uc_particle_trigger_emit(&s->trigger,dt,&n))return 1;
            while(n--&&s->count<s->capacity){
                if(uc_particle_spawn(&birth,rng,s->particles+s->count))return 3;
                ++s->count;
            }
            memcpy(s->previous,birth.current,64);
        }
    }
    return uc_particle_motion(s->particles,s->capacity,&s->count,&motion,&s->bounds)?4:0;
}
int uc_particle_system_step(UCParticleSystem*s,const UCParticleDefinition*d,float dt,const float*w,UCGameRandom*r,const UCParticleSource*source){return uc_particle_system_step_emission(s,d,dt,w,r,source,NULL);}

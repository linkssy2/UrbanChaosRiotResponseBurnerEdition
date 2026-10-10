#include "particle_lifecycle.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
typedef struct System {
    UCParticleLifecycleView view;
    UCParticleSystem state;
    float matrix[16];
    uintptr_t registration;
    UCParticleSource source;
    UCParticleEmission emission;
} System;
struct UCParticleLifecycle {
    System **items;
    uint32_t count,capacity;
    unsigned active,poisoned;
    UCParticleLifecycleHooks hooks;
    void *user;
};
static int bounded(float x){return isfinite(x)&&fabsf(x)<=65536;}
static int dead(void *p,uint8_t *out){(void)p;*out=0;return 0;}
static int sample(void *p,const UCParticleBounds *b,float out[16]){(void)b;memcpy(out,p,64);return 0;}
static int admission(const UCParticleAsset *a,const float *m,UCParticleTrigger *t,uint32_t *estimate){
    if(!a||!m||(a->particle_type!=0&&a->particle_type!=1&&a->particle_type!=2&&a->particle_type!=3&&a->particle_type!=5)||a->emitter_type>1)return 0;
    const UCParticleDefinition *d=&a->simulation;
    if(d->shape>2||!bounded(d->lifetime)||d->lifetime<=0||
       !bounded(d->angle_min)||!bounded(d->angle_max)||!bounded(d->force_min)||!bounded(d->force_max)||
       !bounded(d->drag)||!bounded(d->start_size)||!bounded(d->end_size)||!bounded(d->global_wind))return 0;
    for(unsigned j=0;j<3;j++)if(!bounded(d->size[j])||!bounded(d->gravity[j])||!bounded(d->wind[j]))return 0;
    for(unsigned i=0;i<4;i++)for(unsigned j=0;j<3;j++)if(!bounded(m[i*4+j]))return 0;
    if(a->emitter_type){if(a->burst<0||uc_particle_trigger_burst(t,(uint32_t)a->burst))return 0;*estimate=t->burst;}
    else {
        if(uc_particle_trigger_constant(t,a->rate,a->emitter_lifetime))return 0;
        float span=t->kind==UC_PARTICLE_FINITE&&t->lifetime<d->lifetime?t->lifetime:d->lifetime;
        double n=ceil((double)span*t->rate);if(n>65536)return 0;*estimate=(uint32_t)n;
    }
    UCParticleTriggerInfo info;
    return *estimate<=65536&&!uc_particle_trigger_query(t,0,&info);
}
int uc_particle_lifecycle_create(uint32_t capacity,const UCParticleLifecycleHooks *h,void *user,UCParticleLifecycle **out){
    if(!out||!h||!h->register_system||!h->unregister_system||!capacity||capacity>4096)return 1;
    UCParticleLifecycle *m=calloc(1,sizeof(*m));if(!m)return 2;
    m->items=calloc(capacity,sizeof(*m->items));if(!m->items){free(m);return 2;}
    m->capacity=capacity;m->hooks=*h;m->user=user;*out=m;return 0;
}
static int append(UCParticleLifecycle *m,const UCParticleAsset *a,const float matrix[16],const UCParticleSource *source,const UCParticleEmission *emission){
    UCParticleTrigger t;uint32_t capacity;
    if(!m||m->active||m->poisoned||!admission(a,matrix,&t,&capacity))return 1;
    if(source&&(!source->dead||!source->sample))return 1;
    if(emission&&!emission->emit)return 1;
    if(m->count==m->capacity)return 3;
    System *s=calloc(1,sizeof(*s));if(!s)return 2;
    s->state.particles=calloc(capacity?capacity:1,sizeof(UCParticle));
    if(!s->state.particles){free(s);return 2;}
    s->state.capacity=capacity;s->state.trigger=t;
    if(emission)s->emission=*emission;
    for(unsigned i=0;i<4;i++)memcpy(s->matrix+i*4,matrix+i*4,12);
    s->matrix[15]=1;memcpy(s->state.previous,s->matrix,64);
    m->active=1;
    if(source){
        s->source=*source;
        int failed=s->source.sample(s->source.user,&s->state.bounds,s->matrix);
        for(unsigned i=0;i<16;i++)if(!bounded(s->matrix[i]))failed=1;
        if(failed){free(s->state.particles);free(s);m->active=0;m->poisoned=1;return 4;}
        memcpy(s->state.previous,s->matrix,64);
    }
    s->view.asset=a;s->view.system=&s->state;s->view.source_matrix=s->matrix;
    s->registration=m->hooks.register_system(m->user,&s->view);
    m->items[m->count++]=s;m->active=0;return 0;
}
int uc_particle_lifecycle_append(UCParticleLifecycle *m,const UCParticleAsset *a,const float matrix[16]){return append(m,a,matrix,NULL,NULL);}
int uc_particle_lifecycle_append_source(UCParticleLifecycle *m,const UCParticleAsset *a,const UCParticleSource *source){
    static const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    return source?append(m,a,identity,source,NULL):1;
}
int uc_particle_lifecycle_append_emission(UCParticleLifecycle*m,const UCParticleAsset*a,const UCParticleSource*source,const UCParticleEmission*emission){
    static const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    return source&&emission?append(m,a,identity,source,emission):1;
}
static void destroy(UCParticleLifecycle *m,System *s){
    m->hooks.unregister_system(m->user,s->registration,&s->view);
    free(s->state.particles);/* Fixed source and trigger are owned POD members. */
    free(s);
}
int uc_particle_lifecycle_step(UCParticleLifecycle *m,float dt,const float wind[3],UCGameRandom *rng){
    if(!m||m->active||m->poisoned||!wind||!rng||!isfinite(dt)||dt>1||rng->first>=250||rng->second>=250)return 1;
    for(unsigned j=0;j<3;j++)if(!bounded(wind[j]))return 1;
    m->active=1;
    for(uint32_t i=0;i<m->count;){
        System *s=m->items[i];UCParticleSource source={s->matrix,dead,sample};UCParticleTriggerInfo info;
        if(uc_particle_system_step_emission(&s->state,&s->view.asset->simulation,dt,wind,rng,s->source.sample?&s->source:&source,s->emission.emit?&s->emission:NULL)||
           uc_particle_trigger_query(&s->state.trigger,0,&info)){m->active=0;m->poisoned=1;return 4;}
        if(s->source.sample)memcpy(s->matrix,s->state.previous,64);
        if(!s->state.count&&info.finished){destroy(m,s);if(--m->count)m->items[i]=m->items[m->count];}
        else ++i;
    }
    m->active=0;return 0;
}
uint32_t uc_particle_lifecycle_count(const UCParticleLifecycle *m){return m?m->count:0;}
const UCParticleLifecycleView *uc_particle_lifecycle_at(const UCParticleLifecycle *m,uint32_t i){return m&&i<m->count?&m->items[i]->view:NULL;}
int uc_particle_lifecycle_clear(UCParticleLifecycle *m){
    if(!m||m->active)return 1;m->active=1;
    for(uint32_t i=0;i<m->count;i++)destroy(m,m->items[i]);
    m->count=0;m->poisoned=0;m->active=0;return 0;
}
int uc_particle_lifecycle_close(UCParticleLifecycle *m){
    if(!m)return 0;if(uc_particle_lifecycle_clear(m))return 1;free(m->items);free(m);return 0;
}
int uc_particle_lifecycle_geometry(const UCParticleLifecycleView *v,const float matrix[16],const float table[16],
    const void *texture,unsigned blood,unsigned enabled,unsigned thermal,unsigned near_fade,
    UCParticleQuad *quads,UCParticleSprite *sprites,uint32_t capacity,UCParticleSpriteAdmission *out){
    if(!v||!v->asset||!v->system||!out||capacity<v->system->count)return 1;
    int result;
    if(v->asset->particle_type==1){
        UCParticleColorCurve curve;
        result=uc_particle_plain_prepare(v->asset,blood,enabled,thermal,v->system->count,texture,&curve,out);
        if(result||!out->count)return result;
        return uc_particle_motion_quads(v->asset,&curve,v->system->particles,out->count,matrix,thermal,quads);
    }
    if(v->asset->particle_type==3){
        UCParticleSpriteState state;
        result=uc_particle_sprite_prepare(v->asset,blood,enabled,thermal,v->system->count,texture,&state,out);
        if(result||!out->count)return result;
        return uc_particle_sprite_draw(v->asset,&state,v->system->particles,out->count,matrix,table,near_fade,blood,thermal,sprites);
    }
    return 1;
}

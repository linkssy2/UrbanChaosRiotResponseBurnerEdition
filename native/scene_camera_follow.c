#include "scene_camera_follow.h"
#include "firstperson_camera.h"
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int finite(const float *p,unsigned n){for(unsigned i=0;i<n;i++)if(!isfinite(p[i]))return 0;return 1;}
static int extra(const UCEntityFreshView *v,const UCSceneCamera *c){
    float out[16];
    for(unsigned r=0;r<4;r++)for(unsigned j=0;j<4;j++){
        float a=v->transform[j]*c->extra[r*4],b=v->transform[j+4]*c->extra[r*4+1];
        float d=v->transform[j+8]*c->extra[r*4+2],e=v->transform[j+12]*c->extra[r*4+3];
        out[r*4+j]=(a+b)+(d+e);
    }
    if(!finite(out,16))return 2;memcpy(v->transform,out,sizeof(out));return 0;
}
int uc_scene_camera_follow(const UCEntityFreshView *v,UCSceneProjection *p,
    UCSceneCamera *c,const UCSceneCameraFollowServices *s){
    if(!v||!p||!c||!s||!v->transform||!v->id||!v->target.parent_id||
       !s->registry||!s->debug_flags||!s->preflight||!s->player||
       !s->current_provider||!s->provider_mode||!s->input||!s->remove||
       (_mm_getcsr()&0xE040)||!finite(v->transform,16)||!finite(c->extra,16))return 1;
    if(c->mode==2){
        if(!c->source_enabled||c->target_enabled||c->source_tag||*s->debug_flags||
           !finite(&p->fov,1)||!finite(&c->target_fov,1)||!finite(&c->fov_damper,1)||
           !finite(c->to_offset,3)||!finite(c->target,3)||
           !finite(&c->from_damper,1)||!finite(&c->to_damper,1))return 1;
        UCEntityEventTarget *t=uc_entity_event_lookup(s->registry,c->source_id);
        const UCSceneCameraPlayer *a=t?s->player(s->owner,t):NULL;
        if(t&&(!a||!a->flags))return 1;
        int removing=c->source_id!=-1&&c->source_id==*v->target.parent_id&&(!t||(*a->flags&0x8000));
        if(!removing&&(!a||!a->tag0||!finite(a->tag0,16)))return 1;
    }
    if(s->preflight(s->owner))return 1;
    if(c->mode!=2)return extra(v,c);
    if(p->fov!=c->target_fov)p->fov+=(c->target_fov-p->fov)*c->fov_damper;
    UCEntityEventTarget *t=uc_entity_event_lookup(s->registry,c->source_id);
    const UCSceneCameraPlayer *a=t?s->player(s->owner,t):NULL;
    if(c->source_id!=-1&&c->source_id==*v->target.parent_id&&(!t||(*a->flags&0x8000))){
        s->remove(s->owner);return extra(v,c);
    }
    UCFirstpersonCameraInput in={0};
    memcpy(in.player_frame,a->tag0,64);memcpy(in.to_offset,c->to_offset,12);
    if(s->current_provider(s->owner)){
        const void *provider=s->current_provider(s->owner);
        if(!provider)return 2;
        if(!s->provider_mode(s->owner,provider))s->input(s->owner,1,11,3);
    }
    UCFirstpersonCamera state;float out[16];
    memcpy(state.world,v->transform,64);memcpy(state.target,c->target,12);state.fov=p->fov;
    memcpy(in.extra,c->extra,64);in.from_damper=c->from_damper;in.to_damper=c->to_damper;
    /* Already damped FOV is equal, including signed-zero equality; the existing
     * finite math helper therefore performs no second FOV operation. */
    in.fov_damper=0;in.target_fov=p->fov;in.getter_bypass=1;
    if(uc_firstperson_camera_follow(&state,&in,out)!=UC_CAMERA_OK)return 2;
    memcpy(v->transform,state.world,64);memcpy(c->target,state.target,12);return 0;
}

#include "simple_cutscene.h"
#include <math.h>
#include <xmmintrin.h>
#include <float.h>

static int valid(const UCEntityFreshView *v,const UCSimpleCutscene *s,
                 const UCSimpleCutsceneServices *h){
    unsigned int control=0;_controlfp_s(&control,0,0);
    return v&&v->target.flags&&v->id&&s&&h&&isfinite(s->elapsed)&&
        s->finish_count>=0&&s->finish_count<=4096&&
        s->wait_count>=0&&s->wait_count<=4096&&s->prefade_count>=0&&s->prefade_count<=4096&&
        (!s->finish_count||s->finish_flags)&&(!s->wait_count||s->wait_flags)&&
        (!s->prefade_count||s->prefade_flags)&&
        !(_mm_getcsr()&0xE040u)&&!(control&_MCW_RC)&&
        h->preflight&&h->triggers_update&&h->model_enable&&h->flags_ready&&h->provider&&
        h->provider_ready&&h->resolve_clip&&h->start_animation&&h->start_audio&&h->bind_trigger&&
        h->audio_busy&&h->remove&&h->switch_bone&&h->delta&&h->skip_input&&h->animation_done&&
        h->animation_duration&&h->animation_time&&h->fade_request&&h->sound_position&&
        h->advance_animation&&h->after_animation&&h->set_flag&&h->end_event&&h->remove_camera&&
        h->restart&&h->finish_flags&&h->handoff;
}
#define CALL(x) do{if((x))return 2;}while(0)
int uc_simple_cutscene_update(const UCEntityFreshView *v,UCSimpleCutscene *s,
    const UCSimpleCutsceneServices *h,void *ctx){
    if(!valid(v,s,h)||h->preflight(ctx))return 1;
    CALL(h->triggers_update(ctx));
    if(s->waiting){
        uint8_t ready=1;void *provider;
        CALL(h->model_enable(ctx));
        if(s->wait_count)CALL(h->flags_ready(ctx,s->wait_count,s->wait_flags,&ready));
        if(ready&&(provider=h->provider(ctx))!=NULL){
            provider=h->provider(ctx);CALL(h->provider_ready(ctx,provider,&ready));
            if(ready){
                void *clip=NULL;*v->target.flags&=~0x100u;
                CALL(h->resolve_clip(ctx,&clip));
                if(clip){CALL(h->start_animation(ctx,clip));CALL(h->start_audio(ctx));
                    CALL(h->bind_trigger(ctx,&s->switch_camera));}
                s->waiting=0;
            }
        }
    }
    if(s->ended){uint8_t busy=0;CALL(h->audio_busy(ctx,&busy));if(!busy)CALL(h->remove(ctx));return 0;}
    if(s->switch_camera){s->switch_camera=0;++s->camera_sequence;CALL(h->switch_bone(ctx,s->camera_sequence));}
    if(s->finishing){
        if(!s->fade_complete)goto advance;
        if(s->prefade_flags)for(int32_t i=0;i<s->prefade_count;++i)CALL(h->set_flag(ctx,s->prefade_flags[i]));
        CALL(h->end_event(ctx,s->end_group));
        for(uint32_t count=0;*v->target.flags&0x40000000u;++count){
            if(count==4096)return 2;CALL(h->after_animation(ctx));
        }
        CALL(h->remove_camera(ctx,s->camera_id));
        if((s->skipped||s->goto_restart)&&s->restart>0)CALL(h->restart(ctx,s->restart));
        if((s->skipped&&!s->fade)||(s->fade&&s->fade_up))
            CALL(h->fade_request(ctx,2,.3f,-1,NULL,-1,1));
        CALL(h->finish_flags(ctx));s->ended=1;CALL(h->handoff(ctx));return 0;
    }
    if(!s->waiting){
        double delta=h->delta(ctx);if(!isfinite(delta)||!isfinite(delta+s->elapsed))return 2;
        s->elapsed=(float)(delta+s->elapsed);if(!isfinite(s->elapsed))return 2;
        if(s->can_skip&&s->elapsed>1.5f){double input=0;CALL(h->skip_input(ctx,&input));
            if(!isfinite(input))return 2;if(input!=0)s->skipped=1;}
        if(!s->waiting){
            uint8_t done=0;CALL(h->animation_done(ctx,&done));
            if(done||s->skipped){s->finishing=1;
                if(!s->fade&&!s->skipped){s->fade_complete=1;goto advance;}
                if(!s->fade_requested)goto fade;
            }else{
                CALL(h->animation_done(ctx,&done));
                if(!done&&!s->fade_requested){double duration,time;CALL(h->animation_duration(ctx,&duration));
                    float rounded=(float)duration;CALL(h->animation_time(ctx,&time));
                    if(!isfinite(rounded)||!isfinite(time))return 2;
                    if((double).3f>(double)rounded-time)goto fade;
                }
            }
        }
    }
advance:
    CALL(h->sound_position(ctx));
    {double delta=h->delta(ctx);float rounded=(float)delta;
     if(!isfinite(rounded))return 2;CALL(h->advance_animation(ctx,rounded));}
    CALL(h->after_animation(ctx));return 0;
fade:
    CALL(h->fade_request(ctx,0,.3f,-1,&s->fade_complete,*v->id,1));s->fade_requested=1;goto advance;
}
#undef CALL

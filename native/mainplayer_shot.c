#include "mainplayer_shot.h"
#include <math.h>

static int player_valid(const UCMainplayerShotState *p) {
    if(!p||!isfinite(p->shotgun_accumulator)||!isfinite(p->fov))return 0;
    for(unsigned i=0;i<3;++i)if(!isfinite(p->position[i]))return 0;
    return 1;
}
static double sample(UCGameRandom *random,float low,float high) {
    int32_t word;uc_game_random_next(random,&word);
    return ((double)word*((double)high-low))*0x1p-31+low;
}
UCMainplayerShotStatus uc_mainplayer_shot(uint32_t item,float clock,float delta,
    uint32_t *clip,UCWeaponState *weapon,UCMainplayerShotState *player,
    UCGameRandom *random,const UCMainplayerShotServices *services,
    UCMainplayerShotResult *result) {
    UCWeaponState next;uint32_t next_clip;uint8_t fired;UCMainplayerStatus status;
    UCMainplayerCameraPush push={{0},{0},0,0,0,1,1000,1010};
    float fraction,radius,strength,axis[3];void *user;
    if((item!=2&&item!=17)||!clip||!weapon||weapon->alternate||!player_valid(player)
       ||!random||random->first>=250||random->second>=250||!result||!services
       ||!services->feedback||!services->sound||!services->script||!services->bullets
       ||!services->shell||!services->tag0_axis||!services->camera_push)
        return UC_SHOT_ARGUMENT;
    next=*weapon;next_clip=*clip;
    status=uc_weapon_shot(item,clock,delta,&next_clip,&next,&fired);
    if(status)return status==UC_MAINPLAYER_ARGUMENT?UC_SHOT_ARGUMENT:UC_SHOT_UNSUPPORTED;
    *result=(UCMainplayerShotResult){fired,UC_SHOT_NONE,UC_SHOT_NONE};
    if(!fired)return UC_SHOT_OK;
    *weapon=next;*clip=next_clip;result->completed=UC_SHOT_DEBIT;user=services->user;
    if(item==17) {
        player->shotgun_accumulator+=.4f;
        if(player->shotgun_accumulator>.8f)player->shotgun_accumulator=.8f;
    }
#define COMPLETE(step,call) do { if(call){result->failed=(step);return UC_SHOT_EFFECT_FAILED;} result->completed=(step); } while(0)
    COMPLETE(UC_SHOT_FEEDBACK,services->feedback(user,item));
    COMPLETE(UC_SHOT_SOUND,services->sound(user,item));
    /* C4390: actual GunFlare event; SCRIPT is the retained public enum name. */
    COMPLETE(UC_SHOT_SCRIPT,services->script(user,item));
    COMPLETE(UC_SHOT_BULLETS,services->bullets(user,item));
    player->used[item]=1;
    COMPLETE(UC_SHOT_SHELL,services->shell(user,item));
    ++player->shots_fired;
    /* Both retail6BA40 statistics calls are a single RET, with no effects. */
    /* Re-read after services: CBA20 samples FOV only after its shot effects. */
    if(!player_valid(player)||random->first>=250||random->second>=250) {
        result->failed=UC_SHOT_TAG;return UC_SHOT_EFFECT_FAILED;
    }
    fraction=player->fov-10.f;fraction/=43.f;if(fraction<=.05f)fraction=.05f;
    radius=(item==2?5.f:20.f)*fraction;
    strength=(float)sample(random,item==2?.075f:.16f,item==2?.125f:.21f);
    push.force=strength*fraction;
    COMPLETE(UC_SHOT_TAG,services->tag0_axis(user,axis));
    if(random->first>=250||random->second>=250||!player_valid(player)) {
        result->failed=UC_SHOT_CAMERA;return UC_SHOT_EFFECT_FAILED;
    }
    for(unsigned i=0;i<3;++i) {
        if(!isfinite(axis[i])){result->failed=UC_SHOT_CAMERA;return UC_SHOT_EFFECT_FAILED;}
        float offset=axis[i]*500.f;
        offset=(float)(sample(random,-radius,radius)+(double)offset);
        push.position[i]=offset+player->position[i];
        if(!isfinite(push.position[i])){result->failed=UC_SHOT_CAMERA;return UC_SHOT_EFFECT_FAILED;}
    }
    COMPLETE(UC_SHOT_CAMERA,services->camera_push(user,&push));
#undef COMPLETE
    return UC_SHOT_OK;
}

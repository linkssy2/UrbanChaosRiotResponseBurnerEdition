#include "weapon_state.h"
#include <fenv.h>
#include <limits.h>
#include <math.h>
#include <xmmintrin.h>

static int item_ok(uint32_t item) { return item==2 || item==17; }
static int count_ok(uint32_t count) { return count<=INT32_MAX; }
static int time_ok(float clock,float delta) {
    return isfinite(clock)&&isfinite(delta)&&clock>=0&&delta>=0;
}
static int state_ok(const UCWeaponState *s) {
    if(!s||!isfinite(s->decay)||!isfinite(s->deadline)||!isfinite(s->recoil_amount)
       ||!isfinite(s->last_shot)||!isfinite(s->heat)) return 0;
    for(unsigned i=0;i<2;++i)if(!isfinite(s->recoil[i]))return 0;
    for(unsigned i=0;i<3;++i)if(!isfinite(s->spring[i]))return 0;
    return 1;
}
static int rounding_ok(void) {
    return (_mm_getcsr()&0xE040u)==0 && fegetround()==FE_TONEAREST;
}
static double interval(uint32_t item,uint8_t alternate) {
    double value=item==2?.0625f:.4f;
    return item==17&&alternate?value*.8f:value;
}
UCMainplayerStatus uc_weapon_definition(uint32_t item,UCWeaponDefinition *out) {
    UCWeaponDefinition d;
    if(!item_ok(item)||!out)return UC_MAINPLAYER_ARGUMENT;
    d.interval=item==2?.0625f:.4f;d.decay=1;d.recoil_limit=0;*out=d;
    return UC_MAINPLAYER_OK;
}
UCMainplayerStatus uc_weapon_initialize(UCWeaponState *s) {
    if(!s)return UC_MAINPLAYER_ARGUMENT;
    s->decay=1;s->deadline=0;s->recoil[0]=s->recoil[1]=0;
    s->spring[0]=s->spring[1]=0;s->spring[2]=20;
    s->recoil_amount=0;s->last_shot=-100;s->heat=0;
    s->firing=s->alternate=s->pending_reload=0;return UC_MAINPLAYER_OK;
}
UCMainplayerStatus uc_weapon_reset_shots(UCWeaponState *s) {
    if(!s)return UC_MAINPLAYER_ARGUMENT;
    s->shot_count=0;return UC_MAINPLAYER_OK;
}
UCMainplayerStatus uc_weapon_fire_mode(uint32_t item,uint8_t alternate,uint32_t *out) {
    if(!item_ok(item)||!out)return UC_MAINPLAYER_ARGUMENT;
    *out=alternate?(item==17?5u:6u):4u;return UC_MAINPLAYER_OK;
}
UCMainplayerStatus uc_weapon_can_fire(uint32_t item,uint32_t clip,uint8_t *out) {
    if(!item_ok(item)||!out||!count_ok(clip))return UC_MAINPLAYER_ARGUMENT;
    *out=(uint8_t)(clip>=1);return UC_MAINPLAYER_OK;
}
UCMainplayerStatus uc_weapon_ready(const UCWeaponState *s,float clock,uint8_t *out) {
    if(!s||!out||!isfinite(s->deadline)||!time_ok(clock,0))return UC_MAINPLAYER_ARGUMENT;
    if(!rounding_ok())return UC_MAINPLAYER_UNSUPPORTED;
    *out=(uint8_t)(clock>=s->deadline);return UC_MAINPLAYER_OK;
}
UCMainplayerStatus uc_weapon_cadence(uint32_t item,uint32_t reserve,float clock,
    float delta,UCWeaponState *s,UCWeaponCadenceResult *out) {
    UCWeaponState next;UCWeaponCadenceResult result={0,0};uint32_t mode;
    if(!item_ok(item)||!count_ok(reserve)||!state_ok(s)||!out||!time_ok(clock,delta))
        return UC_MAINPLAYER_ARGUMENT;
    if(!rounding_ok())return UC_MAINPLAYER_UNSUPPORTED;
    next=*s;result.reset_fire_input=(uint8_t)(item==2&&reserve==0);
    uc_weapon_fire_mode(item,s->alternate,&mode);
    if(!(mode==6 && s->shot_count>=3 && s->shot_count<=INT32_MAX)) {
        next.decay=(float)((double)s->decay-(double)delta*1.0f);next.firing=1;
        if(next.decay<0)next.decay=0;
        if(clock>=s->deadline) {
            next.deadline=(float)(interval(item,s->alternate)+(double)clock);
            ++next.shot_count;result.ready=1;
        }
    }
    if(!state_ok(&next))return UC_MAINPLAYER_UNSUPPORTED;
    *s=next;*out=result;return UC_MAINPLAYER_OK;
}
UCMainplayerStatus uc_weapon_reload_amount(uint32_t item,const UCMainplayerItemSpec *d,
    uint32_t clip,uint32_t reserve,uint32_t *out) {
    uint32_t amount;
    if(!item_ok(item)||!d||!out||!d->available||d->category!=0
       ||d->ammo_category!=(item==2?1u:5u)||!count_ok(d->clip_size)
       ||clip>d->clip_size||!count_ok(reserve))return UC_MAINPLAYER_ARGUMENT;
    amount=d->clip_size-clip;
    if(d->reserve_factor && reserve<amount)amount=reserve;
    *out=amount;return UC_MAINPLAYER_OK;
}
UCMainplayerStatus uc_weapon_reload_commit(uint32_t item,const UCMainplayerItemSpec *d,
    uint32_t *clip,uint32_t *reserve,UCWeaponState *s) {
    uint32_t amount;UCMainplayerStatus status;
    if(!clip||!reserve||!s)return UC_MAINPLAYER_ARGUMENT;
    status=uc_weapon_reload_amount(item,d,*clip,*reserve,&amount);if(status)return status;
    *clip+=amount;if(d->reserve_factor)*reserve-=amount;s->pending_reload=0;
    return UC_MAINPLAYER_OK;
}
UCMainplayerStatus uc_weapon_shot(uint32_t item,float clock,float delta,
    uint32_t *clip,UCWeaponState *s,uint8_t *fired) {
    UCWeaponState next;float step,increase;uint32_t mode;
    if(!item_ok(item)||!clip||!count_ok(*clip)||!state_ok(s)||!fired||!time_ok(clock,delta))
        return UC_MAINPLAYER_ARGUMENT;
    if(!rounding_ok())return UC_MAINPLAYER_UNSUPPORTED;
    if(!*clip){*fired=0;return UC_MAINPLAYER_OK;}
    next=*s;step=(float)interval(item,s->alternate);
    increase=step*.2f;next.heat=increase+s->heat;
    if(next.heat<0)next.heat=0;else if(next.heat>1)next.heat=1;
    next.last_shot=clock;
    /* Preserve the zero-limit source arithmetic, including signed-zero adds.
     * The fixed2/17 definitions never request a random recoil sample. */
    increase=step>delta?step:delta;increase*=0.0f;increase*=2.5f;
    next.recoil_amount=increase+s->recoil_amount;
    if(next.recoil_amount>0)next.recoil_amount=0;
    next.spring[0]=next.recoil_amount;
    uc_weapon_fire_mode(item,s->alternate,&mode);
    if(mode<5)next.deadline=(float)(interval(item,s->alternate)+(double)clock);
    if(!state_ok(&next))return UC_MAINPLAYER_UNSUPPORTED;
    --*clip;*s=next;*fired=1;return UC_MAINPLAYER_OK;
}

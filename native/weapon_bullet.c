#include "weapon_bullet.h"
#include <fenv.h>
#include <math.h>
#include <stddef.h>
#include <string.h>
#include <xmmintrin.h>
_Static_assert(sizeof(UCWeaponBulletRequest)==60,"Original bullet record extent");
_Static_assert(offsetof(UCWeaponBulletRequest,flag2c)==0x2C,"Original flag offset");
_Static_assert(offsetof(UCWeaponBulletRequest,origin)==0x30,"Original origin offset");
static int finite_values(const float *p,unsigned n){for(unsigned i=0;i<n;++i)if(!isfinite(p[i]))return 0;return 1;}
static int item_ok(uint32_t item){return item==2||item==17;}
static int rounding_ok(void){return !(_mm_getcsr()&0xE040u)&&fegetround()==FE_TONEAREST;}
UCMainplayerStatus uc_weapon_bullet_prefix(const UCWeaponBulletSource *in,UCWeaponBulletPrefix *out){
    UCWeaponBulletPrefix result;
    if(!in||!out||!item_ok(in->item)||!finite_values(in->origin,3)
       ||!finite_values(in->target,3)||!finite_values(in->selected_target,3)
       ||!isfinite(in->accuracy)||!isfinite(in->accuracy_factor))return UC_MAINPLAYER_ARGUMENT;
    if(!rounding_ok())return UC_MAINPLAYER_UNSUPPORTED;
    result.item=in->item;result.definition_kind=in->definition_kind;
    result.team=in->session_bypass?in->session_class:4;
    result.count=in->item==17?in->pellet_count:1;
    memcpy(result.origin,in->origin,sizeof(result.origin));
    for(unsigned i=0;i<3;++i)result.direction[i]=(in->selected_entity==-1?in->target[i]:in->selected_target[i])-in->origin[i];
    result.accuracy_after=in->accuracy_factor*in->accuracy;
    if(!finite_values(result.direction,3)||!isfinite(result.accuracy_after))return UC_MAINPLAYER_UNSUPPORTED;
    *out=result;return UC_MAINPLAYER_OK;
}
UCMainplayerStatus uc_weapon_bullet_request(const UCWeaponBulletPrefix *prefix,
    const UCWeaponBulletPellet *in,UCWeaponBulletRequest *out){
    UCWeaponBulletRequest result={0};UCMainplayerStatus status;
    if(!prefix||!in||!out||!item_ok(prefix->item)||prefix->count<=0
       ||(prefix->item==2&&prefix->count!=1)||!finite_values(prefix->origin,3)
       ||!finite_values(prefix->direction,3)||!isfinite(in->definition_spread)
       ||!isfinite(in->weapon_accuracy)||!isfinite(in->value20)||!isfinite(in->value28))return UC_MAINPLAYER_ARGUMENT;
    if(!rounding_ok())return UC_MAINPLAYER_UNSUPPORTED;
    result.item=prefix->item;result.definition_kind=prefix->definition_kind;
    result.team=prefix->team;result.actor=in->actor;result.sentinel=-1;
    memcpy(result.direction,prefix->direction,sizeof(result.direction));
    if(!in->spread_disabled){
        UCWeaponSpreadInput spread={prefix->item,in->definition_spread,in->weapon_accuracy,
            prefix->count,prefix->count,in->alternate,{0}};
        memcpy(spread.random,in->random,sizeof(spread.random));
        status=uc_weapon_spread(&spread,result.direction);if(status)return status;
    }
    result.value20=in->value20;result.scale24=1;result.value28=in->value28;
    memcpy(result.origin,prefix->origin,sizeof(result.origin));
    memcpy(result.reserved,out->reserved,sizeof(result.reserved));
    *out=result;return UC_MAINPLAYER_OK;
}

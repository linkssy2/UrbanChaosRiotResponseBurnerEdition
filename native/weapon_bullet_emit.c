#include "weapon_bullet_emit.h"
#include <fenv.h>
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int floats(const float *p,unsigned n){if(!p)return 0;for(unsigned i=0;i<n;i++)if(!isfinite(p[i])||fabsf(p[i])>1e12f)return 0;return 1;}
static int live(const UCWeaponBulletEmitView*v,const UCGameRandom*r){
    return v&&v->entity_id&&v->spread_disabled&&v->selected_entity&&v->pellet_count
        &&v->session_bypass&&v->session_class&&v->upgrades&&v->network_owner
        &&!*v->network_owner&&v->weapon&&v->weapon->alternate<=1
        &&floats(v->position,3)&&floats(v->accuracy,1)&&floats(v->target,3)
        &&floats(v->selected_target,3)&&floats(&v->weapon->decay,1)
        &&*v->pellet_count<=16384&&r&&r->first<250&&r->second<250;
}
int uc_weapon_bullet_emit(uint32_t item,const UCWeaponBulletEmitView*v,UCGameRandom*r,
    const UCWeaponBulletEmitServices*s,UCWeaponBulletEmitResult*out){
    UCWeaponBulletSource source={0};UCWeaponBulletPrefix prefix;void*ctx;
    if((item!=2&&item!=17)||!out||!live(v,r)||!s||!s->preflight||!s->feedback
       ||!s->tag0_origin||!s->bullet||!s->flash.preflight||!s->flash.create
       ||(_mm_getcsr()&0xE040u)||fegetround()!=FE_TONEAREST
       ||!s->preflight(s->context,item,v))return 1;
    *out=(UCWeaponBulletEmitResult){0,UC_BULLET_EMIT_NONE,UC_BULLET_EMIT_NONE};ctx=s->context;
#define FAIL(step) do{out->failed=(step);return 2;}while(0)
#define COMPLETE(step,call) do{if(call)FAIL(step);out->completed=(step);}while(0)
    COMPLETE(UC_BULLET_EMIT_FEEDBACK,s->feedback(ctx,v->position,1000.f,
        item==2?0.f:.13f,1.f,3));
    if(!live(v,r))FAIL(UC_BULLET_EMIT_TAG);
    source.item=item;source.definition_kind=item==2?1:2;
    source.pellet_count=*v->pellet_count;source.session_bypass=*v->session_bypass;
    source.session_class=*v->session_class;
    COMPLETE(UC_BULLET_EMIT_TAG,s->tag0_origin(ctx,source.origin));
    if(!live(v,r))FAIL(UC_BULLET_EMIT_ACCURACY);
    source.selected_entity=*v->selected_entity;
    memcpy(source.target,v->target,12);memcpy(source.selected_target,v->selected_target,12);
    source.accuracy=*v->accuracy;source.accuracy_factor=item==2?1.f:.9f;
    if(uc_weapon_bullet_prefix(&source,&prefix))FAIL(UC_BULLET_EMIT_ACCURACY);
    *v->accuracy=prefix.accuracy_after;out->completed=UC_BULLET_EMIT_ACCURACY;
    for(int32_t i=0;i<prefix.count;i++){
        UCWeaponBulletPrefix current=prefix;UCWeaponBulletRequest request={0};
        UCWeaponBulletPellet pellet={0};
        if(!live(v,r))FAIL(UC_BULLET_EMIT_SPREAD);
        if(!*v->spread_disabled){
            UCWeaponSpreadInput spread={item,item==2?5.f:8.f,v->weapon->decay,
                *v->pellet_count,prefix.count,v->weapon->alternate,{0}};
            if(item==17&&spread.pellet_count<=0)FAIL(UC_BULLET_EMIT_SPREAD);
            for(unsigned j=0;j<4;j++)if(uc_game_random_next(r,spread.random+j))FAIL(UC_BULLET_EMIT_SPREAD);
            if(uc_weapon_spread(&spread,current.direction))FAIL(UC_BULLET_EMIT_SPREAD);
        }
        out->completed=UC_BULLET_EMIT_SPREAD;
        pellet.actor=*v->entity_id;pellet.spread_disabled=1;pellet.value28=1;
        if(*v->session_bypass)pellet.value20=item==2?100.f:45.f;
        else{
            static const float scale[]={1.f,1.2f,1.4f,1.6f};
            unsigned offset=item==2?36:39,tier=0;
            for(unsigned j=0;j<3;j++)tier+=v->upgrades[offset+j]!=0;
            pellet.value20=(float)((double)scale[tier]*(item==2?80.f:55.f));
        }
        if(uc_weapon_bullet_request(&current,&pellet,&request))FAIL(UC_BULLET_EMIT_BULLET);
        COMPLETE(UC_BULLET_EMIT_BULLET,s->bullet(ctx,&request,*v->entity_id));
        out->bullets++;
    }
    if(!live(v,r))FAIL(UC_BULLET_EMIT_FLASH);
    COMPLETE(UC_BULLET_EMIT_FLASH,uc_shot_lightflash_emit(item,v->entity_id,&s->flash));
    return 0;
#undef COMPLETE
#undef FAIL
}

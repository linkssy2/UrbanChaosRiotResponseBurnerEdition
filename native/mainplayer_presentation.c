#include "mainplayer_presentation.h"
#include <string.h>

/* Normal fresh Westside shield:54FE0 publishes450D50 at551B5. */
static const UCMainplayerPresentationResource shield={
    "Assets_PlayerWeaponsAndEquipment\\EQP_Shield\\eqp_shield_skin.AMF",
    "Assets_PlayerWeaponsAndEquipment\\EQP_Shield\\EQP_Shield_Animations.sttxt",0,53.0f,
    {-5.0f,-160.0f,-25.0f},{-5.0f,-160.0f,-25.0f}
};
static const UCMainplayerPresentationResource taser={
    "Assets_PlayerWeaponsAndEquipment\\EQP_Taser\\EQP_Taser_Skin.AMF",
    "Assets_PlayerWeaponsAndEquipment\\EQP_Taser\\EQP_Taser_Animations.sttxt",1,53.0f,
    {18.0f,-155.0f,-35.0f},{18.0f,-155.0f,-35.0f}
};
static const UCMainplayerPresentationResource handgun={
    "Assets_PlayerWeaponsAndEquipment\\EQP_HandGun\\EQP_HandGun_Skin.AMF",
    "Assets_PlayerWeaponsAndEquipment\\EQP_HandGun\\EQP_HandGun_Animations.sttxt",1,48.0f,
    {18.0f,-156.0f,-35.0f},{0.7f,-159.0f,-35.0f}
};
static const UCMainplayerPresentationResource shotgun={
    "Assets_PlayerWeaponsAndEquipment\\EQP_Police_shotgun\\eqp_police_shotgun_skin.amf",
    "Assets_PlayerWeaponsAndEquipment\\EQP_Police_shotgun\\EQP_Police_shotgun_Animations.sttxt",0,38.0f,
    {0.0f,-148.0f,-9.5f},{-3.0f,-146.0f,-9.5f}
};
static const UCMainplayerPresentationResource molotov={
    "Assets_PlayerWeaponsAndEquipment\\EQP_Molotov\\EQP_Molotov_Skin.AMF",
    "Assets_PlayerWeaponsAndEquipment\\EQP_Molotov\\EQP_Molotov_Animations.sttxt",0,53.0f,
    {18.0f,-165.0f,-27.0f},{18.0f,-165.0f,-27.0f}
};
static const UCMainplayerPresentationResource *resource(uint32_t item) {
    return item==0?&shield:item==1?&taser:item==2?&handgun:item==11?&molotov:item==17?&shotgun:0;
}
static const char *const aliases[18]={
    "FlashlightIdle","FlashlightLowerWeapon","FlashlightRaiseWeapon",
    "FlashlightFire","FlashlightReload","FlashlightLowerLight",
    "FlashlightRaiseLight","FlashlightBackswing","FlashlightThrow",
    "FlashlightRollBackswing","FlashlightRollThrow","FlashlightFireRaise",
    "FlashlightFireLower","FlashlightFireLoop","FlashlightFireStart",
    "FlashlightFireIdle","FlashlightFire2","FlashlightFire3"
};

UCAnimationStatus uc_mainplayer_presentation_resource(uint32_t item,
    UCMainplayerPresentationResource *out) {
    const UCMainplayerPresentationResource *r=resource(item);
    if(!r||!out)return UC_ANIMATION_ARGUMENT;
    *out=*r;return UC_ANIMATION_OK;
}
UCAnimationStatus uc_mainplayer_presentation_action(uint32_t item,uint32_t action,
    uint8_t enabled,uint8_t compatible,int32_t repeats,int32_t blend,
    UCMainplayerPresentationAction *out) {
    const UCMainplayerPresentationResource *r=resource(item);
    if(!r||action>=18||!out)return UC_ANIMATION_ARGUMENT;
    UCMainplayerPresentationAction a={0};
    a.alias=aliases[action]+((enabled&&compatible&&r->flashlight_mode==1)?0:10);
    a.animation_catalog=r->animation_catalog;a.action=action;
    a.repeats=action==0?-1:repeats;a.blend_argument=action==3?1:blend;a.owner_slot=-1;
    *out=a;return UC_ANIMATION_OK;
}
UCAnimationStatus uc_mainplayer_presentation_controller(const UCAnimationClip *clip,
    const UCMainplayerPresentationAction *request,UCAnimationController *out) {
    if(!request||!out||request->action>=18||request->owner_slot!=-1||
        (request->action==0&&request->repeats!=-1)||
        (request->action==3&&request->blend_argument!=1))return UC_ANIMATION_ARGUMENT;
    UCAnimationController s;
    UCAnimationStatus result=uc_animation_controller_init(clip,0xc0000009u,&s);
    if(result!=UC_ANIMATION_OK)return result;
    s.repeats=request->repeats;s.blend_out=-1.0f;
    if(request->blend_argument==1)s.blend_in=0.0f;
    *out=s;return UC_ANIMATION_OK;
}
UCAnimationStatus uc_mainplayer_presentation_reset_action(uint8_t alternate_mode,
    int32_t equipped_item,int32_t *out) {
    if(!out||equipped_item < -1||equipped_item>=27)return UC_ANIMATION_ARGUMENT;
    *out=(!alternate_mode&&equipped_item!=-1)?0:-1;return UC_ANIMATION_OK;
}
UCAnimationStatus uc_mainplayer_presentation_reload_gate(uint8_t trigger,
    uint8_t pending,uint8_t *out) {
    if(!out)return UC_ANIMATION_ARGUMENT;
    *out=(uint8_t)(trigger&&pending);return UC_ANIMATION_OK;
}
UCAnimationStatus uc_mainplayer_presentation_reload_publish(
    const UCAnimationEventQueue *queue,uint8_t *out) {
    if(!queue||!out||queue->count>16)return UC_ANIMATION_ARGUMENT;
    for(uint32_t i=0;i<queue->count;i++){
        const UCAnimationEventRef *ref=queue->events+i;
        if(!ref->owner||!ref->owner->events||ref->index>=ref->owner->event_count)
            return UC_ANIMATION_ARGUMENT;
        const UCAnimationEvent *event=ref->owner->events+ref->index;
        if(!event->type||strcmp(event->type,"GunReloadTrigger")||event->code!=4||event->parameter_count)
            return UC_ANIMATION_UNSUPPORTED;
    }
    *out=(uint8_t)(queue->count!=0);return UC_ANIMATION_OK;
}

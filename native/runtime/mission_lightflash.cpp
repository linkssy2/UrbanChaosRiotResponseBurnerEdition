#include "mission_lightflash.h"
UCLightFlashServices NativeMissionLightFlash::hooks(){return {
 [](void*p,const float*position){auto&s=*(NativeMissionLightFlash*)p;return s.services_.create(s.services_.owner,position);},
 [](void*p,UCPointLightState*light){auto&s=*(NativeMissionLightFlash*)p;if(s.services_.release(s.services_.owner,light))s.failure_=2;},
 [](void*p){auto&s=*(NativeMissionLightFlash*)p;if(s.remove())s.failure_=2;}};}
int NativeMissionLightFlash::construct(NativeMissionWorld&w,const UCShotLightFlashRequest&r,const NativeMissionLightFlashServices&h){
 if(world_||r.mode!=2||r.rotation[0]!=0||r.rotation[1]!=0||r.rotation[2]!=0||!h.create||!h.release||
    !uc_entity_event_lookup(&w.registry(),int32_t(r.parent_id)))return 1;
 world_=&w;services_=h;
 if(w.bind_dynamic({&entity.view,this,[](void*p){return ((NativeMissionLightFlash*)p)->update();},
    [](void*p){return ((NativeMissionLightFlash*)p)->late();},[](void*p){return ((NativeMissionLightFlash*)p)->close_detached();},nullptr,
    [](void*p){return ((NativeMissionLightFlash*)p)->remove();},[](void*p){return ((NativeMissionLightFlash*)p)->close_detached();}}))return failure_=2;
 auto c=w.context();const void*definition="LightFlash";c.definition=&definition;*c.registration->creation={0,1,int32_t(r.parent_id)};
 if(uc_entity_fresh_construct_lightflash(&c,&entity.view,r.position))return failure_=2;
 auto hooks= this->hooks();return uc_light_flash_initialize(&state_,&r.parameters,entity.transform+12,&hooks,this)!=UC_LIGHT_FLASH_OK?failure_=2:0;
}
int NativeMissionLightFlash::update(){
 if(!world_||failure_||removed_)return failure_?failure_:1;
 auto h=hooks();auto result=uc_light_flash_step(&state_,*world_->script_services().delta,&h,this);
 if((result!=UC_LIGHT_FLASH_OK&&result!=UC_LIGHT_FLASH_FINISHED)||failure_)return failure_=2;
 //120C60 then218C40: an empty dynamic descriptor has no authored scripts.
 if(entity.actor.flagCC||entity.actor.flagCD||entity.actor.flagCE||entity.actor.scriptC8||entity.helper)return failure_=2;
 return uc_entity_fresh_aux_clear(entity.aux)?failure_=2:0;
}
int NativeMissionLightFlash::late(){
 if(!world_||failure_||removed_)return failure_?failure_:1;
 //218C30→218400, real class3F440 displacement is zero, word90 bit1 freshclear.
 if(!(entity.actor.word90&1)){for(unsigned i=12;i<15;++i)entity.transform[i]+=0.f;entity.transform[15]=1;}return 0;
}
int NativeMissionLightFlash::remove(){
 if(!world_||failure_||removed_||entity.helper||entity.actor.handleBC||entity.actor.audioC0)return 1;
 if(state_.light){if(services_.release(services_.owner,state_.light))return failure_=2;state_.light=nullptr;}
 if(uc_entity_fresh_aux_clear(entity.aux))return failure_=2;
 entity.flags|=0x8100;uint8_t queued=0;if(uc_entity_retire(&world_->retirement(),entity.id,&queued)||!queued)return failure_=2;
 if(entity.identity.listeners[0]){if(uc_entity_listener_list_remove_first(world_->listeners(),entity.id))return failure_=2;for(auto&l:entity.identity.listeners)l=nullptr;}
 entity.flags|=0x80000;removed_=true;return 0;
}
int NativeMissionLightFlash::close_detached(){
 if(!world_)return 0;if(uc_entity_event_lookup(&world_->registry(),entity.id))return 1;
 //Explicit native world shutdown also releases an unfinished effect.
 if(state_.light){if(services_.release(services_.owner,state_.light))return 1;state_.light=nullptr;}
 if(entity.aux&&uc_entity_fresh_aux_destroy(entity.aux))return 1;entity.aux=nullptr;world_=nullptr;return 0;
}

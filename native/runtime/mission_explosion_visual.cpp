#include "mission_explosion_visual.h"
int NativeMissionExplosionVisual::construct(NativeMissionWorld&w,const UCExplosionVisualChild&r,const NativeExplosionVisualServices&h){
 if(world_||r.mode!=1||r.kind>UC_EXPLOSION_CAMERA_KICK||!uc_entity_event_lookup(&w.registry(),int32_t(r.parent_id)))return 1;
 world_=&w;kind_=r.kind;
 if(w.bind_dynamic({&entity.view,this,[](void*p){return ((NativeMissionExplosionVisual*)p)->update();},
  [](void*p){return ((NativeMissionExplosionVisual*)p)->late();},[](void*p){return ((NativeMissionExplosionVisual*)p)->close_detached();},nullptr,
  [](void*p){return ((NativeMissionExplosionVisual*)p)->remove();},[](void*p){return ((NativeMissionExplosionVisual*)p)->close_detached();}}))return failure_=2;
 const char*names[]={"WorldWarpRing","CameraPush","CameraKick"};const void*definition=names[r.kind];auto c=w.context();c.definition=&definition;*c.registration->creation={0,1,int32_t(r.parent_id)};
 if(uc_entity_fresh_construct_parent(&c,&entity.view))return failure_=2;
 if(r.kind==UC_EXPLOSION_WARP_RING){remaining_=r.lifetime;if(uc_world_warp_asset(&r,&asset_)||!h.ring||h.ring(h.owner,entity,asset_))return failure_=2;}
 else {UCCameraPushInput input{};if(uc_explosion_camera_input(&r,entity.transform,&input))return failure_=2;
  uint32_t sender=uint32_t(entity.id);if(uc_entity_listener_broadcast(w.listeners(),&w.registry(),19,&input,sizeof input,&sender))return failure_=2;}
 return 0;
}
int NativeMissionExplosionVisual::update(){
 if(!world_||failure_||removed_)return failure_?failure_:1;
 if(kind_!=UC_EXPLOSION_WARP_RING)return remove();
 return uc_world_warp_step(&remaining_,*world_->script_services().delta,this,[](void*p){return ((NativeMissionExplosionVisual*)p)->remove();})?failure_=2:0;
}
int NativeMissionExplosionVisual::late(){
 if(!world_||failure_||removed_)return failure_?failure_:1;
 if(!(entity.actor.word90&1)){for(unsigned i=12;i<15;i++)entity.transform[i]+=0.f;entity.transform[15]=1;}return 0;
}
int NativeMissionExplosionVisual::remove(){
 if(!world_||failure_||removed_||entity.helper||entity.actor.handleBC||entity.actor.audioC0)return 1;
 if(uc_entity_fresh_aux_clear(entity.aux))return failure_=2;
 entity.flags|=0x8100;uint8_t queued=0;if(uc_entity_retire(&world_->retirement(),entity.id,&queued)||!queued)return failure_=2;
 if(entity.identity.listeners[0]){if(uc_entity_listener_list_remove_first(world_->listeners(),entity.id))return failure_=2;for(auto&l:entity.identity.listeners)l=nullptr;}
 entity.flags|=0x80000;removed_=true;return 0;
}
int NativeMissionExplosionVisual::close_detached(){
 if(!world_)return 0;if(uc_entity_event_lookup(&world_->registry(),entity.id))return 1;
 if(entity.aux&&uc_entity_fresh_aux_destroy(entity.aux))return 1;entity.aux=nullptr;world_=nullptr;return 0;
}

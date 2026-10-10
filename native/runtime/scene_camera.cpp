#include "scene_camera.h"
#include <cmath>
#include <cstdlib>

NativeSceneCamera::NativeSceneCamera() {
    auto&e=entity;
    e.view={{&e.target,&e.flags,&e.parent},e.transform,&e.source,&e.id,&e.flags60,
            &e.definition,&e.model,&e.helper,&e.aux,&e.actor};
}
int NativeSceneCamera::activate(void *p,UCSceneCamera *c,int32_t id) {
    return uc_scene_camera_stack_activate(static_cast<NativeSceneCamera*>(p)->owners_.stack,c,id);
}
int NativeSceneCamera::remove(void *p,UCSceneCamera *c) {
    return uc_scene_camera_stack_remove(static_cast<NativeSceneCamera*>(p)->owners_.stack,c);
}
int NativeSceneCamera::construct(const NativeSceneCameraOwners &o,const UCEntityProperties &properties) {
    if(events_||constructed_||entity.aux||!o.entities||!o.entities->registration||
       !o.entities->registration->registry||!o.listeners||!o.names||!o.stack||
       o.stack->entities!=o.entities||!o.stack->current_viewport||!o.auxiliary||
       !o.event_ids||!o.player||!o.player_fields.flags||!o.player_fields.tag0||
       !o.fire_level||!o.debug_flags||!o.world_flags||!o.feedback_suppressed||
       !o.clock||!o.feedback||!o.random||!o.provider||
       !o.matrix_services.context||!o.matrix_services.upload)return 1;
    owners_=o;
    UCSceneCameraActivation activation{this,activate,remove};
    if(uc_scene_camera_events_create(&entity.camera,&entity.projection,&activation,&events_))return 1;
    UCSceneCameraServices s{};s.owner=this;s.event_ids=o.event_ids;
    s.preflight=[](void*){return 1;};
    s.current_viewport=[](void*p)->void*{auto&c=*static_cast<NativeSceneCamera*>(p);return c.owners_.stack->current_viewport(c.owners_.stack->owner);};
    s.register_listener=[](void*p,UCSceneCamera*,int32_t id){auto&c=*static_cast<NativeSceneCamera*>(p);int inserted=0;
        if(uc_entity_listener_register(c.owners_.entities->registration->registry,id,id,
            uc_scene_camera_events_listener(c.events_),c.owners_.listeners,&inserted)||!inserted)c.failure_=2;};
    s.append=[](void*p,UCSceneCamera*,const UCSceneCameraSubscription*v){auto&c=*static_cast<NativeSceneCamera*>(p);if(uc_scene_camera_events_append(c.events_,v))c.failure_=2;};
    s.auxiliary=[](void*p)->void*{return static_cast<NativeSceneCamera*>(p)->owners_.auxiliary;};
    s.equipped_fov=[](void*p){return static_cast<NativeSceneCamera*>(p)->owners_.equipped_fov;};
    int result=uc_scene_camera_construct(o.entities,&entity.view,&entity.projection,&entity.motion,&entity.camera,&properties,&s);
    if(result||failure_)return result==1?1:2;
    constructed_=true;return 0;
}
int NativeSceneCamera::send(uint32_t event,const void *payload,uint32_t bytes,uint8_t *handled) {
    if(!constructed_||failure_)return 1;
    uint32_t sender=0;
    return uc_entity_event_send(owners_.entities->registration->registry,entity.id,event,payload,bytes,&sender,handled);
}
int NativeSceneCamera::start() {
    if(!constructed_||failure_)return 1;
    return uc_scene_camera_start(&entity.camera,&entity.id,this,
        [](void*p){return static_cast<NativeSceneCamera*>(p)->owners_.names;},owners_.stack);
}
int NativeSceneCamera::frame(uint32_t layer,uint32_t clear,uint32_t flags) {
    if(!constructed_||failure_||entity.camera.mode!=2||*owners_.debug_flags||
       !std::isfinite(*owners_.fire_level)||!std::isfinite(*owners_.clock)||
       (*owners_.player_fields.flags&0x8000)||entity.camera.source_id!=entity.parent||
       uc_entity_event_lookup(owners_.entities->registration->registry,entity.parent)!=owners_.player)return 1;
    UCSceneCameraFollowServices follow{};follow.owner=this;follow.registry=owners_.entities->registration->registry;follow.debug_flags=owners_.debug_flags;
    follow.preflight=[](void*){return 0;};
    follow.player=[](void*p,UCEntityEventTarget*t)->const UCSceneCameraPlayer*{auto&c=*static_cast<NativeSceneCamera*>(p);return t==c.owners_.player?&c.owners_.player_fields:nullptr;};
    // Explicit PC gameplay input provider: mode0. The read-only channel11 query
    // discards its result in1CDEE0 and leaves the same action available to fire.
    follow.current_provider=[](void*p)->const void*{return p;};
    follow.provider_mode=[](void*,const void*)->uint8_t{return 0;};
    follow.input=[](void*p,uint32_t player,uint32_t channel,uint32_t mode){if(player!=1||channel!=11||mode!=3)std::abort();auto&c=*static_cast<NativeSceneCamera*>(p);(void)*c.owners_.fire_level;};
    // Stable-parent admission above makes this source removal branch unreachable.
    follow.remove=[](void*){std::abort();};
    if(uc_scene_camera_follow(&entity.view,&entity.projection,&entity.camera,&follow)){failure_=2;return 2;}
    UCSceneCamera *selected=nullptr;
    if(uc_scene_camera_stack_current(owners_.stack,layer,&selected)||selected!=&entity.camera){failure_=2;return 2;}
    UCSceneViewServices s{};s.owner=this;s.preflight=[](void*,const UCSceneProjection*){return 1;};
    s.fallback_viewport=[](void*p)->void*{auto&c=*static_cast<NativeSceneCamera*>(p);return c.owners_.stack->current_viewport(c.owners_.stack->owner);};
    s.camera_world=[](void*p)->const float*{auto&c=*static_cast<NativeSceneCamera*>(p);const float*out=nullptr;
        UCSceneCameraWorldServices h{};h.owner=p;h.preflight=[](void*){return 1;};
        h.sample=[](void*q,uint32_t player,const float*position,uint32_t kind)->double{auto&v=*static_cast<NativeSceneCamera*>(q);float result=0;
            if(uc_feedback_sample(uc_feedback_manager_queue(v.owners_.feedback),uc_feedback_manager_observers(v.owners_.feedback),player,position,kind,*v.owners_.clock,0,&result)){v.failure_=2;return NAN;}return result;};
        h.perturb=[](void*q,float amplitude,const float*world,float*scratch){auto&v=*static_cast<NativeSceneCamera*>(q);if(uc_feedback_perturb(v.owners_.random,amplitude,world,scratch))v.failure_=2;};
        if(uc_scene_camera_world(&c.entity.camera,c.entity.transform,c.scratch_,c.owners_.world_flags,c.owners_.feedback_suppressed,&h,&out))c.failure_=2;
        return c.failure_?nullptr:out;};
    s.aspect=[](void*,void*v){double values[5];return uc_scene_viewport_values(static_cast<UCSceneViewport*>(v),values)?NAN:static_cast<float>(values[4]);};
    if(uc_scene_view_construct(&view,&entity.projection,clear,flags,&s)||failure_){failure_=2;return 2;}
    if(uc_scene_view_publish(&view,[](void*p){return static_cast<NativeSceneCamera*>(p)->owners_.provider;},this,&owners_.matrix_services)){failure_=2;return 2;}
    return 0;
}
int NativeSceneCamera::close_detached() {
    if(entity.aux){if(uc_entity_fresh_aux_destroy(entity.aux))return 1;entity.aux=nullptr;}
    uc_scene_camera_events_close(events_);events_=nullptr;constructed_=false;failure_=0;return 0;
}

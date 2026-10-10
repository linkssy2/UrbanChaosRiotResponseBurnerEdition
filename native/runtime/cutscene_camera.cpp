#include "cutscene_camera.h"
extern "C" {
#include "../scene_camera_base.h"
}
#include <cstring>
#include <cmath>
#include <exception>

NativeCutsceneCamera::NativeCutsceneCamera(){auto&e=entity;e.view={{&e.target,&e.flags,&e.parent},e.transform,&e.source,&e.id,&e.flags60,&e.definition,&e.model,&e.helper,&e.aux,&e.actor};}
int NativeCutsceneCamera::create(NativeMissionWorld&w,const int32_t*parent,const UCEntityProperties&p,
 UCSceneCameraStack&stack,const uint32_t*bone_event,const uint32_t*fov_event,const NativeCutsceneCameraServices&s){
 if(world_||!parent||!bone_event||!fov_event||stack.entities!=&w.context()||!stack.current_viewport||
    !s.preflight||!s.current_provider||!s.player||!s.control||!s.mode||!s.bone_matrix||
    (!s.remove&&!(s.return_control&&s.release_camera))||!!s.return_control!=!!s.release_camera||
    (s.release_on_close&&!s.release_camera)||p.source_index!=-1||!uc_entity_event_lookup(&w.registry(),*parent)||s.preflight(s.owner))return 1;
 services_=s;stack_=&stack;properties_=p;bone_event_=bone_event;fov_event_=fov_event;
 context_=w.context();context_.definition=&properties_.definition;
 if(w.bind_dynamic({&entity.view,this,[](void*q){return static_cast<NativeCutsceneCamera*>(q)->step();},
    [](void*q){return static_cast<NativeCutsceneCamera*>(q)->late();},
    [](void*q){return static_cast<NativeCutsceneCamera*>(q)->close_detached();},nullptr,
    s.return_control?+[](void*q){return static_cast<NativeCutsceneCamera*>(q)->remove();}:nullptr,
    s.release_camera?+[](void*q){return static_cast<NativeCutsceneCamera*>(q)->destroy();}:nullptr}))return 1;
 world_=&w;
 int32_t id=-1;
 int r=uc_entity_attached_create(w.context().registration->creation,parent,&properties_,1,"LockToParentBoneCamera",this,
  [](void*q,const char*name,UCEntityProperties*d,int32_t*out){auto&c=*static_cast<NativeCutsceneCamera*>(q);d->definition=name;
   if(c.construct())return 2;*out=c.entity.id;return 0;},&id);
 if(r||id!=entity.id){failure_=2;return 2;}constructed_=true;return 0;
}
int NativeCutsceneCamera::construct(){
 UCCutsceneCameraConstructServices h{};h.owner=this;h.bone_event=bone_event_;h.fov_event=fov_event_;
 h.preflight=[](void*p){auto&c=*static_cast<NativeCutsceneCamera*>(p);return c.services_.preflight(c.services_.owner);};
 h.base=[](void*p){auto&c=*static_cast<NativeCutsceneCamera*>(p);return uc_scene_camera_base_construct(&c.context_,&c.entity.view,&c.entity.projection,&c.entity.motion,
  [](void*q)->void*{auto&v=*static_cast<NativeCutsceneCamera*>(q);return v.stack_->current_viewport(v.stack_->owner);},p);};
 h.register_listener=[](void*p)->int{auto&c=*static_cast<NativeCutsceneCamera*>(p);
  if(uc_entity_subscriptions_create(p,&c.subscriptions_))return 1;int inserted=0;
  return uc_entity_listener_register(&c.world_->registry(),c.entity.id,c.entity.id,uc_entity_subscriptions_listener(c.subscriptions_),c.world_->listeners(),&inserted)||!inserted;};
 h.append=[](void*p,uint32_t event,uint32_t handler){auto&c=*static_cast<NativeCutsceneCamera*>(p);if(handler>1)return 1;
  UCEntityEventSubscription row{event,handler==0?
   +[](void*q,const void*v,uint32_t*){return uc_cutscene_camera_bone(&static_cast<NativeCutsceneCamera*>(q)->fields,v);}:
   +[](void*q,const void*v,uint32_t*){return uc_cutscene_camera_fov(&static_cast<NativeCutsceneCamera*>(q)->entity.projection,v);}};
  if(uc_entity_subscriptions_append(c.subscriptions_,&row))return 1;
  c.listener_.count=uc_entity_subscriptions_listener(c.subscriptions_)->count;c.listener_.capacity=uc_entity_subscriptions_capacity(c.subscriptions_);return 0;};
 return uc_cutscene_camera_construct(&entity.view,&fields,&entity.projection,&listener_,&properties_,&h);
}
int NativeCutsceneCamera::step(){
 if(!constructed_||failure_||entity.model||entity.helper||entity.actor.handleBC||entity.actor.audioC0||entity.actor.scriptC8||
    entity.actor.flagCC||entity.actor.flagCD||entity.actor.flagCE||(entity.flags&0xC0000000u)||!uc_entity_fresh_aux_empty(entity.aux))return failure_=2;
 //21E1A0 ->21D8B0/21D9C0, then actual empty218C40/218680 auxiliary suffix.
 if(uc_bullet_motion_begin(&entity.motion)||uc_entity_fresh_aux_clear(entity.aux))return failure_=2;return 0;
}
int NativeCutsceneCamera::late(){
 if(!constructed_||failure_)return 1;
 UCCutsceneCameraServices h{&world_->registry(),world_->names(),this,
  [](void*p){auto&c=*static_cast<NativeCutsceneCamera*>(p);return c.services_.preflight(c.services_.owner);},
  [](void*p)->void*{auto&c=*static_cast<NativeCutsceneCamera*>(p);return c.services_.current_provider(c.services_.owner);},
  [](void*p,void*v,int32_t index,int32_t*out){auto&c=*static_cast<NativeCutsceneCamera*>(p);return c.services_.player(c.services_.owner,v,index,out);},
  [](void*p,int32_t id){auto&c=*static_cast<NativeCutsceneCamera*>(p);return c.services_.control(c.services_.owner,id);},
  [](void*p,int32_t id){auto&c=*static_cast<NativeCutsceneCamera*>(p);return uc_scene_camera_stack_activate(c.stack_,&c.entity.camera,id);},
  [](void*p,void*v,int32_t mode,int32_t arg){auto&c=*static_cast<NativeCutsceneCamera*>(p);return c.services_.mode(c.services_.owner,v,mode,arg);},
  [](void*p,UCEntityEventTarget*t,int32_t bone,float*out,uint8_t*found){auto&c=*static_cast<NativeCutsceneCamera*>(p);return c.services_.bone_matrix(c.services_.owner,t,bone,out,found);},
  [](void*p,UCEntityEventTarget*t)->const float*{auto&c=*static_cast<NativeCutsceneCamera*>(p);auto*v=c.world_->context().resolve(c.world_->context().owner,t);return v?v->transform:nullptr;},
  [](void*p){auto&c=*static_cast<NativeCutsceneCamera*>(p);return c.services_.return_control?c.remove():c.services_.remove(c.services_.owner);}};
 if(uc_cutscene_camera_late(&entity.view,&fields,&h))return failure_=2;return 0;
}
int NativeCutsceneCamera::send(uint32_t event,const void*p,uint32_t bytes,uint8_t*handled){
 if(!constructed_||failure_)return 1;uint32_t sender=0;return uc_entity_event_send(&world_->registry(),entity.id,event,p,bytes,&sender,handled);
}
int NativeCutsceneCamera::publish(uint32_t layer,uint32_t clear,uint32_t flags,UCSceneMatrixProvider&provider,const UCSceneMatrixServices&matrix){
 if(!constructed_||failure_)return 1;UCSceneCamera*selected=nullptr;
 if(uc_scene_camera_stack_current(stack_,layer,&selected)||selected!=&entity.camera)return 1;
 UCSceneViewServices s{this,[](void*,const UCSceneProjection*){return 1;},
  [](void*p)->void*{auto&c=*static_cast<NativeCutsceneCamera*>(p);return c.stack_->current_viewport(c.stack_->owner);},
  [](void*p)->const float*{return static_cast<NativeCutsceneCamera*>(p)->entity.transform;},
  [](void*,void*v){double values[5];return uc_scene_viewport_values(static_cast<UCSceneViewport*>(v),values)?NAN:static_cast<float>(values[4]);}};
 if(uc_scene_view_construct(&view,&entity.projection,clear,flags,&s)||uc_scene_view_publish(&view,
  [](void*p){return static_cast<UCSceneMatrixProvider*>(p);},&provider,&matrix))return failure_=2;return 0;
}
int NativeCutsceneCamera::close_detached(){
 if(stack_&&uc_scene_camera_stack_remove(stack_,&entity.camera))return 1;
 for(auto&listener:entity.target.listeners)if(listener==uc_entity_subscriptions_listener(subscriptions_))listener=nullptr;
 uc_entity_subscriptions_close(subscriptions_);subscriptions_=nullptr;
 if(entity.aux&&uc_entity_fresh_aux_destroy(entity.aux))return 1;entity.aux=nullptr;constructed_=false;
 if(services_.release_on_close){auto release=services_.release_camera;void*owner=services_.owner;release(owner,this);}return 0;
}
UCCutsceneCameraCleanup NativeCutsceneCamera::cleanup(){return {&world_->retirement(),this,
 [](void*p){auto&c=*static_cast<NativeCutsceneCamera*>(p);return c.services_.preflight(c.services_.owner);},
 [](void*p){auto&c=*static_cast<NativeCutsceneCamera*>(p);return uc_scene_camera_stack_remove(c.stack_,&c.entity.camera);},
 [](void*p)->void*{auto&c=*static_cast<NativeCutsceneCamera*>(p);return c.services_.current_provider(c.services_.owner);},
 [](void*p,void*v,int32_t i,int32_t*out){auto&c=*static_cast<NativeCutsceneCamera*>(p);return c.services_.player(c.services_.owner,v,i,out);},
 [](void*p,int32_t id,uint32_t sender){auto&c=*static_cast<NativeCutsceneCamera*>(p);return c.services_.return_control(c.services_.owner,id,sender);},
 [](void*p,void*v,int32_t m,int32_t a){auto&c=*static_cast<NativeCutsceneCamera*>(p);return c.services_.mode(c.services_.owner,v,m,a);},
 [](void*p,UCEntityEventTarget*t){auto*c=static_cast<NativeCutsceneCamera*>(p);if(t!=&c->entity.target)std::terminate();
   uc_entity_subscriptions_close(c->subscriptions_);c->subscriptions_=nullptr;c->entity.aux=nullptr;c->constructed_=false;
   auto release=c->services_.release_camera;void*owner=c->services_.owner;release(owner,c);}};}
int NativeCutsceneCamera::remove(){if(!constructed_||failure_||!services_.return_control)return 1;auto s=cleanup();return uc_cutscene_camera_remove(&entity.view,&fields,&s);}
int NativeCutsceneCamera::destroy(){if(!constructed_||failure_||!services_.release_camera)return 1;auto s=cleanup();return uc_cutscene_camera_destroy(&entity.view,subscriptions_,&s,1);}

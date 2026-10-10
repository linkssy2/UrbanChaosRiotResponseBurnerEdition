#include "mission_fire_damage.h"
#include <cmath>
#include <cstring>
int NativeMissionFireDamage::construct(NativeMissionWorld&w,int32_t parent,NativeNavigationBoxes&nav,const UCFireDamageServices&h,
 float radius,float seconds,int32_t owner,int32_t session,int32_t unique){
 if(world_||!h.session_mode||(*h.session_mode&7)||!h.player_count||!h.player||!h.multiplayer||!h.blocked||!h.damage||
    !std::isfinite(radius)||radius<=0||!std::isfinite(radius*radius)||!std::isfinite(seconds)||!uc_entity_event_lookup(&w.registry(),parent))return 1;
 world_=&w;navigation_=&nav;services_=h;entity.view.actor=nullptr;entity.view.aux84=nullptr;
 if(w.bind_dynamic({&entity.view,this,[](void*p){return ((NativeMissionFireDamage*)p)->update();},[](void*){return 0;},
   [](void*p){return ((NativeMissionFireDamage*)p)->close_detached();},nullptr,[](void*p){return ((NativeMissionFireDamage*)p)->remove();},
   [](void*p){return ((NativeMissionFireDamage*)p)->close_detached();}}))return failure_=2;
 auto context=w.context();const void*definition="FireDamageSphere",*source=nullptr;context.definition=&definition;*context.registration->creation={0,1,parent};
 if(uc_entity_base_construct_parent(&context,&entity.view,&source))return failure_=2;
 memcpy(state_.center,entity.transform+12,12);state_.id=entity.id;state_.remaining=seconds;state_.radius_squared=radius*radius;state_.owner=owner;state_.session=session;state_.unique=unique;
 float size[3]={radius*2.f,radius*2.f,radius*2.f};if(nav.add(entity.id,entity.transform+12,size,0))return failure_=2;box_=true;return 0;
}
int NativeMissionFireDamage::update(){
 if(!world_||failure_||removed_)return failure_?failure_:1;
 memcpy(state_.center,entity.transform+12,12);UCFireDamageServices h{};h.owner=this;h.session_mode=services_.session_mode;
 h.player_count=[](void*p){auto&s=*(NativeMissionFireDamage*)p;return s.services_.player_count(s.services_.owner);};
 h.player=[](void*p,uint32_t i,UCFirePlayer*out){auto&s=*(NativeMissionFireDamage*)p;return s.services_.player(s.services_.owner,i,out);};
 h.multiplayer=[](void*p){auto&s=*(NativeMissionFireDamage*)p;return s.services_.multiplayer(s.services_.owner);};
 h.blocked=[](void*p,const float*a,const float*b,void*actor,uint16_t group){auto&s=*(NativeMissionFireDamage*)p;return s.services_.blocked(s.services_.owner,a,b,actor,group);};
 h.damage=[](void*p,void*actor,float amount,const UCPlayerFireDamage*d){auto&s=*(NativeMissionFireDamage*)p;return s.services_.damage(s.services_.owner,actor,amount,d);};
 h.delta=[](void*p)->double{return *(*(NativeMissionFireDamage*)p).world_->script_services().delta;};
 h.remove=[](void*p){return ((NativeMissionFireDamage*)p)->remove();};
 if(uc_fire_damage_step(&state_,&h))return failure_=2;return 0;
}
int NativeMissionFireDamage::remove(){
 if(!world_||removed_||entity.model||entity.helper||entity.aux||entity.source!=-1)return 1;
 if(box_){if(navigation_->remove(entity.id))return failure_=2;box_=false;}
 entity.flags|=0x8100;uint8_t queued=0;if(uc_entity_retire(&world_->retirement(),entity.id,&queued)||!queued)return failure_=2;
 if(entity.identity.listeners[0])return failure_=2;removed_=true;return 0;
}
int NativeMissionFireDamage::close_detached(){
 if(!world_)return 0;if(uc_entity_event_lookup(&world_->registry(),entity.id))return 1;
 if(box_){if(navigation_->remove(entity.id))return failure_=2;box_=false;}world_=nullptr;navigation_=nullptr;return 0;
}

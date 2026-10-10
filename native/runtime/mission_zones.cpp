#include "mission_zones.h"
#include "../environment_ambient_region.h"
#include <cmath>
#include <cstring>
namespace {uint32_t word(const uint8_t*p){uint32_t v;std::memcpy(&v,p,4);return v;}}
int native_zone_volume(const UCLevelEntity&e,NativeZoneVolume&out){
 NativeZoneVolume v;if(!e.extra.data||e.extra.size<4)return 1;
 if(!word(e.extra.data)){if(e.extra.size!=4)return 1;out=v;return 0;}
 if(e.extra.size!=32||word(e.extra.data+4)!=1)return 1;
 std::memcpy(v.lower,e.extra.data+8,12);std::memcpy(v.upper,e.extra.data+20,12);
 uint32_t inside;if(!uc_environment_ambient_region_contains(v.lower,v.upper,v.lower,&inside))return 1;
 v.present=true;out=v;return 0;
}
int native_zone_contains(const NativeZoneVolume&v,const float*p,uint8_t*out){
 if(!out)return 1;if(!v.present){*out=0;return 0;}uint32_t inside;
 if(!uc_environment_ambient_region_contains(v.lower,v.upper,p,&inside))return 1;
 *out=uint8_t(inside);return 0;
}
int NativeMissionZone::position(int32_t id,const float**out)const{
 auto*t=uc_entity_event_lookup(&world_->registry(),id);if(!t)return 0;
 auto*v=world_->context().resolve(world_->context().owner,t);
 if(!v||!v->transform)return -1;*out=v->transform+12;return 1;
}
int NativeMissionZone::construct(NativeMissionWorld&w,uint32_t source,const NativeColdBreathServices*h){
 if(world_||source>=w.source_count())return 1;const auto&m=w.metadata(source);const auto*d=m.definition;
 if(!d||m.asset_present||*w.active_descriptor()!=&m)return 1;
 bool cold=!std::strcmp(d->class_name,"ColdBreathOnCharacters");
 if(!cold&&std::strcmp(d->class_name,"HotZone"))return 1;
 NativeZoneVolume authored;if(native_zone_volume(*d,authored)||(!cold&&!authored.present))return 1;
 if(cold&&(!h||!h->count||!h->at||!h->health||!h->random||!h->emit))return 1;
 world_=&w;definition_=d;cold_=cold;if(h)cold_services_=*h;
 if(uc_entity_base_construct_authored(&w.context(),&entity.view,w.active_descriptor()))return failure_=2;
 //HOT publishes immediately. COLD startsNULL and refreshes on each1C3130.
 if(!cold_)volume_=authored;
 if(uc_entity_names_bind(w.names(),cold_?"COLD ZONE":"HOT ZONE",entity.id))return failure_=2;named_=true;
 //Normal event table is empty. Query virtual4 is routed separately by owner.
 listener_.owner=this;int inserted=0;
 if(uc_entity_listener_register(&w.registry(),entity.id,entity.id,&listener_,w.listeners(),&inserted)||!inserted)return failure_=2;
 return 0;
}
int NativeMissionZone::query(uint32_t event,int32_t actor,uint8_t*handled,uint8_t*reply)const{
 if(!world_||failure_||!handled||!reply)return 1;
 *handled=0;if(event!=(cold_?28u:107u))return 0;
 *handled=1;*reply=0;if(!(entity.flags&0x10000000u))return 0;
 const float*p=nullptr;int found=position(actor,&p);if(found<=0)return found<0?1:0;
 return native_zone_contains(volume_,p,reply);
}
int NativeMissionZone::update(){
 if(!world_||failure_)return failure_?failure_:1;if(!cold_)return 0;
 if(entity.source!=-1&&native_zone_volume(*definition_,volume_))return failure_=2;
 const auto&h=cold_services_;int32_t count=h.count(h.owner);if(count<0||count>1024)return failure_=2;
 for(int32_t i=0;i<count;i++){
  int32_t id;double health;uint8_t inside=0;
  if(h.at(h.owner,i,&id)||h.health(h.owner,id,&health)||!std::isfinite(health))return failure_=2;
  if(health>0){const float*p=nullptr;if(position(id,&p)!=1||!volume_.present||native_zone_contains(volume_,p,&inside))return failure_=2;}
  if(inside){if(timers_.find(id)==timers_.end()){double timer=h.random(h.owner,.2f,2.f);if(!std::isfinite(timer))return failure_=2;timers_.emplace(id,float(timer));}}
  else timers_.erase(id);
  count=h.count(h.owner);if(count<0||count>1024)return failure_=2;
 }
 for(auto&pair:timers_){double remaining=double(pair.second)-*world_->script_services().delta;
  if(!std::isfinite(remaining))return failure_=2;pair.second=float(remaining);
  //Original x87 compares the unrounded subtraction before emitting.
  if(remaining<=0){if(uc_entity_event_lookup(&world_->registry(),pair.first)&&h.emit(h.owner,pair.first))return failure_=2;pair.second=2.f;}}
 return 0;
}
int NativeMissionZone::close_detached(){
 if(!world_)return 0;if(uc_entity_event_lookup(&world_->registry(),entity.id))return 1;
 if(named_&&uc_entity_names_remove(world_->names(),cold_?"COLD ZONE":"HOT ZONE",entity.id))return 1;
 if(entity.identity.listeners[0]&&uc_entity_listener_list_remove_first(world_->listeners(),entity.id))return 1;
 for(auto&l:entity.identity.listeners)l=nullptr;timers_.clear();named_=false;world_=nullptr;return 0;
}

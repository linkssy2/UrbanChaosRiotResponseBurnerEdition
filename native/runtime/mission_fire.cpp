#include "mission_fire.h"
#include <cmath>
#include <cstring>
int NativeMissionFire::construct(NativeMissionWorld&w,uint32_t source,NativeNavigationBoxes&nav,NativeLevelParticles&particles,
 const UCFireDamageServices&h,NativePlayerAudio&audio,const UCArchive*a,UCArchiveLookup&lookup,const std::filesystem::path&stream){
 if(world_||source>=w.source_count()||!h.session_mode||(*h.session_mode&7)||!h.player_count||!h.player||!h.damage||!a||lookup.archive!=a)return 1;
 const auto&m=w.metadata(source);const auto&d=*m.definition;
 const uint32_t tail[]={0,1,2,3,4,5,0xbf800000,1,6,1,0,0,0,0,1};
 if(*w.active_descriptor()!=&m||m.asset_present||m.condition_count||d.script_count||d.properties.size!=72||d.link_count!=7||
    memcmp((const char*)d.properties.data+12,tail,sizeof tail))return 1;
 memcpy(size_,d.properties.data,12);memcpy(center_,d.transform+12,12);
 for(unsigned k=0;k<3;k++)if(!std::isfinite(size_[k])||size_[k]<=0||size_[k]>65536||!std::isfinite(center_[k]))return 1;
 const char*split=strrchr(d.links[6],'#');if(!split||split==d.links[6]||!split[1])return 1;
 std::string bank(d.links[6],size_t(split-d.links[6]));size_t index=SIZE_MAX;
 if(uc_archive_lookup(&lookup,bank.c_str(),&index)||index==SIZE_MAX)return 1;
 world_=&w;navigation_=&nav;particles_=&particles;services_=h;sound_event_=split+1;
 if(w.bind(source,{&entity.view,this,[](void*p){return ((NativeMissionFire*)p)->update();},[](void*p){return ((NativeMissionFire*)p)->late();},
   [](void*p){return ((NativeMissionFire*)p)->close_detached();},&entity.script,[](void*p){return ((NativeMissionFire*)p)->remove();},
   [](void*p){return ((NativeMissionFire*)p)->close_detached();}}))return 2;
 if(uc_entity_fresh_construct_authored(&w.context(),&entity.view,w.active_descriptor())||entity.script.attach_actor(entity.view,m,w.script_services()))return 2;
 //1907A3: fire's own frame starts at the bottom, with X/Y basis exchanged.
 const float frame[16]={0,1,0,0,1,0,0,0,0,0,1,0,center_[0],center_[1]-size_[1]*.5f,center_[2],1};
 memcpy(entity.transform,frame,sizeof frame);
 if(uc_entity_names_bind(w.names(),"FIRE",entity.id)||nav.add(entity.id,center_,size_,0))return 2;
 subscription_={67,[](void*p,const void*,uint32_t*){((NativeMissionFire*)p)->entity.flags|=1;return 0;}};
 listener_={&subscription_,1,this};int inserted=0;
 if(uc_entity_listener_register(&w.registry(),entity.id,entity.id,&listener_,w.listeners(),&inserted)||!inserted)return 2;
 entity.flags|=0x2000;
 sound_=std::make_unique<NativePlayerAudio::MissionSource>(audio,a,index,stream);
 //216020 resolves the authored bank#event; FireStart is its entity-side tag.
 if(!sound_->play(sound_event_.c_str())||!particles.activate(source))return 2;
 *w.property_cursor(source)=d.properties.data+d.properties.size;active_=true;return 0;
}
int NativeMissionFire::update(){
 if(!world_||!active_||(*services_.session_mode&7))return 1;
 for(uint32_t i=0;;i++){
  int32_t count=services_.player_count(services_.owner);if(count<0||count>4||i>64)return 2;if(i>=uint32_t(count))break;
  UCFirePlayer p{};int found=services_.player(services_.owner,i,&p);if(found<0)return 2;if(!found)continue;if(!p.position||!p.actor)return 2;
  bool feet=true,head=true;for(unsigned k=0;k<3;k++){float low=center_[k]-size_[k]*.5f,high=center_[k]+size_[k]*.5f;
   float v=p.position[k];if(!std::isfinite(v))return 2;feet&=v>=low&&v<=high;v+=k==1?170.f:0.f;head&=v>=low&&v<=high;}
  if(!feet&&!head)continue;
  UCPlayerFireDamage damage{};damage.source=damage.owner=damage.other=damage.session=damage.unique=-1;damage.kind=12;memcpy(damage.center,center_,12);
  float x=p.position[0]-center_[0],y=p.position[1]-center_[1],z=p.position[2]-center_[2];double square=(double)z*z+(double)x*x;square+=(double)y*y;
  // At the exact center there is no direction; do not feed NaNs to damage.
  float inverse=square>0?(float)(1./std::sqrt(square)):0;
  damage.direction[0]=x*inverse;damage.direction[1]=y*inverse;damage.direction[2]=z*inverse;
  if(services_.damage(services_.owner,p.actor,50.f,&damage))return 2;++damage_calls;
 }
 sound_->step(*world_->script_services().delta);++ticks;return entity.script.update();
}
int NativeMissionFire::late(){
 if(!(entity.actor.word90&1)){for(unsigned i=12;i<15;i++){if(!std::isfinite(entity.transform[i]))return 1;entity.transform[i]+=0.0f;}entity.transform[15]=1;}return 0;
}
int NativeMissionFire::stop(){
 if(!active_)return 0;
 if(!particles_->deactivate(uint32_t(entity.source))||navigation_->remove(entity.id)||uc_entity_names_remove(world_->names(),"FIRE",entity.id))return 1;
 sound_->stop(sound_event_.c_str());active_=false;return 0;
}
int NativeMissionFire::remove(){
 if(!world_||entity.model||entity.helper||entity.actor.handleBC||stop())return 1;
 if(entity.script.close_detached())return 1;
 if(uc_entity_fresh_aux_clear(entity.aux))return 1;entity.flags|=0x8100;
 uint8_t queued=0;if(uc_entity_retire(&world_->retirement(),entity.id,&queued)||!queued)return 1;
 uc_entity_listener_list_remove_first(world_->listeners(),entity.id);for(auto&l:entity.identity.listeners)l=nullptr;
 if(entity.flags&0x1000){auto*f=world_->slot(uint32_t(entity.source)).flags;*f=(*f&~15u)|16u;}entity.flags|=0x80000;return 0;
}
int NativeMissionFire::close_detached(){
 if(!world_)return 0;if(uc_entity_event_lookup(&world_->registry(),entity.id)||stop()||entity.script.close_detached())return 1;
 sound_.reset();for(auto&l:entity.identity.listeners)l=nullptr;
 if(entity.aux&&uc_entity_fresh_aux_destroy(entity.aux))return 1;entity.aux=nullptr;world_=nullptr;return 0;
}

#include "camera_feedback.h"
#include "firstperson.h"
#include <cstdlib>
#include <stdexcept>
NativeCameraFeedback::NativeCameraFeedback(NativeMissionWorld&w,UCVectorRandom&r,const int32_t&port):world_(&w),random_(&r),physical_port_(&port){
 UCFeedbackMemory memory{nullptr,[](void*,size_t n){return std::malloc(n);},[](void*,void*p){std::free(p);}};
 if(uc_feedback_manager_create(&w,memory,&manager_))throw std::runtime_error("Native camera feedback allocation");
}
NativeCameraFeedback::~NativeCameraFeedback(){if(manager_&&close())std::abort();}
int NativeCameraFeedback::bind(NativeFirstperson&p){
 if(!manager_||player_||p.feedback)return 1;
 player_=&p;p.feedback=manager_;p.feedback_clock=&world_->time();p.feedback_random=random_;p.feedback_auxiliary=&auxiliary;
 return 0;
}
int NativeCameraFeedback::spatial(const float*p,float radius,float amplitude,float duration,uint32_t kind){
 if(!manager_)return 1;
 return uc_spatial_feedback_enqueue(uc_feedback_manager_queue(manager_),p,radius,amplitude,duration,kind,world_->time(),0)!=UC_FEEDBACK_OK;
}
int NativeCameraFeedback::step(){
 if(!manager_||!physical_port_||*physical_port_!=-1)return 1;
 if(!registered_){for(unsigned i=0;i<4;i++)outputs_[i]=uc_feedback_manager_output(manager_,i);registered_=true;}
 return uc_shot_feedback_advance(uc_feedback_manager_queue(manager_),world_->time(),*world_->script_services().delta,0,
  [](void*p){auto&s=*(NativeCameraFeedback*)p;if(*s.physical_port_!=-1||!s.registered_)return 0;
   for(unsigned i=0;i<4;i++)if(s.outputs_[i]!=uc_feedback_manager_output(s.manager_,i))return 0;return 1;},this)!=UC_FEEDBACK_OK;
}
int NativeCameraFeedback::close(){
 if(!manager_)return 0;
 if(player_){player_->feedback=nullptr;player_->feedback_clock=nullptr;player_->feedback_random=nullptr;player_->feedback_auxiliary=nullptr;player_=nullptr;}
 UCFeedbackManagerClose h{this,[](void*p)->int{auto&s=*(NativeCameraFeedback*)p;auto*v=uc_feedback_manager_observers(s.manager_);return v->position_count||v->player_count;},
  [](void*p,uint32_t i,UCFeedbackOutput*o){auto&s=*(NativeCameraFeedback*)p;if(s.outputs_[i]&&s.outputs_[i]!=o)std::abort();s.outputs_[i]=nullptr;},
  [](void*p,uint32_t i,UCFeedbackOutput*o){auto&s=*(NativeCameraFeedback*)p;if(s.outputs_[i]||uc_feedback_manager_output(s.manager_,i)!=o)std::abort();},
  [](void*,const UCFeedbackPositionObserver*){std::abort();},[](void*,const UCFeedbackPlayerObserver*){std::abort();}};
 int result=uc_feedback_manager_close(manager_,&h);if(!result){manager_=nullptr;registered_=false;}return result;
}

#include "assets.h"
#include "audio.h"
#include <algorithm>
#include <cmath>
#include <cstring>
int NativeTriggeredCameraShake::construct(NativeMissionWorld&w,uint32_t source,UCFeedbackManager*manager,const UCArchive*archive){
 if(world_||!manager||!archive||source>=w.source_count())return 1;
 const auto&m=w.metadata(source);const auto*d=m.definition;
 if(!d||strcmp(d->class_name,"TriggeredCameraShakeAndRumble")||m.asset_present||*w.active_descriptor()!=&m)return 1;
 const uint8_t*cursor=*w.property_cursor(source);int found=0;
 auto get=[&](uint32_t type,void*out){return !uc_entity_stream_get(d,&cursor,type,out,&found)&&found;};
 float radius=1000,amplitude=.1f,duration=1;uint32_t rumble=1,camera=1,centric=0;int32_t fade=0,count=0;const char*sound=nullptr;
 if(!get(2,&radius)||!get(2,&amplitude)||!get(3,&rumble)||!get(3,&camera)||!get(3,&centric)||!get(1,&fade)||!get(2,&duration)||!get(6,&sound)||!get(1,&count)||
    !std::isfinite(radius)||radius<=0||radius>1e6f||!std::isfinite(amplitude)||fabsf(amplitude)>65536||!std::isfinite(duration)||duration<=0||
    (centric&255)||fade!=3||!sound||strchr(sound,'#')||count<0||count>4096)return 1;
 std::vector<uint32_t> flags(count);for(auto&f:flags)if(!get(1,&f)||f>=w.script_services().flag_count)return 1;
 if(cursor!=d->properties.data+d->properties.size)return 1;
 //20E6F0 resolves the bank.20D350->20D320 returns NULL without '#', so
 //F47AB skips216BD0 entirely: no invented sound event or actor helper.
 BytesOwner bank;const auto index=find_resource(archive,sound);
 if(index==SIZE_MAX||uc_archive_read(archive,index,UC_AR_LOCAL_PAYLOAD,&bank.value))return 1;
 NativeAudioBank descriptor;descriptor.load(bank.value.data,bank.value.size);
 world_=&w;manager_=manager;duration_=duration;amplitude_=amplitude;flags_=std::move(flags);
 effect_.radius=radius;effect_.amplitude=amplitude;effect_.kind=((rumble&255)?2u:0u)|((camera&255)?1u:0u);
 observer_={this,[](void*p,uint32_t*out){*out=((NativeTriggeredCameraShake*)p)->effect_.kind;return 0;},
  [](void*p,const float*position,double*out){*out=uc_feedback_spatial_weight(&((NativeTriggeredCameraShake*)p)->effect_,position);return 0;}};
 if(uc_entity_fresh_construct_authored(&w.context(),&entity.view,w.active_descriptor()))return 2;
 *w.property_cursor(source)=cursor;
 return entity.script.attach_actor(entity.view,m,w.script_services())?2:0;
}
int NativeTriggeredCameraShake::stop(){
 if(active_){int changed=0;if(uc_feedback_manager_position(manager_,&observer_,0,&changed)||!changed)return 2;active_=false;}
 return 0;
}
int NativeTriggeredCameraShake::update(){
 if(!world_)return 1;
 bool triggered=true;for(auto f:flags_)triggered&=world_->script_services().flags[f]!=0;
 if(triggered){if(!active_&&!latched_){
   latched_=true;remaining_=duration_;effect_.amplitude=amplitude_;memcpy(effect_.position,entity.transform+12,12);
   int changed=0;if(uc_feedback_manager_position(manager_,&observer_,1,&changed)||!changed)return 2;active_=true;
 }}else latched_=false;
 if(active_){float t=remaining_/duration_;t=(t*t)*t;effect_.amplitude=std::max(.001f,amplitude_*t);
   if(remaining_>0)remaining_-=*world_->script_services().delta;else if(stop())return 2;
 }
 return entity.script.update()?2:0;
}
int NativeTriggeredCameraShake::late(){
 if(!(entity.actor.word90&1)){for(unsigned i=12;i<15;i++){if(!std::isfinite(entity.transform[i]))return 1;entity.transform[i]+=0.f;}entity.transform[15]=1.f;}return 0;
}
int NativeTriggeredCameraShake::remove(){
 if(!world_||entity.model||entity.helper||entity.actor.handleBC||!entity.aux)return 1;
 for(auto*l:entity.identity.listeners)if(l)return 1;
 if((entity.flags&0x1000)&&(entity.source<0||uint32_t(entity.source)>=world_->source_count()))return 1;
 if(stop())return 2;flags_.clear();uc_mission_command_destroy(nullptr,entity.actor.audioC0);entity.actor.audioC0=nullptr;
 if(uc_entity_fresh_aux_clear(entity.aux))return 2;
 entity.flags|=0x8100;uint8_t queued=0;if(uc_entity_retire(&world_->retirement(),entity.id,&queued)||!queued)return 2;
 if(entity.flags&0x1000){auto*f=world_->slot(entity.source).flags;*f=(*f&~15u)|16u;}entity.flags|=0x80000;return 0;
}
int NativeTriggeredCameraShake::close_detached(){
 if(stop()||entity.script.close_detached())return 1;
 if(entity.aux&&uc_entity_fresh_aux_destroy(entity.aux))return 1;
 entity.aux=nullptr;flags_.clear();world_=nullptr;manager_=nullptr;return 0;
}

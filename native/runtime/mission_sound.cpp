#include "mission_sound.h"
#include <cmath>
#include <cstring>
int NativeMissionSound::construct(NativeMissionWorld&w,int32_t parent,NativePlayerAudio&audio,const UCArchive*a,UCArchiveLookup&lookup,
 const std::filesystem::path&stream,const char*start,const char*stop,float seconds){
 if(world_||!a||lookup.archive!=a||!start||!stop||!std::isfinite(seconds)||seconds<=0||!uc_entity_event_lookup(&w.registry(),parent))return 1;
 const char*first=strrchr(start,'#'),*last=strrchr(stop,'#');
 if(!first||!last||first==start||last==stop||first-start!=last-stop||strncmp(start,stop,size_t(first-start))||!first[1]||!last[1])return 1;
 std::string bank(start,size_t(first-start));size_t index=SIZE_MAX;
 if(uc_archive_lookup(&lookup,bank.c_str(),&index)||index==SIZE_MAX)return 1;
 world_=&w;entity.view.actor=nullptr;entity.view.aux84=nullptr;
 if(w.bind_dynamic({&entity.view,this,[](void*p){return ((NativeMissionSound*)p)->update();},[](void*){return 0;},
   [](void*p){return ((NativeMissionSound*)p)->close_detached();},nullptr,[](void*p){return ((NativeMissionSound*)p)->remove();},
   [](void*p){return ((NativeMissionSound*)p)->close_detached();}}))return failure_=2;
 auto context=w.context();const void*definition="Sound",*source=nullptr;context.definition=&definition;*context.registration->creation={0,1,parent};
 if(uc_entity_base_construct_parent(&context,&entity.view,&source))return failure_=2;
 entity.flags|=0x100;start_=first+1;stop_=last+1;
 //216020 resolves stop then start; both qualified names refer to this same bank.
 audio_=std::make_unique<NativePlayerAudio::MissionSource>(audio,a,index,stream);
 state_.remaining=seconds;state_.has_bank=state_.has_start=1;return 0;
}
int NativeMissionSound::update(){
 if(!world_||failure_||removed_||!audio_)return failure_?failure_:1;
 UCMissionSoundServices h{};h.owner=this;
 h.play=[](void*p,unsigned stop){auto&s=*(NativeMissionSound*)p;s.audio_->play(stop?s.stop_.c_str():s.start_.c_str());return 0;};
 h.delta=[](void*p)->double{return *(*(NativeMissionSound*)p).world_->script_services().delta;};
 h.remove=[](void*p){return ((NativeMissionSound*)p)->remove();};
 if(uc_mission_sound_step(&state_,&h))return failure_=2;
 if(!removed_)audio_->step(*world_->script_services().delta);return 0;
}
int NativeMissionSound::remove(){
 if(!world_||removed_||entity.model||entity.helper||entity.aux||entity.source!=-1)return 1;
 entity.flags|=0x8100;uint8_t queued=0;if(uc_entity_retire(&world_->retirement(),entity.id,&queued)||!queued)return failure_=2;
 if(entity.identity.listeners[0])return failure_=2;
 removed_=true;return 0;
}
int NativeMissionSound::close_detached(){
 if(!world_)return 0;if(uc_entity_event_lookup(&world_->registry(),entity.id))return 1;
 audio_.reset();world_=nullptr;return 0;
}

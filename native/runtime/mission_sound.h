#pragma once
#include "mission_world.h"
#include "player_audio.h"
extern "C" {
#include "../mission_sound.h"
#include "../archive_lookup.h"
}
/* Concrete dynamic Sound child admitted for positive duration, same-bank
 * start/stop names. Source110E40/110F70 lifecycle, real shared audio scheduler.
 * Audio output retains MissionSource's documented PC nonspatial mix scope. */
class NativeMissionSound {
 NativeMissionWorld*world_=nullptr;
 std::unique_ptr<NativePlayerAudio::MissionSource>audio_;
 std::string start_,stop_;
 UCMissionSound state_{};bool removed_=false;int failure_=0;
public:
 NativeMissionEntity entity;
 int construct(NativeMissionWorld&,int32_t parent,NativePlayerAudio&,const UCArchive*,UCArchiveLookup&,
     const std::filesystem::path&stream,const char*start,const char*stop,float seconds);
 int update();int remove();int close_detached();
 const UCMissionSound&state()const{return state_;}
};

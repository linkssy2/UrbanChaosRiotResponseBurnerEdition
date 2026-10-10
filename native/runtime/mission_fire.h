#pragma once
#include "mission_world.h"
#include "navigation_boxes.h"
#include "player_audio.h"
#include "level_particles.h"
/* Authored Westside infinite FXFire190440/18FEB0. No invented spherical damage:
 * source18EF10 tests feet and feet+170 against the authored box. Finite fires,
 * extinguishing-team interactions and child lists remain outside this owner. */
class NativeMissionFire {
 NativeMissionWorld*world_=nullptr;NativeNavigationBoxes*navigation_=nullptr;NativeLevelParticles*particles_=nullptr;
 UCFireDamageServices services_{};std::unique_ptr<NativePlayerAudio::MissionSource> sound_;
 float center_[3]{},size_[3]{};std::string sound_event_;bool active_=false;
 UCEntityEventSubscription subscription_{};UCEntityEventListener listener_{};
 int stop();
public:
 NativeMissionEntity entity;
 uint64_t damage_calls=0,ticks=0;
 int construct(NativeMissionWorld&,uint32_t,NativeNavigationBoxes&,NativeLevelParticles&,const UCFireDamageServices&,
               NativePlayerAudio&,const UCArchive*,UCArchiveLookup&,const std::filesystem::path&);
 int update();int late();int remove();int close_detached();
};

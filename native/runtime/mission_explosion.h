#pragma once
#include "mission_world.h"
#include "player_audio.h"
extern "C" {
#include "../explosion_sequence.h"
#include "../rigid_prop_damage.h"
#include "../archive_lookup.h"
}
class NativeMissionExplosion;
struct NativeMissionExplosionServices {
 void*owner=nullptr;
 int(*field_step)(void*,UCExplosionField&,float)=nullptr;
 int(*shake)(void*,const float*,float,float,float,uint32_t)=nullptr;
 int(*online)(void*)=nullptr;
 int(*broadcast)(void*,const UCExplosionBlast&,int32_t)=nullptr;
 //World914/59280 lighting-area operation, NOT the NPC hazard registry.
 int(*area_effect)(void*,const float[12],uint32_t)=nullptr;
 int(*effect)(void*,NativeMissionExplosion&,uint32_t)=nullptr;
 int(*debris)(void*,NativeMissionExplosion&,uint32_t,float,float,const char*)=nullptr;
};
/* Actual E8E00/EA790 entity and retained native continuation. The physics
 * scanner uses the existing qualified force policy and explicit PC body
 * adapter supplied by the caller; no Havok body ownership is invented.
 * Audio uses the shared PC nonspatial scheduler. Required visual/damage
 * consumers must complete; absent or failed services latch mission failure. */
class NativeMissionExplosion {
 NativeMissionWorld*world_=nullptr;NativeCoroutine sequence_;
 NativeMissionExplosionServices services_{};
 UCExplosionConstructor state_{};UCExplosionSequence fields_{};
 std::unique_ptr<UCExplosionField> field_;
 std::unique_ptr<NativePlayerAudio::MissionSource>audio_;
 std::string bank_;bool initialized_=false,removed_=false;int failure_=0;
 int initialize(NativeMissionWorld&,const UCRigidPropExplosion*,uint32_t,
  NativePlayerAudio&,const UCArchive*,UCArchiveLookup&,const std::filesystem::path&,
  const NativeMissionExplosionServices&);
 UCExplosionSequenceServices hooks();
public:
 NativeMissionEntity entity;
 int construct(NativeMissionWorld&,const UCRigidPropExplosion&,NativePlayerAudio&,
  const UCArchive*,UCArchiveLookup&,const std::filesystem::path&,const NativeMissionExplosionServices&);
 //Caller binds this actual owner to the authored source slot before construct.
 int construct(NativeMissionWorld&,uint32_t source,NativePlayerAudio&,const UCArchive*,
  UCArchiveLookup&,const std::filesystem::path&,const NativeMissionExplosionServices&);
 int update();int remove();int close_detached();
 const UCExplosionConstructor&state()const{return state_;}
 const UCExplosionSequence&sequence()const{return fields_;}
};

#pragma once
#include "mission_world.h"
#include <map>

struct NativeZoneVolume {float lower[3]{},upper[3]{};bool present=false;};
//221420 metadata+6C holds an already world-space volume. No entity transform
//is applied again. The admitted source shape is1 (inclusive X,Z,Y bounds).
int native_zone_volume(const UCLevelEntity &,NativeZoneVolume &);
int native_zone_contains(const NativeZoneVolume &,const float position[3],uint8_t *);
struct NativeColdBreathServices {
 void *owner=nullptr;
 int32_t (*count)(void *)=nullptr; //current2F090 character collection
 int (*at)(void *,int32_t,int32_t *)=nullptr; //2F570, source order
 int (*health)(void *,int32_t,double *)=nullptr; //1F7B40("health",0)
 double (*random)(void *,float,float)=nullptr; //shared2D3780 stream
 //219740(id,"Bip01 Head",0), wrapped by218FF0 mode2: after each
 //sample219A50 swaps full matrix rows0 and2. Submit the original asset
 //"coldbreath.simpleparticlesystem" to manager virtual10. Must own the effect; absent emitter is rejected before the cold entity is created.
 int (*emit)(void *,int32_t)=nullptr;
};
//Concrete181D60 HotZone and1C3330 ColdBreathOnCharacters owners. Bind entity
//before construct. Cold update requires all real services; no inert cold owner.
//Explicit shutdown after real registry/record detach; no fake retirement.
class NativeMissionZone {
 NativeMissionWorld *world_=nullptr;const UCLevelEntity *definition_=nullptr;
 NativeColdBreathServices cold_services_{};NativeZoneVolume volume_{};
 UCEntityEventListener listener_{};std::map<int32_t,float> timers_;
 bool cold_=false,named_=false;int failure_=0;
 int position(int32_t,const float **)const;
public:
 NativeMissionEntity entity;
 NativeMissionZone(){entity.view.actor=nullptr;entity.view.aux84=nullptr;}
 int construct(NativeMissionWorld &,uint32_t,const NativeColdBreathServices *cold=nullptr);
 int query(uint32_t event,int32_t actor,uint8_t *handled,uint8_t *reply)const;
 int update();int late()const{return failure_;}int close_detached();
 int failure()const{return failure_;}
 const NativeZoneVolume &volume()const{return volume_;}
 size_t breathing_count()const{return timers_.size();}
};

#pragma once
#include "mission_world.h"
extern "C" {
#include "../explosion_children.h"
}
struct NativeExplosionVisualServices {
 void *owner=nullptr;
 // Actual manager+14 registration of source219530, burst1 and borrowed asset.
 int (*ring)(void *,NativeMissionEntity &,const UCParticleAsset &)=nullptr;
};
class NativeMissionExplosionVisual {
 NativeMissionWorld *world_=nullptr;
 UCParticleAsset asset_{};float remaining_=0;
 uint32_t kind_=0;int failure_=0;bool removed_=false;
public:
 NativeMissionEntity entity;
 int construct(NativeMissionWorld &,const UCExplosionVisualChild &,const NativeExplosionVisualServices &);
 int update();int late();int remove();int close_detached();
 const UCParticleAsset&asset()const{return asset_;}
};

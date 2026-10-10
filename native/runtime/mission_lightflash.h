#pragma once
#include "mission_world.h"
extern "C" {
#include "../shot_lightflash.h"
}
//175D60/176310 dynamic owner. The renderer callbacks create/release the same
//point-light state consumed by the scene; update uses original world delta.
struct NativeMissionLightFlashServices {
 void *owner=nullptr;
 UCPointLightState *(*create)(void *,const float[3])=nullptr;
 int (*release)(void *,UCPointLightState *)=nullptr;
};
class NativeMissionLightFlash {
 NativeMissionWorld *world_=nullptr;
 NativeMissionLightFlashServices services_{};UCLightFlash state_{};
 int failure_=0;bool removed_=false;
 UCLightFlashServices hooks();
public:
 NativeMissionEntity entity;
 int construct(NativeMissionWorld &,const UCShotLightFlashRequest &,const NativeMissionLightFlashServices &);
 int update();int late();int remove();int close_detached();
 const UCLightFlash&state()const{return state_;}
};

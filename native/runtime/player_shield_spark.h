#pragma once
#include "mission_world.h"
#include "coroutine.h"
extern "C" {
#include "../player_shield_hit.h"
}
class NativeNpcThrownParticles;
struct NativePlayerShieldSparkServices {
 void*owner=nullptr;
 int(*feedback)(void*,const float[3],float,float,float,unsigned)=nullptr;
};
//Actual FXShieldBulletSpark, source18EA80/18ECF0. Caller retains the object
//through world retirement; its four static particle sources own their matrices.
class NativePlayerShieldSpark {
 NativeMissionWorld*world_=nullptr;NativeNpcThrownParticles*particles_=nullptr;
 NativePlayerShieldSparkServices services_{};NativeCoroutine coroutine_;
 int failure_=0;bool removed_=false;
public:
 NativeMissionEntity entity;
 int construct(NativeMissionWorld&,int32_t parent,const float position[3],const float rotation[3],NativeNpcThrownParticles&,const NativePlayerShieldSparkServices&);
 int step();int late();int remove();int close_detached();
};

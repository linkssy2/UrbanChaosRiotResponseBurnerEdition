#pragma once
#include "character_navigation.h"
extern "C" {
#include "../model_render.h"
}

//21B2E0/2A7AD0 family1 bodies, retaining the authored bone, capsule and material.
//The PC query world publishes the current simulation pose directly. It does
//not claim the original Havok keyframe integration/contact dynamics.
class NativeCharacterCollision {
    struct Part {UCModelCollisionShape source;UCCharacterCapsule world{};uint16_t bone;uint32_t material;};
    std::vector<Part> parts_;
    uint32_t group_;int32_t entity_;const void *actor_;bool posed_=false;
public:
    NativeCharacterCollision(const UCModelCollisionAsset &,uint32_t bones,uint32_t group,int32_t entity,const void *actor);
    void update(const UCModel &,const UCModelRender &,const float actor_world[16]);
    void append(NativeCharacterWorld &)const;
    size_t size()const{return parts_.size();}
};

#pragma once
#include <array>
#include <cstdint>
struct X86Context;
using TrialCorpseShieldMatrix = std::array<float,16>;
// A private clone in the original shared physics world, never a corpse-pool slot.
// The caller owns model and must destroy this state before model or world.
struct TrialCorpseShieldPhysics {
    uint32_t ragdoll=0,world=0,model=0,chest=0,stack=0;
    TrialCorpseShieldMatrix chest_local{};
};
bool TrialCorpseShieldPhysicsCreate(const X86Context&,uint8_t*,TrialCorpseShieldPhysics&,
                                  uint32_t model,const TrialCorpseShieldMatrix& world,uint32_t owner);
bool TrialCorpseShieldPhysicsBefore(const X86Context&,uint8_t*,TrialCorpseShieldPhysics&,
                                  const TrialCorpseShieldMatrix& world,float dt);
bool TrialCorpseShieldPhysicsAfter(const X86Context&,uint8_t*,TrialCorpseShieldPhysics&,
                                 const TrialCorpseShieldMatrix& world,float dt);
void TrialCorpseShieldPhysicsDestroy(const X86Context&,uint8_t*,TrialCorpseShieldPhysics&);
bool TrialCorpseShieldPhysicsValid(const uint8_t*,const TrialCorpseShieldPhysics&);

#pragma once
#include "mission_world.h"
#include <vector>

struct NativeLinearImpulseServices {
    void *owner=nullptr;
    // Actual virtual74 lookup: status0/null means a confirmed bodyless actor;
    // unsupported classes must fail. Bodies stay alive through impulse().
    int (*body)(void *,int32_t actor,void **)=nullptr;
    // Wake and apply a linear impulse in physics coordinates. The existing
    // NativeRigidProp endpoint does both; it remains a PC physics adapter.
    int (*impulse)(void *,void *body,const float momentum[3])=nullptr;
};

// Complete174310 projection using current210CA0 slots and207D70 registry.
// No target creation, deferred retry, torque, or replacement physics solver.
//0 completed,1 rejected before effects,2 committed callback failure.
int native_linear_impulse_apply(const float transform[16],float magnitude,
    const std::vector<uint32_t> &targets,const UCLevelEntitySlots &,
    const UCEntityEventRegistry &,const NativeLinearImpulseServices &,float momentum[4]);

// Authored174180/174510 owner. Source D0 cannot suspend: its first120C60
// update completes174510, then runs the existing218C40 script suffix.
// The admitted stream property domain matches actual1-01 LinearImpulse471.
// Gameplay removal remains unsupported; close requires external world detach.
class NativeLinearImpulse {
    NativeMissionWorld *world_=nullptr;
    NativeLinearImpulseServices services_{};
    std::vector<uint32_t> targets_;
    float magnitude_=1000,momentum_[4]{};
    bool started_=false;
    int failure_=0;
public:
    NativeMissionEntity entity;
    int construct(NativeMissionWorld &,uint32_t source,const NativeLinearImpulseServices &);
    int update();int late();int close_detached();
    const float *momentum()const{return momentum_;}
    bool started()const{return started_;}
    int failure()const{return failure_;}
};

#pragma once
#include "../collision_asset_query.h"
#include <cstdint>
class NativePlayerAudio;

struct NativeFootstepCounts {
    uint64_t cycles=0,hits=0,misses=0,started=0,unsupported=0;
    uint32_t material=0,side=0;
};
/* PC static-world bridge for the existing CA350 displacement-cycle pulse.
 * Uses C3310's foot-offset/ray/material projection, then the SAME
 * player session's material audio route. Borrowed world/audio outlive this
 * object and remain stable. The static query replaces 214230's world traversal;
 * it does not supply dynamic bodies, category filtering or an actor registry.
 * This is audio only: visual Snow/Slush footprints are not represented as a
 * completed C3310 controller. WoodenHoles audio arbitration is unsupported:
 * those pulses increment unsupported and do not masquerade as Default sounds.
 * Crouching still emits source cycles; idle/repeated renders do not call step.
 * All work is on the owning simulation thread, once per completed due pulse. */
class NativePlayerFootsteps {
    const UCCollisionAsset &world_;
    NativePlayerAudio &audio_;
    NativeFootstepCounts counts_{};
public:
    NativePlayerFootsteps(const UCCollisionAsset &,NativePlayerAudio &);
    bool step(const float actor_frame[16],uint32_t side); // true: sound started
    const NativeFootstepCounts &counts() const {return counts_;}
};

#pragma once
#include <array>
#include <cstdint>
struct X86Context;

// Retail heat passes use fullscreen scratch targets, then return to the scene.
struct TrialCoopEffectsViewportScope {
    X86Context& context_;
    uint8_t* base_;
    uint32_t device_=0,target_=0,depth_=0,world_=0;
    std::array<uint32_t,6> viewport_{};
    int exceptions_;
    TrialCoopEffectsViewportScope(X86Context&,uint8_t*);
    ~TrialCoopEffectsViewportScope() noexcept(false);
};

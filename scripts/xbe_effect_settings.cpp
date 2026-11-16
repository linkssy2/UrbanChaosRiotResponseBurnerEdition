#include "xbe_effect_settings.h"
#include "xbe_graphics_settings.h"
#include "cpu/x86_ops.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include <cmath>
#include <cstring>

namespace {
thread_local const TrialLightingScope* lighting_scope = nullptr;
uint32_t Word(const uint8_t* base, uint32_t address) {
    uint32_t value; std::memcpy(&value, base + address, 4); return value;
}
bool EmergencyPoint(uint32_t index) {
    // Exact 1-01 authored identities retained by the accepted Simple policy.
    return index == 65 || index == 66 || index == 67 || index == 70 ||
           index == 71 || index == 83 || index == 91 || index == 98;
}
bool OutsideMediumPointRange(const uint8_t* base, uint32_t light, uint32_t receiver) {
    if(!TrialGuestReadable(light, 0x1C8) || base[light + 0x50]) return false;
    // 457880+08/0C dispatch to 8EAF0/B5490: world matrix +20, local AABB +80.
    // B2BA0/B2A40 use the B2D60-derived scale/offset and radius .5 volume.
    float world[16], bounds[6], volume[6];
    std::memcpy(world, base + receiver + 0x20, sizeof(world));
    std::memcpy(bounds, base + receiver + 0x80, sizeof(bounds));
    std::memcpy(volume, base + light + 0x1B0, sizeof(volume));
    for(float value : world) if(!std::isfinite(value)) return false;
    for(float value : bounds) if(!std::isfinite(value)) return false;
    for(float value : volume) if(!std::isfinite(value)) return false;
    if(world[3] != 0 || world[7] != 0 || world[11] != 0 || world[15] != 1) return false;
    double center[3], extent[3];
    for(unsigned i = 0; i < 3; ++i) {
        if(bounds[i] > bounds[i + 3] || volume[i] <= 0) return false;
        center[i] = (double(bounds[i]) + bounds[i + 3]) * .5;
        extent[i] = (double(bounds[i + 3]) - bounds[i]) * .5;
    }
    double distance_squared = 0;
    for(unsigned i = 0; i < 3; ++i) {
        double middle = world[12 + i], radius = 0;
        for(unsigned j = 0; j < 3; ++j) {
            middle += world[j * 4 + i] * center[j];
            radius += std::abs(double(world[j * 4 + i])) * extent[j];
        }
        const double low = (middle - radius) * volume[i] + volume[i + 3];
        const double high = (middle + radius) * volume[i] + volume[i + 3];
        const double nearest = low > 0 ? low : high < 0 ? high : 0;
        distance_squared += nearest * nearest;
    }
    // Retain the entire receiver if any of its bounds touches 75% light range.
    return distance_squared > .375 * .375;
}
}

TrialLightingScope::TrialLightingScope(const X86Context& ctx, const uint8_t* base)
    : previous_(lighting_scope), context_(&ctx),
      scenery_(trial_graphics::SceneryLighting()), characters_(trial_graphics::CharacterLighting()),
      simple_(trial_graphics::CharacterQuality() != 0) {
    lighting_scope = this;
    if((scenery_ && characters_ && !simple_) || !TrialGuestReadable(ctx.esp, 4)) return;
    world_ = Word(base, ctx.esp) == 0x9EA8Au;
    if(!world_ || !simple_ || !TrialGuestReadable(0x51C4DC, 4)) return;
    const uint32_t root = Word(base, 0x51C4DC);
    westside_ = TrialGuestReadable(root, 0x31) && Word(base, root) == 0x450C48u &&
                std::memcmp(base + root + 0x2C, "1-01", 5) == 0;
}
TrialLightingScope::~TrialLightingScope() { lighting_scope = previous_; }

bool TrialSkipLightingReceiver(const X86Context& ctx, const uint8_t* base) {
    const auto* scope = lighting_scope;
    if(!scope || scope->context_ != &ctx || !scope->world_ ||
       (scope->scenery_ && scope->characters_ && !scope->simple_)) return false;
    const uint32_t light = ctx.ebx, receiver = ctx.edi;
    if(!TrialGuestReadable(light, 0x7C) || !TrialGuestReadable(receiver, 0xB2)) return false;
    const uint32_t type = Word(base, light);
    // Spotlights include the flashlight; retain those passes in every mode.
    if((type != 0x4573F0u && type != 0x457338u) || Word(base, receiver) != 0x457880u) return false;
    // B5D80/B5DD0 tag first-person depth/projection on the interface at +0C.
    if(Word(base, receiver + 0x60) & 0x180u) return false;
    if(base[receiver + 0xB1]) {
        if(!scope->scenery_) return true;
        const uint32_t index = Word(base, light + 0x78);
        return scope->simple_ && type == 0x4573F0u && int32_t(index) >= 0 &&
               !(scope->westside_ && EmergencyPoint(index)) &&
               OutsideMediumPointRange(base, light, receiver);
    }
    if(!scope->characters_) return true;
    return scope->simple_ && type == 0x4573F0u &&
           !(scope->westside_ && EmergencyPoint(Word(base, light + 0x78)));
}


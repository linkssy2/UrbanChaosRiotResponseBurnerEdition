#include "xbe_effect_settings.h"
#include "xbe_graphics_settings.h"
#include "cpu/x86_ops.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
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
    if(base[receiver + 0xB1]) return !scope->scenery_;
    if(!scope->characters_) return true;
    return scope->simple_ && type == 0x4573F0u &&
           !(scope->westside_ && EmergencyPoint(Word(base, light + 0x78)));
}

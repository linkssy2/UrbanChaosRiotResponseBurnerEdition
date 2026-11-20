#pragma once
#include <cstring>
#include "cpu/x86_ops.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include "xbe_entity_registry.h"

namespace trial_entity_parent {
inline uint32_t Read(const uint8_t* base, uint32_t address) {
    uint32_t value;
    std::memcpy(&value, base + address, sizeof(value));
    return value;
}

inline bool RejectFlare(const X86Context& ctx, const uint8_t* base) {
    if (!TrialGuestReadable(ctx.esp, 16)) return false;
    const uint32_t type = Read(base, ctx.esp + 4);
    if (!TrialGuestReadable(type, 4) || Read(base, type) != 0x458978u) return false;
    // GunFlare consumers, including the patched player update, support an absent child.
    if (!TrialGuestReadable(ctx.ecx, 0x58)) return true;
    const uint32_t parent = Read(base, ctx.ecx + 0x54);
    if (parent == 0xffffffffu) return true;
    return trial_entity_registry::LookupWorld(base, parent) != ctx.ecx;
}
}

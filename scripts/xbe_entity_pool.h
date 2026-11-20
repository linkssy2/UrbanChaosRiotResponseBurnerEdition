#pragma once
#include <cstring>
#include <mutex>
#include <unordered_map>
#include <vector>
#include "kernel/kernel.h"
#include "xbe_input_memory.h"

namespace trial_entity_pool {
struct Owner {
    uint32_t state = 0, initial = 0;
    bool growing = false;
    std::vector<uint32_t> slabs;
};
inline std::unordered_map<uint32_t, Owner> owners;
inline std::recursive_mutex owner_lock;
inline uint32_t Read(const uint8_t* base, uint32_t at) {
    uint32_t value;
    std::memcpy(&value, base + at, 4);
    return value;
}
inline void Write(uint8_t* base, uint32_t at, uint32_t value) {
    std::memcpy(base + at, &value, 4);
}
inline uint32_t Call(X86Context& ctx, uint32_t function, uint32_t argument) {
    struct Restore {
        X86Context& ctx;
        X86Context saved;
        unsigned csr;
        explicit Restore(X86Context& value) : ctx(value), csr(_mm_getcsr()) {
            std::memcpy(&saved, &ctx, sizeof(ctx));
        }
        ~Restore() { std::memcpy(&ctx, &saved, sizeof(ctx)); _mm_setcsr(csr); }
    } restore(ctx);
    return kernel::CallGuest(ctx, function, {argument});
}
inline void Ensure(X86Context& ctx, uint8_t* base) {
    const uint32_t pool = ctx.ecx;
    if (!TrialGuestReadable(pool, 0x18) || Read(base, pool + 8)) return;
    const uint32_t world = Read(base, 0x51c4dc);
    if (!TrialGuestReadable(world, 8)) return;
    const uint32_t state = Read(base, world + 4);
    if (!TrialGuestReadable(state, 0x3c)) return;
    bool owned = false;
    for (uint32_t offset : {0x2cu, 0x30u, 0x34u, 0x38u})
        owned |= Read(base, state + offset) == pool;
    if (!owned) return;
    std::lock_guard<std::recursive_mutex> lock(owner_lock);
    if (Read(base, pool + 8)) return;
    auto [it, inserted] = owners.try_emplace(pool);
    Owner& owner = it->second;
    const uint32_t initial = Read(base, pool + 0x14);
    if (inserted) { owner.state = state; owner.initial = initial; }
    if (owner.state != state || owner.initial != initial || owner.growing)
        x86_trap(ctx, 0x2219e0, "Invalid world entity pool lifetime");
    // Reserve bookkeeping before guest allocation or publishing any free node.
    owner.slabs.reserve(owner.slabs.size() + 1);
    owner.growing = true;
    struct EndGrowth { bool& active; ~EndGrowth() { active = false; } } end{owner.growing};
    constexpr uint32_t count = 1024, bytes = count * 12;
    const uint32_t slab = Call(ctx, 0x2d589f, bytes);
    if (!slab || (slab & 3) || !TrialGuestReadable(slab, bytes))
        x86_trap(ctx, 0x2219e0, "Out of memory extending world entity pool");
    owner.slabs.push_back(slab);
    uint32_t head = Read(base, pool + 8);
    for (uint32_t i = 0; i < count; ++i) {
        const uint32_t node = slab + i * 12;
        Write(base, node, 0);
        Write(base, node + 4, head);
        Write(base, node + 8, 0);
        head = node;
    }
    Write(base, pool + 8, head);
}
inline void Release(X86Context& ctx, uint8_t*) {
    std::lock_guard<std::recursive_mutex> lock(owner_lock);
    auto it = owners.find(ctx.ecx);
    if (it == owners.end()) return;
    if (it->second.growing)
        x86_trap(ctx, 0x221930, "Destroying world entity pool during growth");
    // Detach first: allocator callbacks must not release the same slabs twice.
    auto entry = owners.extract(it);
    auto& slabs = entry.mapped().slabs;
    try {
        while (!slabs.empty()) {
            Call(ctx, 0x2d631d, slabs.back());
            slabs.pop_back();
        }
    } catch (...) {
        owners.insert(std::move(entry));
        throw;
    }
}
}

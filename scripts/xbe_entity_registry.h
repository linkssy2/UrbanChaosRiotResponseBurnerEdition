#pragma once
#include <cstring>
#include <atomic>
#include <mutex>
#include <new>
#include <unordered_map>
#include "cpu/x86_ops.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"

namespace trial_entity_registry {
inline std::unordered_map<uint64_t, uint32_t> extra;
inline std::mutex extra_mutex;
inline std::atomic<bool> has_extra{false};
inline uint64_t Key(uint32_t registry, uint32_t id) { return uint64_t(registry) << 32 | id; }
inline uint32_t Read(const uint8_t* base, uint32_t address) {
    uint32_t value; std::memcpy(&value, base + address, 4); return value;
}
inline void Write(uint8_t* base, uint32_t address, uint32_t value) {
    std::memcpy(base + address, &value, 4);
}
inline bool HasExtra(uint32_t registry, uint32_t id) {
    if (!has_extra.load(std::memory_order_acquire)) return false;
    const std::lock_guard lock(extra_mutex);
    return extra.find(Key(registry, id)) != extra.end();
}
inline uint32_t FindExtra(uint32_t registry, uint32_t id) {
    if (id == UINT32_MAX || !has_extra.load(std::memory_order_acquire)) return 0;
    const std::lock_guard lock(extra_mutex);
    const auto found = extra.find(Key(registry, id));
    return found == extra.end() ? 0 : found->second;
}
inline uint32_t Lookup(const uint8_t* base, uint32_t registry, uint32_t id) {
    if (id == UINT32_MAX) return 0;
    const uint32_t slot = registry + (id & 1023u) * 8;
    return Read(base, slot) == id ? Read(base, slot + 4) : FindExtra(registry, id);
}
inline uint32_t LookupWorld(const uint8_t* base, uint32_t id) {
    if (!base || id == UINT32_MAX || !TrialGuestReadable(0x51c4dc, 4)) return 0;
    const uint32_t world = Read(base, 0x51c4dc);
    if (!TrialGuestReadable(world, 8)) return 0;
    const uint32_t state = Read(base, world + 4);
    return TrialGuestReadable(state, 0x203c) ? Lookup(base, state + 0x3c, id) : 0;
}
inline uint32_t AllocateExtra(X86Context& ctx, uint8_t* base, uint32_t registry, uint32_t object) {
    const std::lock_guard lock(extra_mutex);
    uint32_t id = Read(base, registry + 0x2000);
    const uint64_t attempts = uint64_t(extra.size()) + 1025;
    for (uint64_t attempt = 0; attempt < attempts; ++attempt) {
        if (id >= 0x7ffffbffu) id = 0;
        const uint32_t slot = registry + (id & 1023u) * 8;
        if (Read(base, slot) != id && extra.find(Key(registry, id)) == extra.end()) {
            // Publish the guest counter only after host storage succeeds.
            try { extra.emplace(Key(registry, id), object); }
            catch (const std::bad_alloc&) { x86_trap(ctx, 0x207c70, "Out of memory extending entity registry"); }
            has_extra.store(true, std::memory_order_release);
            Write(base, registry + 0x2000, id + 1 >= 0x7ffffbffu ? 0 : id + 1);
            return id;
        }
        ++id;
    }
    x86_trap(ctx, 0x207c70, "Entity identifier space exhausted");
    return UINT32_MAX;
}
inline void Release(uint8_t* base, uint32_t registry, uint32_t id) {
    if (id == UINT32_MAX) return;
    const uint32_t slot = registry + (id & 1023u) * 8;
    if (Read(base, slot) == id) Write(base, slot, UINT32_MAX);
    else if (has_extra.load(std::memory_order_acquire)) {
        const std::lock_guard lock(extra_mutex);
        extra.erase(Key(registry, id));
        has_extra.store(!extra.empty(), std::memory_order_release);
    }
}
inline void Reset(uint32_t registry) {
    if (!has_extra.load(std::memory_order_acquire)) return;
    const std::lock_guard lock(extra_mutex);
    for (auto it = extra.begin(); it != extra.end();) {
        if (uint32_t(it->first >> 32) == registry) it = extra.erase(it);
        else ++it;
    }
    has_extra.store(!extra.empty(), std::memory_order_release);
}
}

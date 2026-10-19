#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <type_traits>

// Descriptor slots are immutable until their arena resets.
// Each tuple owns references so resource-address reuse cannot produce a hit.
namespace uc_srv_cache {
template<class Resource, class Ref, class View> class Cache {
public:
    struct Key {
        std::array<Resource*, 4> resources{};
        std::array<View, 4> views{};
    };
    static_assert(std::is_trivially_copyable_v<Key>);
    static Key MakeKey(Resource* const* resources, const View* views) {
        Key key{};
        for (unsigned i = 0; i < 4; ++i) key.resources[i] = resources[i];
        std::memcpy(key.views.data(), views, sizeof(key.views));
        return key;
    }
    void Reset() {
        for (auto& entry : entries_) if (entry.valid) {
            entry.valid = false;
            for (auto& ref : entry.held) ref = nullptr;
        }
        heap_ = nullptr;
    }
    bool Find(const void* heap, const Key& key, uint64_t hash, uint32_t& slot) {
        SetHeap(heap);
        const Entry& entry = entries_[hash & 63];
        if (!entry.valid || std::memcmp(&entry.key, &key, sizeof(key))) return false;
        slot = entry.slot;
        return true;
    }
    void Store(const void* heap, const Key& key, uint64_t hash, uint32_t slot) {
        // Allocation can renew the heap after Find. Never publish the old epoch.
        SetHeap(heap);
        Entry& entry = entries_[hash & 63];
        entry.key = key;
        entry.slot = slot;
        for (unsigned i = 0; i < 4; ++i) entry.held[i] = key.resources[i];
        entry.valid = true;
    }
private:
    struct Entry {
        bool valid = false;
        Key key{};
        std::array<Ref, 4> held{};
        uint32_t slot = 0;
    };
    std::array<Entry, 64> entries_{};
    const void* heap_ = nullptr;
    void SetHeap(const void* heap) {
        if (heap_ != heap) { Reset(); heap_ = heap; }
    }
};
}

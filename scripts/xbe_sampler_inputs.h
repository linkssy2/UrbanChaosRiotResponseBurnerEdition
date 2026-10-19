#pragma once
#include <array>
#include <cstdint>
#include <cstring>

namespace uc_sampler_inputs {

struct Key {
    std::array<std::array<uint32_t, 12>, 4> stage;
    uint32_t aniso, clamp, shadow_func;
};
static_assert(sizeof(Key) == 51 * sizeof(uint32_t));

// Slots remain valid until their sampler heap is replaced, including by a blit.
class Cache {
public:
    bool Find(const Key& key, uint32_t& slot) const {
        if (!valid_ || std::memcmp(&key_, &key, sizeof(key)) != 0) return false;
        slot = slot_;
        return true;
    }
    void Store(const Key& key, uint32_t slot) {
        key_ = key;
        slot_ = slot;
        valid_ = true;
    }
    void Reset() { valid_ = false; }
private:
    Key key_{};
    uint32_t slot_ = 0;
    bool valid_ = false;
};

} // namespace uc_sampler_inputs

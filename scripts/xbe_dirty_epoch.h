#pragma once
#include <atomic>
#include <cstdint>
#include <limits>

// Numeric single-entry TLS cache; the dirty map remains mutex-owned.
namespace uc_dirty_epoch {
inline std::atomic<uint64_t> revision{0};
struct Cache {
    uint64_t revision=0, value=0;
    uint32_t data=0;
    bool valid=false;
};
inline thread_local Cache cache;

// Construct and destroy under the map mutex around the original assignment.
// Insertion exceptions still publish a fresh even revision. Exhaustion stays
// permanently odd, forcing the original locked lookup instead of wrapping.
class Write {
    uint64_t finish_;
public:
    Write() noexcept {
        const auto before=revision.load(std::memory_order_relaxed);
        constexpr auto maximum=std::numeric_limits<uint64_t>::max();
        finish_=before>=maximum-1?maximum:before+2;
        revision.store(before>=maximum-1?maximum:before+1,std::memory_order_release);
    }
    Write(const Write&)=delete;
    Write& operator=(const Write&)=delete;
    ~Write() noexcept {revision.store(finish_,std::memory_order_release);}
};

inline bool TryRead(uint32_t data,uint64_t& value) noexcept {
    const uint64_t before=revision.load(std::memory_order_acquire);
    if((before&1)||!cache.valid||cache.data!=data||cache.revision!=before)return false;
    // Verify revision again after matching the numeric TLS entry.
    if(revision.load(std::memory_order_acquire)!=before)return false;
    value=cache.value;return true;
}

// Called after the ordinary lookup, still holding the map mutex.
inline void StoreLocked(uint32_t data,uint64_t value) noexcept {
    const auto current=revision.load(std::memory_order_acquire);
    cache={current,value,data,(current&1)==0};
}
}

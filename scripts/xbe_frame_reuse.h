#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <vector>

namespace uc_frame_reuse {
template<class Key> class Qualification {
    struct Entry { Key key; unsigned bits; };
    std::unordered_map<uint64_t,Entry> entries_;
public:
    template<class Check> unsigned Get(uint64_t hash,const Key& key,Check check) {
        auto it=entries_.find(hash);
        if(it!=entries_.end() && !std::memcmp(&key,&it->second.key,sizeof(Key)))return it->second.bits;
        unsigned bits=check();
        if(entries_.size()>=512)entries_.clear();
        entries_.insert_or_assign(hash,Entry{key,bits});
        return bits;
    }
};

// Upload addresses survive only until the frame's upload arena is reset.
template<class Allocation> class Geometry {
    struct Entry {
        const void* source=nullptr;
        std::vector<uint8_t> bytes;
        Allocation allocation{};
        uint64_t epoch=0;
    };
    std::array<Entry,64> entries_{};
    uint64_t epoch_=1;
public:
    void Reset() { if(++epoch_==0){for(auto& e:entries_)e.epoch=0;epoch_=1;} }
    template<class Upload> Allocation Get(const void* source,size_t size,Upload upload) {
        // Bound shadow storage to 16 MiB; tiny and unusually large streams bypass it.
        if(size<1024 || size>256*1024)return upload();
        auto& e=entries_[(reinterpret_cast<uintptr_t>(source)>>4)%entries_.size()];
        if(e.epoch==epoch_ && e.source==source && e.bytes.size()==size &&
           !std::memcmp(e.bytes.data(),source,size))return e.allocation;
        auto allocation=upload();
        e.bytes.resize(size);
        std::memcpy(e.bytes.data(),source,size);
        e.source=source;e.allocation=allocation;e.epoch=epoch_;
        return allocation;
    }
};
}

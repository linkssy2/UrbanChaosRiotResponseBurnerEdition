#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <unordered_map>

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

// Canals reused less than 1% of geometry bytes: keeping a CPU shadow copied
// nearly every stream twice. Upload each exact snapshot once instead; the
// existing frame arena and fences still own its lifetime.
template<class Allocation> class Geometry {
public:
    void Reset() {}
    template<class Upload> Allocation Get(const void*,size_t,Upload upload) { return upload(); }
};
}

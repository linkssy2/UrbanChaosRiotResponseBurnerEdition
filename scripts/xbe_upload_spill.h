#pragma once
#include <array>
#include <cstdint>

namespace uc_frame_reuse {
// Small overflow uploads share reusable pages; exceptional large draws keep a one-off buffer.
class UploadSpill {
    std::array<uint64_t,2> used_{};
    bool exhausted_=false;
public:
    static constexpr uint64_t page_bytes=16ull<<20;
    static constexpr unsigned page_count=2;
    struct Range {unsigned page;uint64_t offset;};
    Range Get(uint64_t size,uint64_t align){
        if(size<=page_bytes)for(unsigned i=0;i<page_count;++i){
            const auto offset=(used_[i]+align-1)&~(align-1);
            if(offset<=page_bytes-size){used_[i]=offset+size;return {i,offset};}
        }
        exhausted_=true;return {page_count,0};
    }
    bool Exhausted() const{return exhausted_;}
    void Reset(){used_.fill(0);exhausted_=false;}
};
}

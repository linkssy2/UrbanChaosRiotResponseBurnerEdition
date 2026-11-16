#pragma once
#include "cpu/x86_context.h"
#include "kernel/memory.h"
#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <immintrin.h>

// Exact ABI replay for B3370's pinned B33FF inverse call only.
namespace xbe_receiver_inverse {
class Cache {
    struct Entry {
        std::array<uint32_t,16> input{},output{};
        std::array<X86Xmm,8> xmm{};
        std::array<uint8_t,192> scratch{};
        uint32_t before_csr=0,after_csr=0;
        bool valid=false;
    };
    std::array<Entry,128> entries_{};
    static bool Committed(uint32_t first,uint32_t bytes) {
        const uint64_t end=uint64_t(first)+bytes;
        if(first<0x10000u||!bytes||end>0x100000000ull)return false;
        const uint32_t last=uint32_t(end-1)&~0xfffu;
        for(uint32_t page=first&~0xfffu;;page+=0x1000) {
            if(!mem::IsCommitted(page))return false;
            if(page==last)return true;
        }
    }
    static bool Admitted(const X86Context& ctx,const uint8_t* base,uint32_t csr) {
        if(!base||base!=mem::Base()||(csr&0x1f80u)!=0x1f80u||
           ctx.esp<0x10100u||uint64_t(ctx.esp)+0x94u!=ctx.ecx||
           uint64_t(ctx.ecx)+64>0x100000000ull)return false;
        const uint32_t work=((ctx.esp-4)&~15u)-192;
        // Guest stacks are committed above physical RAM, including D0000000.
        if(!Committed(work,uint32_t(uint64_t(ctx.ecx)+64-work))||
           !Committed(0x44c174,4))return false;
        uint32_t caller;std::memcpy(&caller,base+ctx.esp,4);
        return caller==0xb3404u;
    }
    static bool Finite(const std::array<uint32_t,16>& matrix) {
        for(uint32_t word:matrix)if((word&0x7f800000u)==0x7f800000u)return false;
        return true;
    }
    static size_t Index(const std::array<uint32_t,16>& matrix,uint32_t csr) {
        uint32_t hash=matrix[0]^std::rotl(matrix[5],7)^std::rotl(matrix[10],13)^
            std::rotl(matrix[12],19)^std::rotl(matrix[13],23)^matrix[14]^csr;
        hash^=hash>>16;hash*=0x7feb352du;hash^=hash>>15;
        return hash&(128-1);
    }
public:
    void Call(X86Context& ctx,uint8_t* base,X86Func* original) {
        const uint32_t before_csr=_mm_getcsr();
        if(!Admitted(ctx,base,before_csr)){original(ctx,base);return;}
        const uint32_t sp=ctx.esp,matrix=ctx.ecx;
        const uint32_t work=((sp-4)&~15u)-192;
        std::array<uint32_t,16> input;
        std::memcpy(input.data(),base+matrix,64);
        if(!Finite(input)){original(ctx,base);return;}
        auto& entry=entries_[Index(input,before_csr)];
        if(entry.valid&&entry.before_csr==before_csr&&
           !std::memcmp(entry.input.data(),input.data(),64)) {
            std::memcpy(base+sp-4,&ctx.ebp,4);
            std::memcpy(base+work,entry.scratch.data(),192);
            std::memcpy(base+matrix,entry.output.data(),64);
            std::memcpy(ctx.xmm,entry.xmm.data(),sizeof(ctx.xmm));
            ctx.eax=matrix;ctx.esp=sp+4;
            _mm_setcsr(entry.after_csr);
            return;
        }
        original(ctx,base);
        const uint32_t after_csr=_mm_getcsr();
        std::array<uint32_t,16> output;
        std::memcpy(output.data(),base+matrix,64);
        if(!Finite(output))return;
        entry.input=input;entry.output=output;
        std::memcpy(entry.xmm.data(),ctx.xmm,sizeof(ctx.xmm));
        std::memcpy(entry.scratch.data(),base+work,192);
        entry.before_csr=before_csr;entry.after_csr=after_csr;entry.valid=true;
    }
};
}

inline void TrialReceiverInverse(X86Context& ctx,uint8_t* base,X86Func* original) {
    static thread_local xbe_receiver_inverse::Cache cache;
    cache.Call(ctx,base,original);
}

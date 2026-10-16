// Compiles the production export wrapper, CallGuest and actual title completion callback.
#undef NDEBUG
#include <cassert>
#include <cstdio>
#include "kernel/exports.h"

namespace kernel {
static uint8_t memory[16384]{};
uint8_t* g_base=memory;
static KernelFn exports[400]{};
void RegisterExport(uint32_t n,const char*,KernelFn fn){assert(!exports[n]);exports[n]=fn;}
// GENERATED_CALL_GUEST
// GENERATED_ADAPTER
}

// GENERATED_CALLBACK

static uint32_t expected_error, expected_bytes, expected_iosb, calls;
void x86_icall(X86Context& ctx,uint8_t* base,uint32_t fn){
    using namespace kernel;
    assert(fn==0x251690);
    assert(R32(ctx.esp+4)==expected_error);
    assert(R32(ctx.esp+8)==expected_bytes);
    assert(R32(ctx.esp+12)==expected_iosb);
    uint32_t before=ctx.esp;
    __imp__sub_00251690(ctx,base);
    assert(ctx.esp==before+16);
    ++calls;
}

int main(){
    using namespace kernel;
    assert(exports[232] && exports[301]);
    X86Context ctx{};
    ctx.ebx=11;ctx.esi=12;ctx.edi=13;ctx.ebp=14;ctx.ecx=15;ctx.edx=16;
    struct Case {uint32_t status,error,bytes;};
    for(auto c:{Case{0,0,4096},Case{kEndOfFile,38,91},Case{kAccessDenied,5,92},
                Case{kUnsuccessful,317,93},Case{0,0,512},Case{kPending,997,94},
                Case{kNoMoreFiles,18,95}}){
        ctx.esp=8192;
        expected_iosb=1024;expected_bytes=int32_t(c.status)<0?0:c.bytes;
        expected_error=int32_t(c.status)<0?c.error:0;
        W32(1024,c.status);W32(1028,c.bytes);W32(1040,2048);
        W32(2048+8,1);W32(2048+44,c.bytes);W32(2048+48,3072);W32(2048+52,3076);
        W32(3072,1);W32(3076,0xdeadbeef);
        W32(8192,0xfffffff0);W32(8196,0x251690);W32(8200,1024);W32(8204,0);
        exports[232](ctx,g_base);
        assert(ctx.esp==8208 && ctx.eax==0);
        assert(ctx.ebx==11 && ctx.esi==12 && ctx.edi==13 && ctx.ebp==14 && ctx.ecx==15 && ctx.edx==16);
        assert(R32(3072)==0 && R32(3076)==c.bytes && R32(2048+8)==0);
        assert(R32(1024)==c.status && R32(1028)==c.bytes);
        ctx.esp=8192;W32(8196,c.status);exports[301](ctx,g_base);
        assert(ctx.eax==c.error && ctx.esp==8200);
    }
    assert(calls==7);
    std::puts("APC adapter: real title completion, status mapping, bytes, OVERLAPPED, register and stack preservation passed");
}

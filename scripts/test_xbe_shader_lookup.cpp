// Production VS/PS lookup methods; shader generation/compilation is replaced by
// deterministic bytecode fixtures. Existing shader-cache tests cover real DXBC.
#undef NDEBUG
#define NOMINMAX
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>
using Microsoft::WRL::ComPtr;

struct State { std::string source; };
struct Assembled {};
struct PixelKey { std::string source; };
struct DrawShaders {
    uint64_t vkey=0,pkey=0,vh=0,ph=0;
    bool quad=false;
    ID3DBlob* vs_code=nullptr;
    ID3DBlob* ps_code=nullptr;
};
constexpr uint64_t kPixelKeySalt=0x9E3779B97F4A7C15ull;
static std::string VertexHlsl(const State& s,const Assembled&,const DrawShaders&){return s.source;}
static std::string QuadHlsl(std::string source){return source+"|quad";}
static std::string PixelHlsl(const PixelKey& k){return k.source;}
static void SmoothToneShader(std::string& source){source+="|tone";}
static void AnalyticFogShader(std::string& source){source+="|fog";}
static void CoopAimBlurShader(std::string& source){source+="|blur";}
static std::string Bytes(ID3DBlob* code){
    return code?std::string(static_cast<const char*>(code->GetBufferPointer()),code->GetBufferSize()):std::string();
}
static uint64_t Hash(const std::string& bytes){
    uint64_t value=14695981039346656037ull;
    for(unsigned char byte:bytes)value=(value^byte)*1099511628211ull;
    return value;
}
struct Compiler {
    std::unordered_map<uint64_t,ComPtr<ID3DBlob>> blobs_;
    unsigned compile_calls=0,created=0;
    ID3DBlob* Compile(const std::string& source,const char* entry,const char* profile,uint64_t& hash){
        ++compile_calls;
        const std::string bytes=std::string(profile)+"|"+entry+"|"+source;
        hash=Hash(bytes);
        auto found=blobs_.find(hash);
        if(found!=blobs_.end())return found->second.Get();
        ComPtr<ID3DBlob> code;
        if(source.find("FAIL")==std::string::npos){
            assert(SUCCEEDED(D3DCreateBlob(bytes.size(),&code)));
            std::memcpy(code->GetBufferPointer(),bytes.data(),bytes.size());
            ++created;
        }
        blobs_[hash]=code;
        return code.Get();
    }
};
namespace Reference {
struct D3D12Backend : Compiler {
/* BEFORE_MEMBERS */
    void EnsureVertexShader(const State&,const Assembled&,DrawShaders&);
    void EnsurePixelShader(const PixelKey&,DrawShaders&,bool,bool,bool);
};
/* BEFORE */
}
namespace Candidate {
struct D3D12Backend : Compiler {
/* AFTER_MEMBERS */
    void EnsureVertexShader(const State&,const Assembled&,DrawShaders&);
    void EnsurePixelShader(const PixelKey&,DrawShaders&,bool,bool,bool);
};
/* AFTER */
}

static void Pair(Reference::D3D12Backend& before,Candidate::D3D12Backend& after,
                 uint64_t key,const std::string& source,bool quad=false,unsigned flags=0){
    DrawShaders a,b;
    a.vkey=b.vkey=key;a.pkey=b.pkey=key;a.quad=b.quad=quad;
    before.EnsureVertexShader({source},{},a);after.EnsureVertexShader({source},{},b);
    before.EnsurePixelShader({source},a,flags&1,flags&2,flags&4);
    after.EnsurePixelShader({source},b,flags&1,flags&2,flags&4);
    assert(a.vh==b.vh&&a.ph==b.ph);
    assert(Bytes(a.vs_code)==Bytes(b.vs_code)&&Bytes(a.ps_code)==Bytes(b.ps_code));
    assert(bool(a.vs_code)==bool(b.vs_code)&&bool(a.ps_code)==bool(b.ps_code));
    assert(before.compile_calls==after.compile_calls&&before.created==after.created);
    assert(before.shader_src_.size()==after.shader_src_.size()&&before.blobs_.size()==after.blobs_.size());
}
static void Semantics(){
    Reference::D3D12Backend before;Candidate::D3D12Backend after;
    Pair(before,after,1,"first");assert(after.compile_calls==2&&after.created==2);
    const auto* vertex=after.shader_src_.at(1).code.Get();
    const auto* pixel=after.shader_src_.at(1^kPixelKeySalt).code.Get();
    Pair(before,after,1,"first");assert(after.compile_calls==2&&after.created==2);
    assert(after.shader_src_.at(1).code.Get()==vertex&&after.shader_src_.at(1^kPixelKeySalt).code.Get()==pixel);
    // Different guest keys compiling identical sources reuse the same owned blobs.
    Pair(before,after,2,"first");assert(after.compile_calls==4&&after.created==2);
    assert(after.shader_src_.at(2).code.Get()==vertex&&after.shader_src_.at(2^kPixelKeySalt).code.Get()==pixel);
    Pair(before,after,3,"first",true);assert(after.compile_calls==6&&after.created==3);
    for(unsigned flags=0;flags<8;++flags)Pair(before,after,10+flags,"variants",false,flags);
    // Failures stay cached; a null blob must not become a retry or stale success.
    Pair(before,after,30,"FAIL");const auto calls=after.compile_calls;
    Pair(before,after,30,"FAIL");assert(after.compile_calls==calls);
    assert(!after.shader_src_.at(30).code&&!after.shader_src_.at(30^kPixelKeySalt).code);
    // An already warm blob still goes through Compile once for a new program key.
    uint64_t a_hash=0,b_hash=0;
    before.Compile("prewarmed","main","vs_5_0",a_hash);
    after.Compile("prewarmed","main","vs_5_0",b_hash);
    assert(a_hash==b_hash);const auto created=after.created;
    Pair(before,after,40,"prewarmed");assert(after.created==created+1);
    // Rehash both maps, then revisit cached success, failure and all PS variants.
    for(unsigned i=0;i<4096;++i)Pair(before,after,100+i,"many-"+std::to_string(i%17),i%3==0,i%8);
    Pair(before,after,1,"first");Pair(before,after,30,"FAIL");
    for(unsigned flags=0;flags<8;++flags)Pair(before,after,10+flags,"variants",false,flags);
    assert(after.shader_src_.at(1).code.Get()==vertex);
    // Owning entries survive independent source-map growth and retain one ref
    // even if the blob-cache owner is released in this lifetime-only check.
    ComPtr<ID3DBlob> held=after.shader_src_.at(1).code;
    const auto bytes=Bytes(held.Get());after.blobs_.clear();
    DrawShaders draw;draw.vkey=1;after.EnsureVertexShader({"first"},{},draw);
    assert(draw.vs_code==held.Get()&&Bytes(draw.vs_code)==bytes);
    after.shader_src_.clear();assert(Bytes(held.Get())==bytes);
    std::puts("PASS shader lookup: extracted VS/PS hits, misses, null failures, warm reuse, variants, rehash and owning lifetime; shader bytes/source hashes unchanged");
}

static volatile uint64_t sink;
template<class Backend>
static double Bench(unsigned keys,bool cold){
    Backend backend;State state{"bench"};PixelKey pixel{"bench"};Assembled assembled;
    if(!cold)for(unsigned i=0;i<keys;++i){
        DrawShaders draw;draw.vkey=draw.pkey=i;
        backend.EnsureVertexShader(state,assembled,draw);backend.EnsurePixelShader(pixel,draw,false,false,false);
    }
    constexpr unsigned iterations=1000000;
    uint64_t total=0;
    const auto begin=std::chrono::steady_clock::now();
    for(unsigned i=0;i<iterations;++i){
        if(cold&&i%keys==0)backend.shader_src_.clear();
        DrawShaders draw;draw.vkey=draw.pkey=i%keys;
        backend.EnsureVertexShader(state,assembled,draw);backend.EnsurePixelShader(pixel,draw,false,false,false);
        total+=draw.vh^draw.ph;
        total+=reinterpret_cast<uintptr_t>(draw.vs_code)^reinterpret_cast<uintptr_t>(draw.ps_code);
    }
    const auto end=std::chrono::steady_clock::now();sink=total;
    return std::chrono::duration<double,std::nano>(end-begin).count()/iterations;
}
static void Benchmark(unsigned keys,bool cold){
    std::array<double,7> before{},after{};
    for(unsigned n=0;n<before.size();++n){
        if(n&1){after[n]=Bench<Candidate::D3D12Backend>(keys,cold);before[n]=Bench<Reference::D3D12Backend>(keys,cold);}
        else{before[n]=Bench<Reference::D3D12Backend>(keys,cold);after[n]=Bench<Candidate::D3D12Backend>(keys,cold);}
    }
    std::sort(before.begin(),before.end());std::sort(after.begin(),after.end());
    std::printf("BENCH %s %u keys: median ns/draw %.3f -> %.3f (%+.2f%%); two shader lookups/draw, no live FPS claim\n",
        cold?"source-misses":"warm",keys,before[3],after[3],(after[3]/before[3]-1)*100);
}
int main(int argc,char** argv){
    Semantics();
    if(argc==2&&std::string(argv[1])=="--bench"){
        Benchmark(1,false);Benchmark(64,false);Benchmark(4096,false);Benchmark(512,true);
    }else assert(argc==1);
}

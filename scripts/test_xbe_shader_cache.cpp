// Actual Compile body and production cache adapter. Real D3DCompile and WARP
// pipeline creation verify bytecode reuse; this is not a live hitch measurement.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <cstdarg>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <string>
#define CHECK(x) do {if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
using Microsoft::WRL::ComPtr;
namespace app {
static std::filesystem::path fixture_directory;
std::filesystem::path ExeDir(){return fixture_directory;}
}
#include "xbe_shader_cache.cpp"
static unsigned compile_calls=0;
static UINT last_flags=0,last_effect_flags=0;
static std::vector<std::string> errors;
static HRESULT CountedCompile(LPCVOID source,SIZE_T size,LPCSTR filename,const D3D_SHADER_MACRO* defines,
    ID3DInclude* include,LPCSTR entry,LPCSTR profile,UINT flags,UINT effect_flags,ID3DBlob** code,ID3DBlob** error){
    ++compile_calls;last_flags=flags;last_effect_flags=effect_flags;
    return D3DCompile(source,size,filename,defines,include,entry,profile,flags,effect_flags,code,error);
}
static void Error(const char* format,...){
    char text[2048];va_list args;va_start(args,format);std::vsnprintf(text,sizeof(text),format,args);va_end(args);
    errors.emplace_back(text);
}
#define D3DCompile CountedCompile
#define LOGE(...) Error(__VA_ARGS__)
struct {double compile=0;uint64_t shaders=0,shader_disk_hits=0;} g_perf;
struct PerfScope{double& counter;};
static uint64_t Hash(const void* bytes,size_t count,uint64_t seed=14695981039346656037ull){
    auto* p=static_cast<const uint8_t*>(bytes);while(count--)seed=(seed^*p++)*1099511628211ull;return seed;
}
struct D3D12Backend {
    std::unordered_map<uint64_t,ComPtr<ID3DBlob>> blobs_;
    ID3DBlob* Compile(const std::string&,const char*,const char*,uint64_t&);
};
/* COMPILE_IMPLEMENTATION */
#undef D3DCompile

static std::vector<uint8_t> Bytes(ID3DBlob* code){
    CHECK(code);auto* bytes=static_cast<const uint8_t*>(code->GetBufferPointer());
    return {bytes,bytes+code->GetBufferSize()};
}
static const std::string source=R"(
float4 first(uint i:SV_VertexID):SV_Position{return float4(i,0,0,1);}
float4 second(uint i:SV_VertexID):SV_Position{return float4(i,1,0,1);}
float4 pixel():SV_Target{return float4(.2,.4,.6,1);}
)";

static void Pipeline(ID3DBlob* vs,ID3DBlob* ps){
    ComPtr<IDXGIFactory4> factory;ComPtr<IDXGIAdapter> adapter;ComPtr<ID3D12Device> device;
    CHECK(SUCCEEDED(CreateDXGIFactory2(0,IID_PPV_ARGS(&factory))));
    CHECK(SUCCEEDED(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter))));
    CHECK(SUCCEEDED(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device))));
    D3D12_ROOT_SIGNATURE_DESC root_desc{};root_desc.Flags=D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    ComPtr<ID3DBlob> serialized;ComPtr<ID3D12RootSignature> root;
    CHECK(SUCCEEDED(D3D12SerializeRootSignature(&root_desc,D3D_ROOT_SIGNATURE_VERSION_1,&serialized,nullptr)));
    CHECK(SUCCEEDED(device->CreateRootSignature(0,serialized->GetBufferPointer(),serialized->GetBufferSize(),IID_PPV_ARGS(&root))));
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso{};
    pso.pRootSignature=root.Get();pso.VS={vs->GetBufferPointer(),vs->GetBufferSize()};
    pso.PS={ps->GetBufferPointer(),ps->GetBufferSize()};
    pso.BlendState.RenderTarget[0].RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;
    auto& blend=pso.BlendState.RenderTarget[0];
    blend.SrcBlend=blend.SrcBlendAlpha=D3D12_BLEND_ONE;
    blend.DestBlend=blend.DestBlendAlpha=D3D12_BLEND_ZERO;
    blend.BlendOp=blend.BlendOpAlpha=D3D12_BLEND_OP_ADD;blend.LogicOp=D3D12_LOGIC_OP_NOOP;
    pso.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;pso.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;
    pso.RasterizerState.DepthClipEnable=TRUE;pso.SampleMask=UINT_MAX;
    pso.DepthStencilState.DepthFunc=D3D12_COMPARISON_FUNC_LESS;
    pso.DepthStencilState.StencilReadMask=D3D12_DEFAULT_STENCIL_READ_MASK;
    pso.DepthStencilState.StencilWriteMask=D3D12_DEFAULT_STENCIL_WRITE_MASK;
    pso.DepthStencilState.FrontFace={D3D12_STENCIL_OP_KEEP,D3D12_STENCIL_OP_KEEP,D3D12_STENCIL_OP_KEEP,D3D12_COMPARISON_FUNC_ALWAYS};
    pso.DepthStencilState.BackFace=pso.DepthStencilState.FrontFace;
    pso.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pso.NumRenderTargets=1;pso.RTVFormats[0]=DXGI_FORMAT_R8G8B8A8_UNORM;pso.SampleDesc.Count=1;
    ComPtr<ID3D12PipelineState> pipeline;
    CHECK(SUCCEEDED(device->CreateGraphicsPipelineState(&pso,IID_PPV_ARGS(&pipeline))));
}

int main(){
    namespace cache=uc_xbe_shader_cache;
    app::fixture_directory=std::filesystem::current_path()/
        ("shader-cache-test-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount64()));
    const auto identity=cache::CompilerIdentity();CHECK(identity.valid);
    CHECK(cache::CompilerIdentity().digest==identity.digest);
    uint64_t hash=0;D3D12Backend cold;
    auto* first=cold.Compile(source,"first","vs_5_0",hash);auto bytes=Bytes(first);
    CHECK(compile_calls==1&&g_perf.shaders==1&&g_perf.shader_disk_hits==0&&errors.empty());
    CHECK(last_flags==D3DCOMPILE_OPTIMIZATION_LEVEL1&&last_effect_flags==0);
    CHECK(cold.Compile(source,"first","vs_5_0",hash)==first&&compile_calls==1);
    D3D12Backend warm;
    CHECK(Bytes(warm.Compile(source,"first","vs_5_0",hash))==bytes);
    CHECK(compile_calls==1&&g_perf.shaders==1&&g_perf.shader_disk_hits==1);
    CHECK(warm.Compile(source,"second","vs_5_0",hash)&&compile_calls==2);
    CHECK(warm.Compile(source,"first","vs_4_0",hash)&&compile_calls==3);
    CHECK(warm.Compile(source+"\n// exact source key","first","vs_5_0",hash)&&compile_calls==4);
    CHECK(warm.Compile(source,"pixel","ps_5_0",hash)&&compile_calls==5);
    D3D12Backend cached_pipeline;
    auto* vs=cached_pipeline.Compile(source,"first","vs_5_0",hash);
    auto* ps=cached_pipeline.Compile(source,"pixel","ps_5_0",hash);
    CHECK(compile_calls==5);Pipeline(vs,ps);

    auto& store=cache::Active();ComPtr<ID3DBlob> missed;
    unsigned warmed=0;
    store.Warm([&](const std::string& src,const std::string& entry,const std::string& profile,ID3DBlob* code){
        ++warmed;ComPtr<ID3DBlob> expected;
        CHECK(store.Load(src,entry.c_str(),profile.c_str(),D3DCOMPILE_OPTIMIZATION_LEVEL1,0,&expected));
        CHECK(Bytes(expected.Get())==Bytes(code));
    });
    CHECK(warmed==5&&compile_calls==5);
    CHECK(!store.Load(source,"first","vs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&missed));
    CHECK(!store.Load(source,"first","vs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL1,1,&missed));
    auto changed=identity;changed.contract[3]^=1;
    CHECK(!cache::Store(store.directory,changed).Load(source,"first","vs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL1,0,&missed));
    changed=identity;changed.digest[0]^=1;
    CHECK(!cache::Store(store.directory,changed).Load(source,"first","vs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL1,0,&missed));
    changed=identity;changed.valid=false;
    CHECK(!cache::Store(store.directory,changed).Load(source,"first","vs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL1,0,&missed));
    CHECK(!store.Load(std::string("x\0y",3),"first","vs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL1,0,&missed));

    std::string key;std::array<uint32_t,6> contract{};char path[1024];
    CHECK(store.Key(source,"first","vs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL1,0,key,contract));
    CHECK(nv_shader_path(path,store.directory.c_str(),key.c_str(),contract.data()));
    auto fallback=[&]{
        const unsigned before=compile_calls;D3D12Backend fresh;
        CHECK(Bytes(fresh.Compile(source,"first","vs_5_0",hash))==bytes&&compile_calls==before+1);
        D3D12Backend repaired;
        CHECK(Bytes(repaired.Compile(source,"first","vs_5_0",hash))==bytes&&compile_calls==before+1);
    };
    // Path collisions, truncated files, and payload corruption all recompile.
    {
        std::fstream file(path,std::ios::binary|std::ios::in|std::ios::out);CHECK(file);
        NvShaderDiskHeader header{};file.read(reinterpret_cast<char*>(&header),sizeof(header));
        header.contract[1]^=1;file.seekp(0);file.write(reinterpret_cast<const char*>(&header),sizeof(header));
    }
    warmed=0;store.Warm([&](const auto&,const auto&,const auto&,ID3DBlob*){++warmed;});
    CHECK(warmed==4&&compile_calls==5);
    fallback();
    // Warm-up parses untrusted length-prefixed keys before the normal cache load.
    for(const std::string bad:{"999999999999999:","1:","-1:","0:"}){
        std::fstream file(path,std::ios::binary|std::ios::in|std::ios::out);CHECK(file);
        file.seekp(sizeof(NvShaderDiskHeader)+std::string("UC-D3D12-DXBC-1:").size()+64);
        file.write(bad.data(),bad.size());file.close();
        warmed=0;const auto before=compile_calls;
        store.Warm([&](const auto&,const auto&,const auto&,ID3DBlob*){++warmed;});
        CHECK(warmed==4&&compile_calls==before);fallback();
    }
    {std::ofstream file(path,std::ios::binary|std::ios::trunc);file<<"truncated";}fallback();
    {
        std::fstream file(path,std::ios::binary|std::ios::in|std::ios::out);CHECK(file);
        file.seekg(-1,std::ios::end);char byte=0;file.read(&byte,1);byte^=1;
        file.seekp(-1,std::ios::end);file.write(&byte,1);
    }fallback();
    // Even a correctly checksummed non-DXBC payload must fall back to compilation.
    CHECK(nv_shader_disk_save(store.directory.c_str(),key.c_str(),contract.data(),"invalid DXBC",12));fallback();
    // Verify exact source comparison independently of the filename hash.
    {
        std::fstream file(path,std::ios::binary|std::ios::in|std::ios::out);CHECK(file);
        file.seekp(sizeof(NvShaderDiskHeader));file.put('X');
    }fallback();

    const auto previous=store;
    {std::ofstream file(app::fixture_directory/"blocked");file<<"not a directory";}
    store=cache::Store(app::fixture_directory/"blocked"/"child",identity);CHECK(store.directory.empty());
    unsigned before=compile_calls;
    {D3D12Backend a;CHECK(Bytes(a.Compile(source,"first","vs_5_0",hash))==bytes);}
    {D3D12Backend b;CHECK(Bytes(b.Compile(source,"first","vs_5_0",hash))==bytes);}
    CHECK(compile_calls==before+2);store=previous;
    before=compile_calls;D3D12Backend bad;const std::string invalid="this is not valid HLSL";
    CHECK(!bad.Compile(invalid,"first","vs_5_0",hash)&&compile_calls==before+1);
    CHECK(errors.size()==2&&errors[0].find("compile failed")!=std::string::npos&&errors[1]==invalid);
    CHECK(!bad.Compile(invalid,"first","vs_5_0",hash)&&compile_calls==before+1);
    CHECK(!store.Load(invalid,"first","vs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL1,0,&missed));
    std::error_code error;std::filesystem::remove_all(app::fixture_directory,error);CHECK(!error);
    std::puts("PASS actual Compile: disk reuse and byte equality, WARP PSO, exact source/entry/profile/flags/compiler identity, corrupt/unavailable fallback and original compile error behavior");
}

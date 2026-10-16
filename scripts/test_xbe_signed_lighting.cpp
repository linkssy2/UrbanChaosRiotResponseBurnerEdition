// Retail stage setup plus the actual upload conversion sampled through D3D12.
// This does not establish a particular live face's final appearance.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#undef NDEBUG
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <unordered_map>
#include <windows.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include "cpu/x86_ops.h"
#include "xbe_signed_lighting.h"
using Microsoft::WRL::ComPtr;
static std::array<uint8_t, 0x600000> memory;
static std::vector<unsigned> bound;
static uint32_t Read(uint32_t p) { uint32_t v; std::memcpy(&v, memory.data()+p, 4); return v; }
static void Write(uint32_t p,uint32_t v) { std::memcpy(memory.data()+p, &v, 4); }
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*) { std::abort(); }
void x86_icall(X86Context& ctx,uint8_t*,uint32_t target) {
    assert(target==0x11111111 || target==0x22222222);
    ctx.eax=target==0x11111111 ? 0x11000 : 0x12000; ctx.esp+=4;
}
X86_FUNC(sub_003E3840) {
    bound.push_back(Read(ctx.esp+4)); assert(Read(ctx.esp+8)==0x12000); ctx.esp+=12;
}
// GENERATED_ORIGINAL

static void Retail() {
    Write(0x519f60,0x10000); Write(0x10000,0x10100); Write(0x101c4,0x11111111);
    Write(0x11000,0x11100); Write(0x11194,0x22222222);
    auto run=[](X86Func* fn) { X86Context c{}; c.esp=0x500000;
        c.ebx=11;c.esi=12;c.edi=13;Write(c.esp,0xfeedcafe);fn(c,memory.data());
        assert(c.esp==0x500004 && c.ebx==11 && c.esi==12 && c.edi==13); };
    run(__imp__sub_000B1A20);
    assert((bound==std::vector<unsigned>{2,3}));
    assert(Read(0x3f3ec8)==0xe0000000 && Read(0x3f3f48)==0xe0000000);
    run(__imp__sub_000B1AB0);
    assert(Read(0x3f3ec8)==0 && Read(0x3f3f48)==0);
    gpu::TextureDesc d{};d.cube=true;d.format=6;
    for(unsigned mask=0;mask<16;++mask) {
        assert(uc_xbe_signed::CubeSigns(d,mask<<28)==mask);
        for(unsigned byte=0;byte<256;++byte) {
            std::vector<uint8_t> p(4,uint8_t(byte));uint32_t pitch=4;
            uc_xbe_signed::Expand(p,pitch,mask);
            if(!mask) { assert(p.size()==4 && pitch==4 && p[0]==byte);continue; }
            float v[4];std::memcpy(v,p.data(),16);
            for(unsigned c=0;c<4;++c) {
                const unsigned bits[]={2,4,8,1};
                float expected=mask&bits[c] ? (byte<128?int(byte):int(byte)-256)/128.f : byte/255.f;
                assert(v[c]==expected);
            }
        }
    }
    d.cube=false;assert(!uc_xbe_signed::CubeSigns(d,0xe0000000));
    d.cube=true;d.format=0x0b;assert(!uc_xbe_signed::CubeSigns(d,0xe0000000));
    std::unordered_map<uc_xbe_signed::TextureKey,unsigned,uc_xbe_signed::TextureKeyHash> cache;
    for(unsigned m=0;m<16;++m)cache[{0x83c0c000,0,m}]=m;
    cache[{0x83c0c000,0x82300000,0}]=99;assert(cache.size()==17);
    for(unsigned m=0;m<16;++m)assert(cache.at({0x83c0c000,0,m})==m);
}
static void Ok(HRESULT hr) { if(FAILED(hr)){std::fprintf(stderr,"D3D12 HRESULT %08X\n",unsigned(hr));std::abort();} }
static D3D12_RESOURCE_DESC Buffer(uint64_t bytes,D3D12_RESOURCE_FLAGS flags=D3D12_RESOURCE_FLAG_NONE) {
    D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=bytes;
    d.Height=1;d.DepthOrArraySize=1;d.MipLevels=1;d.SampleDesc.Count=1;
    d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;d.Flags=flags;return d;
}
static void Sample(bool warp,unsigned signs) {
    ComPtr<IDXGIFactory4> factory;Ok(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
    ComPtr<IDXGIAdapter> adapter;if(warp)Ok(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
    ComPtr<ID3D12Device> device;Ok(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)));
    auto resource=[&](D3D12_RESOURCE_DESC d,D3D12_HEAP_TYPE heap,D3D12_RESOURCE_STATES state) {
        ComPtr<ID3D12Resource> r;D3D12_HEAP_PROPERTIES p{};p.Type=heap;
        Ok(device->CreateCommittedResource(&p,D3D12_HEAP_FLAG_NONE,&d,state,nullptr,IID_PPV_ARGS(&r)));return r; };
    D3D12_RESOURCE_DESC td{};td.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;td.Width=8;td.Height=8;
    td.DepthOrArraySize=6;td.MipLevels=3;td.SampleDesc.Count=1;
    td.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;
    auto texture=resource(td,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COPY_DEST);
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp[18];UINT rows[18];UINT64 sizes[18],bytes;
    device->GetCopyableFootprints(&td,0,18,0,fp,rows,sizes,&bytes);
    auto upload=resource(Buffer(bytes),D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ);
    uint8_t* mapped;Ok(upload->Map(0,nullptr,reinterpret_cast<void**>(&mapped)));
    for(unsigned face=0;face<6;++face)for(unsigned level=0;level<3;++level) {
        unsigned w=8>>level,index=face*3+level;std::vector<uint8_t> p(w*w*4);
        for(unsigned y=0;y<w;++y)for(unsigned x=0;x<w;++x) {
            unsigned n=(y*w+x)*4;p[n]=128;p[n+1]=255;p[n+2]=(x&1)?128:127;p[n+3]=64; }
        uint32_t pitch=w*4;uc_xbe_signed::Expand(p,pitch,signs);
        assert(pitch==w*16 && p.size()==w*w*16);
        for(unsigned y=0;y<w;++y)std::memcpy(mapped+fp[index].Offset+y*fp[index].Footprint.RowPitch,p.data()+y*pitch,pitch);
    }
    upload->Unmap(0,nullptr);
    auto output=resource(Buffer(18*16,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    auto readback=resource(Buffer(18*16),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=2;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ComPtr<ID3D12DescriptorHeap> heap;Ok(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&heap)));
    auto cpu=heap->GetCPUDescriptorHandleForHeapStart();D3D12_SHADER_RESOURCE_VIEW_DESC sv{};
    sv.Format=td.Format;sv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURECUBE;sv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;sv.TextureCube.MipLevels=3;
    device->CreateShaderResourceView(texture.Get(),&sv,cpu);cpu.ptr+=device->GetDescriptorHandleIncrementSize(hd.Type);
    D3D12_UNORDERED_ACCESS_VIEW_DESC uv{};uv.ViewDimension=D3D12_UAV_DIMENSION_BUFFER;uv.Buffer.NumElements=18;uv.Buffer.StructureByteStride=16;
    device->CreateUnorderedAccessView(output.Get(),nullptr,&uv,cpu);
    D3D12_DESCRIPTOR_RANGE ranges[2]={{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,1,0,0,1}};
    D3D12_ROOT_PARAMETER parameter{};parameter.ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameter.DescriptorTable={2,ranges};
    D3D12_STATIC_SAMPLER_DESC sampler{};sampler.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.MaxLOD=D3D12_FLOAT32_MAX;sampler.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;
    D3D12_ROOT_SIGNATURE_DESC rd{};rd.NumParameters=1;rd.pParameters=&parameter;rd.NumStaticSamplers=1;rd.pStaticSamplers=&sampler;
    ComPtr<ID3DBlob> rootcode,error;Ok(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&rootcode,&error));
    ComPtr<ID3D12RootSignature> root;Ok(device->CreateRootSignature(0,rootcode->GetBufferPointer(),rootcode->GetBufferSize(),IID_PPV_ARGS(&root)));
    const char shader[]=R"(TextureCube<float4> t:register(t0);SamplerState s:register(s0);RWStructuredBuffer<float4> outp:register(u0);
        [numthreads(18,1,1)]void main(uint3 id:SV_DispatchThreadID){
        float3 d[6]={float3(1,0,0),float3(-1,0,0),float3(0,1,0),float3(0,-1,0),float3(0,0,1),float3(0,0,-1)};
        outp[id.x]=t.SampleLevel(s,d[id.x/3],id.x%3);})";
    ComPtr<ID3DBlob> code;Ok(D3DCompile(shader,sizeof(shader),nullptr,nullptr,nullptr,"main","cs_5_0",D3DCOMPILE_OPTIMIZATION_LEVEL1,0,&code,&error));
    D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={code->GetBufferPointer(),code->GetBufferSize()};
    ComPtr<ID3D12PipelineState> pipeline;Ok(device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pipeline)));
    ComPtr<ID3D12CommandAllocator> allocator;Ok(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
    ComPtr<ID3D12GraphicsCommandList> list;Ok(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)));
    for(unsigned i=0;i<18;++i) { D3D12_TEXTURE_COPY_LOCATION from{upload.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};from.PlacedFootprint=fp[i];
        D3D12_TEXTURE_COPY_LOCATION to{texture.Get(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};to.SubresourceIndex=i;list->CopyTextureRegion(&to,0,0,0,&from,nullptr); }
    auto transition=[&](ID3D12Resource* r,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after) {
        D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={r,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,before,after};list->ResourceBarrier(1,&b); };
    transition(texture.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    list->SetPipelineState(pipeline.Get());list->SetComputeRootSignature(root.Get());ID3D12DescriptorHeap* hp=heap.Get();list->SetDescriptorHeaps(1,&hp);
    list->SetComputeRootDescriptorTable(0,heap->GetGPUDescriptorHandleForHeapStart());list->Dispatch(1,1,1);
    transition(output.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
    list->CopyBufferRegion(readback.Get(),0,output.Get(),0,18*16);Ok(list->Close());
    D3D12_COMMAND_QUEUE_DESC qd{};ComPtr<ID3D12CommandQueue> queue;Ok(device->CreateCommandQueue(&qd,IID_PPV_ARGS(&queue)));
    ID3D12CommandList* cl=list.Get();queue->ExecuteCommandLists(1,&cl);
    ComPtr<ID3D12Fence> fence;Ok(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)));Ok(queue->Signal(fence.Get(),1));
    HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);assert(event);Ok(fence->SetEventOnCompletion(1,event));assert(WaitForSingleObject(event,10000)==WAIT_OBJECT_0);CloseHandle(event);
    float* values;D3D12_RANGE range{0,18*16};Ok(readback->Map(0,&range,reinterpret_cast<void**>(&values)));
    float expected[]={-1.f/256,-1.f/128,-1.f,signs&1 ? .5f : 64.f/255};
    for(unsigned n=0;n<18;++n)for(unsigned c=0;c<4;++c) {
        if(std::abs(values[n*4+c]-expected[c])>1e-6f)std::fprintf(stderr,"sample%u channel%u actual%g expected%g\n",n,c,values[n*4+c],expected[c]);
        assert(std::abs(values[n*4+c]-expected[c])<1e-6f); }
    readback->Unmap(0,nullptr);
    std::printf("D3D12 %s signed cube mask %X: 6 faces x 3 mip levels x 4 components passed\n",warp?"WARP":"hardware",signs);
}
int main(int argc,char** argv) {
    Retail();bool hardware=argc>1&&!std::strcmp(argv[1],"hardware");
    Sample(!hardware,14);Sample(!hardware,15);
    std::puts("Retail RGB-signed setup/teardown, 16x256 component cases and cache separation passed.");
}

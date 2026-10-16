// Independent D3D12 WARP witness: real D24 + stencil planes -> production colour alias.
// The expected bytes follow the Xbox memory layout in native/environment_fog.h.
#undef NDEBUG
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <iterator>
#include <d3dcompiler.h>
#include <dxgi1_4.h>
#include "xbe_depth_alias.h"
using Microsoft::WRL::ComPtr;

// PRODUCTION_TEXEL_CONSTANTS

// Same zero-origin position/UV relation as retained 91C30, including fractional render scale.
static constexpr char kSampleHlsl[] = R"(
cbuffer B : register(b0) { float4 texsize; float4 scale; };
Texture2D image : register(t0);
SamplerState linear_clamp : register(s0);
float4 main(float4 p : SV_Position) : SV_Target {
    float2 tc = p.xy / scale.xy;
    return image.SampleLevel(linear_clamp, tc / texsize.xy + texsize.zw, 0);
}
)";

static void Check(HRESULT hr) {
    if (FAILED(hr)) {
        std::fprintf(stderr, "depth alias fixture HRESULT %08lx\n", (unsigned long)hr);
        std::abort();
    }
}

static ComPtr<ID3DBlob> Compile(const char* entry, const char* profile, const char* source = gpu::kDepthColorAliasHlsl) {
    ComPtr<ID3DBlob> code, error;
    HRESULT hr = D3DCompile(source, std::strlen(source),
        "production-depth-alias", nullptr, nullptr, entry, profile,
        D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &code, &error);
    if (FAILED(hr) && error) std::fwrite(error->GetBufferPointer(), 1, error->GetBufferSize(), stderr);
    Check(hr);
    return code;
}

static void Eligibility() {
    gpu::TextureDesc d;
    d.data = 0x83c6c000; d.format = 0x40; d.width = 640; d.height = 480;
    d.pitch = 2560; d.linear = true;
    gpu::Target t;
    t.data = d.data; t.depth = true; t.gw = 640; t.gh = 480; t.w = 1280; t.h = 960;
    assert(gpu::IsDepthColorAlias(d, t)); // render scale does not change guest alias identity
    for (unsigned field = 0; field != 10; ++field) {
        auto bad = d;
        auto target = t;
        switch (field) {
        case 0: bad.format = 40; break; // capture prints format in hexadecimal
        case 1: bad.linear = false; break;
        case 2: bad.cube = true; break;
        case 3: bad.depth = 2; break;
        case 4: bad.levels = 2; break;
        case 5: bad.pitch += 64; break;
        case 6: bad.data += 64; break;
        case 7: bad.width /= 2; break;
        case 8: target.gh /= 2; break;
        case 9: target.depth = false; break;
        }
        assert(!gpu::IsDepthColorAlias(bad, target));
    }
    auto z = gpu::DepthAliasPlane(false), s = gpu::DepthAliasPlane(true);
    assert(z.Format == DXGI_FORMAT_R24_UNORM_X8_TYPELESS && z.Texture2D.PlaneSlice == 0);
    assert(s.Format == DXGI_FORMAT_X24_TYPELESS_G8_UINT && s.Texture2D.PlaneSlice == 1);
}

static D3D12_RESOURCE_DESC Texture(DXGI_FORMAT format, unsigned w, unsigned h,
                                  D3D12_RESOURCE_FLAGS flags) {
    D3D12_RESOURCE_DESC d{};
    d.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    d.Width = w; d.Height = h; d.DepthOrArraySize = 1; d.MipLevels = 1;
    d.Format = format; d.SampleDesc.Count = 1; d.Flags = flags;
    return d;
}

int main() {
    Eligibility();
    ComPtr<IDXGIFactory4> factory;
    Check(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)));
    ComPtr<IDXGIAdapter> warp;
    Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp)));
    ComPtr<ID3D12Device> dev;
    Check(D3D12CreateDevice(warp.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&dev)));
    ComPtr<ID3D12CommandQueue> queue;
    D3D12_COMMAND_QUEUE_DESC qd{};
    Check(dev->CreateCommandQueue(&qd, IID_PPV_ARGS(&queue)));
    ComPtr<ID3D12CommandAllocator> allocator;
    Check(dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)));
    ComPtr<ID3D12GraphicsCommandList> list;
    Check(dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_PPV_ARGS(&list)));
    ComPtr<ID3D12Fence> fence;
    Check(dev->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
    HANDLE done = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    assert(done);

    constexpr unsigned w = 384, h = 3; // 256x2 guest pixels at the current run's 1.5 render scale
    D3D12_HEAP_PROPERTIES hp{D3D12_HEAP_TYPE_DEFAULT};
    auto zd = Texture(DXGI_FORMAT_R24G8_TYPELESS, w, h, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    auto cd = Texture(DXGI_FORMAT_B8G8R8A8_UNORM, w, h, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
    ComPtr<ID3D12Resource> depth, color, sampled;
    Check(dev->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &zd,
        D3D12_RESOURCE_STATE_DEPTH_WRITE, nullptr, IID_PPV_ARGS(&depth)));
    Check(dev->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &cd,
        D3D12_RESOURCE_STATE_RENDER_TARGET, nullptr, IID_PPV_ARGS(&color)));
    Check(dev->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &cd,
        D3D12_RESOURCE_STATE_RENDER_TARGET, nullptr, IID_PPV_ARGS(&sampled)));
    ComPtr<ID3D12DescriptorHeap> dsv, rtv, srv;
    D3D12_DESCRIPTOR_HEAP_DESC hd{};
    hd.NumDescriptors = 1; hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    Check(dev->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&dsv)));
    hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; hd.NumDescriptors = 2;
    Check(dev->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&rtv)));
    hd.NumDescriptors = 4; hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    hd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    Check(dev->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&srv)));
    auto dh = dsv->GetCPUDescriptorHandleForHeapStart();
    auto rh = rtv->GetCPUDescriptorHandleForHeapStart();
    D3D12_CPU_DESCRIPTOR_HANDLE sample_rh{rh.ptr + dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV)};
    D3D12_DEPTH_STENCIL_VIEW_DESC dv{};
    dv.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; dv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dev->CreateDepthStencilView(depth.Get(), &dv, dh);
    dev->CreateRenderTargetView(color.Get(), nullptr, rh);
    dev->CreateRenderTargetView(sampled.Get(), nullptr, sample_rh);
    auto sh = srv->GetCPUDescriptorHandleForHeapStart();
    auto inc = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    for (unsigned i = 0; i != 2; ++i) {
        auto view = gpu::DepthAliasPlane(i != 0);
        dev->CreateShaderResourceView(depth.Get(), &view, {sh.ptr + size_t(i) * inc});
    }
    D3D12_SHADER_RESOURCE_VIEW_DESC cv{};
    cv.Format = cd.Format; cv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    cv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; cv.Texture2D.MipLevels = 1;
    dev->CreateShaderResourceView(color.Get(), &cv, {sh.ptr + size_t(2) * inc});
    dev->CreateShaderResourceView(nullptr, &cv, {sh.ptr + size_t(3) * inc});

    D3D12_DESCRIPTOR_RANGE range{};
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; range.NumDescriptors = 2;
    D3D12_ROOT_PARAMETER params[2]{};
    params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[0].DescriptorTable = {1, &range}; params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    params[1].Constants.Num32BitValues = 8; params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    D3D12_STATIC_SAMPLER_DESC sampler{};
    sampler.Filter = D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS; sampler.MaxLOD = D3D12_FLOAT32_MAX;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    D3D12_ROOT_SIGNATURE_DESC root_desc{2, params, 1, &sampler};
    ComPtr<ID3DBlob> serialized, errors;
    Check(D3D12SerializeRootSignature(&root_desc, D3D_ROOT_SIGNATURE_VERSION_1, &serialized, &errors));
    ComPtr<ID3D12RootSignature> root;
    Check(dev->CreateRootSignature(0, serialized->GetBufferPointer(), serialized->GetBufferSize(), IID_PPV_ARGS(&root)));
    auto vs = Compile("vs", "vs_5_0"), ps = Compile("psmain", "ps_5_0");
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline{};
    pipeline.pRootSignature = root.Get();
    pipeline.VS = {vs->GetBufferPointer(), vs->GetBufferSize()};
    pipeline.PS = {ps->GetBufferPointer(), ps->GetBufferSize()};
    pipeline.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    pipeline.SampleMask = UINT_MAX; pipeline.SampleDesc.Count = 1;
    pipeline.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    pipeline.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    pipeline.RasterizerState.DepthClipEnable = TRUE;
    pipeline.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipeline.NumRenderTargets = 1; pipeline.RTVFormats[0] = cd.Format;
    ComPtr<ID3D12PipelineState> pso;
    Check(dev->CreateGraphicsPipelineState(&pipeline, IID_PPV_ARGS(&pso)));
    auto sample_ps = Compile("main", "ps_5_0", kSampleHlsl);
    pipeline.PS = {sample_ps->GetBufferPointer(), sample_ps->GetBufferSize()};
    ComPtr<ID3D12PipelineState> sample_pso;
    Check(dev->CreateGraphicsPipelineState(&pipeline, IID_PPV_ARGS(&sample_pso)));

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp;
    UINT rows; UINT64 row_bytes, size;
    dev->GetCopyableFootprints(&cd, 0, 1, 0, &fp, &rows, &row_bytes, &size);
    D3D12_RESOURCE_DESC bd{};
    bd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; bd.Width = size; bd.Height = 1;
    bd.DepthOrArraySize = 1; bd.MipLevels = 1; bd.SampleDesc.Count = 1;
    bd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    D3D12_HEAP_PROPERTIES rbp{D3D12_HEAP_TYPE_READBACK};
    ComPtr<ID3D12Resource> readback;
    Check(dev->CreateCommittedResource(&rbp, D3D12_HEAP_FLAG_NONE, &bd,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&readback)));
    auto barrier = [&](ID3D12Resource* resource, D3D12_RESOURCE_STATES from, D3D12_RESOURCE_STATES to) {
        D3D12_RESOURCE_BARRIER b{};
        b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition = {resource, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, from, to};
        list->ResourceBarrier(1, &b);
    };
    constexpr uint32_t depths[] = {0, 1, 0x7f, 0x80, 0xff, 0x100, 0x101, 0x7fff, 0x8000,
        0xffff, 0x10000, 0x7fffff, 0x800000, 0xfeffff, 0xffff00, 0xfffffe, 0xffffff};
    unsigned wrong_sample_pixels = 0;
    for (unsigned pass = 0; pass != 3; ++pass) {
        if (pass) {
            Check(allocator->Reset());
            Check(list->Reset(allocator.Get(), nullptr));
            barrier(depth.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
            barrier(color.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
            barrier(sampled.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        }
        for (unsigned x = 0; x != w; ++x) {
            uint32_t z = depths[(x + pass) % std::size(depths)];
            D3D12_RECT r{LONG(x), 0, LONG(x + 1), h};
            list->ClearDepthStencilView(dh, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
                float(z) / 16777215.0f, UINT8(x ^ (pass * 255)), 1, &r);
        }
        barrier(depth.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        list->SetGraphicsRootSignature(root.Get());
        ID3D12DescriptorHeap* heaps[] = {srv.Get()};
        list->SetDescriptorHeaps(1, heaps);
        list->SetGraphicsRootDescriptorTable(0, srv->GetGPUDescriptorHandleForHeapStart());
        list->SetPipelineState(pso.Get());
        list->OMSetRenderTargets(1, &rh, FALSE, nullptr);
        D3D12_VIEWPORT vp{0, 0, float(w), float(h), 0, 1};
        D3D12_RECT scissor{0, 0, w, h};
        list->RSSetViewports(1, &vp); list->RSSetScissorRects(1, &scissor);
        list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        list->DrawInstanced(3, 1, 0, 0);
        barrier(color.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        gpu::TextureDesc texture; texture.width = 256; texture.height = 2;
        gpu::fixture::DrawTextures stages{};
        gpu::fixture::SetAliasMetadata(0, stages);
        assert(stages.rt[0] && stages.aligned_depth_alias[0]);
        if (pass == 2) stages.aligned_depth_alias[0] = false; // negative control: former generic RT adjustment
        gpu::DrawConstants constants{};
        gpu::fixture::FillTexelConstants(0, texture, stages, constants);
        if (pass != 2) assert(constants.texsize[0][2] == 0 && constants.texsize[0][3] == 0);
        else assert(constants.texsize[0][2] == -.5f / 256 && constants.texsize[0][3] == -.5f / 2);
        float cb[8]{};
        std::memcpy(cb, constants.texsize[0], 16); cb[4] = cb[5] = 1.5f;
        list->SetGraphicsRoot32BitConstants(1, 8, cb, 0);
        auto sample_srv = srv->GetGPUDescriptorHandleForHeapStart(); sample_srv.ptr += 2 * inc;
        list->SetGraphicsRootDescriptorTable(0, sample_srv);
        list->SetPipelineState(sample_pso.Get());
        list->OMSetRenderTargets(1, &sample_rh, FALSE, nullptr);
        list->DrawInstanced(3, 1, 0, 0);
        barrier(sampled.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
        D3D12_TEXTURE_COPY_LOCATION src{sampled.Get(), D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
        D3D12_TEXTURE_COPY_LOCATION dst{readback.Get(), D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};
        dst.PlacedFootprint = fp;
        list->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
        Check(list->Close());
        ID3D12CommandList* commands[] = {list.Get()};
        queue->ExecuteCommandLists(1, commands);
        Check(queue->Signal(fence.Get(), pass + 1));
        Check(fence->SetEventOnCompletion(pass + 1, done));
        assert(WaitForSingleObject(done, 30000) == WAIT_OBJECT_0);
        uint8_t* bytes = nullptr;
        D3D12_RANGE read{0, size_t(size)};
        Check(readback->Map(0, &read, reinterpret_cast<void**>(&bytes)));
        for (unsigned y = 0; y != h; ++y) for (unsigned x = 0; x != w; ++x) {
            uint32_t z = depths[(x + pass) % std::size(depths)];
            const uint8_t expected[] = {uint8_t(z >> 16), uint8_t(z >> 8), uint8_t(z), UINT8(x ^ (pass * 255))};
            const uint8_t* actual = bytes + y * fp.Footprint.RowPitch + x * 4;
            if (std::memcmp(actual, expected, 4)) {
                if (pass == 2) { ++wrong_sample_pixels; continue; }
                std::fprintf(stderr, "pass %u x%u z%06x: got %02x %02x %02x %02x expected %02x %02x %02x %02x\n",
                    pass, x, z, actual[0], actual[1], actual[2], actual[3],
                    expected[0], expected[1], expected[2], expected[3]);
                std::abort();
            }
        }
        D3D12_RANGE none{};
        readback->Unmap(0, &none);
    }
    assert(wrong_sample_pixels >= w); // this fixture catches the packed-byte filtering regression
    CloseHandle(done);
    std::printf("depth alias: both planes, mutation, actual texel metadata and 1.5x linear sampling passed; old offset corrupts %u pixels\n", wrong_sample_pixels);
}

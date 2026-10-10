#include "renderer.h"
#include "../texture_asset/texture_asset.h"
#include <d3dcompiler.h>
#include <climits>
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <utility>

static void check(HRESULT result) {
    if (FAILED(result)) {
        char text[64]; sprintf_s(text, "Native graphics error 0x%08lX", (unsigned long)result);
        throw std::runtime_error(text);
    }
}
static ComPtr<ID3DBlob> shader(const char *source, const char *entry, const char *profile) {
    ComPtr<ID3DBlob> code, error;
    HRESULT result = D3DCompile(source, strlen(source), "native scene", nullptr, nullptr,
        entry, profile, D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &code, &error);
    if (FAILED(result) && error) throw std::runtime_error((const char *)error->GetBufferPointer());
    check(result); return code;
}
void NativeRenderer::open(HWND window, unsigned w, unsigned h, bool software) {
    check(D3D11CreateDevice(nullptr, software ? D3D_DRIVER_TYPE_WARP : D3D_DRIVER_TYPE_HARDWARE,
        nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &device, nullptr, &context));
    if (window) {
        ComPtr<IDXGIDevice> dxgi; ComPtr<IDXGIAdapter> adapter; ComPtr<IDXGIFactory> factory;
        check(device.As(&dxgi)); check(dxgi->GetAdapter(&adapter));
        check(adapter->GetParent(IID_PPV_ARGS(&factory)));
        DXGI_SWAP_CHAIN_DESC desc{}; desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.BufferDesc.Width = w; desc.BufferDesc.Height = h; desc.SampleDesc.Count = 1;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; desc.BufferCount = 2;
        desc.OutputWindow = window; desc.Windowed = TRUE; desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        check(factory->CreateSwapChain(device.Get(), &desc, &swapchain));
        check(factory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER));
    }
    initialize_draw_resources();
    resize(w, h);
}
void NativeRenderer::initialize_draw_resources() {
    const char *source = R"(
cbuffer Frame : register(b0) { row_major float4x4 view_projection; float4 tint;
    uint textured; uint constant_color; uint vertex_exposure; float inverse_exposure; float4 constant_rgba;
    uint vertex_alpha; float vertex_alpha_scale; float2 uv_offset;
    float2 rotor_cs; uint rotor_uv; uint unused; };
Texture2D image : register(t0); SamplerState image_sampler : register(s0);
struct Vertex { float3 position : POSITION; float2 uv : TEXCOORD; float4 color : COLOR; };
struct Pixel { float4 position : SV_Position; float2 uv : TEXCOORD; float4 color : COLOR; };
Pixel VS(Vertex v) { Pixel p; p.position = mul(float4(v.position, 1), view_projection); p.uv = textured==2 ? 0 : v.uv + uv_offset;
    if(rotor_uv) { precise float2 q=v.uv-.5; precise float2 spun=float2(q.x*rotor_cs.x+q.y*rotor_cs.y,q.x*-rotor_cs.y+q.y*rotor_cs.x); p.uv=spun+.5; }
    precise float4 color = v.color.bgra;
    if(vertex_exposure)color *= float4(inverse_exposure.xxx,1);
    p.color = textured>=3 ? 1 : textured ? floor(saturate(color * tint) * 255 + 0.5) / 255 : tint; return p; }
float4 PS(Pixel p) : SV_Target {
    if (!textured) return p.color;
    if (textured==2) return saturate(p.color * constant_rgba);
    float4 texel = image.Sample(image_sampler, p.uv);
    if (textured>=3) {float4 result=saturate(texel*constant_rgba);if(textured==3&&result.a<127.0/255.0)discard;return result;}
    if (constant_color) texel = saturate(texel * constant_rgba);
    float4 result=saturate(p.color * texel);
    if(vertex_alpha)result.a=p.color.a*vertex_alpha_scale;
    return result;
}
)";
    auto vs = shader(source, "VS", "vs_5_0"), ps = shader(source, "PS", "ps_5_0");
    check(device->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), nullptr, &vertex_shader));
    check(device->CreatePixelShader(ps->GetBufferPointer(), ps->GetBufferSize(), nullptr, &pixel_shader));
    const char *fog_source=R"(
cbuffer Frame : register(b0) { row_major float4x4 unused_matrix; float4 fog_color; };
Texture2D<float> scene_depth : register(t0); Texture2D<float> fog_alpha : register(t1);
float4 VS(uint id : SV_VertexID) : SV_Position {
    float2 uv=float2((id<<1)&2,id&2);return float4(uv*float2(2,-2)+float2(-1,1),0,1);
}
float4 PS(float4 position : SV_Position) : SV_Target {
    uint depth24=(uint)round(scene_depth.Load(int3((int2)position.xy,0))*16777215.0);
    // Original dependent GB point/clamp lookup: the high two Z24 bytes.
    float alpha=fog_alpha.Load(int3((depth24>>8)&255,depth24>>16,0));
    return float4(saturate(fog_color.rgb),alpha);
}
)";
    auto fog_vs=shader(fog_source,"VS","vs_5_0"),fog_ps=shader(fog_source,"PS","ps_5_0");
    check(device->CreateVertexShader(fog_vs->GetBufferPointer(),fog_vs->GetBufferSize(),nullptr,&fog_vertex_shader));
    check(device->CreatePixelShader(fog_ps->GetBufferPointer(),fog_ps->GetBufferSize(),nullptr,&fog_pixel_shader));
    const char *tone_source_code=R"(
Texture2D<float4> scene : register(t0); Texture2D<float4> curves : register(t1);
float4 PS(float4 position : SV_Position) : SV_Target {
    uint3 index=(uint3)round(scene.Load(int3((int2)position.xy,0)).rgb*255.0);
    return float4(curves.Load(int3(index.r,0,0)).r,
                  curves.Load(int3(index.g,0,0)).g,
                  curves.Load(int3(index.b,0,0)).b,0);
}
)";
    auto tone_ps=shader(tone_source_code,"PS","ps_5_0");
    check(device->CreatePixelShader(tone_ps->GetBufferPointer(),tone_ps->GetBufferSize(),nullptr,&tone_pixel_shader));
    D3D11_INPUT_ELEMENT_DESC elements[] = {{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
        {"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0},
        {"COLOR",0,DXGI_FORMAT_R8G8B8A8_UNORM,0,20,D3D11_INPUT_PER_VERTEX_DATA,0}};
    check(device->CreateInputLayout(elements, 3, vs->GetBufferPointer(), vs->GetBufferSize(), &layout));
    D3D11_BUFFER_DESC cb{}; cb.ByteWidth = 144; cb.Usage = D3D11_USAGE_DYNAMIC;
    cb.BindFlags = D3D11_BIND_CONSTANT_BUFFER; cb.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    check(device->CreateBuffer(&cb, nullptr, &constants));
    D3D11_RASTERIZER_DESC rs{}; rs.FillMode = D3D11_FILL_SOLID;
    rs.CullMode = D3D11_CULL_NONE; rs.DepthClipEnable = TRUE; rs.MultisampleEnable = TRUE;
    check(device->CreateRasterizerState(&rs, &rasterizer));
    /* Original material selector1: polygon offset factor -0.25, units -1.
     * D24 keeps the constant bias in the same 24-bit depth units. */
    rs.SlopeScaledDepthBias=-.25f;rs.DepthBias=-1;
    check(device->CreateRasterizerState(&rs,&biased_rasterizer));
    rs.SlopeScaledDepthBias=-3.5f;rs.DepthBias=-14;
    check(device->CreateRasterizerState(&rs,&glow_rasterizer));
    rs.SlopeScaledDepthBias=-3.75f;rs.DepthBias=-15;
    check(device->CreateRasterizerState(&rs,&decal_rasterizer));
    // GEO front faces are counterclockwise after the native world projection.
    rs.SlopeScaledDepthBias=0;rs.DepthBias=0;rs.CullMode=D3D11_CULL_BACK;rs.FrontCounterClockwise=TRUE;
    check(device->CreateRasterizerState(&rs,&prepass_rasterizer));
    D3D11_BLEND_DESC blend{};auto &rt=blend.RenderTarget[0];
    rt.BlendOp=rt.BlendOpAlpha=D3D11_BLEND_OP_ADD;
    for(unsigned mode=0;mode<5;++mode)for(unsigned alpha=0;alpha<2;++alpha) {
        rt.BlendEnable=mode!=0;
        rt.BlendOp=rt.BlendOpAlpha=mode==4?D3D11_BLEND_OP_REV_SUBTRACT:D3D11_BLEND_OP_ADD;
        rt.SrcBlend=mode==3?D3D11_BLEND_DEST_COLOR:D3D11_BLEND_SRC_ALPHA;
        rt.SrcBlendAlpha=mode==3?D3D11_BLEND_DEST_ALPHA:D3D11_BLEND_SRC_ALPHA;
        rt.DestBlend=rt.DestBlendAlpha=mode==3?D3D11_BLEND_ZERO:(mode==2||mode==4)?D3D11_BLEND_ONE:D3D11_BLEND_INV_SRC_ALPHA;
        rt.RenderTargetWriteMask=(UINT8)(7|(alpha?8:0));
        check(device->CreateBlendState(&blend,&blends[mode][alpha]));
    }
    D3D11_DEPTH_STENCIL_DESC transparent{};transparent.DepthEnable=TRUE;
    transparent.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ZERO;transparent.DepthFunc=D3D11_COMPARISON_LESS_EQUAL;
    check(device->CreateDepthStencilState(&transparent,&transparent_depth));
    transparent.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL;
    check(device->CreateDepthStencilState(&transparent,&prepass_depth));
    transparent.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ZERO;transparent.DepthEnable=FALSE;
    check(device->CreateDepthStencilState(&transparent,&fog_depth));
    set_anisotropy(anisotropy_);
    D3D11_SAMPLER_DESC ss{};ss.MaxAnisotropy=1;
    ss.Filter=D3D11_FILTER_MIN_MAG_MIP_POINT;ss.MaxLOD=0;
    ss.AddressU=ss.AddressV=ss.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;
    check(device->CreateSamplerState(&ss,&point_sampler));
}
NativeRenderer::SceneTargets NativeRenderer::make_targets(unsigned w,unsigned h,unsigned samples,bool with_color) const {
    SceneTargets result;
    D3D11_TEXTURE2D_DESC desc{}; desc.Width = w; desc.Height = h; desc.MipLevels = desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; desc.SampleDesc.Count = samples; desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    if(with_color) {
        check(device->CreateTexture2D(&desc,nullptr,&result.color));
        check(device->CreateRenderTargetView(result.color.Get(),nullptr,&result.target));
    }
    desc.Format = DXGI_FORMAT_R24G8_TYPELESS; desc.BindFlags = D3D11_BIND_DEPTH_STENCIL|D3D11_BIND_SHADER_RESOURCE;
    check(device->CreateTexture2D(&desc, nullptr, &result.depth));
    D3D11_DEPTH_STENCIL_VIEW_DESC dsv{};dsv.Format=DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsv.ViewDimension=samples>1?D3D11_DSV_DIMENSION_TEXTURE2DMS:D3D11_DSV_DIMENSION_TEXTURE2D;
    check(device->CreateDepthStencilView(result.depth.Get(),&dsv,&result.depth_view));
    D3D11_SHADER_RESOURCE_VIEW_DESC srv{};srv.Format=DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    srv.ViewDimension=samples>1?D3D11_SRV_DIMENSION_TEXTURE2DMS:D3D11_SRV_DIMENSION_TEXTURE2D;
    if(samples==1)srv.Texture2D.MipLevels=1;
    check(device->CreateShaderResourceView(result.depth.Get(),&srv,&result.depth_resource));return result;
}
void NativeRenderer::resize(unsigned w, unsigned h) {
    if (!w || !h || w > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION || h > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION)
        throw std::invalid_argument("Invalid native render size");
    if(w==width&&h==height)return;
    /* Allocate owned resources first, so allocation failure leaves the current
     * display usable. DXGI requires releasing only its backbuffer before resize. */
    auto next=make_targets(w,h,1,!swapchain);SceneTargets next_msaa;
    if(msaa_samples_>1)next_msaa=make_targets(w,h,msaa_samples_,true);
    resolve_scene();
    context->OMSetRenderTargets(0,nullptr,nullptr);
    if(swapchain) {
        target.Reset();single_target_.Reset();color.Reset();
        HRESULT resized=swapchain->ResizeBuffers(0,w,h,DXGI_FORMAT_UNKNOWN,0);
        try {
            check(resized);
            check(swapchain->GetBuffer(0,IID_PPV_ARGS(&next.color)));
            check(device->CreateRenderTargetView(next.color.Get(),nullptr,&next.target));
        } catch(...) {
            next.target.Reset();next.color.Reset();
            if(SUCCEEDED(resized)&&width&&height)
                check(swapchain->ResizeBuffers(0,width,height,DXGI_FORMAT_UNKNOWN,0));
            check(swapchain->GetBuffer(0,IID_PPV_ARGS(&color)));
            check(device->CreateRenderTargetView(color.Get(),nullptr,&single_target_));
            target=single_target_;ID3D11RenderTargetView *rt=target.Get();
            context->OMSetRenderTargets(1,&rt,depth_view.Get());
            throw;
        }
    }
    color=std::move(next.color);depth=std::move(next.depth);
    single_target_=std::move(next.target);single_depth_view_=std::move(next.depth_view);
    depth_resource=std::move(next.depth_resource);msaa_=std::move(next_msaa);
    target=single_target_;depth_view=single_depth_view_;
    tone_source.Reset();tone_snapshot.Reset();width=w;height=h;
}
bool NativeRenderer::supports_msaa(unsigned samples) const {
    if(!device||(samples!=1&&samples!=2&&samples!=4&&samples!=8))return false;
    if(samples==1)return true;
    UINT color_levels=0,depth_levels=0,color_support=0,depth_support=0;
    return SUCCEEDED(device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM,samples,&color_levels))&&color_levels&&
        SUCCEEDED(device->CheckMultisampleQualityLevels(DXGI_FORMAT_D24_UNORM_S8_UINT,samples,&depth_levels))&&depth_levels&&
        SUCCEEDED(device->CheckFormatSupport(DXGI_FORMAT_R8G8B8A8_UNORM,&color_support))&&
        (color_support&D3D11_FORMAT_SUPPORT_MULTISAMPLE_RESOLVE)&&
        SUCCEEDED(device->CheckFormatSupport(DXGI_FORMAT_R24_UNORM_X8_TYPELESS,&depth_support))&&
        (depth_support&D3D11_FORMAT_SUPPORT_MULTISAMPLE_LOAD);
}
void NativeRenderer::prepare_msaa_shaders() {
    if(msaa_fog_shader_)return;
    const char *source=R"(
cbuffer Frame : register(b0) { row_major float4x4 unused_matrix; float4 fog_color; };
Texture2DMS<float> scene_depth : register(t0); Texture2D<float> fog_alpha : register(t1);
float4 Fog(float4 position : SV_Position, uint sample_index : SV_SampleIndex) : SV_Target {
    uint depth24=(uint)round(scene_depth.Load((int2)position.xy,sample_index)*16777215.0);
    float alpha=fog_alpha.Load(int3((depth24>>8)&255,depth24>>16,0));
    return float4(saturate(fog_color.rgb),alpha);
}
float Depth(float4 position : SV_Position) : SV_Depth {
    uint width,height,samples;scene_depth.GetDimensions(width,height,samples);
    float result=1;
    for(uint i=0;i<samples;++i)result=min(result,scene_depth.Load((int2)position.xy,i));
    return result;
}
)";
    auto fog_code=shader(source,"Fog","ps_5_0"),depth_code=shader(source,"Depth","ps_5_0");
    ComPtr<ID3D11PixelShader> fog_shader,depth_shader;ComPtr<ID3D11DepthStencilState> depth_state;
    check(device->CreatePixelShader(fog_code->GetBufferPointer(),fog_code->GetBufferSize(),nullptr,&fog_shader));
    check(device->CreatePixelShader(depth_code->GetBufferPointer(),depth_code->GetBufferSize(),nullptr,&depth_shader));
    D3D11_DEPTH_STENCIL_DESC desc{};desc.DepthEnable=TRUE;
    desc.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL;desc.DepthFunc=D3D11_COMPARISON_ALWAYS;
    check(device->CreateDepthStencilState(&desc,&depth_state));
    msaa_fog_shader_=std::move(fog_shader);resolve_depth_shader_=std::move(depth_shader);resolve_depth_state_=std::move(depth_state);
}
void NativeRenderer::set_msaa(unsigned samples) {
    if(!supports_msaa(samples))throw std::invalid_argument("This graphics device does not support the selected MSAA mode");
    if(samples==msaa_samples_)return;
    SceneTargets next;
    if(samples>1){prepare_msaa_shaders();next=make_targets(width,height,samples,true);}
    resolve_scene();msaa_=std::move(next);msaa_samples_=samples;
}
void NativeRenderer::attach(ID3D11Device *borrowed_device,ID3D11DeviceContext *borrowed_context,bool draw_resources) {
    if(device||context||!borrowed_device||!borrowed_context||borrowed_context->GetType()!=D3D11_DEVICE_CONTEXT_IMMEDIATE)
        throw std::invalid_argument("Invalid borrowed native renderer");
    ComPtr<ID3D11Device> owner;borrowed_context->GetDevice(&owner);
    if(owner.Get()!=borrowed_device)throw std::invalid_argument("Borrowed context belongs to another device");
    device=borrowed_device;context=borrowed_context;
    if(draw_resources){initialize_draw_resources();return;}
    set_anisotropy(1);
    D3D11_SAMPLER_DESC ss{};ss.MaxAnisotropy=1;ss.Filter=D3D11_FILTER_MIN_MAG_MIP_POINT;
    ss.AddressU=ss.AddressV=ss.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;
    check(device->CreateSamplerState(&ss,&point_sampler));
}
void NativeRenderer::set_anisotropy(unsigned level) {
    if(level!=1&&level!=2&&level!=4&&level!=8&&level!=16)
        throw std::invalid_argument("Invalid anisotropic filtering level");
    D3D11_SAMPLER_DESC desc{};
    desc.Filter=level>1?D3D11_FILTER_ANISOTROPIC:D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    desc.AddressW=D3D11_TEXTURE_ADDRESS_WRAP;desc.MaxLOD=D3D11_FLOAT32_MAX;desc.MaxAnisotropy=level;
    ComPtr<ID3D11SamplerState> next[4];
    for(unsigned i=0;i<4;++i) {
        desc.AddressU=(i&1)?D3D11_TEXTURE_ADDRESS_CLAMP:D3D11_TEXTURE_ADDRESS_WRAP;
        desc.AddressV=(i&2)?D3D11_TEXTURE_ADDRESS_CLAMP:D3D11_TEXTURE_ADDRESS_WRAP;
        check(device->CreateSamplerState(&desc,&next[i]));
    }
    for(unsigned i=0;i<4;++i)samplers[i]=std::move(next[i]);
    anisotropy_=level;
}
NativeGpuMesh NativeRenderer::upload(const NativeVertex *v, size_t nv, const uint32_t *ix, size_t ni,bool dynamic) {
    if (!v || !ix || !nv || !ni || ni % 3 || nv > UINT_MAX / sizeof(*v) || ni > UINT_MAX / sizeof(*ix))
        throw std::invalid_argument("Invalid native mesh extent");
    for (size_t i = 0; i < ni; ++i) if (ix[i] >= nv) throw std::invalid_argument("Native mesh index outside vertices");
    NativeGpuMesh mesh; mesh.vertex_count=(uint32_t)nv;mesh.index_count=(uint32_t)ni;mesh.dynamic=dynamic;
    D3D11_BUFFER_DESC desc{}; desc.ByteWidth = (UINT)(nv * sizeof(*v));
    desc.Usage=dynamic?D3D11_USAGE_DYNAMIC:D3D11_USAGE_IMMUTABLE;
    desc.CPUAccessFlags=dynamic?D3D11_CPU_ACCESS_WRITE:0;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER; D3D11_SUBRESOURCE_DATA data{v,0,0};
    check(device->CreateBuffer(&desc, &data, &mesh.vertices));
    desc.ByteWidth = (UINT)(ni * sizeof(*ix)); desc.BindFlags = D3D11_BIND_INDEX_BUFFER; data.pSysMem = ix;
    desc.Usage=D3D11_USAGE_IMMUTABLE;desc.CPUAccessFlags=0;
    check(device->CreateBuffer(&desc, &data, &mesh.indices)); return mesh;
}
void NativeRenderer::update(const NativeGpuMesh &mesh,const void *vertices,size_t count) {
    if(!mesh.dynamic||!mesh.vertices||!vertices||!count||count!=mesh.vertex_count)
        throw std::invalid_argument("Invalid native dynamic mesh update");
    D3D11_MAPPED_SUBRESOURCE mapped{};
    check(context->Map(mesh.vertices.Get(),0,D3D11_MAP_WRITE_DISCARD,0,&mapped));
    memcpy(mapped.pData,vertices,count*sizeof(NativeVertex));context->Unmap(mesh.vertices.Get(),0);
}
ComPtr<ID3D11ShaderResourceView> NativeRenderer::upload(const UCTextureAsset &texture) {
    DXGI_FORMAT formats[]={DXGI_FORMAT_UNKNOWN,DXGI_FORMAT_BC1_UNORM,DXGI_FORMAT_BC2_UNORM,
        DXGI_FORMAT_BC3_UNORM,DXGI_FORMAT_B8G8R8A8_UNORM};
    if(texture.format<UC_TEXTURE_BC1_UNORM||texture.format>UC_TEXTURE_BGRA8_UNORM||
       !texture.width||!texture.height||texture.width>16384||texture.height>16384||
       !texture.mip_count||texture.mip_count>UC_TEXTURE_MAX_MIPS||!texture.storage)
        throw std::invalid_argument("Invalid native texture");
    D3D11_SUBRESOURCE_DATA data[UC_TEXTURE_MAX_MIPS]{};
    unsigned w=texture.width,h=texture.height;
    for(unsigned i=0;i<texture.mip_count;++i) {
        const auto &mip=texture.mips[i]; auto start=(uintptr_t)texture.storage,ptr=(uintptr_t)mip.data;
        unsigned pitch=texture.format==UC_TEXTURE_BGRA8_UNORM?w*4:((w+3)/4)*(texture.format==UC_TEXTURE_BC1_UNORM?8:16);
        unsigned rows=texture.format==UC_TEXTURE_BGRA8_UNORM?h:(h+3)/4;
        if(mip.width!=w||mip.height!=h||mip.row_pitch!=pitch||mip.slice_pitch!=pitch*rows||
           ptr<start||ptr-start>texture.storage_bytes||mip.slice_pitch>texture.storage_bytes-(ptr-start))
            throw std::invalid_argument("Native mip outside owned storage");
        data[i]={mip.data,mip.row_pitch,mip.slice_pitch};w=w>1?w/2:1;h=h>1?h/2:1;
    }
    D3D11_TEXTURE2D_DESC desc{};desc.Width=texture.width;desc.Height=texture.height;desc.MipLevels=texture.mip_count;
    desc.ArraySize=1;desc.Format=formats[texture.format];desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_IMMUTABLE;
    desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;ComPtr<ID3D11Texture2D> resource;ComPtr<ID3D11ShaderResourceView> view;
    check(device->CreateTexture2D(&desc,data,&resource));check(device->CreateShaderResourceView(resource.Get(),nullptr,&view));return view;
}
void NativeRenderer::begin(const float clear[4]) {
    msaa_active_=msaa_samples_>1;
    target=msaa_active_?msaa_.target:single_target_;
    depth_view=msaa_active_?msaa_.depth_view:single_depth_view_;
    if(!color||!single_target_||!target||!depth_view)
        throw std::runtime_error("Native render targets unavailable after display change");
    context->ClearRenderTargetView(target.Get(), clear); context->ClearDepthStencilView(depth_view.Get(), D3D11_CLEAR_DEPTH, 1, 0);
    ID3D11RenderTargetView *rt = target.Get(); context->OMSetRenderTargets(1, &rt, depth_view.Get());
    D3D11_VIEWPORT viewport{0,0,(float)width,(float)height,0,1}; context->RSSetViewports(1,&viewport);
    context->RSSetState(rasterizer.Get()); context->IASetInputLayout(layout.Get());
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(vertex_shader.Get(),nullptr,0); context->PSSetShader(pixel_shader.Get(),nullptr,0);
    ID3D11Buffer *cb = constants.Get(); context->VSSetConstantBuffers(0,1,&cb); context->PSSetConstantBuffers(0,1,&cb);
    ID3D11SamplerState *ss = samplers[0].Get(); context->PSSetSamplers(0,1,&ss);
}
void NativeRenderer::resolve_scene() {
    if(!msaa_active_)return;
    ID3D11ShaderResourceView *none=nullptr;context->PSSetShaderResources(0,1,&none);
    context->OMSetRenderTargets(0,nullptr,nullptr);
    context->ResolveSubresource(color.Get(),0,msaa_.color.Get(),0,DXGI_FORMAT_R8G8B8A8_UNORM);
    /* D3D11 has no depth ResolveSubresource. Keep the nearest covered sample
     * for single-sample depth effects and later transparent geometry. */
    context->OMSetRenderTargets(0,nullptr,single_depth_view_.Get());
    context->OMSetDepthStencilState(resolve_depth_state_.Get(),0);
    context->OMSetBlendState(nullptr,nullptr,0xffffffffu);
    D3D11_VIEWPORT viewport{0,0,(float)width,(float)height,0,1};context->RSSetViewports(1,&viewport);
    context->RSSetState(rasterizer.Get());context->IASetInputLayout(nullptr);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(fog_vertex_shader.Get(),nullptr,0);context->PSSetShader(resolve_depth_shader_.Get(),nullptr,0);
    ID3D11ShaderResourceView *source=msaa_.depth_resource.Get();context->PSSetShaderResources(0,1,&source);
    context->Draw(3,0);context->PSSetShaderResources(0,1,&none);
    msaa_active_=false;target=single_target_;depth_view=single_depth_view_;
    ID3D11RenderTargetView *rt=target.Get();context->OMSetRenderTargets(1,&rt,depth_view.Get());
    context->OMSetDepthStencilState(nullptr,0);context->IASetInputLayout(layout.Get());
    context->VSSetShader(vertex_shader.Get(),nullptr,0);context->PSSetShader(pixel_shader.Get(),nullptr,0);
    ID3D11Buffer *cb=constants.Get();context->VSSetConstantBuffers(0,1,&cb);context->PSSetConstantBuffers(0,1,&cb);
}
ComPtr<ID3D11ShaderResourceView> NativeRenderer::upload_fog(const uint8_t *alpha,size_t count) {
    if(!alpha||count!=65536)throw std::invalid_argument("Invalid native fog lookup");
    D3D11_TEXTURE2D_DESC desc{};desc.Width=desc.Height=256;desc.MipLevels=desc.ArraySize=1;
    desc.Format=DXGI_FORMAT_R8_UNORM;desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_IMMUTABLE;
    desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;D3D11_SUBRESOURCE_DATA data{alpha,256,65536};
    ComPtr<ID3D11Texture2D> texture;ComPtr<ID3D11ShaderResourceView> view;
    check(device->CreateTexture2D(&desc,&data,&texture));
    check(device->CreateShaderResourceView(texture.Get(),nullptr,&view));return view;
}
ComPtr<ID3D11ShaderResourceView> NativeRenderer::upload_tone(const uint8_t *rgb,size_t count) {
    if(!rgb||count!=768)throw std::invalid_argument("Invalid native tone lookup");
    uint8_t rgba[1024];
    for(unsigned i=0;i<256;++i){rgba[4*i]=rgb[i];rgba[4*i+1]=rgb[256+i];rgba[4*i+2]=rgb[512+i];rgba[4*i+3]=0;}
    D3D11_TEXTURE2D_DESC desc{};desc.Width=256;desc.Height=desc.MipLevels=desc.ArraySize=1;
    desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_IMMUTABLE;
    desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;D3D11_SUBRESOURCE_DATA data{rgba,sizeof(rgba),sizeof(rgba)};
    ComPtr<ID3D11Texture2D> texture;ComPtr<ID3D11ShaderResourceView> view;
    check(device->CreateTexture2D(&desc,&data,&texture));
    check(device->CreateShaderResourceView(texture.Get(),nullptr,&view));return view;
}
void NativeRenderer::tone(ID3D11ShaderResourceView *lookup) {
    resolve_scene();
    if(!lookup)throw std::invalid_argument("Invalid native tone resource");
    if(!tone_snapshot) {
        D3D11_TEXTURE2D_DESC desc{};color->GetDesc(&desc);desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
        ComPtr<ID3D11Texture2D> snapshot;ComPtr<ID3D11ShaderResourceView> source;
        check(device->CreateTexture2D(&desc,nullptr,&snapshot));
        check(device->CreateShaderResourceView(snapshot.Get(),nullptr,&source));
        tone_snapshot=snapshot;tone_source=source;
    }
    ID3D11ShaderResourceView *resources[]={nullptr,nullptr};context->PSSetShaderResources(0,2,resources);
    context->OMSetRenderTargets(0,nullptr,nullptr);context->CopyResource(tone_snapshot.Get(),color.Get());
    ID3D11RenderTargetView *rt=target.Get();context->OMSetRenderTargets(1,&rt,nullptr);
    context->OMSetDepthStencilState(fog_depth.Get(),0);context->OMSetBlendState(blends[0][1].Get(),nullptr,0xffffffffu);
    D3D11_VIEWPORT viewport{0,0,(float)width,(float)height,0,1};context->RSSetViewports(1,&viewport);
    context->RSSetState(rasterizer.Get());context->IASetInputLayout(nullptr);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(fog_vertex_shader.Get(),nullptr,0);context->PSSetShader(tone_pixel_shader.Get(),nullptr,0);
    resources[0]=tone_source.Get();resources[1]=lookup;context->PSSetShaderResources(0,2,resources);
    context->Draw(3,0);
    resources[0]=resources[1]=nullptr;context->PSSetShaderResources(0,2,resources);
    context->OMSetRenderTargets(1,&rt,depth_view.Get());context->IASetInputLayout(layout.Get());
    context->VSSetShader(vertex_shader.Get(),nullptr,0);context->PSSetShader(pixel_shader.Get(),nullptr,0);
}
void NativeRenderer::fog(ID3D11ShaderResourceView *lookup,const float rgba[4]) {
    if(!lookup||!rgba)throw std::invalid_argument("Invalid native fog resources");
    for(unsigned i=0;i<4;++i)if(!std::isfinite(rgba[i]))throw std::invalid_argument("Invalid native fog color");
    float value[32]{};memcpy(value+16,rgba,16);D3D11_MAPPED_SUBRESOURCE map{};
    check(context->Map(constants.Get(),0,D3D11_MAP_WRITE_DISCARD,0,&map));
    memcpy(map.pData,value,sizeof(value));context->Unmap(constants.Get(),0);
    ID3D11RenderTargetView *rt=target.Get();context->OMSetRenderTargets(1,&rt,nullptr);
    context->OMSetDepthStencilState(fog_depth.Get(),0);context->OMSetBlendState(blends[1][0].Get(),nullptr,0xffffffffu);
    context->RSSetState(rasterizer.Get());context->IASetInputLayout(nullptr);
    context->VSSetShader(fog_vertex_shader.Get(),nullptr,0);
    context->PSSetShader(msaa_active_?msaa_fog_shader_.Get():fog_pixel_shader.Get(),nullptr,0);
    ID3D11ShaderResourceView *resources[]={msaa_active_?msaa_.depth_resource.Get():depth_resource.Get(),lookup};context->PSSetShaderResources(0,2,resources);
    context->Draw(3,0);
    resources[0]=resources[1]=nullptr;context->PSSetShaderResources(0,2,resources);
    context->OMSetRenderTargets(1,&rt,depth_view.Get());context->IASetInputLayout(layout.Get());
    context->VSSetShader(vertex_shader.Get(),nullptr,0);context->PSSetShader(pixel_shader.Get(),nullptr,0);
}
void NativeRenderer::draw(const NativeGpuMesh &mesh, const float matrix[16], const float tint[4], ID3D11ShaderResourceView *texture,
                          uint32_t first_index, uint32_t index_count, const NativeDrawState &state) {
    if(state.blend_mode>4||(state.depth_bias>1&&state.depth_bias!=14&&state.depth_bias!=15)||!std::isfinite(state.inverse_exposure)||state.inverse_exposure<0||
       !std::isfinite(state.vertex_alpha_scale)||state.vertex_alpha_scale<0||state.vertex_alpha_scale>1||
       (state.prepass_alpha_exempt&&!state.alpha_prepass)||
       (state.untextured_material&&(texture||state.vertex_exposure||state.vertex_alpha)))
        throw std::invalid_argument("Unsupported native material state");
    if (!index_count) index_count = mesh.index_count;
    if (first_index > mesh.index_count || index_count > mesh.index_count-first_index)
        throw std::invalid_argument("Native draw range outside indices");
    struct Constants { float matrix[16], tint[4]; uint32_t textured, constant_color, vertex_exposure; float inverse_exposure,constant_rgba[4];
        uint32_t vertex_alpha;float vertex_alpha_scale,uv_offset[2],rotor_cs[2];uint32_t rotor_uv,unused; } value{};
    static_assert(sizeof(value) == 144); memcpy(value.matrix,matrix,64); memcpy(value.tint,tint,16); value.textured = state.alpha_prepass?(state.prepass_alpha_exempt?4u:3u):state.untextured_material?2u:(texture!=nullptr);
    value.constant_color=state.constant_color;value.vertex_exposure=state.vertex_exposure;value.inverse_exposure=state.inverse_exposure;
    memcpy(value.constant_rgba,state.constant,16);
    if(state.alpha_prepass){
        if(!texture||state.blend_mode!=1||state.vertex_exposure||state.untextured_material||state.depth_disabled||state.depth_bias||state.vertex_alpha||state.uv_period[0]||state.uv_period[1])
            throw std::invalid_argument("Invalid native alpha prepass state");
        for(unsigned k=0;k<3;++k){float v=state.constant[k]*state.inverse_exposure;
            if(!std::isfinite(v))throw std::invalid_argument("Invalid native prepass ambient");
            value.constant_rgba[k]=std::nearbyint(std::clamp(v,0.f,1.f)*255.f)/255.f;
        }
        value.constant_rgba[3]=1;
    }
    value.vertex_alpha=state.vertex_alpha;
    value.vertex_alpha_scale=state.vertex_alpha_scale;
    if(state.rotor_uv) {
        if(!texture||state.alpha_prepass||state.untextured_material||state.uv_period[0]||state.uv_period[1]||
           !std::isfinite(material_phase)||std::abs(material_phase)>16777216.f)
            throw std::invalid_argument("Invalid native rotor material phase/state");
        float angle=std::fmod(material_phase/.3f+0.f,1.f)*6.2831854820251465f;
        value.rotor_cs[0]=(float)std::cos((double)angle);value.rotor_cs[1]=(float)std::sin((double)angle);value.rotor_uv=1;
    }
    for(unsigned axis=0;axis<2;++axis)if(state.uv_period[axis]!=0) {
        if(!std::isfinite(state.uv_period[axis])||std::abs(state.uv_period[axis])<1e-6f||
           !std::isfinite(material_phase)||std::abs(material_phase)>16777216.f)
            throw std::invalid_argument("Invalid native material phase");
        value.uv_offset[axis]=std::fmod(material_phase/state.uv_period[axis]+0.0f,1.0f);
    }
    D3D11_MAPPED_SUBRESOURCE mapped{}; check(context->Map(constants.Get(),0,D3D11_MAP_WRITE_DISCARD,0,&mapped));
    memcpy(mapped.pData,&value,sizeof(value)); context->Unmap(constants.Get(),0);
    ID3D11Buffer *buffer = mesh.vertices.Get(); UINT stride = sizeof(NativeVertex), offset = 0;
    context->IASetVertexBuffers(0,1,&buffer,&stride,&offset); context->IASetIndexBuffer(mesh.indices.Get(),DXGI_FORMAT_R32_UINT,0);
    context->RSSetState(state.alpha_prepass&&!state.prepass_two_sided?prepass_rasterizer.Get():state.depth_bias==15?decal_rasterizer.Get():state.depth_bias==14?glow_rasterizer.Get():state.depth_bias?biased_rasterizer.Get():rasterizer.Get());
    context->OMSetBlendState(blends[state.blend_mode][!state.alpha_prepass&&state.write_alpha?1:0].Get(),nullptr,0xffffffffu);
    context->OMSetDepthStencilState(state.alpha_prepass?(state.prepass_alpha_exempt?transparent_depth.Get():prepass_depth.Get()):state.depth_disabled?fog_depth.Get():state.blend_mode?transparent_depth.Get():nullptr,0);
    if(!state.untextured_material){
        ID3D11SamplerState *ss=state.point_filter?point_sampler.Get():samplers[(state.clamp_u?1:0)|(state.clamp_v?2:0)].Get();context->PSSetSamplers(0,1,&ss);
        context->PSSetShaderResources(0,1,&texture);
    }
    context->DrawIndexed(index_count,first_index,0);
}
void NativeRenderer::present() { resolve_scene();if (swapchain) check(swapchain->Present(vsync_?1:0,0)); }
std::vector<uint8_t> NativeRenderer::read_rgba() {
    resolve_scene();
    D3D11_TEXTURE2D_DESC desc{}; color->GetDesc(&desc); desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0; desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ComPtr<ID3D11Texture2D> staging; check(device->CreateTexture2D(&desc,nullptr,&staging));
    std::vector<uint8_t> pixels((size_t)width*height*4);
    context->CopyResource(staging.Get(),color.Get()); D3D11_MAPPED_SUBRESOURCE map{};
    check(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&map));
    for (unsigned y=0;y<height;++y) memcpy(pixels.data()+(size_t)y*width*4,(uint8_t*)map.pData+(size_t)y*map.RowPitch,(size_t)width*4);
    context->Unmap(staging.Get(),0); return pixels;
}

#include "environment.h"
#include <d3dcompiler.h>
#include <cstring>
#include <stdexcept>

using Microsoft::WRL::ComPtr;
static void check(HRESULT r){if(FAILED(r))throw std::runtime_error("Exposure GPU operation failed");}
void NativeEnvironment::open(ID3D11Device *device,ID3D11DeviceContext *context,
    unsigned w,unsigned h,const UCExposureProfile &profile) {
    if(device_||!device||!context||context->GetType()!=D3D11_DEVICE_CONTEXT_IMMEDIATE||
       !w||!h||w%40||h%30||w>3840||h>2160)
        throw std::invalid_argument("Unsupported exposure owner or viewport");
    ComPtr<ID3D11Device> owner;context->GetDevice(&owner);
    if(owner.Get()!=device)throw std::invalid_argument("Exposure context device mismatch");
    if(uc_environment_view_reset(&view_,&exposure_,&profile,.8f)||
       uc_environment_exposure_tone_reset(&tone_))
        throw std::invalid_argument("Unsupported exposure profile");
    profile_=profile;device_=device;context_=context;width_=w;height_=h;count_=(w/40)*(h/30);
    sparse_.resize((size_t)w*h*4); /* Also supplies the explicit all-black history. */
    D3D11_TEXTURE2D_DESC td{};td.Width=w;td.Height=h;td.MipLevels=td.ArraySize=1;
    td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.SampleDesc.Count=1;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA data{sparse_.data(),w*4,w*h*4};
    check(device->CreateTexture2D(&td,&data,&history_));
    check(device->CreateShaderResourceView(history_.Get(),nullptr,&history_view_));
    td.Width=256;td.Height=1;check(device->CreateTexture2D(&td,nullptr,&curve_));
    check(device->CreateShaderResourceView(curve_.Get(),nullptr,&curve_view_));
    D3D11_BUFFER_DESC bd{};bd.ByteWidth=count_*4;bd.BindFlags=D3D11_BIND_UNORDERED_ACCESS;
    bd.MiscFlags=D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;bd.StructureByteStride=4;
    check(device->CreateBuffer(&bd,nullptr,&samples_));
    check(device->CreateUnorderedAccessView(samples_.Get(),nullptr,&samples_view_));
    bd.Usage=D3D11_USAGE_STAGING;bd.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    bd.BindFlags=bd.MiscFlags=bd.StructureByteStride=0;
    check(device->CreateBuffer(&bd,nullptr,&readback_));
    const char *code=R"(
Texture2D<float4> prior : register(t0); RWStructuredBuffer<uint> samples : register(u0);
[numthreads(64,1,1)] void main(uint3 id:SV_DispatchThreadID) {
    uint w,h; prior.GetDimensions(w,h);uint ny=h/30;
    if(id.x>=(w/40)*ny)return;
    uint2 xy=uint2(20+40*(id.x/ny),15+30*(id.x%ny));
    uint4 p=(uint4)round(prior.Load(int3(xy,0))*255.0);
    samples[id.x]=p.r|(p.g<<8)|(p.b<<16)|(p.a<<24);
})";
    ComPtr<ID3DBlob> compiled,error;
    HRESULT r=D3DCompile(code,strlen(code),"native exposure gather",nullptr,nullptr,"main","cs_5_0",
        D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&compiled,&error);
    if(FAILED(r)&&error)throw std::runtime_error((const char*)error->GetBufferPointer());
    check(r);check(device->CreateComputeShader(compiled->GetBufferPointer(),compiled->GetBufferSize(),nullptr,&gather_));
}
UCExposurePublication NativeEnvironment::begin(const UCExposureInput &input,uint32_t flags) {
    if(!gather_)throw std::logic_error("Exposure owner is not open");
    UCExposurePublication publication{};
    if(uc_environment_view_begin(&view_,&exposure_,&tone_,&profile_,&input,flags,&publication))
        throw std::invalid_argument("Unsupported or repeated exposure begin");
    uint8_t rgba[1024];
    for(unsigned i=0;i<256;++i){rgba[4*i]=tone_.lut[i];rgba[4*i+1]=tone_.lut[256+i];rgba[4*i+2]=tone_.lut[512+i];rgba[4*i+3]=0;}
    context_->UpdateSubresource(curve_.Get(),0,nullptr,rgba,sizeof(rgba),sizeof(rgba));
    return publication;
}
void NativeEnvironment::end(ID3D11Texture2D *completed,unsigned sample,unsigned glow,unsigned guard,int32_t counter) {
    if(!gather_||!view_.pending||!completed||sample>1||glow>1||guard>1)
        throw std::invalid_argument("Unsupported or unbegun exposure end");
    D3D11_TEXTURE2D_DESC td{};completed->GetDesc(&td);ComPtr<ID3D11Device> owner;completed->GetDevice(&owner);
    if(owner.Get()!=device_.Get()||td.Width!=width_||td.Height!=height_||
       td.Format!=DXGI_FORMAT_R8G8B8A8_UNORM||td.MipLevels!=1||td.ArraySize!=1||td.SampleDesc.Count!=1)
        throw std::invalid_argument("Exposure completed-image mismatch");
    if(sample) {
        ComPtr<ID3D11ComputeShader> shader;ComPtr<ID3D11ShaderResourceView> source;
        ComPtr<ID3D11UnorderedAccessView> destination;
        ID3D11ClassInstance *instances[256]{};UINT instance_count=256;
        context_->CSGetShader(&shader,instances,&instance_count);
        context_->CSGetShaderResources(0,1,&source);context_->CSGetUnorderedAccessViews(0,1,&destination);
        ID3D11ShaderResourceView *srv=history_view_.Get();ID3D11UnorderedAccessView *uav=samples_view_.Get();
        context_->CSSetShader(gather_.Get(),nullptr,0);context_->CSSetShaderResources(0,1,&srv);
        context_->CSSetUnorderedAccessViews(0,1,&uav,nullptr);context_->Dispatch((count_+63)/64,1,1);
        srv=source.Get();uav=destination.Get();context_->CSSetShaderResources(0,1,&srv);
        context_->CSSetUnorderedAccessViews(0,1,&uav,nullptr);context_->CSSetShader(shader.Get(),instances,instance_count);
        for(UINT i=0;i<instance_count;++i)instances[i]->Release();
        context_->CopyResource(readback_.Get(),samples_.Get());
        D3D11_MAPPED_SUBRESOURCE mapped{};check(context_->Map(readback_.Get(),0,D3D11_MAP_READ,0,&mapped));
        const auto *pixels=(const uint8_t*)mapped.pData;
        for(unsigned i=0;i<count_;++i){unsigned x=20+40*(i/(height_/30)),y=15+30*(i%(height_/30));
            memcpy(sparse_.data()+((size_t)y*width_+x)*4,pixels+4*i,4);}
        context_->Unmap(readback_.Get(),0);
    }
    UCExposureImage image{sparse_.data(),sparse_.size(),(size_t)width_*4,width_,height_};
    UCExposureViewport viewport{0,0,(float)width_,(float)height_};
    if(uc_environment_view_end(&view_,&retained_,&tone_,&image,&viewport,sample,glow,guard,counter))
        throw std::runtime_error("Unsupported exposure publication");
    context_->CopyResource(history_.Get(),completed);
}

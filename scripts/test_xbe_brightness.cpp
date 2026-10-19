// Execute the production reduction shader against deterministic BGRA surfaces.
#define NOMINMAX
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
using Microsoft::WRL::ComPtr;
#define CHECK(x) do { if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::exit(1);} } while(0)
static const char* shader=R"(/* SHADER */)";
int main() {
    ComPtr<ID3D11Device> device; ComPtr<ID3D11DeviceContext> context;
    CHECK(SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,nullptr,0,
        D3D11_SDK_VERSION,&device,nullptr,&context)));
    ComPtr<ID3DBlob> vs,ps,error;
    CHECK(SUCCEEDED(D3DCompile(shader,std::strlen(shader),nullptr,nullptr,nullptr,"vs","vs_5_0",0,0,&vs,&error)));
    HRESULT hr=D3DCompile(shader,std::strlen(shader),nullptr,nullptr,nullptr,"psmain","ps_5_0",0,0,&ps,&error);
    if(FAILED(hr)&&error)std::fprintf(stderr,"%s",(const char*)error->GetBufferPointer());
    CHECK(SUCCEEDED(hr));
    ComPtr<ID3D11VertexShader> vertex; ComPtr<ID3D11PixelShader> pixel;
    CHECK(SUCCEEDED(device->CreateVertexShader(vs->GetBufferPointer(),vs->GetBufferSize(),nullptr,&vertex)));
    CHECK(SUCCEEDED(device->CreatePixelShader(ps->GetBufferPointer(),ps->GetBufferSize(),nullptr,&pixel)));
    D3D11_TEXTURE2D_DESC desc{};desc.Width=640;desc.Height=480;desc.MipLevels=1;desc.ArraySize=1;
    desc.Format=DXGI_FORMAT_B8G8R8A8_UNORM;desc.SampleDesc.Count=1;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
    ComPtr<ID3D11Texture2D> source,target,staging;
    CHECK(SUCCEEDED(device->CreateTexture2D(&desc,nullptr,&source)));
    ComPtr<ID3D11ShaderResourceView> srv;CHECK(SUCCEEDED(device->CreateShaderResourceView(source.Get(),nullptr,&srv)));
    desc.Width=desc.Height=1;desc.BindFlags=D3D11_BIND_RENDER_TARGET;
    CHECK(SUCCEEDED(device->CreateTexture2D(&desc,nullptr,&target)));
    ComPtr<ID3D11RenderTargetView> rtv;CHECK(SUCCEEDED(device->CreateRenderTargetView(target.Get(),nullptr,&rtv)));
    desc.BindFlags=0;desc.Usage=D3D11_USAGE_STAGING;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    CHECK(SUCCEEDED(device->CreateTexture2D(&desc,nullptr,&staging)));
    float constants[8]={0,0,1,1,0,1,0,1};
    D3D11_BUFFER_DESC bd{};bd.ByteWidth=sizeof(constants);bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
    D3D11_SUBRESOURCE_DATA init{constants};ComPtr<ID3D11Buffer> cb;
    CHECK(SUCCEEDED(device->CreateBuffer(&bd,&init,&cb)));
    auto* c=cb.Get();context->VSSetConstantBuffers(0,1,&c);context->PSSetConstantBuffers(0,1,&c);
    context->VSSetShader(vertex.Get(),nullptr,0);context->PSSetShader(pixel.Get(),nullptr,0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    D3D11_VIEWPORT viewport{0,0,1,1,0,1};context->RSSetViewports(1,&viewport);
    auto* view=rtv.Get();context->OMSetRenderTargets(1,&view,nullptr);
    std::vector<unsigned char> pixels(640*480*4);
    for(unsigned trial=0;trial<272;++trial) {
        unsigned seed=trial+1;
        for(auto& byte:pixels){seed=seed*1664525u+1013904223u;byte=trial<256?trial:seed>>24;}
        unsigned sum=0;
        for(unsigned x=20;x<640;x+=40)for(unsigned y=15;y<480;y+=30)
            for(unsigned channel=0;channel<3;++channel)sum+=pixels[(y*640+x)*4+channel];
        context->UpdateSubresource(source.Get(),0,nullptr,pixels.data(),2560,0);
        auto* s=srv.Get();context->PSSetShaderResources(0,1,&s);context->Draw(3,0);
        context->CopyResource(staging.Get(),target.Get());D3D11_MAPPED_SUBRESOURCE mapped{};
        CHECK(SUCCEEDED(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&mapped)));
        auto* actual=(const unsigned char*)mapped.pData;
        CHECK((unsigned(actual[0])|(unsigned(actual[1])<<8)|(unsigned(actual[2])<<16))==sum&&actual[3]==255);
        context->Unmap(staging.Get(),0);
    }
    std::puts("Brightness GPU: all 256 constant byte values and 16 random surfaces match integer RGB sums exactly");
}

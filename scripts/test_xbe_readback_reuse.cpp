// Execute the production synchronous readback against real D3D12 resources.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdlib>
using Microsoft::WRL::ComPtr;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
static D3D12_RESOURCE_DESC BufferDesc(uint64_t bytes){
    D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;
    d.Width=bytes;d.Height=1;d.DepthOrArraySize=1;d.MipLevels=1;
    d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;return d;
}
struct D3D12Backend {
    ComPtr<ID3D12Device> dev_;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> cl_;
    ComPtr<ID3D12Fence> fence;
    ComPtr<ID3D12Resource> sync_readback_;
    uint64_t sync_readback_bytes_=0,value=0;
    HANDLE event=CreateEvent(nullptr,FALSE,FALSE,nullptr);
    ~D3D12Backend(){CloseHandle(event);}
    void Flush(){
        CHECK(SUCCEEDED(cl_->Close()));ID3D12CommandList* lists[]={cl_.Get()};
        queue->ExecuteCommandLists(1,lists);CHECK(SUCCEEDED(queue->Signal(fence.Get(),++value)));
        CHECK(SUCCEEDED(fence->SetEventOnCompletion(value,event)));
        CHECK(WaitForSingleObject(event,10000)==WAIT_OBJECT_0);
        CHECK(SUCCEEDED(allocator->Reset()));CHECK(SUCCEEDED(cl_->Reset(allocator.Get(),nullptr)));
    }
    bool ReadBack(ID3D12Resource*,uint32_t,uint32_t,std::vector<uint8_t>&);
};
/* READBACK_IMPLEMENTATION */
static void RoundTrip(D3D12Backend& backend,unsigned w,unsigned h,unsigned seed){
    D3D12_RESOURCE_DESC td{};td.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    td.Width=w;td.Height=h;td.DepthOrArraySize=1;td.MipLevels=1;
    td.Format=DXGI_FORMAT_B8G8R8A8_UNORM;td.SampleDesc.Count=1;
    D3D12_HEAP_PROPERTIES hp{D3D12_HEAP_TYPE_DEFAULT};ComPtr<ID3D12Resource> texture,upload;
    CHECK(SUCCEEDED(backend.dev_->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,
        D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&texture))));
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp{};UINT64 bytes=0;
    backend.dev_->GetCopyableFootprints(&td,0,1,0,&fp,nullptr,nullptr,&bytes);
    hp.Type=D3D12_HEAP_TYPE_UPLOAD;auto bd=BufferDesc(bytes);
    CHECK(SUCCEEDED(backend.dev_->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&bd,
        D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&upload))));
    uint8_t* mapped=nullptr;CHECK(SUCCEEDED(upload->Map(0,nullptr,reinterpret_cast<void**>(&mapped))));
    std::vector<uint8_t> expected(w*h*4);
    for(size_t i=0;i<expected.size();++i)expected[i]=uint8_t(i*7+seed);
    for(unsigned y=0;y<h;++y)std::memcpy(mapped+y*fp.Footprint.RowPitch,expected.data()+y*w*4,w*4);
    upload->Unmap(0,nullptr);
    D3D12_TEXTURE_COPY_LOCATION dst{texture.Get(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
    D3D12_TEXTURE_COPY_LOCATION src{upload.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};src.PlacedFootprint=fp;
    backend.cl_->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition={texture.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_COPY_SOURCE};
    backend.cl_->ResourceBarrier(1,&barrier);
    std::vector<uint8_t> actual;CHECK(backend.ReadBack(texture.Get(),w,h,actual));CHECK(actual==expected);
}
int main(int argc,char**){
    D3D12Backend backend;ComPtr<IDXGIFactory4> factory;CHECK(SUCCEEDED(CreateDXGIFactory2(0,IID_PPV_ARGS(&factory))));
    ComPtr<IDXGIAdapter> adapter;
    if(argc==1)CHECK(SUCCEEDED(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter))));
    CHECK(SUCCEEDED(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&backend.dev_))));
    D3D12_COMMAND_QUEUE_DESC q{};CHECK(SUCCEEDED(backend.dev_->CreateCommandQueue(&q,IID_PPV_ARGS(&backend.queue))));
    CHECK(SUCCEEDED(backend.dev_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&backend.allocator))));
    CHECK(SUCCEEDED(backend.dev_->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,backend.allocator.Get(),nullptr,IID_PPV_ARGS(&backend.cl_))));
    CHECK(SUCCEEDED(backend.dev_->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&backend.fence))));
    RoundTrip(backend,127,71,3);auto first=backend.sync_readback_;auto capacity=backend.sync_readback_bytes_;
    for(unsigned i=0;i<12;++i){RoundTrip(backend,127,71,i+17);CHECK(backend.sync_readback_.Get()==first.Get());}
    RoundTrip(backend,511,257,99);CHECK(backend.sync_readback_bytes_>capacity);CHECK(backend.sync_readback_.Get()!=first.Get());
    auto grown=backend.sync_readback_;RoundTrip(backend,9,7,123);CHECK(backend.sync_readback_.Get()==grown.Get());
    std::puts("PASS production readback pixel equality, row padding, repeated reuse, growth and smaller transfers");
}

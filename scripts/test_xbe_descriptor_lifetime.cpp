// Exercise the production allocation/retirement methods without opening a GPU or game.
#include <array>
#include <vector>
#include <memory>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <new>
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
enum D3D12_DESCRIPTOR_HEAP_TYPE {D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER};
struct ID3D12DescriptorHeap {unsigned id;};
template<class T> struct ComPtr : std::shared_ptr<T> {
    using std::shared_ptr<T>::shared_ptr;
    T* Get() const{return this->get();}
};
constexpr uint32_t kFrames=3,kSrvCount=24,kSamplerCount=8;
struct Commands {
    std::array<ID3D12DescriptorHeap*,2> bound{};unsigned binds=0;
    void SetDescriptorHeaps(unsigned count,ID3D12DescriptorHeap** heaps){
        CHECK(count==2);bound={heaps[0],heaps[1]};++binds;
    }
};
struct D3D12Backend {
    struct Frame {uint32_t srv=0;std::vector<ComPtr<ID3D12DescriptorHeap>> retired_heaps;};
    std::array<Frame,kFrames> frames_;uint64_t frame_=0;unsigned created=0;bool fail=false;
    ComPtr<ID3D12DescriptorHeap> srv_heap_,smp_heap_;Commands commands;Commands* cl_=&commands;
    Frame& CurrentFrame(){return frames_[frame_%kFrames];}
    void CreateHeap(D3D12_DESCRIPTOR_HEAP_TYPE,uint32_t,bool,ComPtr<ID3D12DescriptorHeap>& out){
        if(!fail)out=ComPtr<ID3D12DescriptorHeap>(new ID3D12DescriptorHeap{++created});
    }
    void RenewDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE);
    uint32_t AllocSrvSlots(uint32_t);
};
/* DESCRIPTOR_IMPLEMENTATION */
int main(){
    D3D12Backend b;
    b.CreateHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,kSrvCount,true,b.srv_heap_);
    b.CreateHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,kSamplerCount,true,b.smp_heap_);
    std::weak_ptr<ID3D12DescriptorHeap> old_srv=b.srv_heap_,old_smp=b.smp_heap_;
    auto* original=b.srv_heap_.Get();
    CHECK(b.AllocSrvSlots(4)==0);CHECK(b.AllocSrvSlots(4)==4);
    CHECK(b.created==2);CHECK(b.AllocSrvSlots(4)==0);
    CHECK(b.srv_heap_.Get()!=original && !old_srv.expired());
    CHECK(b.commands.bound[0]==b.srv_heap_.Get());
    b.RenewDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    CHECK(!old_smp.expired() && b.commands.bound[1]==b.smp_heap_.Get());
    CHECK(b.CurrentFrame().retired_heaps.size()==2);
    // Advancing to another frame must not destroy heaps referenced by the old one.
    ++b.frame_;CHECK(b.AllocSrvSlots(4)==8);CHECK(!old_srv.expired());
    b.frames_[0].retired_heaps.clear(); // Simulate that frame's fence completing.
    CHECK(old_srv.expired() && old_smp.expired());
    auto* current=b.srv_heap_.Get();b.fail=true;
    CHECK(b.AllocSrvSlots(4)==12);
    try{b.AllocSrvSlots(1);CHECK(false);}catch(const std::bad_alloc&){}
    CHECK(b.srv_heap_.Get()==current && b.CurrentFrame().srv==8);
    CHECK(b.CurrentFrame().retired_heaps.empty());
    std::puts("PASS descriptor overflow retirement, frame ownership, heap binding and failed-allocation preservation");
}

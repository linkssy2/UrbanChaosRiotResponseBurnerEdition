// Production-body command equivalence test. Host COM and constant-fill dependencies
// are mocked; the actual before/after upload, binding, viewport and draw bodies run.
// This verifies submitted state/bytes, not rendered pixels or game performance.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#undef NDEBUG
#define UC_BURNER_RELEASE
#include <windows.h>
#include <d3d12.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <memory>
#include <optional>
#include <vector>
#include <smmintrin.h>
#include "gpu/shaders.h"
#include "gpu/vertex.h"
using namespace gpu;

struct GuestExtent { uint32_t w, h; };
struct Target { uint32_t w=1920, h=1080; };
struct DrawTextures {
    TexInfo info[4]{};
    D3D12_SHADER_RESOURCE_VIEW_DESC sv[4]{};
    ID3D12Resource* res[4]{};
};
struct DrawShaders { bool prog=false, fixed_transform=false; FixedKey fk{}; };
struct PsoKey { uint32_t topo=0, blend=0, src=1; };
/* STRUCT */
/* HELPERS */

struct UploadArena {
    struct Allocation { void* res=nullptr; uint64_t off=0; uint8_t* cpu=nullptr; uint64_t gpu=0; };
    std::vector<std::vector<uint8_t>> blocks;
    uint64_t total=0;
    Allocation Upload(uint64_t size, uint64_t align) {
        assert(align==16 || align==256);
        blocks.emplace_back(size, 0xCD);
        total+=size;
        return {nullptr,0,blocks.back().data(),uint64_t(blocks.size())*65536};
    }
    const uint8_t* Data(uint64_t address, size_t size) const {
        assert(address && address%65536==0 && address/65536<=blocks.size());
        const auto& block=blocks[size_t(address/65536)-1];
        assert(size<=block.size());
        return block.data();
    }
};
struct MockHeap { uint64_t base=0; };
template<class T> struct ComPtr {
    std::shared_ptr<T> value;
    T* Get() const { return value.get(); }
    explicit operator bool() const { return bool(value); }
};
struct Commands {
    UploadArena* arena;
    ID3D12PipelineState* pso=nullptr;
    D3D_PRIMITIVE_TOPOLOGY topology=D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    D3D12_VIEWPORT viewport{};
    D3D12_RECT scissor{};
    std::array<float,4> blend{};
    uint32_t stencil=0;
    uint64_t cb[2]{},tables[2]{};
    uint64_t pso_calls=0, cb_calls=0, table_calls=0, raster_calls=0, output_calls=0, topology_calls=0;
    bool capture=true;
    std::vector<std::vector<uint8_t>> snapshots;
    D3D12_INDEX_BUFFER_VIEW index{};
    std::vector<D3D12_VERTEX_BUFFER_VIEW> vertices;
    explicit Commands(UploadArena* a):arena(a){}
    void SetPipelineState(ID3D12PipelineState* p){pso=p;++pso_calls;}
    void SetGraphicsRootConstantBufferView(unsigned i,uint64_t a){cb[i]=a;++cb_calls;}
    void SetGraphicsRootDescriptorTable(unsigned i,D3D12_GPU_DESCRIPTOR_HANDLE h){tables[i-2]=h.ptr;++table_calls;}
    void RSSetViewports(unsigned n,const D3D12_VIEWPORT* p){assert(n==1);viewport=*p;++raster_calls;}
    void RSSetScissorRects(unsigned n,const D3D12_RECT* p){assert(n==1);scissor=*p;++raster_calls;}
    void OMSetBlendFactor(const float* p){std::memcpy(blend.data(),p,16);++output_calls;}
    void OMSetStencilRef(uint32_t r){stencil=r;++output_calls;}
    void IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY t){topology=t;++topology_calls;}
    void IASetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW* p){index=*p;}
    void IASetVertexBuffers(unsigned start,unsigned n,const D3D12_VERTEX_BUFFER_VIEW* p){assert(start==0);vertices.assign(p,p+n);}
    void SetDescriptorHeaps(unsigned,MockHeap**){tables[0]=tables[1]=0;}
    void SetGraphicsRootSignature(void*){cb[0]=cb[1]=tables[0]=tables[1]=0;}
    void Disturb() {
        pso=nullptr;topology=D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
        viewport={};scissor={};blend={};stencil=0;
        cb[0]=cb[1]=tables[0]=tables[1]=0;
    }
    void Record(unsigned count,int base,bool indexed) {
        if(!capture)return;
        std::vector<uint8_t> result;
        auto add=[&](const void* p,size_t n){const auto* b=static_cast<const uint8_t*>(p);result.insert(result.end(),b,b+n);};
        add(&pso,sizeof(pso));add(&topology,sizeof(topology));add(&viewport,sizeof(viewport));
        add(&scissor,sizeof(scissor));add(blend.data(),sizeof(blend));add(&stencil,sizeof(stencil));
        add(tables,sizeof(tables));add(&count,sizeof(count));add(&base,sizeof(base));add(&indexed,sizeof(indexed));
        add(arena->Data(cb[0],sizeof(State::vsc)),sizeof(State::vsc));
        add(arena->Data(cb[1],sizeof(DrawConstants)),sizeof(DrawConstants));
        for(auto& vb:vertices){add(&vb.StrideInBytes,4);add(arena->Data(vb.BufferLocation,vb.SizeInBytes),vb.SizeInBytes);}
        if(indexed){add(&index.Format,sizeof(index.Format));add(arena->Data(index.BufferLocation,index.SizeInBytes),index.SizeInBytes);}
        snapshots.push_back(std::move(result));
    }
    void DrawInstanced(unsigned n,unsigned instances,unsigned start,unsigned first){assert(instances==1&&!start&&!first);Record(n,0,false);}
    void DrawIndexedInstanced(unsigned n,unsigned instances,unsigned start,int base,unsigned first){assert(instances==1&&!start&&!first);Record(n,base,true);}
};
struct Extension {
    unsigned serial=0;
    std::function<void()> begin,end;
    void BeginDraw(const State&){if(begin)begin();}
    void EndDraw(){if(end)end();}
    void Constants(float (*vsc)[4],float* ext,Target*,uint32_t blend){
        ++serial; vsc[0][0]+=float(serial);ext[0]=float(serial);ext[1]=float(blend);
    }
};
struct Geometry {
    template<class F> auto Get(const void*,uint32_t,F make){return make();}
};
constexpr uint32_t kSrvCount=64,kSamplerCount=64;
struct BackendFixture {
    using Alloc=UploadArena::Allocation;
    UploadArena arena;
    Commands commands{&arena};
    Commands* cl_=&commands;
    Target target{},depth{};
    Target* bound_rt_=&target;
    Target* bound_ds_=&depth;
    ComPtr<MockHeap> srv_heap_{std::make_shared<MockHeap>(MockHeap{0x100000})};
    ComPtr<MockHeap> smp_heap_{std::make_shared<MockHeap>(MockHeap{0x200000})};
    ComPtr<MockHeap> root_{std::make_shared<MockHeap>()};
    uint32_t srv_inc_=32,smp_inc_=16;
    uint32_t views=4,samplers=8;
    bool renew_views=false,renew_samplers=false;
    uint64_t generation=2;
    struct Frame { uint32_t draw_batch_draws=0;std::vector<ComPtr<MockHeap>> retired_heaps; } frame;
    struct SamplerCache {void Reset(){}} last_sampler_inputs_;
    Frame& CurrentFrame(){return frame;}
    void CreateHeap(D3D12_DESCRIPTOR_HEAP_TYPE,uint32_t,bool,ComPtr<MockHeap>& out){out.value=std::make_shared<MockHeap>(MockHeap{(++generation)*0x100000});}
    Alloc Upload(uint64_t size,uint64_t align){return arena.Upload(size,align);}
    D3D12_GPU_DESCRIPTOR_HANDLE Gpu(MockHeap* heap,uint32_t slot,uint32_t inc){return {heap->base+uint64_t(slot)*inc};}
    Geometry geometry_reuse_;
    Extension* ext_=nullptr;
    float fog=1,light=2,transform=3,texture=4,analytic=5;
    DrawShaders shaders{};
    PsoKey key{};
    DrawTextures textures{};
    Assembled assembled{};
    ID3D12PipelineState* pipeline=reinterpret_cast<ID3D12PipelineState*>(0x1234);
    /* FIELDS */
    size_t vsc_comparisons=0;
    int CompareVsc(const void* a,const void* b,size_t n){++vsc_comparisons;return std::memcmp(a,b,n);}
    BackendFixture(){
        assembled.draw_count=3;
        assembled.buffer_count=1;
        static const float vertices[9]={0,0,0,1,0,0,0,1,0};
        assembled.buffers[0]={reinterpret_cast<const uint8_t*>(vertices),sizeof(vertices),12};
    }
    float PointScale(){return 2.0f;}
    // Each unchanged production fill site writes a distinct output region. These
    // dependencies are modeled; exact BEFORE/AFTER bodies preserve their ordering.
    void FillSurfaceConstants(const State&,GuestExtent size,DrawConstants& c){c.surf[0]=float(size.w);c.surf[1]=float(size.h);}
    void FillFogConstants(const State&,bool prog,DrawConstants& c){c.fog[0]=fog;c.fog[1]=float(prog);}
    void FillCombinerConstants(const State& s,DrawConstants& c){c.pc[0][0]=s.vsc[1][0];}
    void FillTextureConstants(const State&,const DrawTextures&,DrawConstants& c){c.texsize[0][0]=texture;}
    void FillTransformConstants(const State&,GuestExtent,DrawConstants& c){c.wvp[0]=transform;}
    void FillLightConstants(const State&,const FixedKey&,DrawConstants& c){c.lt[0][0][0]=light;}
    void AnalyticFogConstants(const State&,DrawConstants& c){c.ext[3]=analytic;}
    bool UnitTexcoords(const State&,const Assembled&){return false;}
    D3D12_SRV_DIMENSION NullViewDimension(uint8_t){return D3D12_SRV_DIMENSION_TEXTURE2D;}
    uint32_t EnsureBlitSamplers(){return 12;}
};
uint32_t SimpleState(const State& s,uint32_t method){return s.simple[method>>2];}

#define DECLARE_BACKEND \
    void InvalidateDrawState(bool uploads=false); \
    void Begin(); \
    void RenewDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type); \
    void SetFullViewport(uint32_t w,uint32_t h); \
    void UploadConstants(const State&,const DrawShaders&,const PsoKey&,const DrawTextures&,GuestExtent,DrawConstants&,bool VSC_REVISION_DECL); \
    void SetTextureViewsAndSamplers(const State&,const Assembled&,DrawTextures&); \
    void SetViewportScissor(const State&,uint32_t); \
    void SetBlendFactorStencilRef(const State&); \
    void UploadVertexBuffers(const Assembled&); \
    void DrawGeometry(const Assembled&); \
    void DrawFullScreen(ID3D12PipelineState*,D3D12_GPU_VIRTUAL_ADDRESS,uint32_t,std::optional<uint32_t> = {}); \
    void SetPipeline(ID3D12PipelineState*); \
    void ExtensionDraw(const State&,bool); \
    uint32_t TextureViewSlots(const DrawTextures&) {if(renew_views){RenewDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);renew_views=false;}return views;} \
    uint32_t EnsureStageSamplers(const State&,const DrawTextures&,bool){if(renew_samplers){RenewDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);renew_samplers=false;}return samplers;} \
    void Guest(const State& s,uint64_t vsc_revision=0){ \
        SetPipeline(pipeline);DrawConstants c{}; \
        UploadConstants(s,shaders,key,textures,{640,480},c,true VSC_REVISION_ARG); \
        SetTextureViewsAndSamplers(s,assembled,textures);SetViewportScissor(s,640); \
        SetBlendFactorStencilRef(s);DrawGeometry(assembled); \
    }
#define VSC_REVISION_DECL
#define VSC_REVISION_ARG
namespace before {struct D3D12Backend:BackendFixture {DECLARE_BACKEND};
/* BEFORE */
}
#undef VSC_REVISION_DECL
#undef VSC_REVISION_ARG
#define VSC_REVISION_DECL ,uint64_t vsc_revision
#define VSC_REVISION_ARG ,vsc_revision
namespace after {struct D3D12Backend:BackendFixture {DECLARE_BACKEND};
/* AFTER */
}
#undef VSC_REVISION_DECL
#undef VSC_REVISION_ARG
#undef DECLARE_BACKEND

State Upper(){State s{};s.viewport.x=0;s.viewport.y=0;s.viewport.w=640;s.viewport.h=180;s.vsc[0][0]=1;return s;}
template<class B> void NewList(B& b){b.commands.Disturb();b.Begin();b.bound_rt_=&b.target;b.bound_ds_=&b.depth;}
template<class B> void Scenario(B& b){
    auto s=Upper();NewList(b);
    for(int i=0;i<8;++i)b.Guest(s);
    // Shared list, independent cameras, then returning to the first camera.
    s.viewport.y=180;s.vsc[0][0]=7;b.Guest(s);b.Guest(Upper());
    s=Upper();s.scissor_count=1;s.scissors={10,15,600,170};b.Guest(s);
    b.target.w=2560;b.target.h=1440;b.Guest(s);
    s.simple[kMBlendColor>>2]=0xFF123456;s.simple[kMStencilRef>>2]=0x123;b.Guest(s);
    b.shaders.fixed_transform=true;b.Guest(s);
    b.light=8;b.Guest(s);b.transform=9;b.Guest(s);b.fog=10;b.Guest(s);b.texture=11;b.Guest(s);b.analytic=12;b.Guest(s);
    b.views=12;b.Guest(s);b.samplers=16;b.Guest(s);
    b.renew_views=true;b.Guest(s);b.renew_samplers=true;b.Guest(s);
    b.renew_views=b.renew_samplers=true;b.Guest(s);
    b.pipeline=reinterpret_cast<ID3D12PipelineState*>(0x4321);b.Guest(s);
    b.assembled.topology=Topology::Lines;b.assembled.strip=true;b.Guest(s);
    const uint16_t indices[3]={2,1,0};b.assembled.index_src=indices;b.assembled.index_count=3;b.assembled.index16=true;b.assembled.base_vertex=1;b.Guest(s);
    b.assembled.index_count=0;b.key.topo=uint32_t(Topology::Points);b.assembled.topology=Topology::Points;b.Guest(s);
    // Final-byte comparisons cover noncanonical floating point representations too.
    uint32_t bits=0x80000000;std::memcpy(&s.vsc[0][0],&bits,4);b.Guest(s);
    bits=0x7FC00011;std::memcpy(&s.vsc[0][0],&bits,4);b.Guest(s);b.Guest(s);
    bits=0x7FC00012;std::memcpy(&s.vsc[0][0],&bits,4);b.Guest(s);
    // Fullscreen fog/depth/postprocess and a direct viewport change disturb state.
    auto cb=b.Upload(std::max(sizeof(State::vsc),sizeof(DrawConstants)),256);
    b.commands.capture=false;b.SetFullViewport(128,128);
    b.DrawFullScreen(reinterpret_cast<ID3D12PipelineState*>(0x9999),cb.gpu,20,24);
    b.commands.capture=true;b.Guest(s);
    b.SetFullViewport(64,64);b.Guest(s);
    NewList(b);b.Guest(s);
    Extension extension;
    extension.begin=[&]{b.commands.Disturb();};
    extension.end=[&]{b.commands.Disturb();};
    b.ext_=&extension;b.ExtensionDraw(s,false);b.ExtensionDraw(s,false);
    b.ExtensionDraw(s,true);b.ext_=nullptr;b.Guest(s);
    // ImGui or a lent command list can replace ALL tracked state.
    b.InvalidateDrawState();b.commands.Disturb();b.Guest(s);
}

namespace replay {
/* REPLAY_COMMAND_TYPES */
/* REPLAY_STATE_RANGES */
struct Sink {
    before::D3D12Backend baseline;
    after::D3D12Backend candidate;
    std::vector<State> expected;
    std::vector<uint64_t> revisions;
    Sink(){NewList(baseline);NewList(candidate);}
    void Draw(const State& s,uint32_t prim,uint32_t start,uint32_t count,const uint8_t* indices,uint64_t revision=0){
        assert(prim==5&&start==0&&count==3&&!indices);
        const auto& wanted=expected.at(revisions.size());
        assert(!std::memcmp(s.vsc.data(),wanted.vsc.data(),sizeof(s.vsc)));
        assert(s.viewport.x==wanted.viewport.x&&s.viewport.y==wanted.viewport.y);
        baseline.Guest(s);candidate.Guest(s,revision);
        assert(baseline.commands.snapshots.back()==candidate.commands.snapshots.back());
        revisions.push_back(revision);
    }
};
class Replay {
public:
    Sink sink;
    uint64_t vsc_revision_=1;
    void Queue(const State& state){
        SendState(state);Cmd c{Op::Draw};c.a=5;c.c=3;cur_->cmds.push_back(c);
        sink.expected.push_back(state);
    }
    void Drain(){
        for(const auto& c:cur_->cmds)Execute(*cur_,c);
        cur_->cmds.clear();cur_->bytes.clear();cur_->vs.clear();cur_->ps.clear();
    }
    // State-only commands can change and restore values before a later draw.
    void QueueState(const State& state){SendState(state);}
private:
    uint32_t Header(uint32_t header){return header;}
    /* REPLAY_SEND_METHODS */
    void Execute(const Batch& b,const Cmd& c){
        const uint8_t* data=b.bytes.empty()?nullptr:b.bytes.data()+c.data;
        switch(c.op){
            /* REPLAY_STATE_AND_DRAW */
            default:assert(false&&"only state/draw commands in this fixture");
        }
    }
    Sink* inner_=&sink;
    std::vector<Range> ranges_=StateRanges();
    State sent_{},state_{};
    Batch batch_{};
    Batch* cur_=&batch_;
};
}

void ReplayConstants(){
    replay::Replay r;State s{};
    // All-zero initial constants have no State delta, but still need an upload.
    r.Queue(s);r.Queue(s);r.Drain();
    assert(r.sink.revisions[0]==1&&r.sink.revisions[1]==1);
    assert(r.sink.candidate.vsc_comparisons==0);
    // Every float's exact bits reach the final upload, including both ends of each block.
    for(unsigned i=0;i<192*4;++i){
        const uint32_t bits=0x3f000000u+i;
        std::memcpy(&s.vsc[i/4][i%4],&bits,4);r.Queue(s);r.Queue(s);
        if(i%13==0)r.Drain();
    }
    s={};r.Drain(); // Mutating the producer after queuing cannot affect delayed replay.
    assert(r.sink.candidate.vsc_comparisons==192*4);
    auto revision=r.sink.revisions.back();
    // A partial final block can also contain PSC/viewport bytes; conservatively advancing is safe.
    r.Queue(s);r.Drain();revision=r.sink.revisions.back();
    s.simple[17]=9;s.viewport.y=180;s.uc_prompt_player=2;r.Queue(s);r.Drain();
    assert(r.sink.revisions.back()==revision);
    s.psc[0][0]=1;r.Queue(s);r.Drain();
    // The producer uses full blocks, so conservative overlap can advance even for PSC-only bytes.
    assert(r.sink.revisions.back()>=revision);
    for(uint32_t bits:{0x80000000u,0x7fc00011u,0x7fc00012u,0x7f800000u}){
        std::memcpy(&s.vsc[191][3],&bits,4);r.Queue(s);r.Queue(s);r.Drain();
    }
    auto comparisons=r.sink.candidate.vsc_comparisons;
    const auto snapshot=r.sink.candidate.commands.snapshots.back();
    s.vsc[17][2]=9;r.QueueState(s);s.vsc[17][2]=0;r.Queue(s);r.Drain();
    assert(r.sink.candidate.vsc_comparisons==comparisons+1);
    assert(r.sink.candidate.commands.snapshots.back()==snapshot);
    // Equal constants on a new list still need a resident upload and root binding.
    const auto allocated=r.sink.candidate.arena.total;
    NewList(r.sink.baseline);NewList(r.sink.candidate);r.Queue(s);r.Drain();
    assert(r.sink.candidate.arena.total>allocated);
    // Do not wrap to an old live token: zero permanently selects full comparison.
    r.vsc_revision_=UINT64_MAX;s.vsc[0][0]=23;r.Queue(s);r.Drain();
    assert(r.sink.revisions.back()==0);
    comparisons=r.sink.candidate.vsc_comparisons;
    r.Queue(s);r.Drain();assert(r.sink.candidate.vsc_comparisons==comparisons+1);
    s.vsc[0][1]=24;r.Queue(s);r.Drain();assert(r.sink.revisions.back()==0);

    before::D3D12Backend expected;after::D3D12Backend actual;
    auto draw=[&](const State& state,uint64_t rev){
        expected.Guest(state);actual.Guest(state,rev);
        assert(expected.commands.snapshots.back()==actual.commands.snapshots.back());
    };
    s=Upper();draw(s,1);comparisons=actual.vsc_comparisons;draw(s,1);
    assert(actual.vsc_comparisons==comparisons);
    State other=s;other.vsc[0][0]=6;draw(other,1); // Different owner, same numeric token.
    draw(s,1);s.vsc[0][0]=7;draw(s,0); // Direct draws always retain byte comparison.
    draw(s,1);s.vsc[0][0]=8;draw(s,0);draw(s,0);
    auto bytes=actual.arena.total;
    expected.InvalidateDrawState(true);actual.InvalidateDrawState(true);draw(s,1);
    assert(actual.arena.total>bytes);
    Extension a,b;expected.ext_=&a;actual.ext_=&b;
    draw(s,1);draw(s,1); // Extensions still get distinct writable uploads.
    expected.ExtensionDraw(s,true);actual.ExtensionDraw(s,true);
    expected.ext_=nullptr;actual.ext_=nullptr;draw(s,1);
    // A missed generation update must be caught by final submitted bytes.
    after::D3D12Backend good,bad;s=Upper();good.Guest(s,1);bad.Guest(s,1);
    s.vsc[0][0]=99;good.Guest(s,2);bad.Guest(s,1);
    assert(good.commands.snapshots.back()!=bad.commands.snapshots.back());
    std::printf("VSC replay: %zu delayed draws match original bytes; unchanged revisions skip comparison; direct/extension/reset/wrap fallbacks verified\n",r.sink.revisions.size());
}

int main(){
    ReplayConstants();
    before::D3D12Backend baseline;after::D3D12Backend candidate;
    Scenario(baseline);Scenario(candidate);
    assert(baseline.commands.snapshots==candidate.commands.snapshots);
    assert(candidate.arena.total<baseline.arena.total);
    assert(candidate.commands.pso_calls<baseline.commands.pso_calls);
    assert(candidate.commands.cb_calls<baseline.commands.cb_calls);
    assert(candidate.commands.table_calls<baseline.commands.table_calls);
    assert(candidate.commands.raster_calls<baseline.commands.raster_calls);
    assert(candidate.commands.output_calls<baseline.commands.output_calls);
    assert(candidate.commands.topology_calls<baseline.commands.topology_calls);
    // Negative witness: identical numeric slots on a NEW heap must rebind both
    // tables. Restore the stale cache to model the omitted invalidation defect.
    after::D3D12Backend good,bad;auto s=Upper();good.Guest(s);bad.Guest(s);
    auto stale=bad.binds_;good.RenewDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    bad.RenewDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);bad.binds_=stale;
    good.Guest(s);bad.Guest(s);
    assert(good.commands.snapshots.back()!=bad.commands.snapshots.back());
    // Negative witness: command-list reset with unchanged guest bytes must upload
    // afresh and rebind. A stale PSO/raster cache would leave the new list empty.
    after::D3D12Backend reset;reset.Guest(s);auto expected=reset.commands.snapshots.back();
    auto saved=reset.binds_;reset.commands.Disturb();reset.InvalidateDrawState(true);
    reset.binds_=saved;reset.Guest(s);assert(expected!=reset.commands.snapshots.back());
    // Detect a stale constants key: modifying vertex bytes must alter the snapshot.
    after::D3D12Backend constants;constants.Guest(s);auto first=constants.commands.snapshots.back();
    s.vsc[0][0]=99;constants.Guest(s);assert(first!=constants.commands.snapshots.back());
    std::printf("draw bindings: %zu production-body draw snapshots equivalent; upload bytes %llu -> %llu; CBV calls %llu -> %llu; invalidation negative witnesses rejected\n",
        candidate.commands.snapshots.size(),static_cast<unsigned long long>(baseline.arena.total),static_cast<unsigned long long>(candidate.arena.total),
        static_cast<unsigned long long>(baseline.commands.cb_calls),static_cast<unsigned long long>(candidate.commands.cb_calls));
}

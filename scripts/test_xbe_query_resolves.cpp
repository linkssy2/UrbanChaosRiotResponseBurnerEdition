// Actual renderer methods are inserted by prepare_xbe_visibility.py. CHECK stays active under NDEBUG.
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <limits>
#include <map>
#include <random>
#include <stdexcept>
#include <tuple>
#include <vector>
static uint64_t checks;
#define CHECK(x) do { ++checks; if (!(x)) throw std::runtime_error(#x); } while (false)
constexpr uint32_t kQueries=8192, kNoQuery=~0u, kMCountEnable=4;
constexpr int D3D12_QUERY_TYPE_OCCLUSION=0;
struct State { std::array<uint32_t,2> simple{}; };
struct Query { uint32_t index; float samples_per_pixel; bool operator==(const Query&)const=default; };
struct Report { uint32_t va; std::vector<Query> queries; uint64_t fence; bool operator==(const Report&)const=default; };
struct Heap {};
struct Buffer { std::array<uint64_t,kQueries> values{}; };
struct Fence { uint64_t value=0; uint64_t GetCompletedValue()const{return value;} };
template<class T> struct Ptr { T value{}; T* Get(){return &value;} T* operator->(){return &value;} };
using Writes=std::vector<std::pair<uint32_t,uint32_t>>;
static Writes* active_writes;
namespace kernel { void W32(uint32_t address,uint32_t value) {active_writes->emplace_back(address,value);} }
enum Kind { Begin,End,Draw,Resolve,Barrier,Marker };
struct Command { Kind kind; uint32_t first=0,count=0; uint64_t value=0; bool operator==(const Command&)const=default; };
struct CL {
    struct Slot { bool active=false,unresolved=false; uint64_t value=0; };
    std::array<Slot,kQueries> slots{};
    std::vector<Command> pending, plain;
    uint64_t resolve_calls=0,resolved_queries=0;
    Buffer* output=nullptr;
    void BeginQuery(Heap*,int,uint32_t q){pending.push_back({Begin,q});plain.push_back({Begin,q});}
    void EndQuery(Heap*,int,uint32_t q){pending.push_back({End,q});plain.push_back({End,q});}
    void ResolveQueryData(Heap*,int,uint32_t first,uint32_t count,Buffer* buffer,uint64_t offset){
        CHECK(first<kQueries && count>0 && count<=kQueries-first && offset==uint64_t(first)*8);
        CHECK(!output || output==buffer);output=buffer;
        pending.push_back({Resolve,first,count,offset});++resolve_calls;resolved_queries+=count;
    }
    void DrawSamples(uint64_t n){pending.push_back({Draw,0,0,n});plain.push_back({Draw,0,0,n});}
    void Boundary(uint32_t n){pending.push_back({Barrier,n});plain.push_back({Barrier,n});}
    void Execute(){
        for(const auto& c:pending){
            if(c.kind==Begin){auto& s=slots[c.first];CHECK(!s.active && !s.unresolved);s.active=true;s.value=0;}
            else if(c.kind==End){auto& s=slots[c.first];CHECK(s.active);s.active=false;s.unresolved=true;}
            else if(c.kind==Draw){for(auto& s:slots)if(s.active)s.value+=c.value;}
            else if(c.kind==Resolve){for(uint32_t i=0;i<c.count;++i){auto& s=slots[c.first+i];CHECK(!s.active&&s.unresolved);output->values[c.first+i]=s.value;s.unresolved=false;}}
        }
        pending.clear();
    }
};
struct Base {
    Ptr<Heap> query_heap_;
    Ptr<Buffer> query_rb_;
    const uint64_t* query_counts_=query_rb_.value.values.data();
    Ptr<Fence> fence_;
    Ptr<CL> cl_;
    uint32_t query_next_=0;
    uint64_t fence_value_=0,presents_=0;
    std::vector<Query> counted_;
    std::deque<Report> reports_;
    Writes writes;
    float factor=1;
    bool ensure=true;
    bool EnsureQueries(){return ensure;}
    float SamplesPerPixel()const{return factor;}
    void ClearReport(){counted_.clear();}
    void GetReport(uint32_t report){reports_.push_back({report,counted_,fence_value_+1});}
    void SubmitCommands(bool complete){cl_->Execute();++fence_value_;if(complete)fence_->value=fence_value_;}
};
namespace before {
struct D3D12Backend:Base {
    uint32_t BeginCountedDraw(const State&);
    void EndCountedDraw(uint32_t);
    void DeliverReports();
    void Submit(bool wait){SubmitCommands(wait);}
};
/* BEFORE_METHODS */
}
namespace after {
struct D3D12Backend:Base {
    uint32_t pending_resolve_first_=0,pending_resolve_count_=0;
    uint32_t BeginCountedDraw(const State&);
    void EndCountedDraw(uint32_t);
    void ResolveCountedDraws();
    void DeliverReports();
    void Submit(bool wait){ResolveCountedDraws();SubmitCommands(wait);}
};
/* AFTER_METHODS */
}
struct Pair {
    before::D3D12Backend a;
    after::D3D12Backend b;
    void Same(){
        CHECK(a.query_next_==b.query_next_);CHECK(a.counted_==b.counted_);CHECK(a.reports_==b.reports_);
        CHECK(a.cl_->plain==b.cl_->plain);CHECK(a.writes==b.writes);
        CHECK(a.fence_value_==b.fence_value_ && a.fence_->value==b.fence_->value);
    }
    void Draw(uint64_t samples,float factor=1,bool enabled=true,bool ensure=true){
        State state;state.simple[kMCountEnable>>2]=enabled;
        a.factor=b.factor=factor;a.ensure=b.ensure=ensure;
        const auto qa=a.BeginCountedDraw(state),qb=b.BeginCountedDraw(state);CHECK(qa==qb);
        a.cl_->DrawSamples(samples);b.cl_->DrawSamples(samples);
        a.EndCountedDraw(qa);b.EndCountedDraw(qb);Same();
    }
    void Clear(){a.ClearReport();b.ClearReport();Same();}
    void ReportAt(uint32_t va){a.GetReport(va);b.GetReport(va);Same();}
    void BarrierAt(uint32_t tag){a.cl_->Boundary(tag);b.cl_->Boundary(tag);Same();}
    void Submit(bool wait){
        a.Submit(wait);b.Submit(wait);CHECK(a.query_rb_.value.values==b.query_rb_.value.values);
        CHECK(!b.pending_resolve_count_);CHECK(a.cl_->resolved_queries==b.cl_->resolved_queries);Same();
    }
    void Deliver(uint64_t complete){
        CHECK(complete<=a.fence_value_);a.fence_->value=b.fence_->value=complete;
        active_writes=&a.writes;a.DeliverReports();active_writes=&b.writes;b.DeliverReports();Same();
    }
};
int main(){try {
    {
        Pair p;p.Submit(false);p.Deliver(0);CHECK(p.b.cl_->resolve_calls==0);
        p.Draw(31,2);p.ReportAt(100);p.Deliver(0);CHECK(p.a.writes.empty());
        p.Submit(false);p.Deliver(1);CHECK(p.a.writes.empty());p.Deliver(2);
        CHECK(p.a.writes[2]==std::make_pair(108u,16u));CHECK(p.b.cl_->resolve_calls==1);
    }
    {
        Pair p;
        for(uint32_t i=0;i<256;++i){p.Draw(i*3+1,1+(i%4));if(i%13==0)p.BarrierAt(i);}
        p.ReportAt(0x100);p.ReportAt(0x200);p.Clear();p.ReportAt(0x300);p.Submit(true);p.Deliver(1);
        CHECK(p.a.cl_->resolve_calls==256 && p.b.cl_->resolve_calls==1);
        CHECK(p.a.writes[10].second==0); // empty report count
        p.Draw(99,1,false);p.Draw(99,1,true,false);p.Submit(true);CHECK(p.b.cl_->resolve_calls==1);
    }
    for(uint32_t start:{0u,1u,8190u,8191u,0xfffffffdu}){
        Pair p;p.a.query_next_=p.b.query_next_=start;
        for(uint32_t i=0;i<kQueries+7;++i){
            p.Draw((uint64_t(i)<<16)|7,1+(i%8));
            if(i%113==0){p.Clear();p.ReportAt(0x400+i*16);}
        }
        p.Submit(false);p.Deliver(0);CHECK(p.a.writes.empty());p.Deliver(1);
        CHECK(p.a.cl_->resolve_calls==kQueries+7);CHECK(p.b.cl_->resolve_calls<=3);
    }
    {
        Pair p;p.a.query_next_=p.b.query_next_=8190;
        for(uint32_t i=0;i<4;++i)p.Draw(10+i,2);
        p.ReportAt(0x800);p.Submit(true);p.Deliver(1);
        CHECK(p.a.writes[2]==std::make_pair(0x808u,23u));
        CHECK(p.b.cl_->resolve_calls==2); // tail range, then head range
    }
    {
        Pair p;std::mt19937 rng(0x712983);
        for(uint32_t i=0;i<6000;++i){
            switch(rng()%12){
            case 0:p.Clear();break;
            case 1:p.ReportAt(0x10000+i*16);break;
            case 2:p.Submit(rng()%2);p.Deliver(p.a.fence_->value);break;
            case 3:p.BarrierAt(i);break;
            default:p.Draw(rng()%100000,1+(rng()%64)/4.f,rng()%7!=0,rng()%11!=0);break;
            }
        }
        p.Submit(true);p.Deliver(p.a.fence_value_);
        CHECK(p.b.cl_->resolve_calls<p.a.cl_->resolve_calls);
    }
    // Negative control: omitting the wrap drain must be detected at old-slot reuse.
    {
        CL cl;Heap heap;Buffer buffer;bool caught=false;
        cl.BeginQuery(&heap,0,0);cl.DrawSamples(3);cl.EndQuery(&heap,0,0);
        cl.BeginQuery(&heap,0,0);cl.EndQuery(&heap,0,0);
        cl.ResolveQueryData(&heap,0,0,1,&buffer,0);
        try{cl.Execute();}catch(const std::runtime_error&){caught=true;}CHECK(caught);
    }
    // Negative control: a pending range containing an unfinished query must fail.
    {
        CL cl;Heap heap;Buffer buffer;bool caught=false;
        cl.BeginQuery(&heap,0,3);cl.ResolveQueryData(&heap,0,3,1,&buffer,24);
        try{cl.Execute();}catch(const std::runtime_error&){caught=true;}CHECK(caught);
    }
    std::printf("PASS %llu CPU assertions: actual Begin/End/Deliver methods, fence visibility, wrap, disabled draws, clear/reports, exact command order and readback bytes.\n",(unsigned long long)checks);
    return 0;
}catch(const std::exception& e){std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}}

// Compare exact production Sync before/after. Only the GPU boundary is modeled;
// test_xbe_readback_reuse.cpp separately executes real production ReadBack on GPU.
namespace sync_allocations {
thread_local bool enabled=false;
thread_local size_t calls=0,bytes=0;
}
void* operator new(size_t size) {
    if(void* p=std::malloc(size?size:1)) {
        if(sync_allocations::enabled){++sync_allocations::calls;sync_allocations::bytes+=size;}
        return p;
    }
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete(void* p,size_t) noexcept {std::free(p);}
void* operator new[](size_t size){return ::operator new(size);}
void operator delete[](void* p) noexcept {std::free(p);}
void operator delete[](void* p,size_t) noexcept {std::free(p);}

#define UC_BURNER_RELEASE
#define LOG_FIRST(...) ((void)0)
namespace sync_pixels {
constexpr uint32_t address=64;
constexpr uint32_t kSigWholeMax=65536,kSigSamples=64,kSigSampleBytes=64;
uint32_t GuestPitch(uint32_t width){return (width*4+63)&~63u;}
uint64_t TargetMapKey(uint32_t data,bool){return data;}
std::vector<uint8_t>* active_guest=nullptr;
namespace kernel {void* HostPtr(uint32_t data){return active_guest->data()+data;}}
namespace diag {bool Perf(){return false;}}
// Fresh preparation precedes release-counter stripping; both source forms run.
struct {double sync=0;uint64_t syncs=0,sigs=0,sig_bytes=0;} g_perf;
struct PerfScope {explicit PerfScope(double&) {}};
uint64_t Hash(const void*,size_t,uint64_t=1469598103934665603ull);
/* HASH_IMPLEMENTATION */
struct Target {
    uint32_t data=address,bytes=0,gw=0,gh=0,w=0,h=0;
    bool drawn=true;
    uint64_t sig=0;
    std::vector<uint8_t> read;
};
/* SNAPSHOT_CONSUMERS */

enum class Failure {none,source,allocation,map};
struct Surface {std::vector<uint8_t> pixels;uint32_t pitch=0;};
struct CopySource {Surface* res=nullptr;};
struct Boundary {
    std::unordered_map<uint64_t,Target> targets_;
    std::vector<uint8_t> guest;
    Surface surface;
    Failure failure=Failure::none;
    bool stale=false;
    unsigned events=0;
    bool TargetStale(uint32_t){events=events*10+1;return stale;}
    CopySource PrepareCopySource(Target&,std::nullptr_t){events=events*10+2;return {failure==Failure::source?nullptr:&surface};}
    bool ReadBack(Surface* src,uint32_t w,uint32_t h,std::vector<uint8_t>& out) {
        events=events*10+3;
        if(failure==Failure::allocation)return false;
        events=events*10+4; // submitted copy and completed wait, even when Map fails
        if(failure==Failure::map)return false;
        out.resize(size_t(w)*h*4);
        for(uint32_t y=0;y<h;++y)std::memcpy(out.data()+size_t(y)*w*4,src->pixels.data()+size_t(y)*src->pitch,w*4);
        return true;
    }
    Target& Setup(uint32_t w,uint32_t h,unsigned seed) {
        auto& t=targets_[address];t.gw=t.w=w;t.gh=t.h=h;t.bytes=GuestPitch(w)*h;t.drawn=true;
        guest.assign(address+t.bytes+64,0xCD);
        surface.pitch=(w*4+255)&~255u;surface.pixels.resize(size_t(surface.pitch)*h);
        for(size_t i=0;i<surface.pixels.size();++i)surface.pixels[i]=uint8_t(i*7+seed);
        return t;
    }
};
namespace before {
struct D3D12Backend:Boundary {void Sync(uint32_t,uint32_t);};
/* SYNC_BEFORE */
}
namespace after {
struct D3D12Backend:Boundary {void Sync(uint32_t,uint32_t);};
/* SYNC_AFTER */
}
template<class Backend> void Run(Backend& backend,uint32_t data=address) {
    active_guest=&backend.guest;backend.events=0;backend.Sync(data,0);
}
void Equal(const before::D3D12Backend& old,const after::D3D12Backend& next) {
    const auto& a=old.targets_.at(address);const auto& b=next.targets_.at(address);
    CHECK(old.guest==next.guest&&old.events==next.events);
    CHECK(a.read==b.read&&a.drawn==b.drawn&&a.sig==b.sig);
}
void Pixels(const Boundary& backend) {
    const auto& t=backend.targets_.at(address);CHECK(!t.drawn);
    CHECK(t.read.size()==size_t(t.gw)*t.gh*4);
    for(uint32_t y=0;y<t.gh;++y) {
        for(uint32_t x=0;x<t.gw*4;++x) {
            const auto expected=backend.surface.pixels[size_t(y)*backend.surface.pitch+x];
            CHECK(t.read[size_t(y)*t.gw*4+x]==expected);
            CHECK(backend.guest[address+size_t(y)*GuestPitch(t.gw)+x]==expected);
        }
        for(uint32_t x=t.gw*4;x<GuestPitch(t.gw);++x)CHECK(backend.guest[address+size_t(y)*GuestPitch(t.gw)+x]==0xCD);
    }
    for(unsigned i=0;i<64;++i)CHECK(backend.guest[i]==0xCD&&backend.guest[address+t.bytes+i]==0xCD);
}
void Verify() {
    before::D3D12Backend old;after::D3D12Backend next;
    for(const auto dims:{std::pair{127u,71u},std::pair{640u,480u},std::pair{9u,7u},std::pair{511u,257u}}) {
        old.Setup(dims.first,dims.second,13);next.Setup(dims.first,dims.second,13);
        auto& a=old.targets_.at(address);auto& b=next.targets_.at(address);
        Run(old);Run(next);Equal(old,next);Pixels(next);CHECK(next.events==1234);
        auto* retained=b.read.data();
        for(unsigned seed=1;seed<=12;++seed) {
            old.Setup(dims.first,dims.second,seed);next.Setup(dims.first,dims.second,seed);
            Run(old);Run(next);Equal(old,next);Pixels(next);CHECK(b.read.data()==retained);
        }
        // The production CPU overlay consumer must still find tiny changed pixels,
        // preserve all untouched bytes, then advance the retained snapshot.
        active_guest=&next.guest;CHECK(!Changed(b));
        const size_t changed=address+size_t(b.gh/2)*GuestPitch(b.gw)+(b.gw/2)*4;
        old.guest[changed]^=0x5A;next.guest[changed]^=0x5A;CHECK(Changed(b));
        active_guest=&old.guest;auto over_old=LatchCpuChanges(a);
        active_guest=&next.guest;auto over_next=LatchCpuChanges(b);
        CHECK(over_old==over_next&&!Changed(b));Equal(old,next);
        const size_t offset=(size_t(b.gh/2)*b.gw+b.gw/2)*4;
        for(size_t i=0;i<over_next.size();++i) {
            const uint8_t expected=i<offset||i>=offset+4?0:i==offset+3?255:next.guest[changed+i-offset];
            CHECK(over_next[i]==expected);
        }
        for(const auto fail:{Failure::source,Failure::allocation,Failure::map}) {
            a.drawn=b.drawn=true;old.failure=next.failure=fail;
            const auto snapshot=b.read,guest=next.guest;const auto sig=b.sig;
            Run(old);Run(next);Equal(old,next);
            CHECK(b.read==snapshot&&next.guest==guest&&b.sig==sig&&b.drawn);
            CHECK(next.events==(fail==Failure::source?12u:fail==Failure::allocation?123u:1234u));
            CHECK(b.read.data()==retained);
        }
        old.failure=next.failure=Failure::none;
        old.stale=next.stale=true;Run(old);Run(next);Equal(old,next);CHECK(next.events==1);
        old.stale=next.stale=false;a.drawn=b.drawn=false;Run(old);Run(next);Equal(old,next);CHECK(!next.events);
        Run(old,address+1);Run(next,address+1);Equal(old,next);CHECK(!next.events);
        // RecordLate clears the snapshot without freeing its storage. An eventual
        // synchronous fallback must repopulate it after the same ordered wait.
        a.read.clear();b.read.clear();a.drawn=b.drawn=true;
        Run(old);Run(next);Equal(old,next);Pixels(next);CHECK(b.read.data()==retained);
    }
    std::puts("PASS exact pre/post Sync bytes, padding/guards, failure ordering, CPU overlays, skip paths and late-readback clear");
}
struct Measurement {double ms;size_t allocations,bytes;uint64_t signature;};
template<class Backend> Measurement Measure(unsigned width,unsigned height,bool mixed) {
    Backend backend;auto& t=backend.Setup(width,height,41);Run(backend); // steady-state capacity
    constexpr unsigned iterations=1500;
    sync_allocations::calls=sync_allocations::bytes=0;sync_allocations::enabled=true;
    const auto start=std::chrono::steady_clock::now();
    for(unsigned i=0;i<iterations;++i) {
        t.drawn=true;backend.surface.pixels[0]=uint8_t(i);
        backend.failure=mixed&&i%8<3?Failure(1+i%8):Failure::none;
        Run(backend);
    }
    const auto end=std::chrono::steady_clock::now();sync_allocations::enabled=false;
    const Measurement result{std::chrono::duration<double,std::milli>(end-start).count(),sync_allocations::calls,sync_allocations::bytes,t.sig};
    CHECK(result.allocations==(std::is_same_v<Backend,before::D3D12Backend>?(mixed?936u:iterations):0u));
    return result;
}
void Benchmark() {
    std::puts("CPU-only repeated exact Sync benchmark; GPU boundary copies padded bytes, no GPU waits/timing or game FPS claim.");
    for(const auto dims:{std::pair{127u,71u},std::pair{640u,480u},std::pair{1280u,720u}})for(bool mixed:{false,true}) {
        std::array<double,7> before_ms{},after_ms{};Measurement a{},b{};
        for(unsigned round=0;round<7;++round) {
            if(round%2){b=Measure<after::D3D12Backend>(dims.first,dims.second,mixed);a=Measure<before::D3D12Backend>(dims.first,dims.second,mixed);}
            else {a=Measure<before::D3D12Backend>(dims.first,dims.second,mixed);b=Measure<after::D3D12Backend>(dims.first,dims.second,mixed);}
            CHECK(a.signature==b.signature);before_ms[round]=a.ms;after_ms[round]=b.ms;
            std::printf("sample %ux%u mixed=%d round=%u before_ms=%.3f after_ms=%.3f\n",dims.first,dims.second,int(mixed),round,a.ms,b.ms);
        }
        std::sort(before_ms.begin(),before_ms.end());std::sort(after_ms.begin(),after_ms.end());
        std::printf("median %ux%u mixed=%d operations=1500 before_ms=%.3f after_ms=%.3f allocations=%zu/%zu allocated_bytes=%zu/%zu\n",
            dims.first,dims.second,int(mixed),before_ms[3],after_ms[3],a.allocations,b.allocations,a.bytes,b.bytes);
    }
}
} // namespace sync_pixels
#undef LOG_FIRST
#undef UC_BURNER_RELEASE

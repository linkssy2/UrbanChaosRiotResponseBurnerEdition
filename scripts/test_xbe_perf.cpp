// Actual production summaries, PSO cache-miss body and Release logger. The fake
// device/clock isolates diagnostic accounting; it is not a GPU timing claim.
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <process.h>
#include <string>
#include "core/diag.h"
#include "core/log.h"
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); std::exit(1); } } while(0)
static_assert(!logx::kVerbose,"The regression must exercise NDEBUG logging");
static int64_t ticks=0;
static unsigned clock_reads=0;
struct PerfClock {
    using duration=std::chrono::milliseconds;
    using time_point=std::chrono::time_point<PerfClock,duration>;
    static time_point now(){++clock_reads;return time_point(duration(ticks));}
};
static double PerfMs(PerfClock::time_point start){
    return std::chrono::duration<double,std::milli>(PerfClock::now()-start).count();
}
/* PERF_IMPLEMENTATION */
static std::atomic<uint64_t> g_perf_capture_ns{},g_perf_wait_ns{},g_perf_waits{},
    g_perf_wait_kind_ns[6]{},g_perf_sync_block{},g_perf_sync_late{};

struct ID3DBlob{};
struct ID3D12PipelineState{};
struct D3D12_INPUT_ELEMENT_DESC{};
struct D3D12_GRAPHICS_PIPELINE_STATE_DESC {
    struct {D3D12_INPUT_ELEMENT_DESC* elements;uint32_t count;} InputLayout{};
};
template<class T> struct ComPtr {
    T* value=nullptr;
    T* Get()const{return value;}
    T** operator&(){return &value;}
};
#define IID_PPV_ARGS(value) value
#define FAILED(value) ((value)<0)
enum class Topology:uint32_t{Triangles,Points};
struct PsoKey{uint32_t topo=0;};
static uint64_t Hash(const void* data,size_t size){
    uint64_t value=1469598103934665603ull;auto* p=static_cast<const uint8_t*>(data);
    while(size--)value=(value^*p++)*1099511628211ull;return value;
}
static uint32_t FillInputLayout(const PsoKey&,D3D12_INPUT_ELEMENT_DESC*){return 0;}
struct Device {
    ID3D12PipelineState result;unsigned calls=0;
    int CreateGraphicsPipelineState(const D3D12_GRAPHICS_PIPELINE_STATE_DESC*,ID3D12PipelineState** out){
        ++calls;ticks+=9;*out=&result;return 0;
    }
};
struct D3D12Backend {
    Device* dev_;std::map<uint64_t,ComPtr<ID3D12PipelineState>> psos_;
    static void FillPipelineDesc(const PsoKey&,ID3DBlob*,ID3DBlob*,D3D12_GRAPHICS_PIPELINE_STATE_DESC&){}
    static void ExpandPoints(D3D12_GRAPHICS_PIPELINE_STATE_DESC&){}
    ID3D12PipelineState* EnsurePipeline(const PsoKey&,ID3DBlob*,ID3DBlob*);
};
/* PIPELINE_IMPLEMENTATION */
/* REPORT_IMPLEMENTATION */
static void PresentDiagnostic(){/* PRESENT_GATE */}

int main(int argc,char** argv){
    const bool enabled=argc>1&&std::string(argv[1])=="enabled";
    CHECK(_putenv_s("XBE_PERF",enabled?"1":"")==0);
    CHECK(diag::Perf()==enabled);
    const auto directory=std::filesystem::temp_directory_path()/
        ("uc-xbe-perf-"+std::to_string(_getpid()));
    logx::Init(directory.string(),"diagnostic",false);
    LOGI("ordinary info must remain disabled");
    ticks=0;PresentDiagnostic();
    double scope_ms=0,manual_ms=0;
    {PerfScope scope{scope_ms};ticks+=7;}
    const auto manual_start=PerfStart();ticks+=3;PerfAdd(manual_ms,manual_start);
    CHECK(scope_ms==(enabled?7:0)&&manual_ms==(enabled?3:0));
    CHECK(enabled?clock_reads>0:clock_reads==0);
    Device device;D3D12Backend backend{&device};PsoKey key;ID3DBlob code;
    const unsigned before=clock_reads;
    CHECK(backend.EnsurePipeline(key,&code,&code)==&device.result);
    CHECK(device.calls==1&&g_perf.psos==1&&g_perf.pipeline==(enabled?9:0));
    CHECK(enabled||clock_reads==before); // Disabled adds no cold-miss clock reads.
    const auto reads=clock_reads;
    CHECK(backend.EnsurePipeline(key,&code,&code)==&device.result);
    CHECK(device.calls==1&&g_perf.psos==1&&clock_reads==reads);
    g_perf.draw=40;g_perf.compile=13;g_perf.shaders=2;g_perf.bind=8;
    g_perf.texup=4;g_perf.wait=6;g_perf.texups=3;g_perf.newtex=2;
    ticks=1001;PresentDiagnostic();logx::Flush();
    std::ifstream file(logx::Path());
    std::string text{std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
    CHECK(text.find("ordinary info")==std::string::npos);
    if(enabled){
        CHECK(text.find("compile 6.50 pipeline 4.50")!=std::string::npos);
        CHECK(text.find("upload 2.00")!=std::string::npos);
        CHECK(text.find("wait 3.00")!=std::string::npos);
        CHECK(text.find("2 shaders 1 psos this second")!=std::string::npos);
        CHECK(g_perf.shaders==0&&g_perf.psos==0&&g_perf.pipeline==0);
    }else CHECK(text.empty()&&g_perf.shaders==2&&g_perf.psos==1&&clock_reads==0);
    std::printf("PASS %s Release perf gate, actual summary, scope/manual/cache-hit/cold PSO clocks; ordinary Info stays suppressed\n",enabled?"enabled":"disabled");
}

"""Share the host stream factory with Urban Chaos's device-method entry point."""
def prepare(source, test_path):
    start = source.index('HLE_STDCALL(DirectSoundCreateStream, 2) {')
    end = source.index('\n#define STREAM(', start)
    original = source[start:end]
    assert original.count('a[0]') == original.count('a[1]') == 1
    factory = original.replace('HLE_STDCALL(DirectSoundCreateStream, 2)',
                               'static uint32_t NewStream(uint32_t desc, uint32_t out)')
    factory = factory.replace('    uint32_t desc = a[0];\n', '')
    factory = factory.replace('W32(a[1], obj);', 'W32(out, obj);')
    wrappers = '''
HLE_STDCALL(DirectSoundCreateStream, 2) { return NewStream(a[0], a[1]); }
// The device method has self and aggregation arguments around the same descriptor/output pair.
#define SYM_UrbanChaos_DeviceCreateStream 00427181
HLE_STDCALL(UrbanChaos_DeviceCreateStream, 4) { return NewStream(a[1], a[2]); }
#define SYM_UrbanChaos_StreamOutput 00426156
HLE_STDCALL(UrbanChaos_StreamOutput, 2) {
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    Voice* v = Find(a[0]);
    if (!v || !v->stream) return 0x80070057u;
    // shortcut: streams currently mix directly; implement host submix buses before accepting nonzero destinations.
    return a[1] ? 0x80004001u : 0;
}
'''
    # Exercise the exact generated factory and wrappers with a small guest-memory fixture.
    fixture = r'''
#include <algorithm>
#undef NDEBUG
#include <cassert>
#include <cstring>
#include <cstdint>
#include <map>
#include <mutex>
static uint8_t memory[4096]{};
static void* HostPtr(uint32_t p){return memory+p;}
static uint32_t R32(uint32_t p){uint32_t v;std::memcpy(&v,memory+p,4);return v;}
static void W32(uint32_t p,uint32_t v){std::memcpy(memory+p,&v,4);}
namespace mem { static uint32_t next=512; uint32_t AllocSystem(uint32_t n){auto p=next;next+=n;return p;} }
struct Voice {uint32_t obj=0,max_packets=0,callback=0,context=0,headroom=0,format=0,bins=0;bool stream=false,is3d=false;};
static std::map<uint32_t,Voice> g_voices;
static Voice* Find(uint32_t p){auto i=g_voices.find(p);return i==g_voices.end()?nullptr:&i->second;}
static std::recursive_mutex g_lock;
static void Start(){}
static void ReadFormat(Voice& v,uint32_t p){v.format=p;}
static void SetBins(Voice& v,uint32_t p){v.bins=p;}
static uint32_t DefaultHeadroom(uint32_t flags){return flags?600:0;}
constexpr uint32_t XDK_CDirectSoundStream_Vtable=0,XDK_CDirectSoundStream_Vtable2=4,kStreamVtable=123,kStreamVtable2=456;
#define XDK_REQUIRE(...)
#define HLE_STDCALL(name,n) constexpr int name##_argc=n; static uint32_t name(const uint32_t* a)
'''
    checks = r'''
int main(){
 static_assert(DirectSoundCreateStream_argc==2 && UrbanChaos_DeviceCreateStream_argc==4);
 W32(64,0x10);W32(68,5);W32(72,200);W32(76,201);W32(80,202);W32(84,203);
 uint32_t global[]={64,128},device[]={0xdeadbeef,64,132,0};
 assert(DirectSoundCreateStream(global)==0);
 assert(UrbanChaos_DeviceCreateStream(device)==0);
 auto one=R32(128),two=R32(132);assert(one!=two);
 for(auto p:{one,two}){auto& v=g_voices.at(p);
  assert(R32(p)==123 && R32(p+4)==456 && v.obj==p && v.stream && v.is3d);
  assert(v.max_packets==5 && v.callback==201 && v.context==202 && v.format==200 && v.bins==203 && v.headroom==600);
 }
 W32(68,0);assert(UrbanChaos_DeviceCreateStream(device)==0);
 assert(g_voices.at(R32(132)).max_packets==1);
 static_assert(UrbanChaos_StreamOutput_argc==2);
 uint32_t route[]={two,0};assert(UrbanChaos_StreamOutput(route)==0);
 route[1]=one;assert(UrbanChaos_StreamOutput(route)==0x80004001u);
 route[0]=0;assert(UrbanChaos_StreamOutput(route)==0x80070057u);
 route[0]=two;g_voices.at(two).stream=false;
 assert(UrbanChaos_StreamOutput(route)==0x80070057u);
}
'''
    test_path.write_text(fixture + factory + wrappers + checks)
    return source[:start] + factory + wrappers + source[end:]

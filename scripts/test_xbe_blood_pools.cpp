#undef NDEBUG
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <xmmintrin.h>
#include "cpu/x86_context.h"
#include "kernel/kernel.h"
#include "kernel/memory.h"
static uint8_t bytes[0x100000];
static unsigned allocations=0,frees=0,queries=0,vertices=0;
static bool count_clip_allocations=false;
static size_t clip_allocations=0;
#include <new>
#include <cstdlib>
void* operator new(size_t n){if(count_clip_allocations)++clip_allocations;if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void operator delete(void* p) noexcept{std::free(p);}
void operator delete(void* p,size_t) noexcept{std::free(p);}
static float draw_u=0,draw_v=0,floor_edge=5,floor_height=0;
static bool fail=false,throws=false;
namespace mem {bool IsCommitted(uint32_t a){return a>=0x10000&&a<sizeof(bytes);}}
namespace kernel {
uint32_t CallGuest(X86Context& c,uint32_t fn,const uint32_t* args,size_t n){
    if(fn==0x214230){assert(n==6&&args[2]==1&&args[3]==0xffff&&args[4]==0x4000&&args[5]==0);float start[3],end[3];std::memcpy(start,bytes+args[0],12);std::memcpy(end,bytes+args[1],12);++queries;return start[0]<=floor_edge&&start[1]>=floor_height&&end[1]<=floor_height;}
    if(fn==0x3ef5c0){assert(n==3&&args[0]==3);std::memcpy(&draw_u,args+1,4);std::memcpy(&draw_v,args+2,4);return 0;}
    if(fn==0x3ef600){assert(n==5&&args[0]==0xffffffff&&args[4]==0x3f800000);float xyz[3];std::memcpy(xyz,args+1,12);assert(xyz[0]<=5.00001f&&xyz[1]==1&&xyz[2]>=0&&xyz[2]<=10);assert(draw_u>=0&&draw_u<=1&&draw_v>=0&&draw_v<=1);assert(std::abs(draw_u-xyz[0]/10)<.00001f);++vertices;return 0;}
    assert(n==1);std::memset(bytes+c.esp-128,0xad,128);c.eax=9;_mm_setcsr(0x1f80);
    if(fn==0x2d589f){++allocations;assert(args[0]==8*76);if(throws)throw 7;return fail?0:0x30000;}
    assert(fn==0x2d631d&&args[0]==0x20000);++frees;return 0;
}}
#include "../common/xbe_blood_pools.cpp"
static void setup(unsigned first,unsigned next){
    std::memset(bytes,0,sizeof(bytes));allocations=frees=0;fail=throws=false;
    Write(bytes,0x10000,0x456ae0);Write(bytes,0x10080,0x20000);
    Write(bytes,0x10084,first);Write(bytes,0x10088,next);Write(bytes,0x1008c,4);
    for(unsigned i=0;i<4;++i)std::memset(bytes+0x20000+i*76,i+1,76);
}
// Frozen pre-cleanup clipper: compare all float bytes, including boundary duplicates.
static blood_clip::Polygon original_intersect(blood_clip::Polygon a,const blood_clip::Polygon& b){
 using namespace blood_clip;
 for(size_t i=0;i<b.size()&&!a.empty();++i){Polygon out;Point p=a.back();float dp=cross(b[i],b[(i+1)%b.size()],p);
  for(Point q:a){float dq=cross(b[i],b[(i+1)%b.size()],q);
   if((dp>=0)!=(dq>=0))out.push_back(lerp(p,q,dp/(dp-dq)));
   if(dq>=0)out.push_back(q);p=q;dp=dq;
  }a=std::move(out);
 }return a;
}
int main(){
    using namespace blood_clip;
    auto flat=support([](Point){return true;});assert(flat.size()==1);
    auto empty=support([](Point){return false;});assert(empty.empty());
    for(float edge:{.1f,.5f,.93f}){
        auto mesh=support([&](Point p){return p.u<=edge;});double area=0;
        for(auto& poly:mesh){for(auto p:poly)assert(p.u<=edge);for(size_t i=1;i+1<poly.size();++i)area+=cross(poly[0],poly[i],poly[i+1])*.5;}
        assert(std::abs(area-edge)<.0001);
        Polygon growing={{.2f,.2f},{.8f,.2f},{.8f,.8f},{.2f,.8f}};
        Polygon clipped,scratch;clipped.reserve(8);scratch.reserve(8);
        for(auto& poly:mesh){clipped=growing;intersect(clipped,poly,scratch);for(auto p:clipped)assert(p.u<=edge+.000001f&&p.u>=.2f-.000001f&&p.v>=.2f-.000001f&&p.v<=.8f+.000001f);}
    }
    auto diagonal=support([](Point p){return p.u+p.v<=1;});
    for(auto& poly:diagonal)for(auto p:poly)assert(p.u+p.v<=1.000001f);
    size_t old_allocations=0,new_allocations=0,comparisons=0;
    Polygon clipped,scratch;clipped.reserve(8);scratch.reserve(8);
    for(int step=0;step<=100;++step){float extent=step*.01f;
        Polygon growing={{.5f-extent,.5f-extent},{.5f+extent,.5f-extent},{.5f+extent,.5f+extent},{.5f-extent,.5f+extent}};
        for(const auto& surface:diagonal){
            clip_allocations=0;count_clip_allocations=true;auto expected=original_intersect(growing,surface);count_clip_allocations=false;old_allocations+=clip_allocations;
            clip_allocations=0;count_clip_allocations=true;clipped.assign(growing.begin(),growing.end());intersect(clipped,surface,scratch);count_clip_allocations=false;new_allocations+=clip_allocations;
            assert(clipped.size()==expected.size());if(!clipped.empty())assert(!std::memcmp(clipped.data(),expected.data(),clipped.size()*sizeof(Point)));++comparisons;
        }
    }
    assert(old_allocations>0&&new_allocations==0);
    std::printf("PASS clipping parity: %zu polygons; temporary allocations %zu -> %zu after scratch reservation\n",comparisons,old_allocations,new_allocations);
    puts("PASS blood clipping: flat floor, absent support, straight and diagonal ledges, growing footprint");

    const auto before=TrialDecalSubmissions();TrialDecalSubmitted();assert(TrialDecalSubmissions()==before+1);
    X86Context c{};c.ecx=0x10000;c.esp=0x80000;const auto original=c;
    for(unsigned first=0;first<4;++first){
        setup(first,(first+3)%4);TrialGrowDecalRing(c,bytes);
        assert(allocations==1&&frees==1&&Read(bytes,0x1008c)==8&&Read(bytes,0x10084)==0&&Read(bytes,0x10088)==3);
        for(unsigned i=0;i<3*76;++i)assert(bytes[0x30000+i]==(first+i/76)%4+1);
        assert(!std::memcmp(&c,&original,sizeof(c)));
        for(unsigned i=c.esp-4096;i<c.esp;++i)assert(bytes[i]==0);
    }
    setup(0,2);TrialGrowDecalRing(c,bytes);assert(!allocations&&!frees);
    setup(0,3);fail=true;TrialGrowDecalRing(c,bytes);assert(allocations==1&&!frees&&Read(bytes,0x10080)==0x20000&&Read(bytes,0x1008c)==4);
    setup(0,3);throws=true;try{TrialGrowDecalRing(c,bytes);assert(false);}catch(int e){assert(e==7);}
    assert(Read(bytes,0x10080)==0x20000&&!frees);
    for(unsigned i=c.esp-4096;i<c.esp;++i)assert(bytes[i]==0);
    setup(0,4);TrialGrowDecalRing(c,bytes);assert(!allocations);
    setup(0,2);Write(bytes,0x11000,123);float quad[12]={0,1,0,10,1,0,10,1,10,0,1,10};std::memcpy(bytes+0x11010,quad,48);
    c.esi=0x11004;{TrialBloodPoolScope pool;TrialBloodSupport(c,bytes);}assert(queries>0);
    for(unsigned i=c.esp-8192;i<c.esp;++i)assert(bytes[i]==0);
    c.ebx=0x11000;
    for(unsigned i=0;i<4;++i){c.esi=i;std::memcpy(bytes+c.esp+0x20,quad+i*3,12);c.ebp=0x3f800000;const auto saved=c;assert(TrialBloodVertex(c,bytes));assert(!std::memcmp(&c,&saved,sizeof(c)));}
    assert(vertices>0&&vertices%4==0);TrialClearBloodSupport();assert(!TrialBloodVertex(c,bytes));
    // Retail quad construction offsets the decal one unit above its floor.
    floor_edge=10;c.esi=0x11004;{TrialBloodPoolScope pool;TrialBloodSupport(c,bytes);}
    c.esi=0;assert(!TrialBloodVertex(c,bytes)); // Entirely supported: retain original draw.
    floor_height=-50;c.esi=0x11004;{TrialBloodPoolScope pool;TrialBloodSupport(c,bytes);}
    const auto previous_vertices=vertices;
    for(unsigned i=0;i<4;++i){c.esi=i;std::memcpy(bytes+c.esp+0x20,quad+i*3,12);c.ebp=0x3f800000;assert(TrialBloodVertex(c,bytes));}
    assert(vertices==previous_vertices);TrialClearBloodSupport();
    puts("PASS raised decal regression: flat floor draws, ledge clips, distant lower floor cannot support floating blood");
    puts("PASS blood runtime adapter: original ground query arguments, clipped position/UV emission, stack/context restoration and cleanup");
    puts("PASS decal growth: wrapped ordering, capacity, allocation failure, invalid indices, register and exception stack restoration");
}

// Original upgrade loop, interior entry and sprite queue; no game/GPU launch.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <vector>
#include "kernel/kernel.h"
namespace kernel {static std::array<uint8_t,0x800000> memory{};uint8_t* g_base=memory.data();}
namespace mem {static uint32_t denied=0xffffffff;bool IsCommitted(uint32_t p){return p<kernel::memory.size()&&p!=denied;}}
#include "xbe_health_upgrade_fade.h"
using namespace kernel;
static constexpr uint32_t owner=0x600000,descriptor=0x601000,session=0x602000;
static constexpr uint32_t engine=0x603000,queue=0x604000,stack=0x700000;
static unsigned tier;
static uint32_t U(uint32_t a){uint32_t v;std::memcpy(&v,g_base+a,4);return v;}
static float F(uint32_t a){float v;std::memcpy(&v,g_base+a,4);return v;}
static void W(uint32_t a,uint32_t v){std::memcpy(g_base+a,&v,4);}
static void P(uint32_t a,float v){std::memcpy(g_base+a,&v,4);}
struct Draw {float x,y,right,bottom;uint32_t color;};
static std::vector<Draw> draws;
X86_FUNC(sub_00065760);X86_FUNC(sub_00065550);X86_FUNC(sub_00065170);
X86_FUNC(sub_0005AAB0){ctx.eax=session;ctx.esp+=4;}
X86_FUNC(sub_0005B140){assert(ctx.ecx==session);ctx.eax=session;ctx.esp+=4;}
X86_FUNC(sub_000643E0){assert(U(ctx.esp+4)==5);ctx.eax=session+0x100;ctx.esp+=8;}
X86_FUNC(sub_00064400){assert(ctx.ecx==session+0x100);ctx.eax=tier;ctx.esp+=4;}
X86_FUNC(sub_002D666E){double value;std::memcpy(&value,base+ctx.esp+4,8);x87_push(ctx,std::floor(value));ctx.esp+=4;}
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
void x86_icall(X86Context& c,uint8_t*,uint32_t target){
    if(target==0xf0001){assert(c.ecx==engine);c.eax=queue;c.esp+=4;return;}
    assert(target==0xf0002&&c.ecx==queue&&U(c.esp+16)==123&&U(c.esp+28)==0&&U(c.esp+32)==0);
    const auto a=U(c.esp+4),z=U(c.esp+8),u=U(c.esp+20),v=U(c.esp+24);
    assert(F(u)==0&&F(u+4)==0&&F(v)==1&&F(v+4)==1);
    draws.push_back({F(a),F(a+4),F(z),F(z+4),U(c.esp+12)});c.esp+=36;
}
// ORIGINAL_MARKER_BODIES

static X86Context Reset(float alpha,float x,float y,unsigned mode,bool resume){
    memory.fill(0);draws.clear();mem::denied=0xffffffff;
    // ORIGINAL_CONSTANTS
    W(owner,0x4530e4);W(owner+4,123);W(session+0x74,mode);
    W(0x5150b0,engine);W(engine,engine+0x100);W(engine+0x148,0xf0001);
    W(queue,queue+0x100);W(queue+0x104,0xf0002);
    P(descriptor,x);P(descriptor+4,y);P(descriptor+28,alpha);
    X86Context c{};c.ecx=owner;c.esp=stack;c.ebp=0x12345678;c.ebx=0x12345679;c.esi=0x1234567a;c.edi=0x1234567b;
    c.fcw=0x37f;c.mxcsr=0x1f80;c.top=3;c.st[3]=7;c.st[4]=-11;c.mm[4]=0x12345678;
    c.xmm[7].u[2]=0x12345678;c.cf=1;c.df=1;
    W(stack,0xfeedface);W(stack+4,descriptor);W(stack+8,0xcafebabe);W(stack-0x200,0xabcdef12);
    if(resume){
        // 733E0's actual aligned frame at the start of 7346D's loop entry.
        W(stack-4,c.ebp);W(stack-0x24,c.esi);W(stack-0x28,c.edi);
        c.ebp=stack-4;c.esp=stack-0x28;c.edi=owner;c.esi=tier;
        c.xmm[0].f[0]=x+175;c.xmm[1].f[0]=y+11;
        P(c.esp+0x10,x+175);P(c.esp+0x14,y+11);
    }
    return c;
}

static void DrawChecks(){
    unsigned cases=0;
    for(bool resume:{false,true})for(unsigned upgrades=resume?1:0;upgrades<=3;++upgrades)
    for(unsigned mode:{0u,1u})for(float alpha:{0.f,.25f,.5f,1.f})
    for(auto origin:{std::array<float,2>{48,36},{16,12},{16,252}}){
        tier=upgrades;auto original=Reset(alpha,origin[0],origin[1],mode,resume);
        _mm_setcsr(0x1f80);
        (resume?Original_0007346D:Original_000733E0)(original,g_base);
        const auto expected=draws;const unsigned original_csr=_mm_getcsr();
        const auto count=resume||!mode?upgrades:0;
        assert(expected.size()==count&&original.esp==stack+8&&original.ebp==0x12345678);
        for(unsigned i=0;i<count;++i){
            assert(expected[i].x==origin[0]+174.5f+i*20&&expected[i].y==origin[1]+10.5f);
            assert(expected[i].right==expected[i].x+2&&expected[i].bottom==expected[i].y+3&&expected[i].color==0x55000000);
        }
        auto fixed=Reset(alpha,origin[0],origin[1],mode,resume);
        std::array<uint8_t,0x4108> owners;std::memcpy(owners.data(),g_base+owner,owners.size());
        _mm_setcsr(0x1f80);(resume?Fixed_0007346D:Fixed_000733E0)(fixed,g_base);
        assert(!std::memcmp(&fixed,&original,sizeof(fixed))&&_mm_getcsr()==original_csr);
        assert(draws.size()==expected.size()&&!std::memcmp(owners.data(),g_base+owner,owners.size()));
        assert(U(stack)==0xfeedface&&U(stack+4)==descriptor&&U(stack+8)==0xcafebabe&&U(stack-0x200)==0xabcdef12);
        const unsigned opacity=alpha==0?0:alpha==.25f?21:alpha==.5f?42:85;
        for(unsigned i=0;i<draws.size();++i){
            assert(!std::memcmp(&draws[i],&expected[i],4*sizeof(float)));
            assert(draws[i].color==opacity<<24);
        }
        ++cases;
    }
    std::printf("PASS %u original health-upgrade loop/resume cases: parent fade, all tiers, SP/split origins, unchanged geometry and CPU/FP/owner state\n",cases);
}

static void GuardChecks(){
    for(unsigned csr:{0x1f80u,0x3f80u,0x5f80u,0x7f80u,0x9fc0u})
    for(unsigned bad=0;bad<11;++bad){
        auto c=Reset(.5f,48,36,0,false);c.ebp=stack-4;W(c.ebp+8,descriptor);
        unsigned expected=42;
        switch(bad){
        case 1:c.ebp=0xfffffffcu;expected=0;break;
        case 2:mem::denied=stack&~0xfffu;expected=0;break;
        case 3:W(c.ebp+8,0xfffffff0u);expected=0;break;
        case 4:mem::denied=descriptor&~0xfffu;expected=0;break;
        case 5:P(descriptor+28,std::numeric_limits<float>::quiet_NaN());expected=0;break;
        case 6:P(descriptor+28,std::numeric_limits<float>::infinity());expected=0;break;
        case 7:P(descriptor+28,-.5f);expected=0;break;
        case 8:P(descriptor+28,2);expected=85;break;
        case 9:W(descriptor+28,1);expected=0;break;
        case 10:expected=0;break;
        }
        const auto saved=c;_mm_setcsr(csr);
        assert(TrialHealthUpgradeColor(c,bad==10?nullptr:g_base)==expected<<24);
        assert(!std::memcmp(&c,&saved,sizeof(c))&&_mm_getcsr()==csr);
    }
    _mm_setcsr(0x1f80);
    std::puts("PASS marker alpha input guards, saturation and host rounding/exception-state isolation");
}
int main(){DrawChecks();GuardChecks();}

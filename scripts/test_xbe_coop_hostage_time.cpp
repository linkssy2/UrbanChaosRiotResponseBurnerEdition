// Original admission, event, ramp, recovery and feedback bodies. LOS/aiming-line,
// event dispatch and audio services are explicit boundaries, not a live scene.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <vector>
#include "kernel/kernel.h"
namespace kernel {static std::array<uint8_t,0x800000> memory{};}
namespace mem {uint32_t denied=0xffffffffu;bool IsCommitted(uint32_t p){return p<kernel::memory.size()&&p!=denied;}}
#include "../common/xbe_coop_hostage_time.cpp"
namespace time_test {
constexpr uint32_t W=0x600000,S=0x601000,L=0x602000,M=0x603000;
constexpr uint32_t P[2]={0x610000,0x612000},LAY[2]={0x620000,0x621000},ID[2]={349,350};
constexpr uint32_t G=0x630000,A=0x632000,SERVICES=0x634000,BUS=0x635000,AUDIO=0x636000,VT=0x640000,C=0x638000,D=0x639000,SP=0x7e0000;
uint32_t admitted[2]{P[0],P[1]};bool los[2]{},line[2]{},throw_retry=false,disturb=false,producer_fixed=false;
unsigned calls=0,cases=0,guards=0,audio_calls=0;std::vector<uint32_t> sight,aim,events;
uint8_t* B(){return kernel::memory.data();}
uint32_t Get(uint32_t a){uint32_t v;std::memcpy(&v,B()+a,4);return v;}
void Put(uint32_t a,uint32_t v){std::memcpy(B()+a,&v,4);}
uint32_t Bits(float f){uint32_t v;std::memcpy(&v,&f,4);return v;}
void Float(uint32_t a,float f){Put(a,Bits(f));}
float Real(uint32_t a){float f;std::memcpy(&f,B()+a,4);return f;}
unsigned Slot(uint32_t id){assert(id==ID[0]||id==ID[1]);return id==ID[1];}
X86Context Context(){X86Context c{};c.ecx=c.esi=G;c.esp=SP;c.fcw=0x37f;c.mxcsr=0x1f80;Put(SP,0xbeef);return c;}
void Seed(unsigned native=1,uint32_t type=0x46d8a0){
    kernel::memory.fill(0);mem::denied=0xffffffffu;throw_retry=disturb=false;calls=audio_calls=0;sight.clear();aim.clear();events.clear();
    trial_coop::Reset();trial_coop::SetAvailable(true);assert(trial_coop::Request());trial_coop::SetActive(true);
    Put(0x509a60,M);Put(M+0x834,ID[native]);Put(0x51c4dc,W);Put(W+4,S);Put(S+4,L);Put(S+8,L+8);
    for(unsigned i=0;i<2;++i){admitted[i]=P[i];los[i]=line[i]=true;Put(P[i],0x46fbe8);Put(P[i]+0x54,ID[i]);Put(P[i]+0x368,i);
        Float(P[i]+0x9c,100);Float(P[i]+0x360,1);Float(P[i]+0x364,1);Put(L+4*i,LAY[i]);Put(LAY[i]+4,P[i]);Put(LAY[i]+0x24,i);}
    Put(G,type);Put(G+0x54,1249);Put(G+0x84,A);Put(G+0x88,A);Float(G+0x114,1);Float(G+0x194,1);Float(G+0x198,.1f);
    Put(0x51c398,SERVICES);Put(SERVICES+8,BUS);Put(W+0x6c,AUDIO);Put(AUDIO,VT);Put(VT+0x50,0xf001);
    Float(W+0x14,.1f);Float(0x4e7d38,1);Put(C+0x54,1241);Put(C+0xd0,D);Put(D+0x1c,1249);
    // ORIGINAL_HOSTAGE_TIME_CONSTANTS
}
}
uint32_t TrialCoopPlayer(unsigned slot){return slot<2?time_test::admitted[slot]:0;}
[[noreturn]]void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
X86_FUNC(sub_000BD640);
X86_FUNC(CoopEvent);
X86_FUNC(OriginalEvent);
// ORIGINAL_HOSTAGE_TIME_DECLARATIONS
X86_FUNC(sub_000E05C0){using namespace time_test;const auto id=Get(ctx.esp+4);sight.push_back(id);
    assert(ctx.ecx==A&&Get(ctx.esp+8)==Bits(-1)&&Get(ctx.esp+12)==1);
    for(unsigned offset:{16u,20u,24u,28u})assert(Get(ctx.esp+offset)==0);
    ctx.eax=los[Slot(id)];ctx.esp+=32;
}
X86_FUNC(sub_000332C0){using namespace time_test;const auto id=Get(ctx.esp+4);aim.push_back(id);
    assert(ctx.ecx==A&&Get(ctx.esp+8)==Bits(500));ctx.eax=line[Slot(id)];ctx.esp+=12;
}
X86_FUNC(sub_001F7AA0){using namespace time_test;assert(Get(ctx.esp+8)==0x44df80&&Get(ctx.esp+12)==0);
    ctx.eax=!Alive(B(),P[Slot(Get(ctx.esp+4))]);ctx.esp+=4;
}
X86_FUNC(sub_00216BD0){ctx.esp+=16;}
X86_FUNC(sub_00216060){ctx.esp+=12;}
X86_FUNC(sub_001F90A0){using namespace time_test;
    if(Get(ctx.esp+8)==Get(0x4f9e4c)){
        assert(ctx.ecx==BUS&&Get(ctx.esp+4)==1249&&Get(ctx.esp+16)==8&&Get(Get(ctx.esp+20))==1241);
        const auto payload=Get(ctx.esp+12);assert((Real(payload)==.25f&&Real(payload+4)==.5f)||(Real(payload)==.1f&&Real(payload+4)==1));
        X86Context delivery=ctx;delivery.ecx=G;delivery.esp-=0x40;Put(delivery.esp,0xbeef);Put(delivery.esp+4,payload);Put(delivery.esp+8,0);
        (producer_fixed?CoopEvent:OriginalEvent)(delivery,base);ctx.esp+=24;return;
    }
    assert(ctx.ecx==BUS&&Get(ctx.esp+8)==Get(0x4f9f6c)&&Get(ctx.esp+16)==4&&Get(Get(ctx.esp+20))==Get(G+0x54));
    const auto id=Get(ctx.esp+4),payload=Get(ctx.esp+12);events.push_back(id);
    X86Context delivery=ctx;delivery.ecx=P[Slot(id)];delivery.esp-=0x40;
    Put(delivery.esp,0xbeef);Put(delivery.esp+4,payload);Put(delivery.esp+8,0);sub_000BD640(delivery,base);ctx.esp+=24;
}
void x86_icall(X86Context& c,uint8_t*,uint32_t fn){using namespace time_test;
    assert(fn==0xf001&&c.ecx==AUDIO);const auto name=Get(c.esp+4);
    assert(name==0x450f80||name==0x457d04||name==0x450f88);++audio_calls;c.esp+=12;
}
// ORIGINAL_HOSTAGE_TIME_BODIES
namespace kernel {
uint32_t CallGuest(X86Context& c,uint32_t address,const uint32_t* args,size_t count){
    using namespace time_test;++calls;assert(address==0x1600d0||address==0x1f90a0);
    const std::array<uint32_t,8> saved{c.eax,c.ecx,c.edx,c.ebx,c.esi,c.edi,c.ebp,c.esp};
    if(disturb){Put(c.esp-0x6000,0x12345678);_mm_setcsr(0x3f80);}
    if(throw_retry)throw std::runtime_error("injected guest boundary failure");
    for(size_t i=count;i;){c.esp-=4;Put(c.esp,args[--i]);}c.esp-=4;Put(c.esp,0xbeef);
    if(address==0x1600d0)CoopQuery(c,B());else sub_001F90A0(c,B());
    assert(c.esp==saved[7]);const auto result=c.eax;
    c.eax=saved[0];c.ecx=saved[1];c.edx=saved[2];c.ebx=saved[3];
    c.esi=saved[4];c.edi=saved[5];c.ebp=saved[6];c.esp=saved[7];return result;
}
}
namespace clockproof {unsigned VerifySharedClock();}
namespace time_test {
void Request(bool fixed,float scale=.25f,float rate=.5f,bool null_payload=false){
    auto c=Context();Float(G+0x300,scale);Float(G+0x304,rate);Put(SP+4,null_payload?0:G+0x300);Put(SP+8,0);
    (fixed?CoopEvent:OriginalEvent)(c,B());assert(c.esp==SP+12);
}
void Producers(){
    for(unsigned native=0;native<2;++native)for(bool death:{false,true})for(bool fixed:{false,true}){
        Seed(native);los[native]=false;producer_fixed=fixed;auto c=Context();c.ecx=c.esi=C;
        if(death)OriginalDeathRequest(c,B());else sub_0011BDC0(c,B());
        const auto target=death?.1f:.25f;assert(Real(G+0x194)==(fixed?target:1.f));
        if(fixed){assert(Real(G+0x198)==(death?1.f:.5f));for(unsigned frame=0;frame<24;++frame){c=Context();OriginalRamp(c,B());}assert(Real(0x4e7d38)==target);}
        assert(Get(M+0x834)==ID[native]);++cases;
    }
}
void Matrix(){
    for(auto type:{0x46d510u,0x46d5f8u,0x46d708u,0x46d7a8u,0x46d8a0u})for(unsigned native=0;native<2;++native)
    for(unsigned mask=0;mask<16;++mask)for(bool fixed:{false,true}){
        Seed(native,type);for(unsigned i=0;i<2;++i){los[i]=(mask>>(i*2))&1;line[i]=(mask>>(i*2+1))&1;}
        const bool expected=(los[native]&&line[native])||(fixed&&los[native^1]&&line[native^1]);
        Request(fixed);assert(Real(G+0x194)==(expected?.25f:1.f));assert(Get(M+0x834)==ID[native]);
        assert(calls==unsigned(fixed&&!(los[native]&&line[native])));++cases;
    }
    // Original request payloads: first/later reload or throw, and successful
    // taker death aftermath. The same native adapter keeps each authored rate.
    for(bool fixed:{false,true})for(bool null_payload:{false,true}){
        Seed();los[1]=false;Request(fixed,.1f,1.f,null_payload);
        assert(Real(G+0x194)==(fixed?(null_payload?.25f:.1f):1.f));
        if(fixed)assert(Real(G+0x198)==(null_payload?.5f:1.f));++cases;
    }
}
void Living(){
    for(unsigned native=0;native<2;++native)for(unsigned dead=0;dead<2;++dead)for(unsigned reason=0;reason<4;++reason){
        Seed(native);if(reason==0)Float(P[dead]+0x9c,0);if(reason==1)Float(P[dead]+0x9c,-10);
        if(reason==2)Put(P[dead]+0x170,12);if(reason==3)Put(P[dead]+0x9c,0x7fc00000);
        Request(true);assert(Real(G+0x194)==.25f);Float(0x4e7d38,.25f);auto c=Context();CoopFeedback(c,B());
        assert(Real(0x4e7d38)==.25f&&events.size()==2);assert(Real(P[0]+0x364)==Real(P[1]+0x364));++cases;
    }
    Seed();Float(P[0]+0x9c,0);Float(P[1]+0x9c,0);Request(true);assert(Real(G+0x194)==1);
    Float(G+0x194,.25f);Float(0x4e7d38,.25f);auto c=Context();CoopFeedback(c,B());
    assert(Real(0x4e7d38)==1&&Real(G+0x194)==1);++cases;
}
void Recovery(){
    for(float target:{.25f,.1f}){
        Seed();los[1]=false;Request(true,target,target==.25f?.5f:1.f);
        for(unsigned frame=0;frame<24;++frame){auto c=Context();OriginalRamp(c,B());}
        assert(Real(0x4e7d38)==target&&audio_calls>0);
        auto c=Context();Put(SP+4,0);Put(SP+8,0);sub_00160840(c,B());assert(Real(G+0x194)==1&&Real(G+0x198)==.5f);
        for(unsigned frame=0;frame<24;++frame){c=Context();OriginalRamp(c,B());}
        assert(Real(0x4e7d38)==1&&B()[G+0x192]==0&&B()[G+0x193]==0);
        for(bool teardown:{false,true}){
            Request(true,target,1);Float(0x4e7d38,target);c=Context();
            if(teardown)OriginalTeardown(c,B());else sub_00160860(c,B());
            assert(Real(0x4e7d38)==1);++cases;
        }
    }
}
void Feedback(){
    for(unsigned native=0;native<2;++native)for(bool fixed:{false,true}){
        Seed(native);Float(G+0x194,.25f);Float(0x4e7d38,.25f);auto c=Context();
        (fixed?CoopFeedback:OriginalFeedback)(c,B());assert(events.size()==(fixed?2u:1u));
        assert(events.back()==ID[native]);if(fixed)assert(events.front()==ID[native^1]);
        assert(std::abs(Real(P[native]+0x364)-.3625f)<1e-6f);
        assert(Real(P[native^1]+0x364)==(fixed?Real(P[native]+0x364):1));
        events.clear();Float(0x4e7d38,1);c=Context();CoopFeedback(c,B());assert(events.empty());++cases;
    }
}
void Guards(){
    for(unsigned test=0;test<20;++test){Seed();los[1]=false;
        switch(test){case 0:trial_coop::SetActive(false);break;case 1:trial_coop::SetAvailable(false);break;
        case 2:Put(G,0x466110);break;case 3:Put(G+0x54,0xffffffff);break;case 4:Put(G+0x5c,1);break;
        case 5:Put(M+0x834,999);break;case 6:Put(S+8,L+4);break;case 7:admitted[0]=0;break;
        case 8:Put(P[0],0);break;case 9:Put(P[0]+0x5c,1);break;case 10:Put(P[0]+0x368,1);break;
        case 11:Put(LAY[0]+4,P[1]);break;case 12:Put(LAY[0]+0x24,1);break;case 13:Put(P[0]+0x54,0xffffffff);break;
        case 14:Put(P[0]+0x54,ID[1]);break;case 15:mem::denied=(SP-0x40)&~0xfffu;break;
        case 16:Float(P[0]+0x9c,0);break;case 17:Put(P[0]+0x40,0x7fc00000);break;
        case 18:Put(P[0]+0x44,0x7f800000);break;case 19:Put(P[0]+0x48,0xff800000);break;}
        auto c=Context();c.eax=0;TrialCoopHostageSlowmoAdmission(c,B());assert((c.eax&255)==0&&calls==0);++guards;
    }
}
void Isolation(){
    for(bool failure:{false,true}){
        Seed();los[1]=false;disturb=true;throw_retry=failure;
        auto c=Context();c.eax=0x12345600;c.ebx=17;c.edi=19;c.cf=c.zf=c.df=1;
        for(unsigned i=0;i<8;++i){c.st[i]=i+.25;c.xmm[i].q[0]=i+3;c.xmm[i].q[1]=i+11;}
        auto expected=c;expected.eax|=failure?0:1;
        std::array<uint8_t,64> saved;std::memset(B()+SP,0xa5,saved.size());std::memcpy(saved.data(),B()+SP,saved.size());
        const auto csr=_mm_getcsr();bool caught=false;
        try{TrialCoopHostageSlowmoAdmission(c,B());}catch(const std::runtime_error&){caught=true;}
        assert(caught==failure&&std::memcmp(&c,&expected,sizeof(c))==0&&_mm_getcsr()==csr);
        assert(std::memcmp(saved.data(),B()+SP,saved.size())==0);
        throw_retry=disturb=false;sight.clear();aim.clear();c=Context();CoopQuery(c,B());
        assert((c.eax&255)==0&&sight==std::vector<uint32_t>{ID[1]}&&aim.empty());++cases;
    }
    for(bool failure:{false,true}){
        Seed();disturb=true;throw_retry=failure;auto c=Context();c.ecx=BUS;
        Put(SP,ID[1]);Put(SP+4,Get(0x4f9f6c));Put(SP+8,G+0x300);Put(SP+12,4);Put(SP+16,G+0x54);Float(G+0x300,.3625f);
        const auto expected=c;const auto csr=_mm_getcsr();std::array<uint8_t,64> saved;
        std::memcpy(saved.data(),B()+SP,saved.size());bool caught=false;
        try{TrialCoopHostageSlowmoFeedback(c,B());}catch(const std::runtime_error&){caught=true;}
        assert(caught==failure&&std::memcmp(&c,&expected,sizeof(c))==0&&_mm_getcsr()==csr);
        assert(std::memcmp(saved.data(),B()+SP,saved.size())==0);
        assert(events.size()==(failure?0u:1u));if(!failure)assert(events.front()==ID[0]&&Real(P[0]+0x364)==.3625f);
        throw_retry=disturb=false;sight.clear();los[1]=false;c=Context();CoopQuery(c,B());
        assert((c.eax&255)==0&&sight==std::vector<uint32_t>{ID[1]});++cases;
    }
}
}
int main(){using namespace time_test;Producers();Matrix();Living();Recovery();Feedback();Guards();Isolation();
    std::printf("hostage time: %u native cases, %u guards, %u shared two-actor frames passed\n",cases,guards,clockproof::VerifySharedClock());}

// Native shared-clock proof only. Actor callbacks observe both original getters;
// physics, animation, audio and event services remain explicit test boundaries.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include "kernel/kernel.h"

namespace clockproof {
static std::array<uint8_t,0x800000> memory{};
constexpr uint32_t W=0x600000,L=0x610000,Q0=0x620000,Q1=0x621000;
constexpr uint32_t LIST=0x622000,BUCKET=0x623000,NODE=0x624000,REC=0x625000;
constexpr uint32_t P[2]={0x630000,0x632000},VT=0x640000,O=0x650000,OVT=0x651000;
constexpr uint32_t DEAD=0x660000,SP=0x7e0000;
static uint32_t expected_scaled,expected_raw;
static unsigned updates[2],animations[2],postupdates[2],cases;
static float elapsed[2];
uint8_t* B(){return memory.data();}
uint32_t Get(uint32_t a){uint32_t v;std::memcpy(&v,B()+a,4);return v;}
void Put(uint32_t a,uint32_t v){std::memcpy(B()+a,&v,4);}
uint32_t Bits(float f){uint32_t v;std::memcpy(&v,&f,4);return v;}
float Real(uint32_t a){float v;std::memcpy(&v,B()+a,4);return v;}
unsigned Slot(uint32_t actor){assert(actor==P[0]||actor==P[1]);return actor==P[1];}

// CLOCK_DECLARATIONS
X86_FUNC(ClockStub_00211B80){ctx.esp+=4;}
X86_FUNC(ClockStub_002145D0){ctx.esp+=4;}
X86_FUNC(ClockStub_00216040){
    assert(base==B()&&Get(ctx.esp)==0x209237&&Get(ctx.esp+4)==expected_scaled);
    ++animations[Slot(ctx.ecx)];ctx.esp+=8;
}
#define CLOCK_UNREACHED(name) X86_FUNC(name){assert(false);}
CLOCK_UNREACHED(ClockStub_001F8FC0)
CLOCK_UNREACHED(ClockStub_001F90A0)
CLOCK_UNREACHED(ClockStub_0020A140)
CLOCK_UNREACHED(ClockStub_00210CA0)
CLOCK_UNREACHED(ClockStub_00221A10)
CLOCK_UNREACHED(ClockStub_002D5CCD)
#undef CLOCK_UNREACHED

float ReadClock(X86Context& ctx,uint8_t* base,X86Func* getter){
    const auto saved_stack=ctx.esp,saved_top=ctx.top;
    ctx.esp-=4;Put(ctx.esp,0xf004);getter(ctx,base);
    const float value=static_cast<float>(ctx.st[ctx.top]);x87_pop(ctx);
    assert(ctx.esp==saved_stack&&ctx.top==saved_top);return value;
}
void ClockIcall(X86Context& ctx,uint8_t* base,uint32_t target){
    assert(base==B());
    if(target==0xf001){
        const auto slot=Slot(ctx.ecx);assert(Get(ctx.esp)==0x20922b);
        const float scaled=ReadClock(ctx,base,ClockGetter_001F9010);
        const float raw=ReadClock(ctx,base,ClockGetter_001F9020);
        assert(Bits(scaled)==expected_scaled&&Bits(raw)==expected_raw);
        elapsed[slot]+=scaled;++updates[slot];ctx.esp+=4;return;
    }
    if(target==0xf002){
        const auto caller=Get(ctx.esp);assert(caller==0x209287||caller==0x20929a);
        ++postupdates[Slot(ctx.ecx)];ctx.esp+=4;return;
    }
    assert(target==0xf003&&ctx.ecx==O);
    assert((Get(ctx.esp)==0x2092e9||Get(ctx.esp)==0x209308)&&Get(ctx.esp+4)==expected_scaled);
    ctx.esp+=8;
}

// CLOCK_NATIVE_BODIES

void Seed(){
    memory.fill(0);for(unsigned i=0;i<2;++i){updates[i]=animations[i]=postupdates[i]=0;elapsed[i]=0;}
    Put(0x51c4dc,W);Put(W+4,L);Put(W+0x10,0x80);
    Put(L+0x2054,O);Put(L+0x205c,Q0);Put(Q0,Q0);
    Put(L+0x2068,Q1);Put(Q1,Q1);Put(L+0x30,LIST);Put(LIST+0x20,BUCKET);
    Put(BUCKET,NODE);Put(NODE,REC);Put(NODE+4,NODE+0x10);Put(NODE+0x10,REC+0x10);
    Put(REC+4,P[0]);Put(REC+0x14,P[1]);Put(L+0x38,DEAD);
    Put(L+0x10,O);Put(L+0x2070,O);Put(O,OVT);Put(OVT+0xc,0xf003);
    for(auto actor:P)Put(actor,VT);Put(VT+0x30,0xf001);Put(VT+0x34,0xf002);
}

unsigned VerifySharedClock(){
    cases=0;
    for(float raw:{1.f/30,1.f/60,1.f/240})for(float scale:{.1f,.25f,.5f,1.f}){
        Seed();Put(0x4e7d38,Bits(scale));float expected_elapsed=0;
        for(unsigned frame=1;frame<=8;++frame){
            X86Context c{};c.esp=SP;c.fcw=0x37f;c.mxcsr=0x1f80;c.xmm[0].f[0]=raw;
            // Actual 5D3BC..5D3D3 instructions, including the one multiplication.
            ClockScale(c,B());expected_scaled=Get(SP+0x14);expected_raw=Get(SP+0x30);
            const float scaled=Real(SP+0x14);assert(Bits(scaled)==Bits(raw*scale)&&expected_raw==Bits(raw));
            const auto global=Get(0x4e7d38);const auto before_time=Real(W+8);
            c.ecx=W;c.esp=SP;Put(SP,0xf005);Put(SP+4,expected_scaled);Put(SP+8,expected_raw);
            ClockOriginalWorld(c,B());expected_elapsed+=scaled;
            assert(c.esp==SP+12&&Real(W+8)==before_time+scaled&&Get(0x4e7d38)==global);
            assert(Get(W+0x14)==expected_scaled&&Get(W+0x18)==expected_raw);
            for(unsigned slot=0;slot<2;++slot){
                assert(updates[slot]==frame&&animations[slot]==frame&&postupdates[slot]==frame);
                assert(elapsed[slot]==expected_elapsed);
            }
            assert(elapsed[0]==elapsed[1]);++cases;
        }
    }
    return cases;
}
}

#ifdef CLOCKPROOF_STANDALONE
int main(){std::printf("original shared clock: %u two-actor frames passed\n",clockproof::VerifySharedClock());}
#endif

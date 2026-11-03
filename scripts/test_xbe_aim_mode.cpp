// Production-derived zoom controller, input query and camera-state update.
// Only inventory lookup, physical button bytes and sound/effect sinks are stubs.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
namespace aim_test {
std::array<uint8_t,0x800000> memory{};
unsigned modes[2]{};
constexpr uint32_t P=0x600000,Q=0x602000,W=0x610000,L=0x620000,V=0x630000;
constexpr uint32_t M=0x640000,PORTS=0x641000,BUTTONS=0x642000,I=0x650000,D=0x660000,S=0x7f0000;
bool committed=true;
uint8_t* Data(){return memory.data();}
uint32_t Word(uint32_t at){uint32_t v;std::memcpy(&v,Data()+at,4);return v;}
void Put(uint32_t at,uint32_t v){std::memcpy(Data()+at,&v,4);}
void Real(uint32_t at,float v){std::memcpy(Data()+at,&v,4);}
std::vector<uint32_t> events;
unsigned queries[2]{},query_modes[2]{};
}
namespace mem {bool IsCommitted(uint32_t page){return aim_test::committed&&page<aim_test::memory.size();}}
namespace trial_controls {unsigned AimMode(unsigned slot){assert(slot<2);return aim_test::modes[slot];}}
uint32_t TrialCoopPlayer(unsigned slot){return slot<2?(slot?aim_test::Q:aim_test::P):0;}
#include "../common/xbe_aim_mode.cpp"
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
X86_FUNC(NativeQuery);
X86_FUNC(sub_00041F80){
    using namespace aim_test;assert(ctx.ecx==P+0x498||ctx.ecx==Q+0x498);
    ctx.eax=I+(ctx.ecx==Q+0x498?0x100:0);ctx.esp+=4;
}
X86_FUNC(sub_00041FA0){sub_00041F80(ctx,base);}
X86_FUNC(sub_000442D0){using namespace aim_test;assert(Word(ctx.esp+4)<27);ctx.eax=D+Word(ctx.esp+4)*0x100;ctx.esp+=4;}
X86_FUNC(sub_000643E0){ctx.eax=aim_test::M;ctx.esp+=4;}
X86_FUNC(sub_00216BD0){
    using namespace aim_test;assert(ctx.ecx==P||ctx.ecx==Q);
    events.push_back(ctx.ecx);events.push_back(Word(ctx.esp+8));ctx.esp+=16;
}
X86_FUNC(sub_00055EC0){
    using namespace aim_test;assert(ctx.ecx==W);
    events.push_back(0x55ec0);events.push_back(Word(ctx.esp+16));ctx.eax=D;ctx.esp+=28;
}
X86_FUNC(sub_000490D0){ctx.esp+=4;}
X86_FUNC(sub_00055BF0){
    using namespace aim_test;const auto slot=Word(ctx.esp+4),action=Word(ctx.esp+8),out=Word(ctx.esp+12);
    assert(slot<2&&action==0x10);Put(out,0);ctx.eax=1;ctx.esp+=16;
}
X86_FUNC(sub_00055CD0){
    using namespace aim_test;const auto port=Word(ctx.esp+4),binding=Word(ctx.esp+8);
    assert((port==PORTS||port==PORTS+0xd0)&&binding==0);ctx.eax=BUTTONS+(port-PORTS)/0xd0;ctx.esp+=12;
}
void x86_icall(X86Context& c,uint8_t* base,uint32_t target){
    using namespace aim_test;assert(target==0x567f0&&c.ecx==W);
    const unsigned slot=Word(c.esp+4),mode=Word(c.esp+12);assert(slot<2&&mode<2);
    ++queries[slot];query_modes[slot]=mode;NativeQuery(c,base);
}
// NATIVE_AIM_CONTRACTS
namespace aim_test {
X86Context Context(uint32_t owner=P){X86Context c{};c.ecx=owner;c.esp=S;c.ebx=17;c.esi=19;c.edi=23;c.top=0;Put(S,0x123456);return c;}
void Seed(bool coop=true){
    memory.fill(0);events.clear();committed=true;
    modes[0]=modes[1]=0;queries[0]=queries[1]=query_modes[0]=query_modes[1]=0;
    trial_coop::SetActive(coop);
    for(unsigned slot=0;slot<2;++slot){
        aim_mode::input[slot]=0;aim_mode::players[slot]={};
        const uint32_t owner=slot?Q:P,id=100+slot;
        Put(owner,0x46fbe8);Put(owner+0x54,id);Put(owner+0x368,slot);Real(owner+0x9c,100);
        Put(owner+0x4fc,21);Put(I+slot*0x100+4,21);
        Put(L+0x3c+(id&1023)*8,id);Put(L+0x40+(id&1023)*8,owner);
        Data()[M+12+slot]=uint8_t(slot);Data()[M+16+slot]=uint8_t(slot);Data()[M+20+slot]=1;
    }
    Put(0x51c4dc,W);Put(W+4,L);Put(W,V);Put(V+8,0x567f0);Put(W+0x20,PORTS);Put(0x5096e0,M);
    Put(0x56cc8,0x56872);Put(0x56ccc,0x568b0);Real(0x44c174,0);Real(0x44bf94,1);
    for(unsigned weapon=0;weapon<27;++weapon){
        Real(D+weapon*0x100+0x90,1); // Native BE8EB rejects a zero zoom factor.
        Real(D+weapon*0x100+0xdc,weapon>=20&&weapon<=22?1.f:0.f);
    }
}
void Weapon(unsigned slot,unsigned weapon){const uint32_t owner=slot?Q:P;Put(owner+0x4fc,weapon);Put(I+slot*0x100+4,weapon);}
void Step(unsigned slot,bool down,bool edge=false,bool cycle=false,bool allowed=true){
    const auto owner=slot?Q:P;Data()[BUTTONS+slot]=uint8_t((down?1:0)|(edge?2:0));
    TrialAimInput(slot,cycle,allowed);auto c=Context(owner);TrialAimUpdate(c,Data());
    HeldZoom(c,Data());assert(c.esp==S+4);
    c=Context(owner);NativeCameraZoom(c,Data());assert(c.esp==S+4);
}
void ToggleParity(){
    unsigned comparisons=0;
    for(bool coop:{false,true})for(unsigned slot=0;slot<(coop?2u:1u);++slot)
    for(unsigned weapon:{1u,20u,21u,22u,26u})for(unsigned zoom=0;zoom<3;++zoom)
    for(unsigned buttons=0;buttons<4;++buttons)for(unsigned blocked=0;blocked<4;++blocked){
        Seed(coop);Weapon(slot,weapon);const uint32_t owner=slot?Q:P;
        Put(owner+0x438,zoom);Data()[BUTTONS+slot]=uint8_t(buttons);
        if(blocked==1)Data()[owner+0x848]=1;
        if(blocked==2)Data()[owner+0x2ac]=1;
        if(blocked==3)Real(D+weapon*0x100+0x90,0);
        auto original=Context(owner);const auto initial=original;const std::vector<uint8_t> bytes(memory.begin(),memory.end());
        OriginalZoom(original,Data());const std::vector<uint8_t> expected(memory.begin(),memory.end());const auto expected_events=events;const unsigned csr=_mm_getcsr();
        std::memcpy(Data(),bytes.data(),bytes.size());events.clear();auto adapted=initial;TrialAimUpdate(adapted,Data());HeldZoom(adapted,Data());
        assert(std::memcmp(&original,&adapted,sizeof original)==0&&!std::memcmp(Data(),expected.data(),expected.size())&&events==expected_events&&_mm_getcsr()==csr);
        if(queries[slot])assert(query_modes[slot]==1);++comparisons;
    }
    std::printf("Toggle: %u complete native context/RAM/event comparisons passed\n",comparisons);
}
void Holds(){
    for(bool coop:{false,true})for(unsigned slot=0;slot<(coop?2u:1u);++slot)
    for(unsigned weapon:{1u,20u,21u,22u}){
        Seed(coop);modes[slot]=1;Weapon(slot,weapon);const auto owner=slot?Q:P,other=slot?P:Q;
        Step(slot,false);Step(slot,true,true);assert(Word(owner+0x438)==1&&query_modes[slot]==0);
        const auto sound_count=events.size();
        for(unsigned i=0;i<60;++i)Step(slot,true);
        assert(Word(owner+0x438)==1&&events.size()==sound_count&&Word(other+0x438)==0);
        Step(slot,true,false,true);
        assert(Word(owner+0x438)==((weapon==21||weapon==22)?2u:1u));
        for(unsigned i=0;i<10;++i)Step(slot,true,false,true);
        assert(Word(owner+0x438)==((weapon==21||weapon==22)?2u:1u));
        Step(slot,true);Step(slot,true,false,true);assert(Word(owner+0x438)==1);
        Step(slot,false);assert(Word(owner+0x438)==0&&Word(owner+0x1cc)==0&&Word(owner+0x35c)==0);
        assert(Data()[owner+0x2d0]==1&&Data()[owner+0x2d1]==1);
    }
    Seed();modes[0]=modes[1]=1;Step(0,false);Step(1,false);Step(0,true);Step(1,true);
    Step(1,true,false,true);assert(Word(P+0x438)==1&&Word(Q+0x438)==2);
    Step(0,false);assert(Word(P+0x438)==0&&Word(Q+0x438)==2);
    Step(1,false);assert(Word(Q+0x438)==0);
    // The shield itself must not reject zoom (the retail controller permits it).
    Seed();modes[0]=1;Data()[P+0x434]=1;Step(0,false);Step(0,true);assert(Word(P+0x438)==1);
    // BE8FF intentionally rejects weapon 26; holding/cycling must not add ADS.
    Seed();modes[0]=1;Weapon(0,26);Step(0,false);Step(0,true);Step(0,true,false,true);
    assert(Word(P+0x438)==0&&Word(P+0x1cc)==0);
}
void CancelAndIdentity(){
    for(unsigned reason=0;reason<14;++reason){
        Seed();modes[0]=1;Step(0,false);Step(0,true);assert(Word(P+0x438)==1);
        switch(reason){
            case 0:TrialAimInput(0,false,false);TrialAimInput(0,false,true);break; // entire paused interval
            case 1:Weapon(0,22);break;
            case 2:Put(P+0x170,12);break;
            case 3:Real(P+0x9c,0);break;
            case 4:Put(P+0x170,7);break;
            case 5:Put(P+0x170,3);break;
            case 6:Put(P+0x170,11);break;
            case 7:Put(W+0x10,0x800);break;
            case 8:Data()[P+0x2ac]=1;break;
            case 9:Data()[P+0x848]=1;break;
            case 10:Real(D+21*0x100+0x90,0);break;
            case 11:modes[0]=0;{auto c=Context();TrialAimUpdate(c,Data());}modes[0]=1;break;
            case 12:Put(P+0x54,102);Put(L+0x3c+102*8,102);Put(L+0x40+102*8,P);break;
            case 13:Put(P+0x170,13);break;
        }
        Step(0,true,false,reason==10);assert(Word(P+0x438)==0);
        Put(P+0x170,0);Real(P+0x9c,100);Put(W+0x10,0);Data()[P+0x848]=Data()[P+0x2ac]=0;Real(D+21*0x100+0x90,1);
        Step(0,true);assert(Word(P+0x438)==0);Step(0,false);Step(0,true);assert(Word(P+0x438)==1);
    }
    // Registered object identity is mandatory; unrelated actors retain Toggle.
    for(unsigned fault=0;fault<6;++fault){
        Seed();modes[0]=1;Put(P+0x438,1);
        if(fault==0)Put(P,0x46fbe9);
        if(fault==1)Put(P+0x5c,1);
        if(fault==2)Put(P+0x54,0xffffffffu);
        if(fault==3)Put(L+0x3c+100*8,101);
        if(fault==4)Put(L+0x40+100*8,Q);
        if(fault==5)committed=false;
        auto c=Context();const std::vector<uint8_t> bytes(memory.begin(),memory.end());TrialAimUpdate(c,Data());assert(!std::memcmp(Data(),bytes.data(),bytes.size()));
        TrialAimModeScope scope(c,Data());assert(scope.QueryMode()==1);committed=true;
    }
    Seed(false);modes[1]=1;auto c=Context(Q);TrialAimModeScope unrelated(c,Data());assert(unrelated.QueryMode()==1);
}
}
int main(){
    aim_test::ToggleParity();aim_test::Holds();aim_test::CancelAndIdentity();
    std::puts("Held aim: native input/zoom/camera, scoped levels, two-player isolation, shield, cancellation and identity passed");
}

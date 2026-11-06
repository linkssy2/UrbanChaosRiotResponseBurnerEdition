// Execute complete retail hazard loops, registration/count/indexed lookup and
// isolated original burn-cadence/health/age instructions. The damage slice
// starts after difficulty, shield and immunity admission. Audio and obstruction
// are explicit service boundaries; full C8E90/D2AB0 behavior is not claimed.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <vector>
#include "kernel/kernel.h"
namespace kernel {static std::array<uint8_t,0x800000> memory{};}
namespace mem {
uint32_t denied_page=0xffffffffu;
bool IsCommitted(uint32_t page){return page!=denied_page&&page<kernel::memory.size();}
}
#include "../common/xbe_coop_hazards.cpp"
namespace hazards_test {
constexpr uint32_t world=0x600000,state=0x601000,layers=0x602000,manager=0x603000;
constexpr uint32_t owner[2]={0x610000,0x612000},layer[2]={0x620000,0x621000};
constexpr uint32_t hazard=0x630000,manager_table=0x640000,stack=0x7f0000,damage_stack=0x7e0000;
constexpr uint32_t ids[2]={349,350},returns[3]={0x18ef41,0x191247,0x1a5c92};
uint32_t tracked[2]={owner[0],owner[1]};
bool repair=true,cadence=false,multiplayer=false,blocked[2]={false,false};
double delta=1.0/16.0;
unsigned sounds=0,obstructions=0,guards=0,cases=0;
struct Hit {unsigned slot;uint32_t amount;std::array<uint8_t,54> packet;};
std::vector<Hit> hits;
std::vector<std::array<uint32_t,2>> lookups;
std::vector<std::array<uint32_t,4>> sound_packets;
uint8_t* Bytes(){return kernel::memory.data();}
uint32_t Word(uint32_t a){uint32_t v;std::memcpy(&v,Bytes()+a,4);return v;}
void Put(uint32_t a,uint32_t v){std::memcpy(Bytes()+a,&v,4);}
float Real(uint32_t a){float v;std::memcpy(&v,Bytes()+a,4);return v;}
void Float(uint32_t a,float v){std::memcpy(Bytes()+a,&v,4);}
void Lookup(const X86Context& c){lookups.push_back({Word(c.esp+4),c.eax});}
unsigned Slot(uint32_t actor){assert(actor==owner[0]||actor==owner[1]);return actor==owner[1];}
uint32_t Resolve(uint32_t id){for(unsigned i=0;i<2;++i)if(id==ids[i])return owner[i];return 0;}
void OriginalRegister(unsigned);
void ApplyCadence(unsigned,uint32_t,uint32_t);
X86Context Context(){
    X86Context c{};c.ecx=manager;c.esp=stack;c.eax=ids[1];
    c.ebx=17;c.ebp=23;c.esi=31;c.edi=47;c.edx=59;
    c.cf=c.zf=c.sf=c.of=c.pf=c.df=1;c.fcw=0x37f;c.mxcsr=0x1f80;
    for(unsigned i=0;i<8;++i){c.xmm[i].q[0]=i+1;c.xmm[i].q[1]=i+9;c.st[i]=i+.25;}
    return c;
}
void Setup(unsigned inside=3){
    kernel::memory.fill(0);mem::denied_page=0xffffffffu;
    trial_coop::Reset();trial_coop::SetAvailable(true);assert(trial_coop::Request());trial_coop::SetActive(true);
    Put(0x51c4dc,world);Put(world+4,state);Put(state+4,layers);Put(state+8,layers+8);
    Put(0x509a60,manager);Put(manager,manager_table);Put(manager_table+0x20,0xf1000);
    Put(manager+0x834,0xffffffffu);Put(0x46fbe8+0x6c,0xf1010);
    // ORIGINAL_COOP_HAZARD_CONSTANTS
    for(unsigned i=0;i<2;++i){
        tracked[i]=owner[i];Put(owner[i],0x46fbe8);Put(owner[i]+0x54,ids[i]);Put(owner[i]+0x368,i);
        Put(layers+i*4,layer[i]);Put(layer[i]+4,owner[i]);Put(layer[i]+0x24,i);
        Float(owner[i]+0x40,(inside&(1u<<i))?1.f+.5f*i:1000.f);
        Float(owner[i]+0x44,1);Float(owner[i]+0x48,1);Float(owner[i]+0x9c,1000);
        Float(owner[i]+0x234,.75f);Put(owner[i]+0x94,100+i);blocked[i]=false;
    }
    // Fire/electricity boxes and sphere deliberately share the same inside set.
    for(unsigned axis=0;axis<3;++axis){
        Float(hazard+0x10c+axis*4,0);Float(hazard+0x118+axis*4,2);
        Float(hazard+0xe0+axis*4,0);Float(hazard+0xec+axis*4,2);
    }
    Float(hazard+0x88,9);Put(hazard+0x84,77);Put(hazard+0x8c,88);Put(hazard+0x90,99);
    repair=true;cadence=false;multiplayer=false;sounds=obstructions=0;hits.clear();lookups.clear();sound_packets.clear();
    OriginalRegister(0);OriginalRegister(1);
    assert(Word(manager+0x10)==2&&Word(manager+0x834)==ids[1]);
}
}
uint32_t TrialCoopPlayer(unsigned slot){return slot<2?hazards_test::tracked[slot]:0;}
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
X86_FUNC(sub_001F8FC0){ctx.eax=hazards_test::Resolve(hazards_test::Word(ctx.esp+4));ctx.esp+=4;}
X86_FUNC(sub_00207D70){assert(ctx.ecx==hazards_test::world);ctx.eax=hazards_test::Resolve(hazards_test::Word(ctx.esp+4));ctx.esp+=8;}
X86_FUNC(sub_001F9010){x87_push(ctx,hazards_test::delta);ctx.esp+=4;}
X86_FUNC(sub_0006BA40){ctx.esp+=4;} // Original debug formatting, cdecl.
X86_FUNC(sub_00216BD0){
    using namespace hazards_test;
    sound_packets.push_back({Slot(ctx.ecx),Word(ctx.esp+4),Word(ctx.esp+8),Word(ctx.esp+12)});
    ++sounds;ctx.esp+=16;
}
X86_FUNC(sub_00216020){ctx.eax=0xf2000;ctx.esp+=12;} // Sound-bank lookup; the pending event/flag stay for 216BD0.
X86_FUNC(sub_0020A690){ctx.eax=0;ctx.esp+=12;} // Named incapacitated predicate.
X86_FUNC(sub_000E90C0){
    using namespace hazards_test;const unsigned slot=Slot(Word(ctx.esp+12));
    assert(Word(ctx.esp+16)==100+slot);++obstructions;ctx.eax=blocked[slot];ctx.esp+=4;
}
void x86_icall(X86Context& c,uint8_t*,uint32_t target){
    using namespace hazards_test;
    if(target==0xf1000){assert(c.ecx==manager);c.eax=multiplayer;c.esp+=4;return;}
    assert(target==0xf1010);const unsigned slot=Slot(c.ecx);
    const uint32_t amount=Word(c.esp+4),packet=Word(c.esp+8);
    Hit hit{slot,amount,{}};std::memcpy(hit.packet.data(),Bytes()+packet,hit.packet.size());hits.push_back(hit);
    assert(Word(packet)==0xffffffffu&&(Word(packet+4)==12||Word(packet+4)==13));
    assert(Word(packet+8)==0&&!Bytes()[packet+52]&&!Bytes()[packet+53]);
    if(cadence)ApplyCadence(slot,amount,packet);
    c.esp+=12;
}
extern "C" void sub_001F8FC0(X86Context&,uint8_t*);
extern "C" void sub_0004A840(X86Context&,uint8_t*);
extern "C" void sub_0004ACC0(X86Context&,uint8_t*);
extern "C" void sub_0004ACD0(X86Context&,uint8_t*);
extern "C" void sub_002180D0(X86Context&,uint8_t*);
// ORIGINAL_COOP_HAZARD_BODIES
namespace hazards_test {
void OriginalRegister(unsigned slot){auto c=Context();Put(stack+4,ids[slot]);sub_0004A780(c,Bytes());assert(c.esp==stack+8);}
void ApplyCadence(unsigned slot,uint32_t amount,uint32_t packet){
    auto c=Context();c.esp=damage_stack;c.esi=owner[slot];c.edi=packet;
    Put(damage_stack+0x1c,amount);Bytes()[damage_stack+0x2a]=1;
    OriginalBurnCadence(c,Bytes());assert(c.esp==damage_stack&&c.top==0);
}
void Age(unsigned slot){auto c=Context();c.esp=damage_stack;c.ebx=owner[slot];OriginalBurnAge(c,Bytes());assert(c.esp==damage_stack&&c.top==0);}
void Run(unsigned family){
    auto c=Context();c.ecx=hazard;const auto before=c;
    if(family==0)sub_0018EF10(c,Bytes());
    else if(family==1)sub_00191210(c,Bytes());
    else sub_001A5C60(c,Bytes());
    assert(c.esp==stack+4&&c.ebx==before.ebx&&c.ebp==before.ebp&&c.esi==before.esi&&c.edi==before.edi&&c.top==0);
}
void Routing(){
    unsigned old_duplicate_hits=0;
    for(unsigned family=0;family<3;++family)for(unsigned inside=0;inside<4;++inside){
        Setup(inside);repair=false;Run(family);
        assert(lookups.size()==2&&lookups[0][0]==0&&lookups[1][0]==1);
        assert(lookups[0][1]==ids[1]&&lookups[1][1]==ids[1]);
        assert(hits.size()==((inside&2)?2:0));for(const auto& hit:hits)assert(hit.slot==1);
        old_duplicate_hits+=unsigned(hits.size());
        Setup(inside);Run(family);const auto fixed=hits;
        assert(lookups.size()==2&&lookups[0][1]==ids[0]&&lookups[1][1]==ids[1]);
        assert(fixed.size()==unsigned(bool(inside&1))+unsigned(bool(inside&2)));
        unsigned cursor=0;
        for(unsigned slot=0;slot<2;++slot){
            // Compare every routed packet to an independent unchanged retail
            // single-player loop targeting that same actor and location.
            Setup(inside);trial_coop::Reset();Put(manager+0x10,1);Put(manager+0x834,ids[slot]);Run(family);
            assert(lookups.size()==1&&lookups[0][1]==ids[slot]);
            if(inside&(1u<<slot)){
                assert(hits.size()==1&&fixed[cursor].slot==slot);
                assert(hits[0].amount==fixed[cursor].amount&&hits[0].packet==fixed[cursor].packet);++cursor;
            }else assert(hits.empty());
        }
        ++cases;
    }
    assert(old_duplicate_hits==12);
    std::printf("original_duplicate_hits=%u original_missed_primary_cases=6\n",old_duplicate_hits);
}
void HazardSemantics(){
    Setup();multiplayer=true;blocked[0]=true;Run(2);
    assert(obstructions==2&&hits.size()==1&&hits[0].slot==1);++cases;
    Setup();multiplayer=true;blocked[1]=true;Run(2);
    assert(obstructions==2&&hits.size()==1&&hits[0].slot==0);++cases;
    // The box uses its original second sample (+40); the sphere uses +170.
    Setup(1);Float(owner[0]+0x44,-39);Run(0);assert(hits.size()==1&&hits[0].slot==0);++cases;
    Setup(1);Float(owner[0]+0x44,-169);Run(2);assert(hits.size()==1&&hits[0].slot==0);++cases;
    Setup(1);Float(owner[0]+0x44,-39);Run(1);assert(hits.empty());++cases;
    Setup();Run(1);assert(hits.size()==2&&hits[0].amount==Word(0x44c178)&&sounds==2);
    assert((sound_packets==std::vector<std::array<uint32_t,4>>{{0,0xf2000,0x4744a4,0},{1,0xf2000,0x4744a4,0}}));++cases;
    Setup();Bytes()[hazard+0x121]=1;Run(1);
    assert(hits.size()==2&&hits[0].amount==Word(0x44db94)&&hits[1].amount==Word(0x44db94)&&sounds==2);++cases;
}
void Cadence(){
    std::array<float,2> fixed_health{},fixed_timer{},old_health{};
    for(bool fixed:{true,false}){
        Setup();repair=fixed;cadence=true;
        for(unsigned frame=0;frame<32;++frame){Age(0);Age(1);Run(0);}
        if(fixed)for(unsigned i=0;i<2;++i){fixed_health[i]=Real(owner[i]+0x9c);fixed_timer[i]=Real(owner[i]+0x234);}
        else for(unsigned i=0;i<2;++i)old_health[i]=Real(owner[i]+0x9c);
    }
    assert(fixed_health[0]==fixed_health[1]&&fixed_health[0]<1000&&fixed_timer[0]==fixed_timer[1]);
    assert(old_health[0]==1000&&old_health[1]<fixed_health[1]);
    for(unsigned family=0;family<3;++family){
        Setup();cadence=true;Float(owner[0]+0x234,0);Float(owner[1]+0x234,.5f);Run(family);
        const float amount=family==1?100.f:50.f;
        assert(Real(owner[0]+0x9c)==1000-amount&&Real(owner[0]+0x234)==.75f);
        assert(Real(owner[1]+0x9c)==1000&&Real(owner[1]+0x234)==.375f);
        ++cases;
    }
    Setup();cadence=true;Bytes()[hazard+0x121]=1;
    Float(owner[0]+0x234,0);Float(owner[1]+0x234,0);Run(1);
    assert(Real(owner[0]+0x9c)==0&&Real(owner[1]+0x9c)==0);++cases;
    std::printf("fixed_health=%.0f,%.0f original_health=%.0f,%.0f\n",fixed_health[0],fixed_health[1],old_health[0],old_health[1]);
}
void Guards(){
    for(unsigned caller:returns)for(unsigned slot=0;slot<2;++slot){
        Setup();auto c=Context();Put(stack,caller);Put(stack+4,slot);const auto before=c;
        const std::vector<uint8_t> memory(Bytes(),Bytes()+kernel::memory.size());const auto csr=_mm_getcsr();TrialCoopHazardPlayer(c,Bytes());
        auto expected=before;expected.eax=ids[slot];assert(!std::memcmp(&c,&expected,sizeof c));
        assert(!std::memcmp(Bytes(),memory.data(),memory.size())&&_mm_getcsr()==csr);++guards;
    }
    for(unsigned which=0;which<29;++which){
        Setup();auto c=Context();Put(stack,returns[0]);Put(stack+4,0);
        switch(which){
        case 0:trial_coop::Reset();break;
        case 1:trial_coop::SetActive(false);break;
        case 2:trial_coop::SetAvailable(false);break;
        case 3:Put(stack,0x168b00);break; // Medic/other campaign callers remain original.
        case 4:Put(stack+4,2);break;
        case 5:Put(manager+0x10,1);break;
        case 6:Put(manager+0x10,3);break;
        case 7:c.ecx=manager+4;break;
        case 8:c.eax=ids[0];break; // Foreign intermediate override is not replaced.
        case 9:Put(manager+0x834,123);c.eax=123;break;
        case 10:Put(0x51c4dc,0);break;
        case 11:Put(world+4,0);break;
        case 12:Put(state+8,layers+4);break;
        case 13:Put(state+4,0xfffffffcu);break;
        case 14:tracked[0]=0;break;
        case 15:tracked[1]=0;break;
        case 16:Put(owner[0],0);break;
        case 17:Put(owner[1],0);break;
        case 18:Put(owner[0]+0x5c,1);break;
        case 19:Put(owner[1]+0x5c,1);break;
        case 20:Put(owner[0]+0x368,1);break;
        case 21:Put(owner[1]+0x368,0);break;
        case 22:Put(owner[0]+0x54,0xffffffffu);break;
        case 23:Put(owner[1]+0x54,ids[0]);break;
        case 24:Put(layer[0]+4,owner[1]);break;
        case 25:Put(layer[1]+0x24,0);break;
        case 26:mem::denied_page=owner[1];break;
        case 27:c.esp=0xfffffffcu;break;
        case 28:Put(stack,0x18ef42);break;
        }
        const auto before=c;const std::vector<uint8_t> memory(Bytes(),Bytes()+kernel::memory.size());const auto csr=_mm_getcsr();
        TrialCoopHazardPlayer(c,Bytes());assert(!std::memcmp(&c,&before,sizeof c));
        assert(!std::memcmp(Bytes(),memory.data(),memory.size())&&_mm_getcsr()==csr);++guards;
    }
}
}
int main(){
    _mm_setcsr(0x1f80);hazards_test::Routing();hazards_test::HazardSemantics();hazards_test::Cadence();hazards_test::Guards();
    std::printf("{\"passed\":true,\"hazard_cases\":%u,\"guards\":%u}\n",hazards_test::cases,hazards_test::guards);
}

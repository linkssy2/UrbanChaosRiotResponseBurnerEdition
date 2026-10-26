// Execute the pinned original MaskPickup update, native distance/bit commit,
// initialization/finish and popup dispatch. Entity lookup, effect playback and
// localized text storage are explicit boundaries; no helper emulates the commit.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cstdio>
#include <limits>
#include "kernel/kernel.h"
namespace kernel {static std::array<uint8_t,0x800000> memory{};}
namespace mem {bool IsCommitted(uint32_t p){return p<kernel::memory.size();}}
#include "../common/xbe_coop_collectibles.cpp"
constexpr uint32_t P[2]={0x610000,0x612000},W=0x620000,PROFILE=0x630000,STATS=0x632000;
constexpr uint32_t ITEM=0x640000,M[2]={0x650000,0x651000},SP=0x7e0000;
uint32_t admitted[2]={P[0],P[1]};
unsigned sounds=0,hidden=0,retired=0,updates=0,notifications[2]{},last_kind=0;
bool effect_playing=true,fail_popup=false,missing_hud=false;
uint8_t* B(){return kernel::memory.data();}
uint32_t Read(uint32_t p){return evidence_pickups::Word(B(),p);}
void Put(uint32_t p,uint32_t v){std::memcpy(B()+p,&v,4);}
void Real(uint32_t p,float v){std::memcpy(B()+p,&v,4);}
uint32_t TrialCoopPlayer(unsigned slot){return slot<2?admitted[slot]:0;}
uint32_t TrialCoopRewardManager(const uint8_t*,uint32_t actor){return missing_hud?0:actor==P[0]?M[0]:actor==P[1]?M[1]:0;}
// ORIGINAL_0004A840
// ORIGINAL_0004ACD0
// ORIGINAL_00171C70
X86_FUNC(sub_001F8FC0){const auto id=Read(ctx.esp+4);ctx.eax=id==100?P[0]:id==101?P[1]:0;ctx.esp+=4;}
X86_FUNC(sub_00218C40){assert(ctx.ecx==ITEM);++updates;ctx.esp+=4;}
X86_FUNC(sub_002160C0){assert(ctx.ecx==ITEM&&Read(ctx.esp+8)==0x46fa98);ctx.eax=effect_playing?1:0;ctx.esp+=12;}
X86_FUNC(sub_00216BD0){assert(ctx.ecx==ITEM&&Read(ctx.esp+8)==0x46fa98&&Read(ctx.esp+12)==0);++sounds;ctx.esp+=16;}
X86_FUNC(sub_0020A140){assert(ctx.ecx==ITEM&&Read(ctx.esp+4)==0x100);++hidden;ctx.esp+=8;}
X86_FUNC(sub_002620E0){const auto p=Read(ctx.esp+4);std::memcpy(base+p,"1/5",4);ctx.esp+=4;}
X86_FUNC(sub_0025CE20){assert(Read(ctx.esp+4)==3);ctx.eax=5;ctx.esp+=4;}
X86_FUNC(sub_001F92B0){const auto p=Read(ctx.esp+4);Put(p,Read(ctx.esp+8));ctx.eax=p;ctx.esp+=4;}
X86_FUNC(sub_00077E60){
    assert(ctx.ecx==M[0]||ctx.ecx==M[1]);++notifications[ctx.ecx==M[1]];
    last_kind=Read(ctx.esp+4);assert(Read(ctx.esp+20)==3);
    if(fail_popup)throw 7;
    ctx.esp+=24;
}
// ORIGINAL_POPUP
void x86_icall(X86Context& ctx,uint8_t*,uint32_t fn){assert(fn==0x218b30&&ctx.ecx==ITEM);++retired;Put(ITEM+0x5c,1);ctx.esp+=4;}
// ORIGINAL_PICKUP
void Position(uint32_t p,float x,float y=0,float z=0){Real(p+0x40,x);Real(p+0x44,y);Real(p+0x48,z);}
void Setup(unsigned singleton=1){
    kernel::memory.fill(0);trial_coop::Reset();trial_coop::Request();trial_coop::SetActive(true);
    admitted[0]=P[0];admitted[1]=P[1];sounds=hidden=retired=updates=notifications[0]=notifications[1]=last_kind=0;
    effect_playing=true;fail_popup=missing_hud=false;
    Put(0x51c4dc,W);Put(0x509a60,PROFILE);Put(PROFILE+0xc,STATS);Put(PROFILE+0x834,100+singleton);
    Put(0x51613c,M[singleton]);Put(0x46f9a8+0x24,0x218b30);Real(0x44c178,100);Real(0x44d528,22500);
    Put(ITEM,0x46f9a8);Put(ITEM+0x54,110);Put(ITEM+0xd0,2);Put(ITEM+0xd8,0x1234);
    for(unsigned i=0;i<2;++i){Put(P[i],0x46fbe8);Put(P[i]+0x54,100+i);Put(P[i]+0x368,i);Real(P[i]+0x9c,100);Position(P[i],1000);}
    Position(ITEM,0,100);assert(!evidence_pickups::current);
}
void Tick(bool patched=true){
    X86Context c{};c.esp=SP;c.ecx=ITEM;c.esi=0xabcdef;c.edi=0x123456;c.ebp=0x98765;
    if(patched)CoopPickup(c,B());else OriginalPickup(c,B());
    assert(c.esp==SP+4&&c.esi==0xabcdef&&c.edi==0x123456&&c.ebp==0x98765&&!evidence_pickups::current);
}
void Initialized(){Tick();assert(B()[ITEM+0xd4]&&!sounds&&!hidden&&!notifications[0]&&!notifications[1]);}
void Collected(unsigned slot){
    assert(Read(STATS+0x44)==4&&B()[ITEM+0xd5]==1&&sounds==1&&hidden==1&&notifications[slot]==1&&!notifications[1-slot]);
    assert(last_kind==1); // original ordinary evidence notification
    Tick();Tick();assert(sounds==1&&hidden==1&&notifications[slot]==1&&!retired);
    effect_playing=false;Tick();assert(retired==1&&sounds==1&&notifications[slot]==1);
}
int main(){
    for(unsigned singleton=0;singleton<2;++singleton)for(unsigned collector=0;collector<2;++collector){
        // Baseline proves that only the last registered player could collect.
        Setup(singleton);Position(P[collector],0);Initialized();Tick(false);
        assert(bool(sounds)==(singleton==collector));
        Setup(singleton);Position(P[collector],0);Initialized();Tick();Collected(collector);
        assert(Read(PROFILE+0x834)==100+singleton&&Read(0x51613c)==M[singleton]);
    }
    Setup();Position(P[0],0);Position(P[1],0);Initialized();Tick();Collected(0); // simultaneous overlap, single commit
    Setup();Put(STATS+0x44,4);Position(P[0],0);Tick();assert(retired==1&&!sounds&&!notifications[0]&&!notifications[1]);
    Setup();Put(STATS+0x44,0x1b);Position(P[0],0);Initialized();Tick();assert(Read(STATS+0x44)==0x1f&&last_kind==3&&notifications[0]==1); // final mask medal
    for(unsigned slot=0;slot<2;++slot){
        Setup();Position(P[slot],150);Initialized();Tick();assert(!sounds); // native strict boundary
        Position(P[slot],std::nextafter(150.0f,0.0f));Tick();Collected(slot);
        for(float invalid:{std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity()}){
            Setup();Position(P[slot],invalid);Initialized();Tick();assert(!sounds);
        }
        Setup();Position(P[slot],0,150);Initialized();Tick();assert(!sounds); // actual +100 vertical offset: 150 apart
        Position(P[slot],0,std::nextafter(150.0f,0.0f));Tick();Collected(slot);
    }
    Setup();Position(P[0],0);Position(P[1],0);Real(P[0]+0x9c,0);Put(P[0]+0x170,12);Initialized();Tick();Collected(1);
    Setup();Position(P[0],0);Position(P[1],0);Put(P[0]+0x5c,1);Initialized();Tick();Collected(1);
    Setup();Position(P[0],0);Position(P[1],0);admitted[0]=0;Initialized();Tick();Collected(1);
    Setup();Position(P[0],0);Position(P[1],0);Real(P[0]+0x9c,0);Real(P[1]+0x9c,0);Initialized();Tick();assert(!sounds);
    Setup(1);trial_coop::SetActive(false);Position(P[0],0);Initialized();Tick();assert(!sounds); // disabled is original
    Setup(0);trial_coop::SetActive(false);Position(P[0],0);Initialized();Tick();Collected(0);
    // Helper mutates only resolved EAX; no guest writes, flags, FP state or singleton edits.
    Setup();B()[ITEM+0xd4]=1;Position(P[0],0);X86Context c{};c.esp=SP;c.ecx=ITEM;c.esi=ITEM;c.eax=P[1];c.cf=1;c.xmm[2].u[3]=99;c.st[5]=2.5;
    static std::array<uint8_t,0x800000> memory_before;memory_before=kernel::memory;auto wanted=c;wanted.eax=P[0];const unsigned csr=_mm_getcsr();
    {TrialCoopEvidenceScope scope(c,B());scope.Select(c,B());assert(!std::memcmp(&c,&wanted,sizeof c)&&kernel::memory==memory_before&&_mm_getcsr()==csr);
     // A nested/foreign publisher is never relabelled as the collector.
     Put(SP,0x12345);c.ecx=M[1];TrialCoopEvidencePopup(c,B());assert(c.ecx==M[1]);
     Put(SP,0x171f1f);TrialCoopEvidencePopup(c,B());assert(c.ecx==M[0]);
     c.ecx=M[1];missing_hud=true;TrialCoopEvidencePopup(c,B());assert(c.ecx==0);missing_hud=false;
     c.ecx=M[1];Put(P[0]+0x54,999);TrialCoopEvidencePopup(c,B());assert(c.ecx==0);}
    assert(!evidence_pickups::current);
    Setup();Position(P[0],0);Initialized();missing_hud=true;Tick();assert(Read(STATS+0x44)==4&&sounds==1&&hidden==1&&!notifications[0]&&!notifications[1]);
    Setup();Position(P[0],0);Initialized();fail_popup=true;try{Tick();assert(false);}catch(int n){assert(n==7&&!evidence_pickups::current);}
    puts("co-op evidence: pinned native collection, both singleton orders/collectors, strict sphere, shared once/medal/checkpoint flags, native sound and retirement, collector popup, dead/retired partner, disabled path and scope restoration passed (offline; live acceptance pending)");
}

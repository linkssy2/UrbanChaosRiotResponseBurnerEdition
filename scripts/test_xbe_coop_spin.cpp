// Execute byte-pinned retail grant, option, reticle and killcam regions.
// Full gesture/input qualification remains in test_pistol_spin.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include "kernel/kernel.h"
namespace kernel {static std::array<uint8_t,0x800000> memory{};}
namespace mem {bool IsCommitted(uint32_t page){return page<kernel::memory.size();}}
#include "../common/xbe_coop_spin.cpp"
#include "cpu/x86_ops.h"
namespace spin_test {
constexpr uint32_t P[2]={0x600000,0x602000},W=0x610000,H[2]={0x620000,0x621000};
constexpr uint32_t C=0x630000,OPTIONS=0x640000,SP=0x7f0000,LATCH=0x51bdf4;
uint32_t admitted[2]={P[0],P[1]};unsigned animations[2]{};bool pistol=true;
uint8_t* B(){return kernel::memory.data();}
uint32_t Read(uint32_t a){uint32_t v;std::memcpy(&v,B()+a,4);return v;}
void Put(uint32_t a,uint32_t v){std::memcpy(B()+a,&v,4);}
void Setup(){
    kernel::memory.fill(0);TrialCoopSpinReset();trial_coop::Reset();trial_coop::SetAvailable(true);
    assert(trial_coop::Request());trial_coop::SetActive(true);Put(0x51c4dc,W);
    for(unsigned slot=0;slot<2;++slot){
        admitted[slot]=P[slot];animations[slot]=0;
        Put(P[slot],0x46fbe8);Put(P[slot]+0x54,7+slot);Put(P[slot]+0x368,slot);
        Put(P[slot]+0x4fc,100+slot);Put(H[slot]+4,P[slot]);
    }
    Put(C+0x54,90);Put(C+0xb8,7);Put(0x44bf94,0x3f800000);B()[OPTIONS+0x34]=1;pistol=true;
}
X86Context Context(uint32_t object){
    X86Context c{};c.esi=object;c.esp=SP;c.eax=0x12345678;c.ebx=0;c.edi=7;c.ecx=0x654321;
    c.edx=0x456123;c.cf=c.zf=c.pf=c.of=c.sf=1;c.df=1;c.fcw=0x37f;c.mxcsr=0x1f80;
    for(unsigned i=0;i<8;++i){c.xmm[i].q[0]=i+1;c.xmm[i].q[1]=i+9;c.st[i]=i+.25;}
    return c;
}
}
uint32_t TrialCoopPlayer(unsigned slot){return slot<2?spin_test::admitted[slot]:0;}
X86_FUNC(sub_00043150){assert(spin_test::Read(ctx.esp+4)==100||spin_test::Read(ctx.esp+4)==101);ctx.eax=spin_test::pistol;ctx.esp+=4;}
X86_FUNC(sub_000BFE00){
    using namespace spin_test;assert(ctx.ecx==P[0]||ctx.ecx==P[1]);
    assert(Read(ctx.esp+4)==2&&Read(ctx.esp+8)==0);++animations[ctx.ecx==P[1]];ctx.esp+=12;
}
X86_FUNC(sub_000643E0){ctx.eax=spin_test::OPTIONS;ctx.esp+=4;}
// ORIGINAL_AND_COOP_SPIN_REGIONS
namespace spin_test {
void Grant(unsigned slot,bool fixed=true){auto c=Context(P[slot]);(fixed?CoopGrant:OriginalGrant)(c,B());assert(c.esp==SP&&c.esi==P[slot]);}
void Option(unsigned slot){auto c=Context(P[slot]);CoopOption(c,B());assert(c.esp==SP);}
bool Cyan(unsigned slot,bool fixed=true){
    auto c=Context(H[slot]);(fixed?CoopHud:OriginalHud)(c,B());assert(c.esp==SP&&c.esi==H[slot]);
    assert(Read(H[slot]+0x14)==0x3f800000&&Read(H[slot]+0x18)==0x3f800000&&Read(H[slot]+0x1c)==0x3f800000);
    return Read(H[slot]+0x10)==0;
}
bool Kill(unsigned slot,bool fixed=true){
    auto c=Context(C);c.edi=7+slot;Put(C+0xb8,c.edi);B()[SP+0x1b]=0;
    (fixed?CoopReward:OriginalReward)(c,B());assert(c.esp==SP&&c.esi==C&&c.edi==7+slot);
    return B()[SP+0x1b]!=0;
}
}
int main(){
    using namespace spin_test;
    // Negative witness: original singleton colors both HUDs and the partner consumes it.
    Setup();Grant(0,false);assert(Cyan(0,false)&&Cyan(1,false));assert(Kill(1,false));assert(!Cyan(0,false));
    for(unsigned owner=0;owner<2;++owner){
        Setup();Grant(owner);assert(animations[owner]==1&&!animations[1-owner]);
        assert(Cyan(owner)&&!Cyan(1-owner)&&!B()[LATCH]);
        assert(!Kill(1-owner)&&Cyan(owner));assert(Kill(owner));
        assert(!Cyan(0)&&!Cyan(1)&&!Kill(owner));
        // Re-arming is one next-kill reward, not an accumulating charge counter.
        Grant(owner);Grant(owner);assert(Kill(owner)&&!Kill(owner));
        // The original option gate clears only the player currently being updated.
        Grant(0);Grant(1);B()[OPTIONS+0x34]=0;Option(owner);
        assert(!Cyan(owner)&&Cyan(1-owner));Option(1-owner);assert(!Cyan(0)&&!Cyan(1));
        // Other supported weapons keep the original ability without a pistol animation.
        Setup();pistol=false;Grant(owner);assert(!animations[owner]&&Cyan(owner)&&Kill(owner));
        // Reinitialization retires only this actor, including same-address/same-ID reuse.
        // CC319 establishes MainPlayer in EBX; ESI later becomes a title string.
        Setup();Grant(0);Grant(1);auto c=Context(P[1-owner]);c.ecx=P[owner];CoopActorOwner(c,B());CoopActorReset(c,B());
        assert(!Cyan(owner)&&Cyan(1-owner));
        c=Context(P[1-owner]);c.ebx=0x650000;CoopActorReset(c,B());assert(Cyan(1-owner));
        // Owner ID/pointer changes cannot inherit a pending reward.
        Setup();Grant(0);Grant(1);Put(P[owner]+0x54,17+owner);assert(!Cyan(owner)&&Cyan(1-owner));
        Setup();Grant(0);Grant(1);admitted[owner]=0x650000;
        Put(admitted[owner],0x46fbe8);Put(admitted[owner]+0x54,7+owner);Put(admitted[owner]+0x368,owner);
        Put(H[owner]+4,admitted[owner]);assert(!Cyan(owner)&&Cyan(1-owner));
        // Per-actor cleanup, corrupt owner links and unrelated killers cannot steal a charge.
        Setup();Grant(owner);c=Context(C);c.edi=55;Put(C+0xb8,55);B()[SP+0x1b]=0;CoopReward(c,B());
        assert(!B()[SP+0x1b]&&Cyan(owner));
        c.edi=7+owner;CoopReward(c,B());assert(!B()[SP+0x1b]&&Cyan(owner));
        Put(C+0xb8,c.edi);Put(C+0x5c,1);CoopReward(c,B());assert(!B()[SP+0x1b]&&Cyan(owner));
        Put(H[1-owner]+4,0x10);assert(!Cyan(1-owner)&&Cyan(owner));
        Put(P[owner]+0x5c,1);assert(!Cyan(owner));Put(P[owner]+0x5c,0);assert(!Cyan(owner));
    }
    // Both spin rewards coexist; consuming either one leaves the other's HUD/reward ready.
    for(unsigned first=0;first<2;++first){
        Setup();Grant(first);Grant(1-first);assert(Cyan(0)&&Cyan(1));
        assert(Kill(first)&&!Cyan(first)&&Cyan(1-first));assert(Kill(1-first));assert(!Cyan(0)&&!Cyan(1));
    }
    // World/session transitions discard both sidecars rather than resurrecting them.
    Setup();Grant(0);Grant(1);Put(0x51c4dc,W+0x1000);assert(!Cyan(0)&&!Cyan(1));
    Setup();Grant(0);Grant(1);TrialCoopSpinReset();assert(!Cyan(0)&&!Cyan(1));
    Setup();Grant(0);Grant(1);trial_coop::SetActive(false);assert(!Cyan(0));trial_coop::SetActive(true);assert(!Cyan(0)&&!Cyan(1));
    // Stale global state is ignored in co-op; invalid/ambiguous owners fail closed.
    Setup();B()[LATCH]=1;assert(!Cyan(0)&&!Cyan(1)&&!Kill(0));
    Grant(0);Put(P[1]+0x54,7);assert(!Cyan(0)&&!Cyan(1));
    Setup();Grant(0);Put(0x51c4dc,0);assert(!Cyan(0));Put(0x51c4dc,W);assert(!Cyan(0));
    // Inactive single-player retains the exact original global byte path.
    Setup();trial_coop::Reset();Grant(0);assert(B()[LATCH]&&Cyan(0)&&Kill(0)&&!B()[LATCH]);
    Grant(0);B()[OPTIONS+0x34]=0;Option(0);assert(!B()[LATCH]);
    // At a qualified owner, emitted consumers preserve the original register/stack
    // contract and actual HUD color writes; only latch storage ownership differs.
    for(bool active:{false,true})for(unsigned owner=0;owner<2;++owner)for(bool ready:{false,true}){
        Setup();trial_coop::SetActive(active);if(ready)Grant(owner);B()[LATCH]=ready;
        auto original=Context(H[owner]),fixed=original;const auto csr=_mm_getcsr();
        OriginalHud(original,B());std::array<uint8_t,16> color{};std::memcpy(color.data(),B()+H[owner]+0x10,16);
        std::memset(B()+H[owner]+0x10,0,16);CoopHud(fixed,B());
        assert(!std::memcmp(&original,&fixed,sizeof(fixed))&&!std::memcmp(color.data(),B()+H[owner]+0x10,16));
        original=Context(C);original.edi=7+owner;Put(C+0xb8,original.edi);fixed=original;
        B()[SP+0x1b]=0;OriginalReward(original,B());const auto awarded=B()[SP+0x1b];
        B()[SP+0x1b]=0;B()[LATCH]=ready;CoopReward(fixed,B());
        assert(!std::memcmp(&original,&fixed,sizeof(fixed))&&B()[SP+0x1b]==awarded&&csr==_mm_getcsr());
    }
    std::puts("co-op spin: native grant, per-player reticle color, owned kill reward, both orders, option/reset/lifetime and single-player fallback passed");
}

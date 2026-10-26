// Execute retail PLAYER query, actor overlap, flag setter and trigger-disable
// branches. Name iteration is a storage boundary with the real P1-only alias.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cstdio>
#include "kernel/kernel.h"
namespace kernel {static std::array<uint8_t,0x800000> memory{};}
namespace mem {bool IsCommitted(uint32_t p){return p<kernel::memory.size();}}
#include "../common/xbe_coop_shields.cpp"
constexpr uint32_t P[2]={0x610000,0x612000},W=0x620000,S=0x624000,Q=0x650000;
constexpr uint32_t V=0x660000,L=0x680000,E=0x681000,D=0x682000,A=0x683000;
constexpr uint32_t C=0x684000,BX=0x685000,T=0x686000,F=0x690000,PF=0x691000;
constexpr uint32_t N=0x692000,CMD=0x693000,SP=0x7e0000;
static unsigned permissions=0,blocked=0,overlaps=0;
uint8_t* B(){return kernel::memory.data();}
void Put(uint32_t p,uint32_t v){std::memcpy(B()+p,&v,4);}
void Real(uint32_t p,float v){std::memcpy(B()+p,&v,4);}
uint32_t TrialCoopPlayer(unsigned i){return i<2?P[i]:0;}
// ORIGINAL_002084E0
X86_FUNC(sub_002084E0){__imp__sub_002084E0(ctx,base);}
// ORIGINAL_002107C0
X86_FUNC(sub_002107C0){__imp__sub_002107C0(ctx,base);}
// ORIGINAL_00210770
X86_FUNC(sub_00210770){__imp__sub_00210770(ctx,base);}
// ORIGINAL_00210320
X86_FUNC(sub_00210320){__imp__sub_00210320(ctx,base);}
X86_FUNC(sub_00210780){assert(false);}
X86_FUNC(sub_002102F0){assert(false);}
X86_FUNC(sub_001F7EA0){assert(Read(base,ctx.esp+4)==0x44b828);ctx.eax=1;ctx.esp+=8;}
X86_FUNC(sub_001F7DA0){ctx.eax=1;ctx.esp+=4;}
X86_FUNC(sub_001F7E00){assert(Read(base,ctx.esp+4)==0);ctx.eax=N;ctx.esp+=8;}
X86_FUNC(sub_001F8FC0){assert(Read(base,ctx.esp+4)==100);ctx.eax=P[0];ctx.esp+=4;}
X86_FUNC(sub_00207D70){assert(false);}
// ORIGINAL_002D3830
X86_FUNC(sub_002D3830){__imp__sub_002D3830(ctx,base);}
X86_FUNC(sub_00214490){assert(false);}
// ORIGINAL_002187A0
// ORIGINAL_00211450
// ORIGINAL_SHIELD_REGIONS
void x86_icall(X86Context& c,uint8_t* b,uint32_t fn){
    assert(fn==0x2187a0);++overlaps;__imp__sub_002187A0(c,b);
}
namespace kernel {
uint32_t CallGuest(X86Context& c,uint32_t fn,const uint32_t* args,size_t count){
    assert(fn==0x2187a0&&count==1);const auto sp=c.esp;
    c.esp-=4;Put(c.esp,args[0]);c.esp-=4;Put(c.esp,0);
    ++overlaps;__imp__sub_002187A0(c,B());assert(c.esp==sp);return c.eax;
}
}
X86Context Context(){
    X86Context c{};c.esp=SP;c.eax=0xabc12345;c.ebx=21;c.edx=22;c.edi=23;c.esi=24;c.ebp=25;
    c.cf=1;c.st[0]=1.25;c.top=3;c.mxcsr=0x3f80;c.xmm[5].u[2]=91;return c;
}
void Setup(){
    kernel::memory.fill(0);trial_coop::Reset();TrialCoopShieldReset();permissions=blocked=overlaps=0;
    trial_coop::Request();trial_coop::SetActive(true);trial_coop::ResourcesReady(true);
    Put(0x51c4dc,W);Put(W+4,S);Put(S+0x2054,Q);Put(Q+4,L);Put(Q+0x10,102);Put(Q+0x1c,V);Put(Q+0x2c,F);
    Put(0x46fbe8+0x48,0x2187a0);Put(N,100);
    // Retail actor-overlap shape dispatch, pinned with the executable bytes.
    Put(0x2189f8,0x2187c3);Put(0x2189fc,0x218814);
    Put(0x218a00,0x218868);Put(0x218a04,0x218960);
    for(unsigned i=0;i<2;++i){
        const auto off=i*0x100,vol=V+ordinals[i]*0xc0;
        Put(P[i],0x46fbe8);Put(P[i]+0x54,100+i);Put(P[i]+0x368,i);Real(P[i]+0x9c,100);
        Put(P[i]+0x84c,PF+i*4);Put(PF+i*4,87);Put(P[i]+0x850,1);B()[P[i]+0x854]=1;Real(P[i]+0x88,100);
        Put(T+off,0x480888);Put(T+off+0x50,100+i);Put(T+off+0x54,200+i);
        Put(L+(100+i)*4,E+off);Put(E+off+4,200+i);Put(E+off+8,8);Put(E+off+0xc,D+off);Put(E+off+0x10,vol);
        Put(D+off+0x64,A+off);Put(A+off,1);Put(A+off+4,A+off+0x20);Put(A+off+0x20,C+off);
        std::memcpy(B()+C+off,scripts[i],words[i]*4);
        Put(vol+0x50,37);Put(vol+0x54,3);B()[vol+0x6c]=56;Put(vol+0x74,BX+off);Put(vol+0x78,BX+off+48);
        for(unsigned axis=0;axis<3;++axis){Real(BX+off+axis*4,-10);Real(BX+off+12+axis*4,10);}
    }
}
bool Overlap(unsigned which){
    auto c=Context();c.df=1;c.ecx=T+which*0x100;auto wanted=c;wanted.esp+=4;
    const auto csr=_mm_getcsr();_mm_setcsr(0x3f80);
    assert(TrialCoopShieldOverlap(c,B()));wanted.eax=(wanted.eax&0xffffff00)|(c.eax&255);
    assert(!std::memcmp(&c,&wanted,sizeof c)&&_mm_getcsr()==0x3f80);_mm_setcsr(csr);
    return (c.eax&255)!=0;
}
void Command(unsigned which,unsigned opcode,unsigned arg,bool patched){
    Put(CMD,0x480b48);Put(CMD+4,opcode);Put(CMD+8,arg);Put(CMD+12,opcode==10);
    Put(T+which*0x100+0xc0,CMD); // 216FA0 owns its current parsed command
    auto c=Context();c.ecx=T+which*0x100;Put(c.esp+4,CMD);const auto before=c;
    if(patched&&TrialCoopShieldCommand(c,B())){
        auto expected=before;expected.eax=(expected.eax&0xffffff00)|1;expected.esp+=8;
        assert(!std::memcmp(&c,&expected,sizeof c));return;
    }
    assert(!std::memcmp(&c,&before,sizeof c));
    // Register state on entry to the original 217BB0 command branches.
    c.esp-=28;c.esi=CMD;c.edi=c.ecx;
    if(opcode==10)OriginalSetFlag(c,B());else OriginalDisableTrigger(c,B());
    assert(c.esp==SP+8&&(c.eax&255)==1);
}
bool Permission(unsigned slot,bool patched){
    auto c=Context();c.esi=P[slot];const auto old=permissions;
    if(patched)CoopPermission(c,B());else OriginalPermission(c,B());
    assert(c.esp==SP);return permissions!=old;
}
int main(){
    for(unsigned first=0;first<2;++first){
        Setup();Real(P[first]+0x40,0);Real(P[1-first]+0x40,1000);
        auto c=Context();c.ecx=T;__imp__sub_00211450(c,B());
        assert(bool(c.eax&255)==(first==0)); // retail PLAYER alias cannot see P2
        assert(Overlap(0));assert(!Permission(0,true)&&!Permission(1,true));
        Command(0,10,87,true);assert(B()[F+87]==1&&acquired==(1u<<first));
        assert(Permission(0,false)&&Permission(1,false)); // shared-flag negative witness
        assert(Permission(first,true)&&!Permission(1-first,true));
        Command(0,53,32,false);assert(!(B()[V+32*0xc0+0x6c]&16));
        B()[V+32*0xc0+0x6c]=56;Command(0,53,32,true);assert(B()[V+32*0xc0+0x6c]&16);
        assert(!Overlap(1)); // one player cannot collect both physical shields
        Real(P[1-first]+0x40,0);assert(Overlap(1));Command(1,10,87,true);
        assert(acquired==3&&Permission(0,true)&&Permission(1,true));
        assert(!Overlap(0)&&!Overlap(1));
        trial_coop::SetActive(false);TrialCoopShieldReset();trial_coop::SetActive(true);
        B()[F+87]=0;assert(Overlap(0));Command(0,10,87,true);assert(acquired==1); // identical-identity restart
    }
    Setup();B()[F+87]=1;assert(!Overlap(0)&&acquired==3); // original checkpoint equipment
    // Both enter tests can run before either script reaches its flag command.
    Setup();assert(Overlap(0)&&Overlap(1));
    assert(candidate[0]==P[0]&&candidate[1]==P[1]);
    Command(1,10,87,true);assert(acquired==2);Command(0,10,87,true);assert(acquired==3);
    Setup();Real(P[1]+0x40,1000);assert(Overlap(0)&&!Overlap(1));
    Put(T+0xd4,1); // first original script started, but has not set flag87 yet
    Real(P[0]+0x40,1000);assert(!Overlap(1)&&candidate[0]==P[0]);
    Command(0,10,87,true);assert(acquired==1);
    // A rejected/unstarted enter test releases its reservation after leaving.
    Setup();Real(P[1]+0x40,1000);assert(Overlap(0));
    for(unsigned axis=0;axis<3;++axis){
        Real(BX+0x100+axis*4,axis? -10.f:990.f);
        Real(BX+0x100+12+axis*4,axis?10.f:1010.f);
    }
    Real(P[0]+0x40,1000);Real(P[1]+0x40,2000);assert(Overlap(1));
    assert(!candidate[0]&&candidate[1]==P[0]);Command(1,10,87,true);assert(acquired==1);
    Setup();assert(Overlap(0));Put(CMD+4,10);Put(CMD+8,87);Put(CMD+12,1);
    auto foreign=Context();foreign.ecx=T;Put(foreign.esp+4,CMD);auto saved=foreign;
    assert(!TrialCoopShieldCommand(foreign,B())&&acquired==0&&
           !std::memcmp(&foreign,&saved,sizeof foreign)); // not this trigger's active command
    for(unsigned invalid=0;invalid<7;++invalid){
        Setup();auto c=Context();c.ecx=T;const auto before=c;
        if(invalid==0)trial_coop::SetActive(false);
        if(invalid==1)Put(T,0x480889);
        if(invalid==2)Put(E+4,999);
        if(invalid==3)Put(C+24,11); // altered script must never receive policy
        if(invalid==4)Put(PF,86);
        if(invalid==5)Put(V+30*0xc0+0x78,BX+24);
        if(invalid==6)Put(P[1]+0x5c,1);
        assert(!TrialCoopShieldOverlap(c,B())&&!std::memcmp(&c,&before,sizeof c));
    }
    Setup();Real(P[0]+0x40,20);Real(P[1]+0x40,1000);assert(!Overlap(0)); // strict radius tangency
    Real(P[0]+0x40,19);assert(Overlap(0));
    Setup();Real(P[0]+0x9c,0);Real(P[1]+0x40,1000);assert(!Overlap(0));
    puts("co-op shields: retail P1-only overlap/shared-flag/cross-disable witnesses, both pickup orders, original AABB/flag operations, per-player admission, checkpoint/reset and foreign-script guards passed (offline; live pickup presentation pending)");
}

// Original target update, proximity and shooting bodies plus bounded facing and
// dragging excerpts. Animation, audio, entity lookup and weapon services are
// explicit boundaries; this does not claim a complete live AI/state-machine test.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <vector>
#include "kernel/kernel.h"
namespace kernel {static std::array<uint8_t,0x800000> memory{};}
namespace mem {uint32_t denied=0xffffffffu;bool IsCommitted(uint32_t p){return p<kernel::memory.size()&&p!=denied;}}
#include "../common/xbe_coop_hostage.cpp"
namespace hostage_test {
constexpr uint32_t W=0x600000,S=0x601000,L=0x602000,M=0x603000;
constexpr uint32_t P[2]={0x610000,0x612000},LAY[2]={0x620000,0x621000},ID[2]={349,350};
constexpr uint32_t C=0x630000,A=0x632000,WEAPON=0x634000,MODEL=0x636000,ANIM=0x638000,VT=0x640000,SP=0x7e0000;
uint32_t admitted[2]{P[0],P[1]},cached=0,resolved=0,eligible=0,shot=0;
uint32_t facing_position=0;
unsigned warnings=0,cases=0,guards=0;bool allow_shot=true;
uint8_t* B(){return kernel::memory.data();}
uint32_t Word(uint32_t a){uint32_t v;std::memcpy(&v,B()+a,4);return v;}
void Put(uint32_t a,uint32_t v){std::memcpy(B()+a,&v,4);}
void Float(uint32_t a,float v){std::memcpy(B()+a,&v,4);}
float Real(uint32_t a){float v;std::memcpy(&v,B()+a,4);return v;}
uint32_t Resolve(uint32_t id){for(unsigned i=0;i<2;++i)if(id==Word(P[i]+0x54))return P[i];return 0;}
X86Context Context(uint32_t owner=C){
    X86Context c{};c.ecx=owner;c.esi=C;c.esp=SP;c.eax=ID[1];
    c.ebx=17;c.edx=19;c.ebp=23;c.edi=29;c.cf=c.zf=c.sf=c.of=c.pf=c.df=1;
    c.fcw=0x37f;c.mxcsr=0x1f80;
    for(unsigned i=0;i<8;++i){c.xmm[i].q[0]=i+1;c.xmm[i].q[1]=i+9;c.st[i]=i+.25;}
    Put(SP,0xbeef);Put(SP+4,0);return c;
}
void Seed(unsigned native=1,bool dragging=false){
    kernel::memory.fill(0);mem::denied=0xffffffffu;
    trial_coop::Reset();trial_coop::SetAvailable(true);assert(trial_coop::Request());trial_coop::SetActive(true);
    Put(0x509a60,M);Put(M+0x834,ID[native]);Put(0x51c4dc,W);Put(W+4,S);Put(S+4,L);Put(S+8,L+8);
    for(unsigned i=0;i<2;++i){
        admitted[i]=P[i];Put(P[i],0x46fbe8);Put(P[i]+0x54,ID[i]);Put(P[i]+0x368,i);
        Float(P[i]+0x9c,100);Float(P[i]+0x40,i?1000:100);
        Put(L+4*i,LAY[i]);Put(LAY[i]+4,P[i]);Put(LAY[i]+0x24,i);
    }
    Put(C,dragging?0x466368:0x466f08);Put(C+0x54,1231);Put(C+0xd4,A);Put(A+0x54,2231);
    Put(C+0xf4,0xffffffffu);Put(C+0x200,WEAPON);Put(C+0x11c,WEAPON);
    Put(A+0x118,MODEL);Put(MODEL,VT);Put(ANIM,VT+0x100);Put(WEAPON,VT+0x200);
    Put(VT+0xa8,0xf001);Put(VT+0x108,0xf002);
    for(unsigned offset:{0x34u,0x38u,0x3cu})Put(VT+0x200+offset,0xf003);
    Put(VT+0x224,0xf004);Put(VT+0x228,0xf005);
    Float(C+0x19c,500);Float(C+0x1b0,10);Float(C+0x1a4,6);Float(C+0x1a8,2);
    Float(C+0x250,1);Put(C+0x14c,100);Put(C+0x124,1);
    cached=resolved=eligible=shot=facing_position=0;warnings=0;allow_shot=true;
    // ORIGINAL_HOSTAGE_CONSTANTS
}
}
uint32_t TrialCoopPlayer(unsigned slot){return slot<2?hostage_test::admitted[slot]:0;}
[[noreturn]]void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
// ORIGINAL_HOSTAGE_DECLARATIONS
X86_FUNC(sub_001F8FC0){using namespace hostage_test;resolved=Word(ctx.esp+4);ctx.eax=Resolve(resolved);ctx.esp+=4;}
X86_FUNC(sub_0011D020){hostage_test::cached=hostage_test::Word(ctx.ecx+0xf4);ctx.esp+=4;}
X86_FUNC(sub_00032680){ctx.esp+=8;}
X86_FUNC(sub_001F9010){x87_push(ctx,1.0/60);ctx.esp+=4;}
X86_FUNC(sub_0021AAE0){++hostage_test::warnings;ctx.esp+=12;}
X86_FUNC(sub_00032E80){ctx.eax=0;ctx.esp+=12;}
X86_FUNC(sub_0011ABC0){ctx.esp+=4;}
X86_FUNC(sub_000E2DD0){ctx.esp+=4;}
X86_FUNC(sub_000E2EE0){ctx.esp+=8;}
X86_FUNC(sub_001F7AA0){using namespace hostage_test;const auto actor=Resolve(Word(ctx.esp+4));ctx.eax=actor&&!Alive(B(),actor);ctx.esp+=4;}
X86_FUNC(sub_0002EEA0){ctx.eax=0;ctx.esp+=4;}
X86_FUNC(sub_002D3780){x87_push(ctx,.5);ctx.esp+=4;}
X86_FUNC(sub_002D37D0){ctx.eax=0;ctx.esp+=4;}
X86_FUNC(sub_00033E70){using namespace hostage_test;facing_position=Word(ctx.esp+4);assert(Word(ctx.esp+8)==0x40a00000);ctx.esp+=12;}
void x86_icall(X86Context& c,uint8_t*,uint32_t fn){
    using namespace hostage_test;
    switch(fn){
    case 0xf001:assert(c.ecx==MODEL);c.eax=ANIM;c.esp+=4;break;
    case 0xf002:assert(c.ecx==ANIM);c.esp+=8;break;
    case 0xf003:assert(c.ecx==WEAPON);c.eax=0;c.esp+=4;break;
    case 0xf004:assert(c.ecx==WEAPON);eligible=Word(c.esp+4);c.eax=allow_shot;c.esp+=16;break;
    case 0xf005:assert(c.ecx==WEAPON);shot=Word(c.esp+4);c.esp+=20;break;
    default:std::abort();
    }
}
// ORIGINAL_HOSTAGE_BODIES
namespace hostage_test {
void Generic(){
    for(unsigned native=0;native<2;++native)for(unsigned closer=0;closer<2;++closer)for(bool fixed:{false,true}){
        Seed(native);Float(P[closer]+0x40,100);Float(P[closer^1]+0x40,1000);
        auto c=Context();(fixed?CoopUpdate:OriginalUpdate)(c,B());const auto expected=ID[fixed?closer:native];
        assert(cached==expected&&Word(C+0xf4)==expected&&Word(M+0x834)==ID[native]&&c.esp==SP+8);
        c=Context();(fixed?CoopFacing:OriginalFacing)(c,B());assert(resolved==expected&&c.eax==Resolve(expected));
        c=Context();sub_0011BB30(c,B());assert(warnings==(expected==ID[closer]?1u:0u));
        Float(C+0x250,1);c=Context();Put(SP+4,cached);sub_0011ADD0(c,B());
        assert(eligible==expected&&shot==expected&&Word(M+0x834)==ID[native]);++cases;
    }
    // Movement after selection must not make facing pick a different player
    // during the same update. A following update may legitimately switch.
    Seed();auto c=Context();CoopUpdate(c,B());assert(cached==ID[0]);
    Float(P[0]+0x40,1000);Float(P[1]+0x40,100);c=Context();CoopFacing(c,B());assert(resolved==ID[0]);
    c=Context();CoopUpdate(c,B());assert(cached==ID[1]);
    for(unsigned prior=0;prior<2;++prior){
        Float(P[0]+0x40,-100);Float(P[1]+0x40,100);Put(C+0xf4,ID[prior]);
        c=Context();CoopUpdate(c,B());assert(cached==ID[prior]);++cases;
    }
    Put(C+0xf4,99999);c=Context();CoopUpdate(c,B());assert(cached==ID[1]);
    // Unarmed/lowered P1 remains a proximity threat; native selection has no
    // equipped-weapon requirement. Do not reproduce the private spawn defect.
    Float(P[0]+0x40,10);B()[P[0]+0x2ac]=B()[P[0]+0x2ad]=1;
    c=Context();CoopUpdate(c,B());assert(cached==ID[0]);++cases;
}
void Dragging(){
    for(unsigned native=0;native<2;++native)for(unsigned closer=0;closer<2;++closer)for(bool fixed:{false,true}){
        Seed(native,true);Float(P[closer]+0x40,100);Float(P[closer^1]+0x40,1000);
        auto c=Context();(fixed?CoopDragging:OriginalDragging)(c,B());const auto expected=ID[fixed?closer:native];
        assert(c.edi==expected&&eligible==expected&&shot==expected&&resolved==expected);
        assert(c.xmm[2].f[0]==(expected==ID[closer]?100.f:1000.f));
        assert(Word(M+0x834)==ID[native]);++cases;
    }
}
void Eligibility(){
    for(bool dragging:{false,true})for(unsigned dead=0;dead<2;++dead)for(unsigned reason=0;reason<5;++reason){
        Seed(dead,dragging);Float(P[dead]+0x40,1);Float(P[dead^1]+0x40,1000);
        switch(reason){
        case 0:Float(P[dead]+0x9c,0);break;
        case 1:Float(P[dead]+0x9c,-10);break;
        case 2:Put(P[dead]+0x170,12);break;
        case 3:Put(P[dead]+0x9c,0x7fc00000);break;
        case 4:Put(P[dead]+0x40,0x7f800000);break;
        }
        auto c=Context(M);c.eax=ID[dead];TrialCoopHostageTarget(c,B(),dragging);assert(c.eax==ID[dead^1]);++cases;
    }
    Seed();Float(P[0]+0x9c,0);Float(P[1]+0x9c,0);auto c=Context(M);const auto before=c;
    TrialCoopHostageTarget(c,B());assert(!std::memcmp(&c,&before,sizeof c));++cases;
    // Original weapon eligibility remains authoritative after target selection.
    Seed();c=Context();CoopUpdate(c,B());allow_shot=false;c=Context();Put(SP+4,cached);sub_0011ADD0(c,B());
    assert(eligible==ID[0]&&!shot);++cases;
    // Single-player preserves original getters even though the fixture contains
    // two synthetic actors. Explicit scripted-NPC behavior is a different type.
    for(bool dragging:{false,true}){
        Seed(1,dragging);trial_coop::SetActive(false);c=Context();
        if(dragging){CoopDragging(c,B());assert(shot==ID[1]);}
        else{CoopUpdate(c,B());assert(cached==ID[1]);c=Context();CoopFacing(c,B());assert(resolved==ID[1]);}
        ++cases;
    }
}
void IdleFacing(){
    for(unsigned native=0;native<2;++native)for(unsigned closer=0;closer<2;++closer)
    for(bool authored:{false,true})for(bool fixed:{false,true}){
        Seed(native);Put(C,0x466248);Put(C+0xf4,1134);B()[C+0xf9]=authored;
        Float(P[closer]+0x40,100);Float(P[closer^1]+0x40,1000);
        auto c=Context();(fixed?CoopIdleFacing:OriginalIdleFacing)(c,B());
        const auto expected=ID[fixed?closer:native];
        assert(facing_position==(authored?Resolve(expected)+0x40:0));
        assert(resolved==(authored?expected:0)&&Word(C+0xf4)==1134&&Word(M+0x834)==ID[native]);++cases;
    }
    Seed();Put(C,0x466248);Put(C+0xf4,ID[0]);B()[C+0xf9]=1;
    Float(P[0]+0x40,100);Float(P[1]+0x40,-100);
    auto c=Context();CoopIdleFacing(c,B());assert(resolved==ID[1]&&Word(C+0xf4)==ID[0]);++cases;
    Float(P[1]+0x9c,0);c=Context();CoopIdleFacing(c,B());assert(resolved==ID[0]);++cases;
}
void Guards(){
    for(bool facing:{false,true})for(unsigned bad=0;bad<22;++bad){
        Seed();Put(C+0xf4,ID[0]);auto c=Context(M);
        switch(bad){
        case 0:trial_coop::SetAvailable(false);break;
        case 1:trial_coop::Reset();break;
        case 2:Put(C,0x466248);break; // Scripted IdleUntil is not generic combat.
        case 3:Put(C,0x466110);break; // Authored ShootAtEntity (e.g. cop 1146) untouched.
        case 4:Put(C+0x54,0xffffffffu);break;
        case 5:Put(C+0x5c,1);break;
        case 6:c.ecx=M+4;break;
        case 7:c.eax=99;break;
        case 8:Put(W+4,0);break;
        case 9:Put(S+8,L+4);break;
        case 10:Put(P[0],0);break;
        case 11:Put(P[1]+0x5c,1);break;
        case 12:Put(P[1]+0x368,0);break;
        case 13:Put(LAY[0]+4,P[1]);break;
        case 14:Put(LAY[1]+0x24,0);break;
        case 15:Put(P[0]+0x54,ID[1]);break;
        case 16:Put(P[0]+0x54,0xffffffffu);break;
        case 17:admitted[1]=0;break;
        case 18:mem::denied=P[1];break;
        case 19:c.esi=0xfffffff0u;break;
        case 20:Put(S+4,0xfffffffcu);break;
        case 21:Put(0x51c4dc,0);break;
        }
        const auto before=c;const std::vector<uint8_t> memory(B(),B()+kernel::memory.size());const auto csr=_mm_getcsr();
        if(facing)TrialCoopHostageFacing(c,B());else TrialCoopHostageTarget(c,B());
        assert(!std::memcmp(&c,&before,sizeof c)&&!std::memcmp(B(),memory.data(),memory.size())&&_mm_getcsr()==csr);++guards;
    }
    // Successful overrides change EAX only, with no persistent singleton writes.
    for(bool dragging:{false,true}){
        Seed(1,dragging);auto c=Context(M),expected=c;expected.eax=ID[0];const auto csr=_mm_getcsr();
        const std::vector<uint8_t> memory(B(),B()+kernel::memory.size());TrialCoopHostageTarget(c,B(),dragging);
        assert(!std::memcmp(&c,&expected,sizeof c)&&!std::memcmp(B(),memory.data(),memory.size())&&_mm_getcsr()==csr);++guards;
    }
    Seed();Put(C+0xf4,ID[0]);Put(P[0]+0x54,777);auto c=Context(M);const auto before=c;
    TrialCoopHostageFacing(c,B());assert(!std::memcmp(&c,&before,sizeof c));++guards;
}
}
int main(){
    _mm_setcsr(0x1f80);hostage_test::Generic();hostage_test::Dragging();hostage_test::Eligibility();hostage_test::IdleFacing();hostage_test::Guards();
    std::printf("{\"passed\":true,\"hostage_cases\":%u,\"guards\":%u}\n",hostage_test::cases,hostage_test::guards);
}

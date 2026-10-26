// Retail defaults, stick layout, crouch and feedback predicates execute whole
// original bodies. Look inversion/centering/sensitivity execute pinned excerpts.
// Device samples, crouch animation and feedback amplitude are explicit boundaries.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "cpu/x86_ops.h"
#include "kernel/memory.h"
#include "xbe_controls.h"
static std::array<uint8_t,0x800000> memory{};
namespace mem {bool IsCommitted(uint32_t page){return page<memory.size();}}
constexpr uint32_t P[]={0x600000,0x602000},W=0x604000,STATE=0x606000,M=0x609000,VT=0x60a000,DATA=0x60b000,SP=0x780000,PROFILE=0x5152b0;
static uint32_t admitted[2]={P[0],P[1]};
static unsigned options[7],queries,query_slot,query_action[2],query_mode,crouches;
static float sample=.5f;
uint8_t* B(){return memory.data();}
uint32_t Word(uint32_t a){uint32_t v;std::memcpy(&v,B()+a,4);return v;}
void Put(uint32_t a,uint32_t v){std::memcpy(B()+a,&v,4);}
float Real(uint32_t a){float v;std::memcpy(&v,B()+a,4);return v;}
void Float(uint32_t a,float v){std::memcpy(B()+a,&v,4);}
uint32_t TrialCoopPlayer(unsigned slot){return slot<2?admitted[slot]:0;}
namespace trial_controls {unsigned SecondProfileValue(unsigned i,unsigned fallback){return i<7?options[i]:fallback;}}
#include "../common/xbe_coop_controls.cpp"
// ORIGINAL_DECLARATIONS
X86_FUNC(sub_000643E0){ctx.eax=PROFILE;ctx.esp+=4;}
X86_FUNC(sub_000BDF40){assert(ctx.ecx==P[0]||ctx.ecx==P[1]);B()[ctx.ecx+0x434]=uint8_t(Word(ctx.esp+4));++crouches;ctx.esp+=12;}
X86_FUNC(sub_000684E0){x87_push(ctx,.75);ctx.esp+=8;}
X86_FUNC(sub_00068540){std::abort();}
[[noreturn]]void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
void x86_resume(X86Context&,uint8_t*,const X86Resume&){std::abort();}
void x86_icall(X86Context& c,uint8_t*,uint32_t fn){
    assert(fn==0xf001&&c.ecx==W);query_slot=Word(c.esp+4);assert(query_slot<2);
    query_action[queries%2]=Word(c.esp+8);query_mode=Word(c.esp+12);++queries;
    x87_push(c,sample);c.esp+=16;
}
// ORIGINAL_BODIES
static X86Context Context(uint32_t self=0){X86Context c{};c.ecx=self;c.esp=SP;c.ebp=0x1111;c.esi=0x2222;c.edi=0x3333;c.ebx=0x4444;Put(SP,0xbeef);return c;}
static void Seed(){
    memory.fill(0);trial_coop::SetActive(true);queries=crouches=0;sample=.5f;
    Put(0x51c4dc,W);Put(W,VT);Put(VT+8,0xf001);Put(W+4,STATE);Put(W+0x90c,M);Put(M,0x452640);
    for(unsigned slot=0;slot<2;++slot){
        admitted[slot]=P[slot];Put(P[slot],0x46fbe8);Put(P[slot]+0x368,slot);Put(P[slot]+0x54,100+slot);
        Put(STATE+0x3c+(100+slot)*8,100+slot);Put(STATE+0x40+(100+slot)*8,P[slot]);
    }
    // ORIGINAL_CONSTANTS
    auto c=Context(PROFILE+0x24);Original_00063B70(c,B());assert(c.esp==SP+4);
    const trial_controls::Draft defaults;std::memcpy(options,defaults.native_profile,sizeof(options));
    const unsigned native[]={B()[PROFILE+0x30],B()[PROFILE+0x24],B()[PROFILE+0x36],B()[PROFILE+0x35],B()[PROFILE+0x31],Word(PROFILE+0x2c),Word(PROFILE+0x28)};
    assert(!std::memcmp(options,native,sizeof(native)));
}
static void Layout(){
    Seed();options[4]=0;
    for(unsigned slot=0;slot<2;++slot)for(bool fixed:{false,true}){
        auto c=Context();c.esi=slot;Put(SP+4,DATA);Put(SP+8,DATA+4);queries=0;
        (fixed?Coop_000BC4B0:Original_000BC4B0)(c,B());
        assert(c.esp==SP+4&&queries==2&&query_slot==slot&&query_mode==3);
        assert(query_action[0]==1&&query_action[1]==(fixed&&slot?2:0));
        c=Context();c.edi=P[slot];c.eax=PROFILE;queries=0;
        (fixed?Coop_Layout:Original_Layout)(c,B());
        assert(queries==1&&query_slot==slot&&query_action[0]==(fixed&&slot?0:2)&&query_mode==3);
    }
}
static void Crouch(){
    Seed();options[3]=0;
    for(unsigned slot=0;slot<2;++slot)for(bool fixed:{false,true})for(unsigned crouched=0;crouched<2;++crouched)for(float strength:{0.f,1.f}){
        auto c=Context(P[slot]);B()[P[slot]+0x434]=uint8_t(crouched);sample=strength;crouches=queries=0;
        (fixed?Coop_000BE7D0:Original_000BE7D0)(c,B());
        const bool toggle=fixed&&slot;
        assert(c.esp==SP+4&&c.esi==0x2222&&queries==1&&query_slot==slot&&query_action[0]==15&&query_mode==unsigned(toggle));
        assert(B()[P[slot]+0x434]==(toggle?(strength?1-crouched:crouched):unsigned(strength!=0)));
    }
}
static void Look(){
    Seed();options[1]=1;options[2]=0;options[5]=0;options[6]=100;
    for(unsigned slot=0;slot<2;++slot)for(bool fixed:{false,true}){
        auto c=Context();c.edi=P[slot];c.eax=PROFILE;Float(SP+0xc,.25f);
        (fixed?Coop_Invert:Original_Invert)(c,B());
        assert(Real(SP+0xc)==(fixed&&slot?.25f:-.25f));
        c=Context();c.edi=P[slot];c.eax=PROFILE;
        (fixed?Coop_AutoCentre:Original_AutoCentre)(c,B());assert(c.eax==unsigned(!(fixed&&slot)));
        c=Context();c.edi=P[slot];c.eax=PROFILE;
        (fixed?Coop_Horizontal:Original_Horizontal)(c,B());
        const float h=float(fixed&&slot?0:50)*Real(0x44c538)*Real(0x44c5fc)-Real(0x44bf94);
        assert(std::fabs(Real(SP+0x1c)-h)<.00001f);
        c=Context();c.edi=P[slot];c.eax=PROFILE;
        (fixed?Coop_Vertical:Original_Vertical)(c,B());
        const float v=float(fixed&&slot?100:50)*Real(0x44c538)*Real(0x44c5fc)-Real(0x44bf94);
        assert(std::fabs(Real(SP+0x2c)-v)<.00001f);
    }
}
static void Feedback(){
    Seed();const uint32_t node=DATA+0x100,head=DATA+0x180;
    Put(M+0x34,DATA);Float(DATA+8,1.f);Put(M+8,head);Put(head,node);Put(node,head);
    Put(node+0x14,3);B()[M+0x54]=B()[M+0x55]=1;
    for(unsigned at:{0x14u,0x20u,0x2cu}){Put(M+at,DATA+at);Put(DATA+at,DATA+at);}
    for(unsigned slot=0;slot<2;++slot)for(unsigned type:{1u,2u})for(bool fixed:{false,true})for(unsigned enabled:{0u,1u}){
        options[0]=enabled;Put(node+8,slot);auto c=Context(M);Put(SP+4,slot);Put(SP+8,DATA);Put(SP+12,type);
        (fixed?Coop_000682F0:Original_000682F0)(c,B());
        assert(c.esp==SP+16&&std::fabs(c.st[c.top&7]-(fixed&&slot&&type==2&&!enabled?0.:.75))<.00001);
    }
    assert(B()[M+0x54]==1&&B()[M+0x55]==1);
}
static void Ownership(){
    Seed();options[1]=1;
    assert(TrialCoopControlValue(B(),P[1],1,0)==1);
    assert(TrialCoopControlValue(B(),P[0],1,0)==0);
    assert(TrialCoopControlValue(B(),P[1],7,73)==73);
    static auto unchanged=memory;unchanged=memory;
    for(unsigned repeat=0;repeat<4;++repeat)for(unsigned i=0;i<7;++i)
        assert(TrialCoopControlValue(B(),P[1],i,99)==options[i]);
    assert(memory==unchanged);
    trial_coop::SetActive(false);assert(!TrialCoopControlValue(B(),P[1],1,0));trial_coop::SetActive(true);
    Put(P[1]+0x368,0);assert(!TrialCoopControlValue(B(),P[1],1,0));Put(P[1]+0x368,1);
    Put(P[1]+0x5c,1);assert(!TrialCoopControlValue(B(),P[1],1,0));Put(P[1]+0x5c,0);
    Put(STATE+0x3c+101*8,999);assert(!TrialCoopControlValue(B(),P[1],1,0));Put(STATE+0x3c+101*8,101);
    admitted[1]=P[0];assert(!TrialCoopControlValue(B(),P[1],1,0));admitted[1]=P[1];
    assert(!TrialCoopControlValue(nullptr,P[1],1,0));
    assert(!TrialCoopControlSlotValue(B(),2,1,0));
    options[0]=0;assert(TrialCoopVibrationValue(B(),M+4,1,1)==1);
}
int main(){Layout();Crouch();Look();Feedback();Ownership();puts("Co-op native controls: retail defaults/layout/crouch/vibration, look excerpts and owner isolation passed.");}

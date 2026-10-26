// Execute the original player proximity query, pickup acceptance/grant, inventory
// mutations, property getters, ID lookup and item retirement. Resource definitions,
// spatial-tag enumeration, vector growth, audio/HUD rendering and event-handler
// dispatch are explicit engine boundaries, not replacement grant implementations.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <limits>
#include "kernel/kernel.h"
namespace kernel {static std::array<uint8_t,0x800000> memory{};}
namespace mem {bool IsCommitted(uint32_t p){return p<kernel::memory.size();}}
#include "../common/xbe_coop_pickups.cpp"
constexpr uint32_t P[2]={0x600000,0x602000},I[2]={0x610000,0x612000},H[2]={0x620000,0x621000};
constexpr uint32_t W=0x630000,STATE=0x631000,ITEM=0x640000,PROFILE=0x650000,OPTIONS=0x651000;
constexpr uint32_t DEFS=0x660000,LISTS=0x670000,CELL=0x680000,SP=0x7e0000,ID=123;
unsigned hud[2]{},sounds[2]{},retired=0,switched[2]{};bool patched=true,button=false;
uint32_t admitted[2]={P[0],P[1]};
uint8_t* B(){return kernel::memory.data();}
uint32_t Read(uint32_t a){uint32_t v;std::memcpy(&v,B()+a,4);return v;}
void Put(uint32_t a,uint32_t v){std::memcpy(B()+a,&v,4);}
void Real(uint32_t a,float v){std::memcpy(B()+a,&v,4);}
uint32_t TrialCoopPlayer(unsigned s){return s<2?admitted[s]:0;}
unsigned Owner(uint32_t actor){assert(actor==P[0]||actor==P[1]);return actor==P[1];}
// ORIGINAL_PICKUP_DECLARATIONS
X86_FUNC(sub_001F7AA0){ctx.eax=Read(ctx.esp+8)==0x45836c&&button;ctx.esp+=4;}
X86_FUNC(sub_001F7EA0){assert(Read(ctx.esp+4)==0x44e3c4);ctx.eax=1;ctx.esp+=8;}
X86_FUNC(sub_001F7DA0){ctx.eax=1;ctx.esp+=4;}
X86_FUNC(sub_001F7E00){assert(Read(ctx.esp+4)==0);ctx.eax=CELL;ctx.esp+=8;}
X86_FUNC(sub_000442D0){const auto type=Read(ctx.esp+4);assert(type<27);ctx.eax=DEFS+type*0x100;ctx.esp+=4;}
X86_FUNC(sub_00044210){ctx.eax=Read(ctx.ecx+0x24);ctx.esp+=4;}
X86_FUNC(sub_00044160){ctx.eax=Read(ctx.ecx+0x28);ctx.esp+=4;}
X86_FUNC(sub_000440D0){ctx.eax=0;ctx.esp+=4;}
X86_FUNC(sub_000421B0){assert(ctx.ecx==P[0]+0x498||ctx.ecx==P[1]+0x498);ctx.esp+=4;}
X86_FUNC(sub_00042ED0){
    const auto end=Read(ctx.ecx+8),value=Read(ctx.esp+12);
    assert(Read(ctx.esp+4)==end&&Read(ctx.esp+8)==1);Put(end,Read(value));Put(ctx.ecx+8,end+4);ctx.esp+=16;
}
X86_FUNC(sub_000643E0){ctx.eax=OPTIONS;ctx.esp+=4;}
X86_FUNC(sub_0004A840){ctx.eax=PROFILE;ctx.esp+=4;}
X86_FUNC(sub_001F92B0){const auto p=Read(ctx.esp+4);Put(p,Read(ctx.esp+8));ctx.eax=p;ctx.esp+=4;}
X86_FUNC(sub_00018140){assert(false);}
X86_FUNC(sub_000BFE00){++switched[Owner(ctx.ecx)];ctx.esp+=12;}
X86_FUNC(sub_000BC560){Owner(ctx.ecx);ctx.esp+=4;}
X86_FUNC(sub_00216BD0){++sounds[Owner(ctx.ecx)];ctx.esp+=16;}
X86_FUNC(sub_0006BA50){ctx.esp+=4;}
X86_FUNC(sub_00207F00){assert(Read(ctx.esp+4)==ID&&ctx.ecx==W);++retired;ctx.esp+=8;}
X86_FUNC(sub_0028A190){ctx.eax=0;ctx.esp+=4;}
X86_FUNC(sub_0028D790){assert(false);}
X86_FUNC(sub_002084E0){ctx.eax=PROFILE;ctx.esp+=8;}
X86_FUNC(sub_00210890){ctx.esp+=4;}
void x86_icall(X86Context& ctx,uint8_t* base,uint32_t fn){
    if(fn==0xef600){sub_000EF600(ctx,base);return;}
    if(fn==0x1eefd0){sub_001EEFD0(ctx,base);return;}
    if(fn==0xf001){ctx.eax=0;ctx.esp+=16;return;} // profile resource query
    if(fn==0xf002){
        assert(ctx.ecx==H[0]||ctx.ecx==H[1]);++hud[ctx.ecx==H[1]];ctx.esp+=12;return;
    }
    assert(fn==0xf003&&ctx.ecx==ITEM&&Read(ctx.esp+12)==4);
    const auto sender=Read(ctx.esp+16),payload=Read(ctx.esp+8);
    const auto collector=Read(sender);assert(collector==100||collector==101);
    assert(Read(ctx.esp+4)==0x12345678); // original item's consume event
    // Event receiver dispatch supplies data/sender to the authored callback.
    const auto saved=ctx.esp;ctx.esp-=12;Put(ctx.esp,0xfeed);Put(ctx.esp+4,payload);Put(ctx.esp+8,sender);
    if(Read(ITEM)==0x460898)sub_000EF820(ctx,base);else sub_000D8520(ctx,base);
    assert(ctx.esp==saved);ctx.esp=saved+20;
}
// ORIGINAL_PICKUP_BODIES
X86_FUNC(sub_000C5040){if(patched)CoopPickup(ctx,base);else OriginalPickup(ctx,base);}
void Setup(unsigned type=3,bool ammo=false,bool owned=true){
    kernel::memory.fill(0);trial_coop::Reset();trial_coop::SetAvailable(true);assert(trial_coop::Request());trial_coop::SetActive(true);
    patched=true;button=false;retired=0;hud[0]=hud[1]=sounds[0]=sounds[1]=switched[0]=switched[1]=0;
    Put(0x51c4dc,W);Put(W+4,STATE);Put(STATE+0x3c+(ID&0x3ff)*8,ID);Put(STATE+0x40+(ID&0x3ff)*8,ITEM);
    Put(ITEM,ammo?0x47f6e8:0x460898);Put(ITEM+0x54,ID);Put(ITEM+0xd0,type);Put(ITEM+0x98,type);Put(CELL,ID);
    Put(0x460898,0xf003);Put(0x460898+8,0xef600);Put(0x47f6e8,0xf003);Put(0x47f6e8+8,0x1eefd0);
    std::memcpy(B()+0x44caec,"weapon type",12);Put(0x4fa034,0x12345678);Put(0x5096e0,PROFILE);Put(0x51c398,PROFILE+0x800);Put(PROFILE+0x808,PROFILE);
    Put(PROFILE,PROFILE+0x900);Put(PROFILE+0x91c,0xf001);Real(0x45709c,1000000);Real(0x457e38,22500);
    for(unsigned k=0;k<27;++k){const auto d=DEFS+k*0x100;Put(d,k);Put(d+8,0);Put(d+0x10,0);Put(d+0x24,10);Put(d+0x28,4);Put(d+0x2c,12);}
    for(unsigned s=0;s<2;++s){
        admitted[s]=P[s];Put(P[s],0x46fbe8);Put(P[s]+0x54,100+s);Put(P[s]+0x368,s);Real(P[s]+0x9c,100);Real(P[s]+0x40,1000);
        Put(P[s]+0x474,ID);Put(P[s]+0x4fc,owned?type:0);Put(P[s]+0x7e8,H[s]);Put(H[s],H[s]+0x100);Put(H[s]+0x104,0xf002);
        const auto inv=P[s]+0x498;Put(inv,I[s]);Put(inv+0x64,type);Put(inv+0x68,type);Put(inv+0x6c,type);Put(inv+0x70,type);
        Put(inv+0xac,3);Put(inv+0xd0,40);Put(inv+0x80,LISTS+s*0x100);Put(inv+0x84,LISTS+s*0x100);Put(inv+0x88,LISTS+s*0x100+0x100);
        for(unsigned k=0;k<27;++k){const auto r=I[s]+k*0x5c;Put(r+4,k);Put(r+0x10,inv+0xac);Put(r+0x14,inv+0xd0);}
        const auto r=I[s]+type*0x5c;B()[r]=owned;Put(r+0xc,owned?1:0);
    }
}
void Collect(unsigned s,bool fixed=true,bool query=false){
    patched=fixed;X86Context c{};c.esp=SP;c.ecx=P[s];c.esi=0xaabb;c.edi=0xccdd;c.ebp=0x1122;c.ebx=0x3344;c.fcw=0x37f;c.mxcsr=0x1f80;Put(SP+4,ID);
    std::array<uint8_t,0x890> partner;std::memcpy(partner.data(),B()+P[1-s],partner.size());
    std::array<uint8_t,0x9b4> partner_weapons;std::memcpy(partner_weapons.data(),B()+I[1-s],partner_weapons.size());
    const auto csr=_mm_getcsr();
    if(query)sub_000C7CC0(c,B());else sub_000C5040(c,B());
    assert(c.esp==SP+(query?4:8)&&c.esi==0xaabb&&c.edi==0xccdd&&c.ebp==0x1122&&c.ebx==0x3344);
    assert(std::memcmp(partner.data(),B()+P[1-s],partner.size())==0&&std::memcmp(partner_weapons.data(),B()+I[1-s],partner_weapons.size())==0);
    assert(_mm_getcsr()==csr);
}
int main(){
    for(bool ammo:{false,true})for(unsigned first=0;first<2;++first){
        // Witness: native ID resolution still sees an item marked for deletion,
        // so two local actors can both receive it before the next world cleanup.
        Setup(3,ammo);Collect(first,false);Collect(1-first,false);assert(retired==2&&hud[0]==1&&hud[1]==1);
        Setup(3,ammo);Collect(first);const auto after=Read(P[first]+0x498+0xac);assert(after==15);
        Collect(1-first);Collect(first);assert(retired==1&&hud[first]==1&&!hud[1-first]);
        assert(Read(P[1-first]+0x498+0xac)==3&&Read(P[first]+0x498+0xac)==after);
        assert(sounds[first]==1&&!sounds[1-first]&&(Read(ITEM+0x5c)&1));
        // A full inventory rejects without consuming; the other actor may take it.
        Setup(3,ammo);Put(P[first]+0x498+0xac,40);Put(I[first]+3*0x5c+0xc,10);Collect(first);assert(!retired&&!hud[first]);
        Collect(1-first);assert(retired==1&&hud[1-first]==1&&!hud[first]);
    }
    for(unsigned s=0;s<2;++s){
        Setup(17,false,false);Collect(s);assert(B()[I[s]+17*0x5c]&&!B()[I[1-s]+17*0x5c]);
        assert(Read(I[s]+17*0x5c+0xc)==10&&Read(I[1-s]+17*0x5c+0xc)==0&&retired==1);
        assert(B()[PROFILE+0x430+17]==1); // encountered-weapon profile flag remains mission-wide
        assert(Read(P[s]+0x498+0x84)==LISTS+s*0x100+4&&Read(P[1-s]+0x498+0x84)==LISTS+(1-s)*0x100);
        Setup();Real(P[s]+0x40,0);Collect(s,true,true);assert(hud[s]==1&&!hud[1-s]); // original query uses this actor's position
        Setup();Collect(s,true,true);assert(!retired); // out of native range
        Setup();button=true;Real(P[s]+0x40,0);Collect(s,true,true);assert(!retired&&Read(P[s]+0x478)==2&&Read(P[s]+0x47c)==1);
        Setup();Put(P[s]+0x170,12);Collect(s);assert(!retired&&!hud[s]);
        Setup();Real(P[s]+0x9c,0);Collect(s);assert(!retired);
        Setup();Real(P[s]+0x9c,std::numeric_limits<float>::quiet_NaN());Collect(s);assert(!retired);
        Setup();Put(ITEM+0x54,ID+1);Collect(s);assert(!retired);
        Setup();Put(STATE+0x3c+(ID&0x3ff)*8,ID+1);Collect(s);assert(!retired);
        Setup();Put(STATE+0x40+(ID&0x3ff)*8,0);Collect(s);assert(!retired);
        Setup();Put(0x51c4dc,0);Collect(s);assert(!retired);
        Setup();trial_coop::SetActive(false);Collect(s);assert(retired==1&&hud[s]==1&&!hud[1-s]);
    }
    puts("co-op pickups: original duplicate-retired-item witness, owner-local native grants/ammo/HUD, fresh-weapon list/profile behavior, both collectors, nearest-query/input admission, full-inventory rejection and dead/stale guards passed (offline; live pickup presentation pending)");
}

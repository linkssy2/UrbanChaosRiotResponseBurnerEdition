// Retail mount/update/remove builders and real Mainplayer event callbacks run.
// Inventory storage, physics publication, model loading and event registration
// are completing boundaries; this does not claim GPU/mesh clearance acceptance.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cstdio>
#include <vector>
#include "kernel/kernel.h"
namespace kernel {static std::array<uint8_t,0x800000> memory{};}
namespace mem {bool IsCommitted(uint32_t p){return p<kernel::memory.size();}}
#include "../common/xbe_coop_turrets.cpp"
namespace turret_test {
constexpr uint32_t P[2]={0x610000,0x612000},W=0x620000,S=0x624000,L=0x630000;
constexpr uint32_t H=0x640000,M=0x650000,Services=0x660000,Events=0x661000,SP=0x7f0000;
constexpr uint32_t Items[2]={0x670000,0x671000},Cameras[2]={0x680000,0x681000},Def=0x690000;
constexpr uint32_t EvMatrix=11,EvEquip=12,EvRemove=13,EvYaw=14,EvPitch=15;
struct Event {unsigned slot;uint32_t kind,size;std::array<uint8_t,96> data;};
std::vector<Event> events;std::array<unsigned,2> grants{},removes{},poses{},idles{},camera_writes{};
bool throw_event=false;unsigned relay_calls=0;bool patched_camera=false,asset_ready=true;
uint8_t* B(){return kernel::memory.data();}
uint32_t Get(uint32_t p){uint32_t v;std::memcpy(&v,B()+p,4);return v;}
void Set(uint32_t p,uint32_t v){std::memcpy(B()+p,&v,4);}
void Float(uint32_t p,float v){std::memcpy(B()+p,&v,4);}
float F(uint32_t p){float v;std::memcpy(&v,B()+p,4);return v;}
unsigned Slot(uint32_t p){assert(p==P[0]||p==P[1]);return p==P[1];}
void Identity(uint32_t p){for(unsigned i=0;i<16;++i)Float(p+4*i,i%5==0?1.f:0.f);}
}
uint32_t TrialCoopPlayer(unsigned i){return i<2?turret_test::P[i]:0;}
bool TrialCoopTurretAssetReady(const uint8_t*,uint32_t vehicle){return turret_test::asset_ready&&vehicle==turret_test::H;}
void TrialHealthPackMedicPlayer(X86Context&,uint8_t*,bool){} // unrelated medic scope inactive
X86_FUNC(CoopCamera);
X86_FUNC(OriginalCamera);
X86_FUNC(sub_000BD150){using namespace turret_test;std::memcpy(B()+ctx.ecx+0x10,B()+Get(ctx.esp+4),64);ctx.esp+=8;}
X86_FUNC(sub_000BD250){++turret_test::poses[turret_test::Slot(ctx.ecx)];ctx.esp+=4;}
X86_FUNC(sub_002A42B0){assert(false);}
X86_FUNC(sub_00054CE0){ctx.eax=turret_test::Get(ctx.esp+4);ctx.esp+=8;}
X86_FUNC(sub_00042D60){using namespace turret_test;unsigned i=Slot(ctx.ecx-0x498);assert(Get(ctx.esp+4)==25);++grants[i];Set(Items[i]+4,25);ctx.eax=Items[i];ctx.esp+=8;}
X86_FUNC(sub_00042230){using namespace turret_test;assert(Get(ctx.esp+4)==25);Set(ctx.ecx-0x498+0x4fc,25);ctx.esp+=8;}
X86_FUNC(sub_000BFE00){using namespace turret_test;Set(ctx.ecx+0x170,Get(ctx.esp+4));ctx.esp+=12;}
X86_FUNC(sub_000C8930){using namespace turret_test;const auto saved=ctx;const unsigned slot=Slot(ctx.ecx);ctx.esi=ctx.ecx;ctx.ebp=ctx.ecx+0x498;ctx.ebx=0;
    if(patched_camera)CoopCamera(ctx,base);else OriginalCamera(ctx,base);
    ctx.ebx=saved.ebx;ctx.esi=saved.esi;ctx.edi=saved.edi;ctx.ebp=saved.ebp;ctx.esp=saved.esp+4;++camera_writes[slot];}
X86_FUNC(sub_000C1E60){using namespace turret_test;assert(!Get(ctx.esp+4)&&Get(ctx.esp+8)==0xffffffff&&Get(ctx.esp+12)==1);++idles[Slot(ctx.ecx)];ctx.esp+=16;}
X86_FUNC(sub_00041FC0){using namespace turret_test;unsigned i=Slot(ctx.ecx-0x498);assert(Get(ctx.esp+4)==25&&!Get(ctx.esp+8)&&Get(Items[i]+4)==25);ctx.eax=Items[i];ctx.esp+=12;}
X86_FUNC(sub_00043B80){using namespace turret_test;unsigned i=ctx.ecx==Items[1];assert(ctx.ecx==Items[i]);++removes[i];Set(Items[i]+4,0);ctx.esp+=4;}
X86_FUNC(sub_00042870){turret_test::Set(ctx.ecx-0x498+0x4fc,1);ctx.esp+=4;}
X86_FUNC(sub_00214F40){using namespace turret_test;const unsigned i=Get(ctx.esp+4);assert(i<2&&ctx.ecx==W);ctx.eax=Cameras[i];ctx.esp+=8;}
X86_FUNC(sub_00041F80){ctx.eax=turret_test::Items[turret_test::Slot(ctx.ecx-0x498)];ctx.esp+=4;}
X86_FUNC(sub_000442D0){assert(turret_test::Get(ctx.esp+4)==25);ctx.eax=turret_test::Def;ctx.esp+=4;}
X86_FUNC(sub_001F8FC0){using namespace turret_test;const auto id=Get(ctx.esp+4);ctx.eax=id==100?P[0]:id==101?P[1]:0;ctx.esp+=4;}
X86_FUNC(sub_002D6802){double divisor=X87_ST(0),numerator=X87_ST(1);x87_pop(ctx);X87_ST(0)=std::fmod(numerator,divisor);ctx.esp+=4;}
// ORIGINAL_COOP_TURRET_WITNESSES
void x86_icall(X86Context& c,uint8_t* b,uint32_t target){
    using namespace turret_test;assert(target==0x20a540);const unsigned slot=Slot(c.ecx);
    const uint32_t kind=Get(c.esp+4),data=Get(c.esp+8),size=Get(c.esp+12),sender=Get(c.esp+16);
    assert(Get(sender)==200&&size<=96);Event e{slot,kind,size,{}};std::memcpy(e.data.data(),B()+data,size);events.push_back(e);
    if(throw_event)throw 17;
    // Original 20A540's registered component dispatch has this two-argument
    // callback ABI. Registration is a fixture boundary, not a second policy.
    const uint32_t sp=c.esp;c.esp-=4;Set(c.esp,sender);c.esp-=4;Set(c.esp,data);c.esp-=4;Set(c.esp,0);
    switch(kind){case EvMatrix:sub_000BD450(c,b);break;case EvEquip:sub_000CC190(c,b);break;
        case EvRemove:sub_000C25F0(c,b);break;case EvYaw:sub_000C8550(c,b);break;case EvPitch:sub_000C20E0(c,b);break;default:assert(false);}
    assert(c.esp==sp);c.esp+=20;c.eax=1;
}
namespace kernel {
uint32_t CallGuest(X86Context& c,uint32_t target,const uint32_t* args,size_t count){
    using namespace turret_test;assert(target==0x1f90a0&&count==5);++relay_calls;const auto sp=c.esp;
    for(size_t i=count;i;--i){c.esp-=4;Set(c.esp,args[i-1]);}c.esp-=4;Set(c.esp,0);
    sub_001F90A0(c,B());assert(c.esp==sp);const uint32_t result=c.eax;
    c.eax=c.ebx=c.ecx=c.edx=c.esi=c.edi=c.ebp=0xdeadbeef;c.esp-=44;
    c.cf=c.zf=c.sf=c.of=c.pf=c.df=1;c.xflags=0xf00;c.top=5;c.st[3]=19.5;
    c.mm[3]=0xabcd;c.xmm[5].q[1]=0xabcdef;c.mxcsr=0x5f80;_mm_setcsr(0x5f80);return result;
}
}
namespace turret_test {
X86Context Context(){X86Context c{};c.esp=SP;c.ecx=c.esi=H;c.eax=31;c.edx=32;c.ebx=33;c.edi=34;c.ebp=35;
    c.cf=1;c.zf=0;c.sf=1;c.of=1;c.pf=0;c.df=1;c.mxcsr=0x1f80;c.top=3;c.st[0]=1.25;c.xmm[5].q[1]=77;return c;}
void Setup(){
    kernel::memory.fill(0);trial_coop::Reset();trial_coop::SetAvailable(true);assert(trial_coop::Request());trial_coop::SetActive(true);
    TrialCoopTurretsReset();events.clear();grants={};removes={};poses={};idles={};camera_writes={};throw_event=false;relay_calls=0;patched_camera=false;asset_ready=true;
    Set(0x51c4dc,W);Set(W+4,S);Set(S+4,L);Set(S+8,L+8);Set(0x509a60,M);Set(M+0x834,101);
    Set(0x51c398,Services);Set(Services+8,Events);Set(0x46fbe8,0x20a540);
    Set(0x4f9f2c,EvMatrix);Set(0x4f9e0c,EvEquip);Set(0x4f9fec,EvRemove);Set(0x4f9f28,EvYaw);Set(0x4f9f24,EvPitch);
    Float(0x475440,-.65f);Float(0x475444,1.1623892784f);Float(0x475448,.7539822459f);Float(0x47544c,.38f);
    Float(0x44d660,.65f);Float(0x44bf94,1.f);Float(0x44c5a8,3.14159265359f);double tau=6.28318530718;std::memcpy(B()+0x44c5a0,&tau,8);
    Set(H,0x4753a0);Set(H+0x54,200);B()[H+0x171]=1;Set(Def+0x94,0x3f333333);
    for(unsigned i=0;i<2;++i){Set(P[i],0x46fbe8);Set(P[i]+0x54,100+i);Set(P[i]+0x368,i);Float(P[i]+0x9c,100);Float(P[i]+0x37c,1.3f);
        Set(L+4*i,L+0x100+0x100*i);Set(L+0x104+0x100*i,P[i]);Set(L+0x124+0x100*i,i);Identity(P[i]+0x10);}
}
void Payload(X86Context& c,uint32_t at,bool update,float x,float y,float z,bool turn=false){
    Identity(at+0x10);B()[at]=update;Float(at+0x40,x);Float(at+0x44,y);Float(at+0x48,z);Float(at+0x50,.1f);
    if(turn){Float(at+0x10,0);Float(at+0x14,1);Float(at+0x20,-1);Float(at+0x24,0);}
    std::memcpy(&c.xmm[0],B()+at+0x40,16);
}
void Mount(bool fixed){auto c=Context();Payload(c,SP+0x100,false,10,20,30);patched_camera=fixed;
    if(fixed){TrialCoopTurretScope scope(c,B(),TrialCoopTurretScope::Phase::State);CoopMount(c,B());}else OriginalMount(c,B());assert(c.esp==SP);}
void Update(bool fixed,float x,float y,float z,bool turn){auto c=Context();c.edi=EvMatrix;Payload(c,SP+0xb0,true,x,y,z,turn);
    Float(SP+0x14,F(P[0]+0x688));Float(SP+0x18,.4f);
    if(fixed){TrialCoopTurretScope scope(c,B(),TrialCoopTurretScope::Phase::Update);CoopUpdate(c,B());}else OriginalUpdate(c,B());assert(c.esp==SP);}
void Exit(bool fixed){auto c=Context();c.edi=EvRemove;
    if(fixed){TrialCoopTurretScope scope(c,B(),TrialCoopTurretScope::Phase::Exit);CoopExit(c,B());}else OriginalExit(c,B());assert(c.esp==SP);}
void Near(float a,float b){assert(std::fabs(a-b)<.0001f);}
void NativeCases(){
    Setup();Mount(false);assert(grants[0]==0&&grants[1]==1&&poses[0]==0&&poses[1]==1);
    assert(Get(Cameras[0]+0xe4)==Get(Def+0x94)&&!Get(Cameras[1]+0xe4)); // original P2 rewrites P1 camera
    Setup();Mount(true);assert(grants[0]==1&&grants[1]==1&&idles[0]==1&&idles[1]==1);
    for(unsigned i=0;i<2;++i){Near(F(P[i]+0x40),10+coop_turret_mount::kPlayerOffset[i][0]);Near(F(P[i]+0x44),20);
        assert(Get(P[i]+0x4fc)==25&&Get(Items[i]+4)==25&&Get(Cameras[i]+0xe4)==Get(Def+0x94)&&TrialCoopTurretMount(B(),P[i])==H);}
    Float(P[0]+0x688,-.25f);Float(P[1]+0x688,.5f);events.clear();Update(true,50,60,70,true);
    for(unsigned i=0;i<2;++i){Near(F(P[i]+0x40),50);Near(F(P[i]+0x44),60+coop_turret_mount::kPlayerOffset[i][0]);Near(F(P[i]+0x48),70);}
    unsigned yaw_events=0;for(const auto& e:events)if(e.kind==EvYaw){float limits[2];std::memcpy(limits,e.data.data(),8);float amount=(e.slot?.5f:-.25f)*.38f;
        Near(limits[0],(.4f-.7539822459f)-amount);Near(limits[1],(amount+.4f)+1.1623892784f);++yaw_events;}assert(yaw_events==2);
    for(unsigned n=0;n<20;++n)Update(true,50,60,70,true);
    Near(F(P[0]+0x44),60+coop_turret_mount::kPlayerOffset[0][0]);Near(F(P[1]+0x44),60+coop_turret_mount::kPlayerOffset[1][0]);
    Exit(true);for(unsigned i=0;i<2;++i)assert(removes[i]==1&&Get(P[i]+0x4fc)==1&&!TrialCoopTurretMount(B(),P[i]));
    Mount(true);assert(grants[0]==2&&grants[1]==2);TrialCoopTurretsReset();assert(!TrialCoopTurretMount(B(),P[0]));
    Setup();trial_coop::SetActive(false);Mount(true);assert(!grants[0]&&grants[1]==1&&!relay_calls);
    Setup();asset_ready=false;Mount(true);assert(!grants[0]&&grants[1]==1&&!relay_calls);Near(F(P[1]+0x40),10); // retained original asset/layout
    Setup();Float(P[1]+0x9c,0);Mount(true);assert(grants[0]==1&&!grants[1]); // do not equip a downed partner
    Near(F(P[0]+0x40),10+coop_turret_mount::kPlayerOffset[0][0]);
    auto p2_position=F(P[1]+0x40);Update(true,50,60,70,true);
    Near(F(P[0]+0x44),60+coop_turret_mount::kPlayerOffset[0][0]);Near(F(P[1]+0x40),p2_position);
    Float(P[1]+0x9c,100);Update(true,50,60,70,true);Near(F(P[1]+0x44),60+coop_turret_mount::kPlayerOffset[1][0]);
    assert(grants[1]==1&&Get(P[1]+0x4fc)==25&&TrialCoopTurretMount(B(),P[1])==H);
    Exit(true);assert(removes[0]==1&&removes[1]==1);
    for(unsigned manager_slot=0;manager_slot<2;++manager_slot){Setup();Set(M+0x834,100+manager_slot);Mount(true);assert(grants[0]==1&&grants[1]==1);}
    for(unsigned reset=0;reset<3;++reset){Setup();Mount(true);Set(0x51c4dc,W+0x1000);assert(!TrialCoopTurretMount(B(),P[0]));
        TrialCoopTurretsReset();Set(0x51c4dc,W);assert(!TrialCoopTurretMount(B(),P[0]));Mount(true);assert(TrialCoopTurretMount(B(),P[0])==H);}
}
void DownedCases(){
    for(unsigned dead=0;dead<2;++dead)for(bool before_mount:{false,true})for(bool revive:{false,true}){
        Setup();
        if(before_mount){Float(P[dead]+0x9c,0);Set(P[dead]+0x170,12);}
        Mount(true);
        if(!before_mount){Float(P[dead]+0x9c,0);Set(P[dead]+0x170,12);}
        const unsigned living=dead^1;
        const unsigned dead_poses=poses[dead];std::array<uint8_t,64> body{};
        std::memcpy(body.data(),B()+P[dead]+0x10,64);
        const float pitch=F(P[dead]+0x688),yaw=F(P[dead]+0x68c);
        events.clear();for(unsigned n=0;n<3;++n)Update(true,100+n*5,200,300,true);
        assert(poses[dead]==dead_poses&&!std::memcmp(body.data(),B()+P[dead]+0x10,64));
        Near(F(P[dead]+0x688),pitch);Near(F(P[dead]+0x68c),yaw);
        assert(Get(P[dead]+0x170)==12&&Get(P[living]+0x4fc)==25&&TrialCoopTurretMount(B(),P[living])==H);
        for(const auto& event:events)assert(event.slot==living);
        if(revive){
            Float(P[dead]+0x9c,100);Set(P[dead]+0x170,8);
            // Native revival may restore a different inventory selection.
            Set(P[dead]+0x4fc,1);Update(true,150,250,350,true);
            assert(Get(P[dead]+0x4fc)==25&&TrialCoopTurretMount(B(),P[dead])==H);
            Near(F(P[dead]+0x44),250+coop_turret_mount::kPlayerOffset[dead][0]);
            const auto counts=grants;
            for(unsigned n=0;n<5;++n)Update(true,150,250,350,true);
            assert(grants==counts);
        }
        Exit(true);
        assert(removes[living]==1&&removes[dead]==unsigned(!before_mount||revive));
        if(!revive)assert(Get(P[dead]+0x170)==12);
        assert(!TrialCoopTurretMount(B(),P[0])&&!TrialCoopTurretMount(B(),P[1]));
    }
}
X86Context EventContext(){auto c=Context();c.ecx=Events;Set(SP,100);Set(SP+4,EvPitch);Set(SP+8,SP+0x100);Set(SP+12,8);Set(SP+16,H+0x54);Float(SP+0x100,-.65f);Float(SP+0x104,.65f);return c;}
void IsolationCases(){
    Setup();auto owner=Context();TrialCoopTurretScope scope(owner,B(),TrialCoopTurretScope::Phase::Update);
    Float(SP+0x18,.4f);TrialCoopTurretAim(owner,B());auto c=EventContext(),before=c;
    std::array<uint8_t,0x400> scratch{};for(unsigned i=0;i<scratch.size();++i)scratch[i]=uint8_t(i*37);std::memcpy(B()+SP-0x400,scratch.data(),scratch.size());
    const auto csr=_mm_getcsr();_mm_setcsr(0x3f80);TrialCoopTurretEvent(c,B());assert(relay_calls==1&&!std::memcmp(&c,&before,sizeof c)&&_mm_getcsr()==0x3f80);
    assert(!std::memcmp(B()+SP-0x400,scratch.data(),scratch.size()));
    throw_event=true;try{TrialCoopTurretEvent(c,B());assert(false);}catch(int e){assert(e==17);}throw_event=false;
    assert(!std::memcmp(&c,&before,sizeof c)&&_mm_getcsr()==0x3f80&&!std::memcmp(B()+SP-0x400,scratch.data(),scratch.size()));
    TrialCoopTurretEvent(c,B());assert(relay_calls==3);_mm_setcsr(csr);
    const unsigned calls=relay_calls;
    for(unsigned bad=0;bad<10;++bad){auto e=EventContext();uint32_t address=0,old=0;
        switch(bad){case 0:address=SP;Set(address,101);break;case 1:address=SP+4;Set(address,99);break;case 2:address=SP+12;Set(address,9);break;
        case 3:address=H+0x54;old=200;Set(address,201);break;case 4:address=H;old=0x4753a0;Set(address,0);break;
        case 5:address=P[1]+0x54;old=101;Set(address,999);break;case 6:address=0x51c4dc;old=W;Set(address,W+4);break;
        case 7:e.ecx=Events+4;break;case 8:e.esp=0xfffffff0;break;case 9:address=P[1]+0x5c;Set(address,1);break;}
        auto copy=e;TrialCoopTurretEvent(e,B());assert(relay_calls==calls&&!std::memcmp(&copy,&e,sizeof e));if(address&&address<SP)Set(address,old);
    }
    for(unsigned bad=0;bad<2;++bad){auto e=EventContext();Set(SP+4,EvMatrix);Set(SP+12,96);Payload(e,SP+0x100,true,10,20,30);
        if(bad)Float(SP+0x110,std::numeric_limits<float>::quiet_NaN());else std::memset(B()+SP+0x110,0,64);
        auto copy=e;TrialCoopTurretEvent(e,B());assert(relay_calls==calls&&!std::memcmp(&copy,&e,sizeof e));}
}
}
int main(){turret_test::NativeCases();turret_test::DownedCases();turret_test::IsolationCases();std::puts("co-op helicopter native event/camera contracts passed; mesh and live firing verification pending");}

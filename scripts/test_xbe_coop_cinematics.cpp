// Execute the retained camera-stack selection, viewport setter and HUD caller.
// Camera animation, mission scripts, GPU rasterization and sound remain live checks.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
#include "kernel/kernel.h"
namespace kernel {static std::array<uint8_t,0x800000> memory{};}
namespace mem {bool IsCommitted(uint32_t page){return page<kernel::memory.size();}}
#include "../common/xbe_coop_cinematics.cpp"
namespace cinematic_test {
constexpr uint32_t world=0x600000,state=0x601000,layers=0x602000,stack=0x7f0000;
constexpr uint32_t owner[2]={0x610000,0x612000},layer[2]={0x620000,0x621000};
constexpr uint32_t camera[2]={0x630000,0x631000},chain[2]={0x640000,0x641000};
constexpr uint32_t half[2]={0x650000,0x651000},full=0x652000;
constexpr uint32_t graphics=0x660000,graphics_table=0x661000,provider=0x662000,provider_table=0x663000;
constexpr uint32_t callback[2]={0x664000,0x665000},callback_table=0x666000;
constexpr uint32_t manager=0x670000,profile=0x672000,weapon=0x673000,controller=0x674000;
constexpr uint32_t cutscene=0x675000,acquire_event=0x70001,release_event=0x70002;
uint8_t* Bytes(){return kernel::memory.data();}
uint32_t Word(uint32_t a){uint32_t v;std::memcpy(&v,Bytes()+a,4);return v;}
void Put(uint32_t a,uint32_t v){std::memcpy(Bytes()+a,&v,4);}
std::vector<std::array<uint32_t,2>> hud;
std::vector<uint32_t> begins,ends;
std::vector<uint32_t> acquired,released,focused;
std::vector<std::array<uint32_t,4>> popups;
unsigned setter_calls=0;
X86Context Setup(int cinematic=-1,uint32_t type=0x46fd70){
    kernel::memory.fill(0);trial_coop::Reset();trial_coop::SetAvailable(true);
    assert(trial_coop::Request());trial_coop::SetActive(true);
    Put(0x51c4dc,world);Put(world+4,state);Put(state+4,layers);Put(state+8,layers+8);
    Put(0x509a60,manager);Put(manager+0x834,7);Put(manager+0x0c,profile);
    Put(manager,0x44bf20);Put(0x44bf2c,0x128f0);
    Put(weapon+0x54,50);Put(weapon+0x58,8);Put(controller,0x44d9c0);
    Put(controller+0xb8,8);Bytes()[controller+0x91]=1;Put(controller+0xd8,0x3f800000);
    Put(0x4f9e90,acquire_event);Put(0x4f9f64,release_event);Put(0x4f9d54,0x22);
    Put(0x46fd70+0x94,0xf1040);Put(cutscene,0x465910);Put(cutscene+0x54,19);
    Bytes()[0x44df20]='E'; // Distinguish the special end-level camera class query.
    Put(state+0x10,provider);Put(provider,provider_table);Put(provider_table+0x10,0xf1030);
    Put(0x5150b0,graphics);Put(graphics,graphics_table);Put(graphics_table+0x58,0xf1000);
    Put(callback_table+8,0xf1010);Put(callback_table+12,0xf1020);
    for(unsigned s=0;s<2;++s){
        Put(owner[s],0x46fbe8);Put(owner[s]+0x54,7+s);Put(owner[s]+0x368,s);
        Bytes()[owner[s]+0x2ac]=1;Bytes()[owner[s]+0x848]=1;
        Put(layers+s*4,layer[s]);Put(layer[s],callback[s]);Put(callback[s],callback_table);
        Put(layer[s]+4,owner[s]);Put(layer[s]+8,chain[s]);Put(layer[s]+0x20,half[s]);Put(layer[s]+0x24,s);
        Put(chain[s]+8,1);Put(chain[s]+4,chain[s]+0x20);Put(chain[s]+0x24,chain[s]+0x40);
        Put(chain[s]+0x48,camera[s]);Put(camera[s],int(s)==cinematic?type:0x47c780);
        Put(camera[s]+0x54,17+s);Put(camera[s]+0xd0,0x478d20);Put(camera[s]+0xe8,half[s]);
        Bytes()[camera[s]+0x1e3]=1;
    }
    hud.clear();begins.clear();ends.clear();popups.clear();setter_calls=0;
    acquired.clear();released.clear();focused.clear();
    X86Context c{};c.ecx=world;c.esp=stack;c.eax=12;c.edx=23;c.ebx=34;c.esi=45;c.edi=56;c.ebp=67;
    c.cf=c.of=c.zf=c.sf=c.pf=1;c.df=1;c.fcw=0x37f;c.mxcsr=0x1f80;
    for(unsigned i=0;i<8;++i){c.xmm[i].q[0]=i+1;c.xmm[i].q[1]=i+9;c.st[i]=i+.25;}
    return c;
}
}
uint32_t TrialCoopPlayer(unsigned slot){return slot<2?cinematic_test::owner[slot]:0;}
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
extern "C" void sub_000128F0(X86Context&,uint8_t*);
extern "C" void sub_000C2670(X86Context&,uint8_t*);
extern "C" void sub_000CC240(X86Context&,uint8_t*);
extern "C" void sub_000BD3F0(X86Context&,uint8_t*);
void x86_icall(X86Context& c,uint8_t* base,uint32_t target){
    using namespace cinematic_test;
    if(target==0x128f0){assert(c.ecx==manager);sub_000128F0(c,base);return;}
    if(target==0xf1000){assert(c.ecx==graphics);c.eax=full;c.esp+=4;return;}
    if(target==0xf1010){begins.push_back(Word(c.esp+4));c.esp+=8;return;}
    if(target==0xf1020){ends.push_back(c.ecx);c.esp+=4;return;}
    if(target==0xf1040){focused.push_back(Word(c.esp+4));c.esp+=8;return;}
    assert(target==0xf1030&&c.ecx==provider);hud.push_back({Word(c.esp+4),Word(c.esp+8)});c.esp+=12;
}
X86_FUNC(sub_0008BD60){ctx.eax=cinematic_test::Word(ctx.ecx+0x18);ctx.esp+=4;}
X86_FUNC(sub_0006BA40){ctx.esp+=4;} // logging/stat label only, not the goal counter
X86_FUNC(sub_001F8FC0){
    using namespace cinematic_test;const uint32_t id=Word(ctx.esp+4);
    assert((id>=7&&id<=8)||(id>=17&&id<=18));
    ctx.eax=id<17?owner[id-7]:camera[id-17];ctx.esp+=4;
}
// Event routing and unrelated weapon services are boundaries; retained retail
// C2670/CC240/BD3F0 perform the actual lock/unlock and pitch-reset branches.
void CameraEvent(X86Context& c,uint8_t* b,uint32_t id,uint32_t event,uint32_t sender){
    using namespace cinematic_test;assert(id==7||id==8);
    const uint32_t sp=c.esp;c.esp-=0x100;c.ecx=owner[id-7];
    Put(c.esp+4,0);Put(c.esp+8,c.esp+0x40);Put(c.esp+0x40,sender);
    if(event==acquire_event){acquired.push_back(id);sub_000C2670(c,b);}
    else if(event==release_event){released.push_back(id);sub_000CC240(c,b);}
    else{assert(event==0x22);sub_000BD3F0(c,b);}
    assert(c.esp==sp-0x100+12);c.esp=sp;
}
X86_FUNC(sub_001AB150){
    using namespace cinematic_test;
    const uint32_t id=Word(ctx.esp+4),event=Word(ctx.esp+8),sender=Word(ctx.ecx+0x54);
    CameraEvent(ctx,base,id,event,sender);ctx.esp+=16;
}
X86_FUNC(sub_001F90A0){
    using namespace cinematic_test;
    const uint32_t id=Word(ctx.esp+4),event=Word(ctx.esp+8),sender=Word(Word(ctx.esp+20));
    CameraEvent(ctx,base,id,event,sender);ctx.esp+=24;
}
X86_FUNC(sub_000BFE00){
    using namespace cinematic_test;assert(ctx.ecx==owner[0]||ctx.ecx==owner[1]);
    Put(ctx.ecx+0x170,Word(ctx.esp+4));ctx.esp+=12;
} // Existing player-state machine boundary; its requested state remains retail.
X86_FUNC(sub_000423E0){ctx.esp+=12;}
X86_FUNC(sub_000BDDD0){ctx.esp+=4;}
X86_FUNC(sub_000C8930){ctx.esp+=4;}
X86_FUNC(sub_000C1E60){std::abort();}
X86_FUNC(sub_00041F80){std::abort();}
X86_FUNC(sub_00216060){std::abort();}
X86_FUNC(sub_00042140){std::abort();}
X86_FUNC(sub_002107C0){std::abort();}
X86_FUNC(sub_002084E0){std::abort();}
X86_FUNC(sub_0020A6D0){
    using namespace cinematic_test;assert(ctx.ecx==owner[0]||ctx.ecx==owner[1]);
    assert(Word(ctx.esp+4)==0x44bf84&&Word(ctx.esp+8)==0);ctx.esp+=12;
} // Campaign registration's independent actor property notification.
X86_FUNC(sub_00262120){ctx.esp+=4;} // Popup formatting service boundary.
X86_FUNC(sub_0025CE20){assert(cinematic_test::Word(ctx.esp+4)==1);ctx.eax=40;ctx.esp+=4;}
X86_FUNC(sub_001F92B0){
    using namespace cinematic_test;ctx.eax=Word(ctx.esp+4);Put(ctx.eax,Word(ctx.esp+8));ctx.esp+=4;
} // Return a stable label token instead of invoking the localization service.
X86_FUNC(sub_002D6322){ctx.esp+=4;} // Decimal formatting is not player attribution.
X86_FUNC(sub_00073060){
    using namespace cinematic_test;
    popups.push_back({Word(ctx.esp+4),Word(ctx.esp+8),Word(ctx.esp+12),Word(profile+8)});ctx.esp+=4;
}
// ORIGINAL_COOP_CINEMATIC_CALLERS
// ORIGINAL_COOP_REWARD_CALLERS
namespace kernel {
uint32_t CallGuest(X86Context& c,uint32_t address,const uint32_t* args,size_t count){
    using namespace cinematic_test;
    assert(address==0x21e0a0&&count==1);++setter_calls;
    c.esp-=4;Put(c.esp,args[0]);c.esp-=4;Put(c.esp,0xf2000);
    sub_0021E0A0(c,Bytes());
    // Isolate all service register/FP side effects and writes below its stack.
    std::memset(Bytes()+c.esp-0x80,0xa5,0x40);c.eax=0xdead;c.ecx=0xbeef;c.edx=0x123;
    c.esi=4;c.edi=5;c.ebp=6;c.ebx=7;c.esp-=0x70;c.cf^=1;c.df^=1;c.top=3;
    c.fcw=0x27f;c.mxcsr=0x3f80;c.st[0]=987;c.xmm[7].q[0]=1234;_mm_setcsr(0x3f80);return 0;
}
}
namespace cinematic_test {
using Caller=void(*)(X86Context&,uint8_t*,TrialCoopCinematicView&);
bool Scene(Caller call,X86Context& c,TrialCoopCinematicView& scope,unsigned slot){
    c.esp=stack;c.ebx=world;c.edi=slot;c.eax=state;c.edx=half[slot];
    call(c,Bytes(),scope);return c.eax!=0xffffffffu;
}
void Hud(Caller call,X86Context& c,TrialCoopCinematicView& scope,unsigned slot){
    c.esp=stack;c.edi=world;Put(stack+0x10,slot);call(c,Bytes(),scope);
}
void CheckOriginalAndShared(){
    unsigned cases=0;
    for(uint32_t type:{0x46fd70u,0x477da8u,0x477c58u})for(unsigned slot=0;slot<2;++slot){
        auto original=Setup(slot,type);
        {TrialCoopCinematicView scope(original,Bytes(),true);
            assert(Scene(OriginalScene,original,scope,0));assert(Scene(OriginalScene,original,scope,1));
            assert(Word(camera[slot]+0xe8)==half[slot]);
        }
        original.ecx=world;
        {TrialCoopCinematicView scope(original,Bytes(),false);
            Hud(OriginalHud,original,scope,0);Hud(OriginalHud,original,scope,1);
        }
        assert(hud.size()==2&&hud[0][1]==half[0]&&hud[1][1]==half[1]&&!setter_calls);
        auto fixed=Setup(slot,type);const unsigned csr=_mm_getcsr();_mm_setcsr(0x5f80);
        {TrialCoopCinematicView scope(fixed,Bytes(),true);
            assert(TrialCoopPresentation()==2);
            assert(Scene(CoopScene,fixed,scope,0)==(slot==0));
            assert(Scene(CoopScene,fixed,scope,1)==(slot==1));
            assert(Word(camera[slot]+0xe8)==full&&Word(camera[1-slot]+0xe8)==half[1-slot]);
        }
        assert(Word(camera[slot]+0xe8)==half[slot]&&Bytes()[camera[slot]+0x100]);
        assert(_mm_getcsr()==0x5f80);fixed.ecx=world;
        {TrialCoopCinematicView scope(fixed,Bytes(),false);
            Hud(CoopHud,fixed,scope,0);Hud(CoopHud,fixed,scope,1);
        }
        assert(hud.size()==1&&hud[0][0]==7+slot&&hud[0][1]==full);
        assert(begins.size()==1&&begins[0]==camera[slot]&&ends.size()==1&&ends[0]==callback[slot]);
        assert(Word(camera[slot]+0xe8)==half[slot]&&setter_calls==4);
        assert(_mm_getcsr()==0x5f80);_mm_setcsr(csr);++cases;
        // The original camera stack returning to gameplay restores both views next frame.
        Put(camera[slot],0x47c780);fixed.ecx=world;
        {TrialCoopCinematicView scope(fixed,Bytes(),true);
            assert(TrialCoopPresentation()==1);assert(Scene(CoopScene,fixed,scope,0));assert(Scene(CoopScene,fixed,scope,1));
        }
        assert(Word(camera[0]+0xe8)==half[0]&&Word(camera[1]+0xe8)==half[1]);
    }
    std::printf("co-op cinematics: %u original two-view negatives/shared scene and HUD cases plus split return passed\n",cases);
}
void CheckGuards(){
    for(unsigned invalid=0;invalid<12;++invalid){
        auto c=Setup(0);
        switch(invalid){
            case 0:trial_coop::SetAvailable(false);break;
            case 1:trial_coop::Reset();break;
            case 2:trial_coop::SetActive(false);break;
            case 3:c.ecx=world+4;break;
            case 4:Put(state+8,layers+12);break;
            case 5:Put(owner[1]+0x5c,1);break;
            case 6:Put(layer[1]+4,owner[0]);break;
            case 7:Put(layer[0]+0x24,1);break;
            case 8:Put(camera[0],0x47c780);break;
            case 9:Put(camera[0]+0x5c,1);break;
            case 10:Put(camera[0]+0xd0,0);break;
            case 11:Put(camera[0]+0x54,0xffffffffu);break;
        }
        {TrialCoopCinematicView scope(c,Bytes(),true);
            assert(TrialCoopPresentation()!=2);assert(Scene(CoopScene,c,scope,0));assert(Scene(CoopScene,c,scope,1));
        }
        assert(!setter_calls&&Word(camera[0]+0xe8)==half[0]);
    }
    auto c=Setup(0);const auto before=c;
    std::array<uint8_t,256> scratch{};std::memcpy(scratch.data(),Bytes()+stack-256,256);
    {TrialCoopCinematicView scope(c,Bytes(),true);
        c.ebx=world;c.edi=0;c.esi=camera[0];const auto select=c;
        assert(!scope.Scene());assert(!std::memcmp(&select,&c,sizeof c));
        assert(!std::memcmp(scratch.data(),Bytes()+stack-256,256));
    }
    assert(Word(camera[0]+0xe8)==half[0]);
    c=Setup(1);
    try{TrialCoopCinematicView scope(c,Bytes(),true);assert(Scene(CoopScene,c,scope,1));throw 17;}
    catch(int x){assert(x==17);}
    assert(setter_calls==1&&Word(camera[1]+0xe8)==half[1]&&Bytes()[camera[1]+0x100]);
    c=Setup(0);
    {TrialCoopCinematicView scope(c,Bytes(),true);assert(Scene(CoopScene,c,scope,0));Put(camera[0]+0x54,999);}
    assert(setter_calls==1); // A destroyed/reused camera is never written on exit.
    std::puts("co-op cinematics: admission, caller context/scratch, unwind and camera-lifetime guards passed (no GPU)");
}
using RewardCaller=void(*)(X86Context&,uint8_t*);
void RegisterPlayers(X86Context& c,unsigned last){
    for(unsigned slot:{1-last,last}){
        c.esp=stack;c.esi=owner[slot];OriginalPlayerRegistration(c,Bytes());
        assert(c.esp==stack&&Word(manager+0x834)==7+slot);
    }
    assert(Word(manager+0x10)==2&&Bytes()[manager+0x838]==1);
}
void Headshot(RewardCaller caller,X86Context& c,unsigned player,bool eligible=true){
    Put(weapon+0x58,Word(owner[player]+0x54));c.esp=stack;c.edi=weapon;
    Put(stack+0x10,0x80);caller(c,Bytes());
    // Continue the retained player-attribution and headshot accounting branches.
    // Collision classification/target eligibility are fixture inputs; no new goal
    // counter or popup is implemented by the correction.
    c.ebx=Word(stack+0x10);c.esi=controller;OriginalHeadshotEligibility(c,Bytes());
    Put(stack+0x14,eligible?1:0);Put(stack+0x64,0);OriginalHeadshotCount(c,Bytes());
    assert(c.esp==stack);
}
void CheckRewards(){
    X86Context c{};
    for(unsigned last=0;last<2;++last){
        // Execute the real startup virtual call and both original setters. P2
        // starts last in game; an assumed manager=P1 fixture missed the defect.
        c=Setup();RegisterPlayers(c,last);Put(world+0xc,100);
        Headshot(OriginalDamagePlayer,c,last);assert(Word(profile+8)==1&&popups.size()==1);
        Put(world+0xc,101);Headshot(OriginalDamagePlayer,c,1-last);
        assert(Word(profile+8)==1&&popups.size()==1);
        c.esp=stack;c.esi=controller;Put(controller+0xb8,8-last);
        OriginalKillcamKiller(c,Bytes());assert(c.eax==0&&c.esp==stack);
        CoopKillcamKiller(c,Bytes());assert(c.eax==1&&c.esp==stack);

        c=Setup();RegisterPlayers(c,last);
        for(unsigned player=0;player<2;++player){
            Put(world+0xc,100+player*3);Headshot(CoopDamagePlayer,c,player);
            assert(Word(profile+8)==player+1&&Word(profile+0x40)==player+1);
            assert(popups.back()==(std::array<uint32_t,4>{1,0x44dc3c,0x44dc60,player+1}));
            // Retail same-tick dedup and target eligibility remain authoritative.
            Headshot(CoopDamagePlayer,c,player);assert(Word(profile+8)==player+1);
            const size_t published=popups.size();Put(world+0xc,101+player*3);
            Headshot(CoopDamagePlayer,c,player,false);
            assert(Word(profile+8)==player+1&&popups.size()==published);
            c.esp=stack;c.esi=controller;Put(controller+0xb8,7+player);
            OriginalKillcamKiller(c,Bytes());assert(c.eax==unsigned(player==last));
            CoopKillcamKiller(c,Bytes());assert(c.eax==1&&c.esp==stack);
        }
        assert(Word(manager+0x834)==7+last&&Word(manager+0x10)==2);
        // Only the missing native lookup changes, and only EAX; neither hook
        // mutates the campaign registration or any guest bytes/FP/context state.
        for(unsigned player=0;player<2;++player){
            c.eax=0xffffffffu;c.ecx=manager;c.edi=weapon;Put(weapon+0x58,7+player);
            const std::vector<uint8_t> memory_before(kernel::memory.begin(),kernel::memory.end());
            auto expected=c;expected.eax=player;
            const auto csr=_mm_getcsr();TrialCoopDamagePlayer(c,Bytes());
            assert(!std::memcmp(&c,&expected,sizeof c)&&!std::memcmp(Bytes(),memory_before.data(),memory_before.size())&&_mm_getcsr()==csr);
            c.eax=7+last;c.edi=7+player;c.esi=controller;Put(controller+0xb8,c.edi);
            expected=c;expected.eax=c.edi;TrialCoopKillcamKiller(c,Bytes());
            assert(!std::memcmp(&c,&expected,sizeof c)&&_mm_getcsr()==csr);
            c.eax=0;c.ecx=manager;c.edi=weapon;expected=c;
            TrialCoopDamagePlayer(c,Bytes());assert(!std::memcmp(&c,&expected,sizeof c));
        }
    }
    for(unsigned invalid=0;invalid<11;++invalid){
        c=Setup();c.eax=0xffffffffu;c.ecx=manager;c.edi=weapon;
        switch(invalid){
            case 0:trial_coop::Reset();break;
            case 1:trial_coop::SetActive(false);break;
            case 2:trial_coop::SetAvailable(false);break;
            case 3:Put(owner[1]+0x54,0xffffffffu);break;
            case 4:Put(owner[1]+0x5c,1);break;
            case 5:Put(layer[1]+4,owner[0]);break;
            case 6:Put(state+8,layers+4);break;
            case 7:Put(manager+0x834,99);break;
            case 8:Put(weapon+0x58,99);break;
            case 9:Put(weapon+0x5c,1);break;
            case 10:Put(weapon+0x54,0xffffffffu);break;
        }
        const auto before=c;TrialCoopDamagePlayer(c,Bytes());assert(!std::memcmp(&c,&before,sizeof c));
    }
    c=Setup();c.eax=0xffffffffu;c.ecx=manager;c.edi=weapon;
    auto expected=c;expected.eax=1;TrialCoopDamagePlayer(c,Bytes());assert(!std::memcmp(&c,&expected,sizeof c));
    c.eax=7;c.edi=8;c.esi=controller;expected=c;expected.eax=8;
    TrialCoopKillcamKiller(c,Bytes());assert(!std::memcmp(&c,&expected,sizeof c));
    for(unsigned invalid=0;invalid<4;++invalid){
        c=Setup();c.eax=7;c.edi=8;c.esi=controller;
        if(invalid==0)Put(controller+0x54,0xffffffffu);
        if(invalid==1)Put(controller+0xb8,99);
        if(invalid==2)c.eax=99;
        if(invalid==3)c.edi=99;
        const auto before=c;TrialCoopKillcamKiller(c,Bytes());assert(!std::memcmp(&c,&before,sizeof c));
    }
    std::puts("co-op rewards: original last-player registration negative, both registration orders/players, shared headshot counter/dedup, popup publication and killer comparison passed; collision, camera animation and popup pixels remain live checks");
}
void CheckCameraOwnership(){
    // P2 starts last. Retail locks/focuses P2 even though the authored camera
    // requests slot zero, leaving the PLAYER-trigger owner P1 in movement state.
    auto c=Setup(0);RegisterPlayers(c,1);c.esp=stack;c.esi=camera[0];
    OriginalCameraAcquire(c,Bytes());
    assert(acquired==std::vector<uint32_t>{8}&&focused==std::vector<uint32_t>{8});
    assert(Word(owner[0]+0x170)==0&&Word(owner[1]+0x170)==13&&c.esp==stack);
    c.esp=stack;c.esi=camera[0];OriginalCameraRelease(c,Bytes());
    assert(released==std::vector<uint32_t>{8}&&Word(owner[1]+0x170)==0&&c.esp==stack);
    for(unsigned last=0;last<2;++last)for(unsigned slot=0;slot<2;++slot){
        c=Setup(0);RegisterPlayers(c,last);Put(camera[0]+0x1dc,slot);
        c.esp=stack;c.esi=camera[0];CoopCameraAcquire(c,Bytes());
        assert(acquired==std::vector<uint32_t>{7+slot}&&focused==acquired&&c.esp==stack);
        assert(Word(owner[slot]+0x170)==13&&Word(owner[1-slot]+0x170)==0);
        // Retirement is allowed to have marked the camera before releasing it.
        Put(camera[0]+0x5c,1);c.esp=stack;c.esi=camera[0];CoopCameraRelease(c,Bytes());
        assert(released==acquired&&Word(owner[slot]+0x170)==0&&c.esp==stack);
        assert(Word(manager+0x834)==7+last);
    }
    c=Setup(0);RegisterPlayers(c,1);Bytes()[camera[0]+0x1e3]=0;
    c.esp=stack;c.esi=camera[0];CoopCameraAcquire(c,Bytes());
    assert(acquired.empty()&&focused==std::vector<uint32_t>{7});
    assert(Word(owner[0]+0x170)==0&&Word(owner[1]+0x170)==0);
    for(bool fixed:{false,true}){
        c=Setup(0);RegisterPlayers(c,1);
        Put(owner[0]+0x688,0x3f800000);Put(owner[1]+0x688,0x3f800000);
        c.esp=stack;c.esi=cutscene;c.edi=0x22;
        (fixed?CoopCutscenePitch:OriginalCutscenePitch)(c,Bytes());
        assert(Word(owner[fixed?0:1]+0x688)==0);
        assert(Word(owner[fixed?1:0]+0x688)==0x3f800000&&c.esp==stack);
    }
    for(unsigned invalid=0;invalid<15;++invalid){
        c=Setup(0);c.ecx=manager;c.eax=7;c.esi=camera[0];Put(camera[0]+0x1dc,1);
        switch(invalid){
            case 0:trial_coop::Reset();break;
            case 1:trial_coop::SetActive(false);break;
            case 2:trial_coop::SetAvailable(false);break;
            case 3:Put(owner[1]+0x54,0xffffffffu);break;
            case 4:Put(owner[1]+0x5c,1);break;
            case 5:Put(layer[1]+4,owner[0]);break;
            case 6:Put(state+8,layers+4);break;
            case 7:Put(manager+0x834,99);break;
            case 8:Put(camera[0],0x47c780);break;
            case 9:Put(camera[0]+0xd0,0);break;
            case 10:Put(camera[0]+0x54,0xffffffffu);break;
            case 11:Put(camera[0]+0x1dc,2);break;
            case 12:c.ecx=manager+4;break;
            case 13:c.eax=8;break;
            case 14:c.esi=0xfffffff0u;break;
        }
        const auto expected=c;TrialCoopCameraPlayer(c,Bytes());assert(!std::memcmp(&c,&expected,sizeof c));
    }
    c=Setup(0);c.ecx=manager;c.eax=7;c.esi=camera[0];Put(camera[0]+0x1dc,1);
    auto expected=c;expected.eax=8;
    const std::vector<uint8_t> saved(kernel::memory.begin(),kernel::memory.end());const auto csr=_mm_getcsr();
    TrialCoopCameraPlayer(c,Bytes());assert(!std::memcmp(&c,&expected,sizeof c));
    assert(!std::memcmp(Bytes(),saved.data(),saved.size())&&_mm_getcsr()==csr);
    c.ecx=manager;c.eax=7;c.esi=cutscene;Put(manager+0x834,7);
    expected=c;TrialCoopCutscenePlayer(c,Bytes());assert(!std::memcmp(&c,&expected,sizeof c));
    Put(cutscene,0x47c780);expected=c;TrialCoopCutscenePlayer(c,Bytes());assert(!std::memcmp(&c,&expected,sizeof c));
    std::puts("co-op camera ownership: retail P2 lock/focus negative, both authored slots/registration orders, release after retirement, no-control focus, original pitch reset and guarded context preservation passed; trigger traversal and scene timing remain live checks");
}
}
int main(){cinematic_test::CheckOriginalAndShared();cinematic_test::CheckGuards();cinematic_test::CheckRewards();cinematic_test::CheckCameraOwnership();}

// Admission/state tests. Guest factories are observed at their call boundary;
// this deliberately does not claim a simulated factory proves live co-op.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cstdio>
#include <limits>
#include <vector>
#include "kernel/kernel.h"
namespace kernel {static std::array<uint8_t,0x800000> memory{};}
namespace mem {bool IsCommitted(uint32_t page){return page<kernel::memory.size();}}
#ifdef UC_BURNER_RELEASE
// Compile the real gameplay source with diagnostic sinks deliberately absent.
// Removing a release guard must fail, even if a trace's runtime gate is closed.
#define fprintf ReleaseDiagnosticFprintfMustNotCompile
#define fputc ReleaseDiagnosticFputcMustNotCompile
#define fflush ReleaseDiagnosticFflushMustNotCompile
#endif
#include "../common/xbe_coop_guest.cpp"
#ifdef UC_BURNER_RELEASE
#undef fprintf
#undef fputc
#undef fflush
#endif
void TrialCoopBodiesEnsure(X86Context&,uint8_t*){}
static unsigned death_resets=0,downed_camera_resets=0,body_resets=0,shield_resets=0,progression_resets=0,interaction_resets=0,reward_resets=0,ko_reward_resets=0,turret_resets=0;
static unsigned spin_resets=0;
void TrialCoopDeathReset(){++death_resets;}
void TrialCoopDownedCameraReset(){++downed_camera_resets;}
void TrialCoopBodiesReset(){++body_resets;}
void TrialCoopShieldReset(){++shield_resets;}
void TrialCoopProgressionReset(){++progression_resets;}
void TrialCoopInteractionReset(){++interaction_resets;}
void TrialCoopRewardsReset(){++reward_resets;}
void TrialCoopSpinReset(){++spin_resets;}
void TrialCoopKORewardsReset(){++ko_reward_resets;}
void TrialCoopTurretsReset(){++turret_resets;}
X86_FUNC(sub_002180A0);
X86_FUNC(sub_00218120);
X86_FUNC(sub_00172380);
X86_FUNC(sub_00207CD0);X86_FUNC(sub_00207D20);X86_FUNC(sub_00207E50);
X86_FUNC(sub_00221BA0);X86_FUNC(sub_00221CA0);
X86_FUNC(sub_0009F590);X86_FUNC(sub_003E3610);X86_FUNC(sub_002D6780);
X86_FUNC(sub_003F1900);X86_FUNC(sub_003E35D0);X86_FUNC(sub_003E35F0);
X86_FUNC(sub_003E2FD0);X86_FUNC(sub_003E4270);X86_FUNC(sub_003E55B0);
// RETAIL_DEATH_LISTENERS
// RETAIL_AUDIO_LISTENER_SELECTION
// RETAIL_VIEWPORT_FUNCTIONS
namespace {
constexpr uint32_t p1=0x610000,p2=0x612000,w=0x620000,ws=0x624000;
constexpr uint32_t views=0x630000,v1=0x631000,v2=0x632000,services=0x640000;
constexpr uint32_t flag_owner=0x650000,flag_map=0x651000,flag_data=0x652000;
unsigned creates=0,begins=0,ends=0;
unsigned health_sets=0,startup_advances=0,ordinary_updates=0,hud_updates=0;
constexpr uint32_t shield_flags=0x653800,shield_owned=0x654000,shield_animation=0x655000,shield_item=0x656000;
unsigned shield_allocations=0,shield_frees=0,shield_inputs=0,shield_animations=0;
bool shield_allocation_failure=false,shield_factory_mismatch=false;
float factory_health=0;
constexpr uint32_t registration_list=0x680000,registration_record=0x681000;
constexpr uint32_t registration_node=0x682000,registration_free=0x683000,registration_buckets=0x684000;
uint32_t registration_heap=0x687000;
constexpr uint32_t render_parameters=0x690000,split_viewport=0x692000,viewport_table=0x694000;
constexpr uint32_t distortion=0x695000,distortion_texture=0x696000,distortion_target=0x697000;
constexpr uint32_t scene_target=0x698000,scene_depth=0x699000,graphics=0x69a000,graphics_table=0x69b000;
constexpr uint32_t sdk_device=0x700000,sdk_viewport=sdk_device+0xee0;
uint32_t current_target=scene_target,current_depth=scene_depth;
std::array<uint32_t,4> caller_rect{};
unsigned viewport_restores=0,viewport_sets=0,distortion_clears=0;
bool viewport_throw=false;
std::vector<std::array<uint32_t,2>> target_changes;
std::array<float,16> expected{};
uint8_t* B(){return kernel::memory.data();}
void PutReal(uint32_t address,float value){std::memcpy(B()+address,&value,4);}
void Setup(){
    Forget();trial_coop::Reset();kernel::memory.fill(0);creates=begins=ends=0;
    health_sets=startup_advances=ordinary_updates=hud_updates=0;factory_health=0;
    shield_allocations=shield_frees=shield_inputs=shield_animations=0;
    shield_allocation_failure=shield_factory_mismatch=false;
    Write(B(),p1,0x46fbe8);Write(B(),p1+0x54,349);Write(B(),p1+0x308,900);
    PutReal(p1+0x9c,.75f);PutReal(p1+0xa0,1.f);
    for(unsigned i=0;i<16;++i)expected[i]=i%5==0?1.f:0.f;
    expected[12]=123;expected[13]=456;expected[14]=789;
    std::memcpy(B()+p1+0x10,expected.data(),64);
    Write(B(),0x51c4dc,w);Write(B(),w+4,ws);Write(B(),ws+4,views);Write(B(),ws+8,views+4);
    Write(B(),views,v1);Write(B(),v1+4,p1);
    Write(B(),0x51c398,services);Write(B(),services+4,services+0x100);
    Write(B(),0x51c3e0,0x654321);Write(B(),ws+0x2044,0xaabbccdd);
    Write(B(),ws+0x2048,0x12345678);Write(B(),ws+0x204c,0x87654321);
    Write(B(),w+0x948,2);Write(B(),w+0x94c,1);B()[0x51bdf4]=1;
    Write(B(),w+0xf74,4);Write(B(),w+0xf78,5);
    Write(B(),ws+0x2054,flag_owner);Write(B(),flag_owner+0x2c,flag_data);
    Write(B(),flag_owner+0x34,flag_map);Write(B(),flag_owner+0x38,32);
    for(unsigned i=0;i<32;++i){Write(B(),flag_map+i*8,i);Write(B(),flag_map+i*8+4,i);}
    B()[flag_data+7]=2;B()[flag_data+0x10]=0;B()[flag_data+0x11]=1;B()[flag_data+23]=7;
}
X86Context Context(){X86Context c{};c.esp=0x7f0000;c.eax=p1;c.ecx=p1;return c;}
void PrepareUpdateGate(){
    Write(B(),p2+0x300,0x653000);Write(B(),0x653000,0x653100);
    Write(B(),0x653104,0xf0001);Write(B(),0x46fbe8+0x64,0x2180a0);
    Write(B(),0x44c770,0x3a83126f); // Exact retail 0.001f comparison constant.
}
void RunUpdateGate(){
    auto c=Context();c.ecx=p2;c.esi=0x12345678;
    sub_00172380(c,B());
    assert(c.esp==Context().esp+4&&c.esi==0x12345678&&c.top==0);
}
}
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
void x86_icall(X86Context& c,uint8_t* b,uint32_t target){
    if(target==0xf2000){
        assert(c.ecx==w&&Read(b,c.esp+4)==1&&Read(b,c.esp+8)==8&&Read(b,c.esp+12)==0);
        ++shield_inputs;x87_push(c,0);c.esp+=16;return;
    }
    if(target==0xf1000){assert(c.ecx==render_parameters);c.eax=Read(b,c.ecx+4);c.esp+=4;return;}
    if(target>=0xf1001&&target<=0xf1004){
        assert(c.ecx==split_viewport||c.ecx==split_viewport+0x100);
        x87_push(c,double(caller_rect[target-0xf1001]));c.esp+=4;return;
    }
    if(target==0xf1005){assert(c.ecx==graphics);c.eax=distortion;c.esp+=4;return;}
    if(target==0xf0001){assert(c.ecx==0x653000);++hud_updates;c.esp+=8;return;}
    assert(target==0x2180a0&&c.ecx==p2);sub_002180A0(c,b);
}
X86_FUNC(sub_00070D90){std::abort();}
X86_FUNC(sub_002D6780){ctx.eax=uint32_t(X87_ST(0));x87_pop(ctx);ctx.esp+=4;}
X86_FUNC(sub_003E3610){
    const uint32_t viewport=Read(base,ctx.esp+4);
    std::memcpy(base+sdk_viewport,base+viewport,24);++viewport_sets;ctx.eax=0;ctx.esp+=8;
}
X86_FUNC(sub_003F1900){
    assert(Read(base,ctx.esp+4)==distortion_texture&&Read(base,ctx.esp+8)==0);
    ctx.eax=distortion_target;ctx.esp+=12;
}
X86_FUNC(sub_003E35D0){ctx.eax=current_target;ctx.esp+=4;}
X86_FUNC(sub_003E35F0){ctx.eax=current_depth;ctx.esp+=4;}
X86_FUNC(sub_003E2FD0){
    current_target=Read(base,ctx.esp+4);current_depth=Read(base,ctx.esp+8);
    target_changes.push_back({current_target,current_depth});
    // Original 3E321C -> 3E7F70 -> 3E3610 applies immutable 3F1CD8
    // {0,0,INT_MAX,INT_MAX,0,1}, clamped to the newly bound target.
    const uint32_t side=current_target==distortion_target?32:640;
    const std::array<uint32_t,6> reset{0,0,side,side==32?32u:480u,0,0x3f800000};
    std::memcpy(base+sdk_viewport,reset.data(),24);ctx.eax=0;ctx.esp+=12;
}
X86_FUNC(sub_003E4270){
    assert(current_target==distortion_target&&!current_depth&&Read(base,ctx.esp+4)==1);
    const uint32_t rectangle=Read(base,ctx.esp+8);
    assert(Read(base,rectangle)==0&&Read(base,rectangle+4)==0&&Read(base,rectangle+8)==32&&Read(base,rectangle+12)==32);
    assert(Read(base,ctx.esp+12)==0xf0);++distortion_clears;ctx.esp+=28;
}
X86_FUNC(sub_003E55B0){ctx.esp+=8;}
X86_FUNC(sub_001FB910){assert(ctx.ecx==p2+0x888);++startup_advances;ctx.esp+=4;}
X86_FUNC(sub_000D2AB0){assert(ctx.ecx==p2);++ordinary_updates;ctx.esp+=4;}
X86_FUNC(sub_000CA9B0){ctx.eax=0;ctx.esp+=4;} // No alternate inventory item26.
X86_FUNC(sub_00041FC0){assert(ctx.ecx==p2+0x498&&Read(base,ctx.esp+4)==0);ctx.eax=shield_item;ctx.esp+=8;}
X86_FUNC(sub_000BFE00){
    assert(ctx.ecx==p2&&Read(base,ctx.esp+4)==1&&Read(base,ctx.esp+8)==0);
    assert(Read(base,p2+0x858)==shield_animation);++shield_animations;ctx.esp+=12;
}
// Equip/presentation branches remain outside this gate test. The original
// consumer must stop at its absent shield-inventory check after input is read.
#define UNREACHED_SHIELD(name) X86_FUNC(name){std::abort();}
UNREACHED_SHIELD(sub_00041F50) UNREACHED_SHIELD(sub_00041F80)
UNREACHED_SHIELD(sub_00042870) UNREACHED_SHIELD(sub_00042DA0)
UNREACHED_SHIELD(sub_000BBD80) UNREACHED_SHIELD(sub_000BC580)
UNREACHED_SHIELD(sub_000C8930) UNREACHED_SHIELD(sub_000CF6A0)
UNREACHED_SHIELD(sub_000CF8A0) UNREACHED_SHIELD(sub_00216BD0)
#undef UNREACHED_SHIELD
// RETAIL_SHIELD_FUNCTIONS
// Retail list insertion runs below; only allocation/actor construction are
// boundary stubs. Each test factory still selects its original insertion mode.
X86_FUNC(sub_002D57FB){assert(Read(base,ctx.esp+4)==8);ctx.eax=registration_heap;registration_heap+=8;ctx.esp+=4;}
X86_FUNC(sub_001FBC50){
    assert(ctx.ecx==services+0x100&&Read(base,ctx.esp+12)==0&&Read(base,ctx.esp+16)==0);
    const auto saved=ctx;const uint32_t return_sp=ctx.esp;
    Write(base,p2+0x54,1500);Write(base,p2+0x58,Read(base,ws+0x204c));
    ctx.esp-=16;Write(base,ctx.esp,0);Write(base,ctx.esp+4,p2);
    Write(base,ctx.esp+8,1500);Write(base,ctx.esp+12,registration_list);ctx.ecx=w;
    sub_00207E50(ctx,base);assert(ctx.esp==return_sp);
    ctx=saved;ctx.eax=p2;ctx.esp+=20;
}
// Actual retail health getter, setter/max update, and Mainplayer update gate.
// HUD and coroutine execution are observed boundaries, not simulated cameras.
// RETAIL_STARTUP_FUNCTIONS
namespace kernel {
uint32_t CallGuest(X86Context& c,uint32_t fn,const uint32_t* a,size_t count){
    if(fn==0x2d589f){
        assert(count==1&&a[0]==Read(B(),p1+0x850)*4&&a[0]);++shield_allocations;
        c.eax=shield_allocation_failure?0:shield_owned;return c.eax;
    }
    if(fn==0x2d631d){assert(count==1&&a[0]==shield_owned);++shield_frees;return 0;}
    if(fn==0x3e3610){
        assert(count==1);++viewport_restores;
        c.esp-=8;Write(B(),c.esp,0);Write(B(),c.esp+4,a[0]);sub_003E3610(c,B());
        c.eax=0xdead;c.ecx=0xbeef;c.edx=3;c.ebx=4;c.ebp=5;c.esi=6;c.edi=7;c.esp-=80;
        c.cf^=1;c.zf^=1;c.sf^=1;c.of^=1;c.pf^=1;c.df^=1;c.xflags^=0x200000;
        c.top=3;c.fcw=0x27f;c.fsw=0x1234;c.mxcsr=0x3f80;
        for(unsigned i=0;i<8;++i){c.st[i]=double(i)+.25;c.mm[i]=0x1234567800000000ull+i;c.xmm[i].q[0]=i+1;c.xmm[i].q[1]=i+2;}
        _mm_setcsr(0x3f80);
        if(viewport_throw)throw 7;
        return 0;
    }
    if(fn==0x159c0){
        ++begins;assert(count==2&&a[0]==0&&a[1]==0xffffffffu);
        Write(B(),c.ecx+0x10,a[0]);Write(B(),c.ecx+0x14,a[1]);return c.ecx;
    }
    if(fn==0x1f8ba0){++ends;assert(count==0);return 0;}
    if(fn==0x2180a0){
        assert(count==1&&c.ecx==p2);++health_sets;
        c.esp-=8;Write(B(),c.esp,0);Write(B(),c.esp+4,a[0]);
        sub_002180A0(c,B());return c.eax;
    }
    assert(fn==0x207cd0&&count==3&&c.ecx==w);++creates;
    const uint32_t props=a[2];
    assert(a[1]==0xffffffffu&&Read(B(),a[0])==0x45a87cu);
    assert(Read(B(),props+0x10)==0&&Read(B(),props+0x14)==0xffffffffu);
    assert(Read(B(),props+0x2a4)==5&&Read(B(),props+0x2ac)==0);
    assert(!std::memcmp(B()+props+0x260,expected.data(),64));
    Write(B(),0x51c3e0,props);Write(B(),ws+0x2044,0);Write(B(),ws+0x2048,1);Write(B(),ws+0x204c,0xffffffffu);
    // Simulate CC2F0's shared writes while retaining P2's new private state.
    Write(B(),w+0x948,0);Write(B(),w+0x94c,3);B()[0x51bdf4]=0;
    Write(B(),w+0xf74,0);Write(B(),w+0xf78,0);
    B()[flag_data+7]=0;B()[flag_data+0x10]=1;B()[flag_data+0x11]=0;B()[flag_data+23]=9;
    X86Context child=c;child.esi=p2;
    Write(B(),p2,0x458ca8);Write(B(),p2+0x368,0);
    TrialCoopPlayerSlot(child,B());assert(Read(B(),p2+0x368)==1);
    child.esp=props-32;Write(B(),child.esp,1500);Write(B(),child.esp+4,0x44b828);
    TrialCoopPlayerAlias(child,B());assert(Read(B(),child.esp)==349);
    Write(B(),child.esp,1500);Write(B(),child.esp+4,0x44e0d8);
    TrialCoopPlayerAlias(child,B());assert(Read(B(),child.esp)==1500);
    Write(B(),p2,0x46fbe8);Write(B(),p2+0x54,1500);Write(B(),p2+0x308,0xffffffffu);
    PutReal(p2+0x9c,factory_health);PutReal(p2+0xa0,factory_health>0?factory_health:0);
    Write(B(),p2+0x4c4,0x1234);
    child.eax=v2;Write(B(),v2+4,p2);TrialCoopPlayerLayer(child,B());assert(Read(B(),v2+0x24)==1);
    Write(B(),views+4,v2);Write(B(),ws+8,views+8);
    child.eax=p2;TrialCoopMainplayerCreated(child,B());
    assert(TrialCoopPlayer(0)==p1&&TrialCoopPlayer(1)==p2&&!trial_coop::Active());
    c.eax=0xdead;c.ecx=0xbeef;c.esp-=80;return shield_factory_mismatch?1501:1500;
}
}
namespace {
void CheckShieldGate(){
    const auto seed=[&](unsigned count,bool locked){
        Setup();Write(B(),p1+0x84c,count?shield_flags:0);Write(B(),p1+0x850,count);
        B()[p1+0x854]=locked;Write(B(),p1+0x858,shield_animation);
        std::memcpy(B()+shield_animation,"original shield animation",26);
        Write(B(),shield_flags,87);Write(B(),shield_flags+4,88);
        Write(B(),w,0x657000);Write(B(),0x657008,0xf2000);
    };
    const auto create=[&](){
        auto c=Context();assert(trial_coop::Request());trial_coop::ResourcesReady(true);
        TrialCoopMainplayerCreated(c,B());const auto before=c;const auto csr=_mm_getcsr();
        TrialCoopPlayerStarted(c,B(),p1);
        assert(!std::memcmp(&c,&before,sizeof c)&&_mm_getcsr()==csr);
    };
    const auto update=[&](){
        auto c=Context();c.ecx=p2;c.esi=0x76543210;
        sub_000D0C30(c,B());
        assert(c.esp==Context().esp+4&&c.esi==0x76543210&&c.top==0);
    };
    // Negative witness: generic construction leaves the gate unlocked even
    // while the campaign's authored flag87 is false.
    seed(1,true);Write(B(),p2,0x46fbe8);Write(B(),p2+0x368,1);
    update();assert(shield_inputs==1&&!shield_animations&&!B()[flag_data+87]);
    for(unsigned count:{1u,2u}){
        seed(count,true);const auto primary=std::array<uint32_t,4>{Read(B(),p1+0x84c),Read(B(),p1+0x850),Read(B(),p1+0x854),Read(B(),p1+0x858)};
        create();assert(creates==1&&shield_allocations==1&&!shield_frees);
        assert(Read(B(),p2+0x84c)==shield_owned&&shield_owned!=shield_flags&&Read(B(),p2+0x850)==count);
        assert(!std::memcmp(B()+shield_owned,B()+shield_flags,count*4)&&B()[p2+0x854]);
        assert(Read(B(),p2+0x858)==shield_animation);
        update();assert(!shield_inputs&&!shield_animations&&B()[p2+0x854]);
        B()[flag_data+87]=1;
        if(count==2){update();assert(!shield_inputs&&!shield_animations&&B()[p2+0x854]);B()[flag_data+88]=1;}
        update();assert(!B()[p2+0x854]&&shield_animations==1&&!shield_inputs);
        update();assert(shield_inputs==1&&shield_animations==1);
        // Runtime retains its one-way unlock and leaves all P1 gate fields alone.
        B()[flag_data+87]=0;update();assert(shield_inputs==2&&shield_animations==1);
        const auto after=std::array<uint32_t,4>{Read(B(),p1+0x84c),Read(B(),p1+0x850),Read(B(),p1+0x854),Read(B(),p1+0x858)};
        assert(after==primary&&Read(B(),shield_flags)==87);
    }
    for(unsigned count:{0u,1u}){
        seed(count,false);create();update();
        assert(shield_inputs==1&&!shield_animations&&!B()[p2+0x854]&&shield_allocations==unsigned(count!=0));
    }
    // Failed allocation must not create an unlocked secondary. An unclaimed
    // allocation is released through the same allocator pair as retail C28CA.
    seed(1,true);shield_allocation_failure=true;create();
    assert(shield_allocations==1&&!creates&&!attempted&&!shield_frees);
    seed(2,true);shield_factory_mismatch=true;create();
    assert(creates==1&&shield_allocations==1&&shield_frees==1&&!Read(B(),p2+0x84c));
    for(unsigned invalid=0;invalid<5;++invalid){
        seed(1,true);
        if(invalid==0)Write(B(),p1+0x850,4097);
        if(invalid==1)Write(B(),p1+0x84c,0xfffffffcu);
        if(invalid==2)Write(B(),shield_flags,0xffffffffu);
        if(invalid==3)Write(B(),p1+0x858,0x800000);
        if(invalid==4)Write(B(),p1+0x5c,1);
        create();assert(!creates&&!shield_allocations&&!attempted);
    }
}
X86Context ViewportContext(unsigned slot,bool effect,bool fullscreen=false){
    Setup();assert(trial_coop::Request());trial_coop::SetActive(true);
    players[0]=p1;players[1]=p2;world=w;primary_id=349;
    Write(B(),p2,0x46fbe8);Write(B(),p2+0x54,1500);Write(B(),p2+0x368,1);Write(B(),p2+0x308,901);
    Write(B(),ws+8,views+8);Write(B(),views+4,v2);
    Write(B(),v2+4,p2);Write(B(),v2+0x24,1);
    for(unsigned i=0;i<2;++i){Write(B(),i?v2+0x20:v1+0x20,split_viewport+i*0x100);Write(B(),split_viewport+i*0x100,viewport_table);}
    Write(B(),render_parameters,0x480a6c);Write(B(),render_parameters+4,split_viewport+slot*0x100);
    Write(B(),render_parameters+0x128,0xffffffef);Write(B(),0x480a78,0xf1000);
    Write(B(),viewport_table+8,0xf1001);Write(B(),viewport_table+4,0xf1002);
    Write(B(),viewport_table+0x24,0xf1003);Write(B(),viewport_table+0x28,0xf1004);
    Write(B(),0x519f60,graphics);Write(B(),graphics,graphics_table);Write(B(),graphics+0x6c,0x12345);
    Write(B(),graphics_table+0x9c,0xf1005);
    Write(B(),distortion+0x28,effect);Write(B(),distortion+0x34,distortion_texture);
    Write(B(),distortion+0x20,32);Write(B(),distortion+0x24,32);
    Write(B(),0x3f4238,sdk_device);PutReal(0x44c1f8,.5f);PutReal(0x44bf94,1.f);
    caller_rect={0,fullscreen?0:slot*240,640,fullscreen?480u:240u};
    current_target=scene_target;current_depth=scene_depth;
    viewport_restores=viewport_sets=distortion_clears=0;target_changes.clear();
    X86Context c=Context();c.esi=render_parameters;c.ebx=0x5555;c.edi=0x6666;
    c.cf=1;c.zf=1;c.sf=1;c.df=1;c.fcw=0x37f;c.mxcsr=0x1f80;
    Write(B(),c.esp+0x4c,graphics);return c;
}
std::array<uint32_t,6> ActualViewport(){std::array<uint32_t,6> value;std::memcpy(value.data(),B()+sdk_viewport,24);return value;}
void CheckViewportRestore(){
    const std::array<uint32_t,6> full{0,0,640,480,0,0x3f800000};
    unsigned cases=0;
    for(unsigned slot=0;slot<2;++slot)for(bool effect:{false,true})for(bool fullscreen:{false,true}){
        auto original=ViewportContext(slot,effect,fullscreen);const unsigned csr=_mm_getcsr();
        _mm_setcsr(0x5f80);OriginalViewportCaller(original,B());
        const unsigned expected_csr=_mm_getcsr();_mm_setcsr(csr);
        const std::array<uint32_t,6> expected{caller_rect[0],caller_rect[1],caller_rect[2],caller_rect[3],0,0x3f800000};
        // Broken witness executes the exact caller and whole original effect.
        assert(ActualViewport()==(effect?full:expected));assert(!viewport_restores);
        const auto targets=target_changes;const unsigned clears=distortion_clears;
        std::array<uint8_t,0x1b0> original_frame{};std::memcpy(original_frame.data(),B()+original.esp,original_frame.size());
        auto fixed=ViewportContext(slot,effect,fullscreen);_mm_setcsr(0x5f80);
        CoopViewportCaller(fixed,B());assert(_mm_getcsr()==expected_csr);_mm_setcsr(csr);
        assert(!std::memcmp(&fixed,&original,sizeof fixed));
        assert(!std::memcmp(original_frame.data(),B()+fixed.esp,original_frame.size()));
        assert(ActualViewport()==expected&&viewport_restores==unsigned(effect&&!fullscreen));
        assert(target_changes==targets&&distortion_clears==clears&&clears==unsigned(effect));
        assert(current_target==scene_target&&current_depth==scene_depth&&!Read(B(),distortion+0x28));
        if(effect)assert((targets==std::vector<std::array<uint32_t,2>>{{distortion_target,0},{scene_target,scene_depth}}));
        ++cases;
    }
    // Alternate both views over several frames; the effect may be dirty in
    // either view. Its clear and target restoration must still occur unchanged.
    auto c=ViewportContext(0,true);
    for(unsigned frame=0;frame<6;++frame)for(unsigned slot=0;slot<2;++slot){
        c=Context();c.esi=render_parameters;Write(B(),c.esp+0x4c,graphics);
        Write(B(),render_parameters+4,split_viewport+slot*0x100);caller_rect={0,slot*240,640,240};
        const bool dirty=(frame+slot)%2==0;Write(B(),distortion+0x28,dirty);
        const unsigned repairs=viewport_restores;CoopViewportCaller(c,B());
        assert((ActualViewport()==std::array<uint32_t,6>{0,slot*240,640,240,0,0x3f800000}));
        assert(viewport_restores==repairs+unsigned(dirty));++cases;
    }
    // Unqualified passes remain bit-for-bit unchanged by the helper.
    for(unsigned invalid=0;invalid<16;++invalid){
        c=ViewportContext(1,true);OriginalViewportCaller(c,B());
        switch(invalid){
            case 0:trial_coop::Reset();break;
            case 1:trial_coop::SetActive(false);break;
            case 2:Write(B(),0x51c4dc,w+4);break;
            case 3:Write(B(),render_parameters,0x450be0);break;
            case 4:Write(B(),render_parameters+0x128,0);break;
            case 5:Write(B(),render_parameters+4,split_viewport+0x200);break;
            case 6:Write(B(),v2+4,p1);break;
            case 7:Write(B(),v2+0x24,0);break;
            case 8:Write(B(),c.esp+0x9c,0);break;
            case 9:Write(B(),c.esp+0x98,481);break;
            case 10:Write(B(),c.esp+0xa4,0x3e800000);break;
            case 11:Write(B(),sdk_viewport+4,1);break;
            case 12:Write(B(),0x3f4238,0xfffffff0);break;
            case 13:Write(B(),ws+8,views+12);break;
            case 14:Write(B(),p1+0x5c,1);break;
            case 15:Write(B(),p2+0x54,0xffffffffu);break;
        }
        const auto saved=c;static std::array<uint8_t,0x800000> memory_before;memory_before=kernel::memory;
        TrialCoopRestoreViewViewport(c,B());assert(!viewport_restores&&!std::memcmp(&c,&saved,sizeof c));
        assert(kernel::memory==memory_before);++cases;
    }
    // Even an exceptional guest boundary restores caller registers/flags/x87
    // state and host MXCSR. The already-issued SDK side effect is not rolled back.
    c=ViewportContext(1,true);OriginalViewportCaller(c,B());const auto saved=c;
    const unsigned csr=_mm_getcsr();_mm_setcsr(0x5f80);viewport_throw=true;
    try{TrialCoopRestoreViewViewport(c,B());assert(false);}catch(int code){assert(code==7);}
    viewport_throw=false;assert(_mm_getcsr()==0x5f80);_mm_setcsr(csr);
    assert(!std::memcmp(&c,&saved,sizeof c)&&viewport_restores==1);
    std::printf("co-op viewport: %u original caller/effect cases, SDK reset witness, per-view restoration and context safety passed (no GPU)\n",cases);
}
}
int main(){
    using Character=trial_coop::Character;
    trial_coop::Reset();
    assert(!trial_coop::Request(static_cast<Character>(5),Character::TZero));
    assert(!trial_coop::Request(Character::TZero,static_cast<Character>(-1)));
    assert(!trial_coop::Requested()&&!trial_coop::Active());
    assert(trial_coop::Request(Character::Avenger,Character::Prophet));
    assert(trial_coop::Selected(0)==Character::Avenger&&trial_coop::Selected(1)==Character::Prophet);
    // Character ownership freezes at the request, before the second actor starts.
    assert(!trial_coop::Request(Character::Banger,Character::Magnum));
    assert(trial_coop::Selected(0)==Character::Avenger&&trial_coop::Selected(1)==Character::Prophet);
    trial_coop::SetActive(true);trial_coop::ResourcesReady(true);trial_coop::SetResourceFailure(true);
    assert(!trial_coop::Request());
    assert(trial_coop::Selected(0)==Character::Avenger&&trial_coop::Selected(1)==Character::Prophet);
    trial_coop::Reset();
    assert(!trial_coop::Requested()&&!trial_coop::Active()&&!trial_coop::ResourcesReady()&&!trial_coop::ResourceFailure());
    assert(trial_coop::Request(Character::Magnum,Character::Banger));
    assert(trial_coop::Selected(0)==Character::Magnum&&trial_coop::Selected(1)==Character::Banger);
    Setup();auto c=Context();const auto original=c;
    TrialCoopMainplayerCreated(c,B());TrialCoopPlayerStarted(c,B(),p1);
    assert(!creates&&!TrialCoopPlayer(0)&&!trial_coop::Active());
    const auto previous_death_resets=death_resets,previous_shield_resets=shield_resets,previous_camera_resets=downed_camera_resets;
    const auto previous_body_resets=body_resets;
    const auto previous_progression_resets=progression_resets,previous_interaction_resets=interaction_resets,previous_reward_resets=reward_resets,previous_ko_reward_resets=ko_reward_resets;
    const auto previous_spin_resets=spin_resets;
    trial_coop::Request();TrialCoopMainplayerCreated(c,B());
    assert(death_resets==previous_death_resets+1&&shield_resets==previous_shield_resets+1);
    assert(downed_camera_resets==previous_camera_resets+1);
    assert(body_resets==previous_body_resets+1);
    assert(progression_resets==previous_progression_resets+1&&interaction_resets==previous_interaction_resets+1);
    assert(reward_resets==previous_reward_resets+1);
    assert(spin_resets==previous_spin_resets+1);
    assert(ko_reward_resets==previous_ko_reward_resets+1);
    TrialCoopPlayerStarted(c,B(),p1);
    assert(!creates&&TrialCoopPlayer(0)==p1&&!trial_coop::Active()&&trial_coop::ResourceFailure());
    trial_coop::ResourcesReady(true);
    TrialCoopPlayerStarted(c,B(),p1+4);assert(!creates);
    Write(B(),ws+8,views+8);TrialCoopPlayerStarted(c,B(),p1);assert(!creates);
    Write(B(),ws+8,views+4);Write(B(),flag_owner+0x38,4097);
    TrialCoopPlayerStarted(c,B(),p1);assert(!creates);
    Write(B(),flag_owner+0x38,32);TrialCoopPlayerStarted(c,B(),p1);
    assert(creates==1&&begins==1&&ends==1&&!trial_coop::Active()&&!trial_coop::ResourceFailure());
    assert(!std::memcmp(&c,&original,sizeof c));
    assert(Read(B(),0x51c3e0)==0x654321&&Read(B(),ws+0x2044)==0xaabbccddu);
    assert(Read(B(),ws+0x2048)==0x12345678&&Read(B(),ws+0x204c)==0x87654321);
    assert(Read(B(),w+0x948)==2&&B()[0x51bdf4]==1);
    assert(Read(B(),w+0xf74)==4&&Read(B(),w+0xf78)==5);
    assert(B()[flag_data+7]==2&&B()[flag_data+0x10]==0&&B()[flag_data+0x11]==1);
    assert(B()[flag_data+23]==9&&Read(B(),w+0x94c)==3&&Read(B(),p2+0x4c4)==0x1234);
    TrialCoopPlayerStarted(c,B(),p1);assert(creates==1);
    TrialCoopPlayerStarted(c,B(),p2);assert(!trial_coop::Active());
    Write(B(),p2+0x308,901);TrialCoopPlayerStarted(c,B(),p2);assert(trial_coop::Active());
    // Each hooked retail lookup has just returned game-owner +834 (P1).
    // Correct only an active P2 death sequence, leaving every other register.
    constexpr uint32_t sequence=0x660000;
    Write(B(),sequence,0x46b928);Write(B(),sequence+0x54,2001);
    Write(B(),sequence+0xd4,1);Write(B(),sequence+0x114,1);
    c.eax=349;const auto before_death=c;TrialCoopDeathPlayer(c,B(),sequence);
    auto expected_death=before_death;expected_death.eax=1500;
    assert(!std::memcmp(&c,&expected_death,sizeof c));
    for(uint32_t slot:{0u,2u,0xffffffffu}){
        Write(B(),sequence+0xd4,slot);c=before_death;TrialCoopDeathPlayer(c,B(),sequence);
        assert(!std::memcmp(&c,&before_death,sizeof c));
    }
    Write(B(),sequence+0xd4,1);
    for(uint32_t state:{0u,11u}){
        Write(B(),sequence+0x114,state);c=before_death;TrialCoopDeathPlayer(c,B(),sequence);
        assert(c.eax==349);
    }
    Write(B(),sequence+0x114,1);Write(B(),sequence,0x46b929);
    c=before_death;TrialCoopDeathPlayer(c,B(),sequence);assert(c.eax==349);
    Write(B(),sequence,0x46b928);Write(B(),p2+0x5c,1);
    TrialCoopDeathPlayer(c,B(),sequence);assert(c.eax==349);
    Write(B(),p2+0x5c,0);trial_coop::SetActive(false);
    TrialCoopDeathPlayer(c,B(),sequence);assert(c.eax==349);
    trial_coop::SetActive(true);
    // Both original rumble sites have just pushed {port=0,left,right,type}.
    c=before_death;Write(B(),c.esp,0);Write(B(),c.esp+4,0x3dcccccd);
    Write(B(),c.esp+8,0x3dcccccd);Write(B(),c.esp+12,2);
    TrialCoopDeathRumble(c,B(),sequence);
    assert(Read(B(),c.esp)==1&&Read(B(),c.esp+4)==0x3dcccccd&&
           Read(B(),c.esp+8)==0x3dcccccd&&Read(B(),c.esp+12)==2);
    assert(!std::memcmp(&c,&before_death,sizeof c));
    Write(B(),sequence+0xd4,0);Write(B(),c.esp,0);
    TrialCoopDeathRumble(c,B(),sequence);assert(Read(B(),c.esp)==0);
    Write(B(),sequence+0xd4,1);trial_coop::SetActive(false);
    TrialCoopDeathRumble(c,B(),sequence);assert(Read(B(),c.esp)==0);
    // Execute the extracted retail callback, including its original idle guard.
    const auto event=[&](uint32_t seq,unsigned slot,bool modified){
        auto e=Context();e.ecx=seq;Write(B(),e.esp+4,0x663000);Write(B(),0x663000,slot);
        if(modified)CoopDeathSequence(e,B());else OriginalDeathSequence(e,B());
        assert(e.esp==Context().esp+12);return e;
    };
    for(unsigned slot:{0u,1u}){
        Write(B(),sequence+0x114,0);Write(B(),sequence+0xd4,99);
        const auto expected_event=event(sequence,slot,false);
        std::array<uint8_t,0x140> expected_sequence;
        std::memcpy(expected_sequence.data(),B()+sequence,expected_sequence.size());
        Write(B(),sequence+0x114,0);Write(B(),sequence+0xd4,99);
        const auto actual_event=event(sequence,slot,true);
        assert(!std::memcmp(&actual_event,&expected_event,sizeof actual_event));
        assert(!std::memcmp(B()+sequence,expected_sequence.data(),expected_sequence.size()));
    }
    trial_coop::SetActive(true);
    // A callback on an already-running instance did not perform retail admission
    // and must not claim the shared sequence. A different idle instance wins.
    constexpr uint32_t other_sequence=0x661000;
    Write(B(),other_sequence,0x46b928);Write(B(),other_sequence+0x54,2002);
    event(sequence,0,true);assert(!death_sequence);
    event(other_sequence,1,true);
    assert(death_sequence==other_sequence&&Read(B(),other_sequence+0x114)==1&&Read(B(),other_sequence+0xd4)==1);
    Write(B(),sequence+0x114,0);Write(B(),sequence+0xd4,99);
    event(sequence,0,true);assert(Read(B(),sequence+0x114)==0&&Read(B(),sequence+0xd4)==99);
    event(other_sequence,0,true);assert(Read(B(),other_sequence+0xd4)==1);
    c.ecx=p1+4;TrialCoopPlayerDestroyed(c,B());assert(trial_coop::Active());
    c.ecx=p2;TrialCoopPlayerDestroyed(c,B());
    assert(!trial_coop::Active()&&!TrialCoopPlayer(0)&&!TrialCoopPlayer(1)&&trial_coop::Requested());
    assert(!death_sequence);
    // Same actor address in a new mission is admitted once again.
    const auto previous_turret_resets=turret_resets;
    Write(B(),ws+8,views+4);c=original;TrialCoopMainplayerCreated(c,B());TrialCoopPlayerStarted(c,B(),p1);
    assert(turret_resets==previous_turret_resets+1);
    assert(creates==2&&begins==2&&ends==2);
    Write(B(),p2+0x308,901);TrialCoopPlayerStarted(c,B(),p2);assert(trial_coop::Active());
    Write(B(),sequence+0x114,0);Write(B(),other_sequence+0x114,0);
    event(sequence,0,true);event(other_sequence,1,true);
    assert(death_sequence==sequence&&Read(B(),sequence+0xd4)==0&&Read(B(),other_sequence+0x114)==0);
    // Negative witness: the same original update runs the HUD and common update
    // forever, but never advances camera startup when a fresh actor has no health.
    Setup();Write(B(),p2,0x46fbe8);PrepareUpdateGate();
    for(unsigned i=0;i<3;++i)RunUpdateGate();
    assert(!startup_advances&&ordinary_updates==3&&hud_updates==3);
    const float nan=std::numeric_limits<float>::quiet_NaN();
    struct HealthCase{float primary,maximum,secondary;bool repaired;};
    const HealthCase cases[]={{.75f,1,0,true},{.75f,1,-1,true},{.75f,1,.4f,false},
        {35,100,0,true},{.75f,nan,0,true},{.75f,.5f,0,true},{.75f,.75f,0,true},
        {0,1,0,false},{-1,1,0,false},{nan,1,0,false},{.75f,1,nan,false},
        {.001f,1,0,false},{.00100001f,1,0,true}};
    for(const auto& h:cases){
        Setup();PutReal(p1+0x9c,h.primary);PutReal(p1+0xa0,h.maximum);factory_health=h.secondary;
        assert(trial_coop::Request());trial_coop::ResourcesReady(true);
        c=Context();const auto saved=c;TrialCoopMainplayerCreated(c,B());
        const unsigned csr=_mm_getcsr();_mm_setcsr(0x5f80);
        TrialCoopPlayerStarted(c,B(),p1);
        assert(_mm_getcsr()==0x5f80);_mm_setcsr(csr);
        const bool valid_max=std::isfinite(h.maximum)&&h.maximum>h.primary;
        assert(creates==1&&health_sets==(h.repaired?1+unsigned(valid_max):0)&&!trial_coop::Active());
        assert(!std::memcmp(&c,&saved,sizeof c));
        const float wanted=h.repaired?h.primary:h.secondary;
        assert(std::isnan(wanted)?std::isnan(Real(B(),p2+0x9c)):Real(B(),p2+0x9c)==wanted);
        if(h.repaired)assert(Real(B(),p2+0xa0)==(valid_max?h.maximum:h.primary));
        if(h.secondary>0)assert(Real(B(),p2+0xa0)==h.secondary);
        assert(std::isnan(h.maximum)?std::isnan(Real(B(),p1+0xa0)):Real(B(),p1+0xa0)==h.maximum);
        if(std::isfinite(wanted)){
            PrepareUpdateGate();RunUpdateGate();
            assert(startup_advances==unsigned(wanted>.001f)&&ordinary_updates==1&&hud_updates==1);
        }
    }
    // Execute 20E019-20E026 itself: the original branch throws away a valid
    // listener camera as soon as a second view is registered, before Active().
    Setup();trial_coop::Request();trial_coop::ResourcesReady(true);
    c=Context();TrialCoopMainplayerCreated(c,B());TrialCoopPlayerStarted(c,B(),p1);
    constexpr uint32_t camera1=0x670000,camera2=0x671000;
    for(const auto pair:{std::array<uint32_t,2>{camera1,p1},std::array<uint32_t,2>{camera2,p2}}){
        Write(B(),pair[0]+0x54,pair[1]==p1?900:901);
        Write(B(),pair[0]+0x58,Read(B(),pair[1]+0x54));
        Write(B(),pair[0]+0x22c,Read(B(),pair[1]+0x54));Write(B(),pair[0]+0x210,2);
    }
    const auto listener=[&](uint32_t count,uint32_t camera,uint32_t wanted){
        auto old=Context();old.eax=count;old.ecx=0x12345678;old.edx=0x87654321;
        Write(B(),old.esp+0x60,camera);auto fixed=old;
        OriginalAudioListenerSelection(old,B());CoopAudioListenerSelection(fixed,B());
        assert(old.ecx==(count>1?0:camera));
        auto expected_listener=old;expected_listener.ecx=wanted;
        if(std::memcmp(&fixed,&expected_listener,sizeof fixed))
            std::fprintf(stderr,"listener mismatch: views=%u camera=%08X expected=%08X actual=%08X available=%u active=%u\n",
                count,camera,wanted,fixed.ecx,unsigned(trial_coop::Available()),unsigned(trial_coop::Active()));
        assert(!std::memcmp(&fixed,&expected_listener,sizeof fixed));
    };
    assert(!trial_coop::Active());listener(2,camera1,camera1);
    listener(2,camera2,0); // P2 has no camera yet: keep the original null fallback.
    Write(B(),p2+0x308,901);trial_coop::SetActive(true);
    listener(2,camera1,camera1);listener(2,camera2,0); // Retail caller resolves slot zero only.
    for(uint32_t count:{0u,1u}){
        listener(count,camera1,camera1);listener(count,0,0);
        listener(count,0x12345678,0x12345678); // Preserve the original one-view path.
    }
    listener(3,camera1,0);
    for(uint32_t camera:{0u,0xffffffffu,0x7fffffu,0x672000u})listener(2,camera,0);
    for(uint32_t offset:{0x54u,0x58u,0x22cu,0x210u}){
        const uint32_t value=Read(B(),camera1+offset);Write(B(),camera1+offset,value+1);
        listener(2,camera1,0);Write(B(),camera1+offset,value);
    }
    Write(B(),p1+0x308,0xffffffffu);listener(2,camera1,0);Write(B(),p1+0x308,900);
    Write(B(),camera1+0x5c,1);listener(2,camera1,0);Write(B(),camera1+0x5c,0);
    Write(B(),p2+0x5c,1);listener(2,camera1,0);Write(B(),p2+0x5c,0);
    Write(B(),0x51c4dc,w+4);listener(2,camera1,0);Write(B(),0x51c4dc,w);
    // The actual world camera resolver supplies the cinematic from P1's stack;
    // it must remain the listener while that same view is shared fullscreen.
    const bool previous_available=trial_coop::Available();trial_coop::SetAvailable(true);
    constexpr uint32_t cinematic_camera=0x674000,camera_stack=0x675000;
    Write(B(),v1+8,camera_stack);Write(B(),camera_stack+8,1);
    Write(B(),camera_stack+4,camera_stack+0x20);Write(B(),camera_stack+0x24,camera_stack+0x40);
    Write(B(),camera_stack+0x48,cinematic_camera);
    Write(B(),cinematic_camera+0x54,2005);Write(B(),cinematic_camera+0xd0,0x478d20);
    for(uint32_t type:{0x46fd70u,0x477da8u,0x477c58u}){
        Write(B(),cinematic_camera,type);
        auto resolved=Context();resolved.ecx=w;Write(B(),resolved.esp+4,0);
        sub_00214F40(resolved,B());assert(resolved.eax==cinematic_camera);
        listener(2,resolved.eax,cinematic_camera);
        Write(B(),camera_stack+0x48,camera1);listener(2,cinematic_camera,0);
        Write(B(),camera_stack+0x48,cinematic_camera);
    }
    for(unsigned bad=0;bad<8;++bad){
        switch(bad){
            case 0:Write(B(),cinematic_camera,0x47c780);break;
            case 1:Write(B(),cinematic_camera+0x54,0xffffffffu);break;
            case 2:Write(B(),cinematic_camera+0x5c,1);break;
            case 3:Write(B(),cinematic_camera+0xd0,0);break;
            case 4:Write(B(),v1+4,p2);break;
            case 5:Write(B(),camera_stack+8,0);break;
            case 6:trial_coop::SetActive(false);break;
            case 7:trial_coop::SetAvailable(false);break;
        }
        listener(2,cinematic_camera,0);
        Write(B(),cinematic_camera,0x477c58);Write(B(),cinematic_camera+0x54,2005);
        Write(B(),cinematic_camera+0x5c,0);Write(B(),cinematic_camera+0xd0,0x478d20);
        Write(B(),v1+4,p1);Write(B(),camera_stack+8,1);trial_coop::SetActive(true);trial_coop::SetAvailable(true);
    }
    trial_coop::SetAvailable(previous_available);
    trial_coop::Reset();listener(2,camera1,0);
    trial_coop::Request();players[1]=0;listener(2,camera1,0);
    // Negative witness: relative insertion with no parent allocates a record
    // but never links it. Absolute insertion links that same parentless actor.
    for(const auto scenario:{std::array<uint32_t,2>{1,0xffffffffu},
                             std::array<uint32_t,2>{0,0xffffffffu},
                             std::array<uint32_t,2>{1,349}}){
        Setup();registration_heap=0x687000;
        Write(B(),registration_list+8,registration_free);
        Write(B(),registration_list+0x1c,registration_buckets);
        Write(B(),registration_list+0x20,registration_buckets+0x10);
        Write(B(),registration_list+0x24,1);Write(B(),registration_list+0x28,2);
        Write(B(),registration_buckets+0x10,registration_node);
        Write(B(),registration_node,registration_record);
        Write(B(),registration_record,349);Write(B(),registration_record+4,p1);
        Write(B(),registration_free+4,registration_free+0x10);
        c=Context();c.ecx=w;c.esp-=16;Write(B(),c.esp,0);
        Write(B(),c.esp+4,0x45a87c);Write(B(),c.esp+8,scenario[1]);Write(B(),c.esp+12,0x686000);
        if(scenario[0])sub_00207D20(c,B());else sub_00207CD0(c,B());
        assert(c.esp==Context().esp&&c.eax==1500);
        assert(B()[ws+0x2044]==scenario[0]&&Read(B(),ws+0x2048)==1&&Read(B(),p2+0x58)==scenario[1]);
        unsigned primary_found=0,secondary_found=0;
        uint32_t node=Read(B(),registration_buckets+0x10);
        for(unsigned i=0;node&&i<4;++i){
            const uint32_t record=Read(B(),node),actor=Read(B(),record+4);
            primary_found+=actor==p1;secondary_found+=actor==p2;node=Read(B(),node+4);
        }
        assert(!node&&primary_found==1&&secondary_found==unsigned(!scenario[0]||scenario[1]==349));
    }
#ifndef UC_BURNER_RELEASE
    // Startup evidence is private, read-only and independently bounded per stage.
    Setup();trial_coop::Request();trial_coop::ResourcesReady(true);
    c=Context();TrialCoopMainplayerCreated(c,B());TrialCoopPlayerStarted(c,B(),p1);
    startup_trace_counts.fill(0);
    const auto update=TrialCoopTraceStage::Update;
    trial_coop::SetAvailable(false);TrialCoopTraceStartup(c,B(),p2,update);
    assert(startup_trace_counts[0]==0);
    trial_coop::SetAvailable(true);trial_coop::Reset();TrialCoopTraceStartup(c,B(),p2,update);
    assert(startup_trace_counts[0]==0);trial_coop::Request();
    TrialCoopTraceStartup(c,B(),p1,update);TrialCoopTraceStartup(c,nullptr,p2,update);
    TrialCoopTraceStartup(c,B(),p2,static_cast<TrialCoopTraceStage>(4));
    Write(B(),0x51c4dc,w+4);TrialCoopTraceStartup(c,B(),p2,update);Write(B(),0x51c4dc,w);
    assert((startup_trace_counts==std::array<unsigned,4>{}));
    constexpr uint32_t coroutine=0x673000;
    Write(B(),p2+0x888,coroutine);Write(B(),coroutine+0x28,p2);
    Write(B(),p2+0x60,2);Write(B(),ws+0x30,0x674000);PutReal(w+0x18,1.f/60);
    static std::array<uint8_t,0x800000> before_trace;
    before_trace=kernel::memory;const auto before_trace_context=c;
    const unsigned trace_csr=_mm_getcsr();_mm_setcsr(0x5f80);
    for(unsigned stage=0;stage<startup_trace_counts.size();++stage){
        for(unsigned attempt=0;attempt<10;++attempt)
            TrialCoopTraceStartup(c,B(),p2,static_cast<TrialCoopTraceStage>(stage));
        assert(startup_trace_counts[stage]==startup_trace_limits[stage]);
    }
    assert(_mm_getcsr()==0x5f80);_mm_setcsr(trace_csr);
    assert(kernel::memory==before_trace&&!std::memcmp(&c,&before_trace_context,sizeof c));
    // Invalid subordinate pointers are reported without following them.
    startup_trace_counts.fill(0);Write(B(),p2+0x888,0xffffffffu);Write(B(),w+4,0x7fffffu);
    TrialCoopTraceStartup(c,B(),p2,update);assert(startup_trace_counts[0]==1);
    Write(B(),w+4,ws);Write(B(),ws+4,0xfffffff0u);Write(B(),ws+8,0xffffffffu);
    TrialCoopTraceStartup(c,B(),p2,update);assert(startup_trace_counts[0]==2);
#else
    // No-op release hooks cannot read even an invalid base or modify context.
    const auto before_context=c;
    const auto before_csr=_mm_getcsr();
    for(unsigned stage=0;stage<5;++stage)
        TrialCoopTraceStartup(c,reinterpret_cast<const uint8_t*>(1),0xffffffffu,static_cast<TrialCoopTraceStage>(stage));
    assert(!std::memcmp(&c,&before_context,sizeof c)&&_mm_getcsr()==before_csr);
#endif
    trial_coop::SetAvailable(false);
    CheckViewportRestore();
    CheckShieldGate();
    puts("co-op admission, retail startup/registration, death ownership and audio listener selection checks passed (allocation/coroutine boundaries)");
}

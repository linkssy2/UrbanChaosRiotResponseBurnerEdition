// Original transform/crouch/visibility operations plus guarded factory-boundary tests.
// Model/animation resource initialization still requires live acceptance.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cstdio>
#include <vector>
#include "kernel/kernel.h"
namespace kernel {static std::array<uint8_t,0x800000> memory{};}
namespace mem {bool IsCommitted(uint32_t p){return p<kernel::memory.size();}}
#include "../common/xbe_coop_body.cpp"
static constexpr uint32_t owners[]={0x610000,0x612000},objects[]={0x650000,0x652000};
uint32_t TrialCoopPlayer(unsigned i){return i<2?owners[i]:0;}
static bool turret_mounted[2]{};
uint32_t TrialCoopTurretMount(const uint8_t*,uint32_t p){
    for(unsigned i=0;i<2;++i)if(p==owners[i]&&turret_mounted[i])return 0x710000;
    return 0;
}
void TrialCoopTraceStartup(const X86Context&,const uint8_t*,uint32_t owner,TrialCoopTraceStage stage){
    assert(owner==owners[1]&&stage==TrialCoopTraceStage::UnreadyView);
}
namespace {
uint32_t Lookup(uint32_t id);
void RemoveEntity(uint32_t id);
unsigned attachment_parent_reads=0;
unsigned flare_cleanup=0,flare_frees=0;
}
X86_FUNC(sub_001F8FC0){ctx.eax=Lookup(Word(base,ctx.esp+4));ctx.esp+=4;}
X86_FUNC(sub_00011000){ctx.esp+=20;} // Temporary matrix array construction.
X86_FUNC(sub_00216000){ctx.esp+=4;}
X86_FUNC(sub_00207FF0){RemoveEntity(Word(base,ctx.esp+4));ctx.esp+=8;}
X86_FUNC(sub_002084E0){assert(false);}
X86_FUNC(sub_00210F10){assert(false);}
X86_FUNC(sub_001F9150){assert(false);}
// ORIGINAL_ATTACHMENT_PARENT_READ
// ORIGINAL_WORLD_UPDATE_ADMISSION
// ORIGINAL_ENTITY_RETIREMENT
X86_FUNC(sub_0020A620){__imp__sub_0020A620(ctx,base);}
X86_FUNC(sub_00187B30){++flare_cleanup;ctx.esp+=4;}
X86_FUNC(sub_002D5CCD){++flare_frees;ctx.esp+=4;}
// ORIGINAL_FLARE_RETIREMENT
// ORIGINAL_BODY_TRANSFORM
// ORIGINAL_BODY_CROUCH
// ORIGINAL_BODY_VISIBILITY
X86_FUNC(sub_000E4300){__imp__sub_000E4300(ctx,base);}
X86_FUNC(sub_000E7800){__imp__sub_000E7800(ctx,base);}
// ORIGINAL_BODY_SYNC_CALLER
// ORIGINAL_INVENTORY_SHIELD_GATE
X86_FUNC(sub_00041F50){__imp__sub_00041F50(ctx,base);}
X86_FUNC(sub_000422C0){ctx.eax=ctx.ecx==owners[0]+0x498?0x660000:0x660010;ctx.esp+=4;}
X86_FUNC(sub_000442D0){const uint32_t id=Word(base,ctx.esp+4);assert(id<27);ctx.eax=0x670000+id*0x100;ctx.esp+=4;}
// ORIGINAL_BODY_WEAPON_SELECTION
X86_FUNC(sub_000C1D60);X86_FUNC(sub_001FB180);X86_FUNC(sub_000E57B0);
X86_FUNC(sub_000C1EDF);X86_FUNC(sub_0028FF80);X86_FUNC(sub_00287610);
X86_FUNC(sub_00216B10);
// ORIGINAL_BODY_ACTION_SOUND
// ORIGINAL_BODY_ACTION_TAIL
X86_FUNC(sub_000C1EDF){__imp__sub_000C1EDF(ctx,base);}
// ORIGINAL_BODY_ACTION_CALLER
// ORIGINAL_RENDER_SET_VISIBLE
// ORIGINAL_RENDER_GET_VISIBLE
// ORIGINAL_ANIMATED_RENDER_SET_VISIBLE
// ORIGINAL_ANIMATED_RENDER_GET_VISIBLE
// ORIGINAL_RENDER_ADMISSION
X86_FUNC(sub_00208D40);X86_FUNC(sub_0021E0A0);X86_FUNC(sub_00214AB0);
X86_FUNC(sub_00087100);X86_FUNC(sub_0021AB30);X86_FUNC(sub_00220200);
X86_FUNC(sub_0008BD60);X86_FUNC(sub_002D4360);X86_FUNC(sub_00214A50);
// The camera-FOV scope is outside this fixture's render-admission concern.
// Its production boundary is retained so the original loop's ordering is tested.
struct TrialPlayerViewScope {void Select(const X86Context&,uint8_t*){} void Clear(){}};
void x86_resume(X86Context&,uint8_t*,const X86Resume&){assert(false);}
// ORIGINAL_VIEW_COUNT
// ORIGINAL_VIEWPORT_ASSIGN
// ORIGINAL_RENDER_PARAMETERS
// ORIGINAL_WORLD_RENDER
X86_FUNC(sub_00208D40){__imp__sub_00208D40(ctx,base);}
X86_FUNC(sub_0021E0A0){__imp__sub_0021E0A0(ctx,base);}
X86_FUNC(sub_00214AB0){__imp__sub_00214AB0(ctx,base);}
X86_FUNC(sub_00087100){Put(base,ctx.ecx,0xff000000);ctx.eax=ctx.ecx;ctx.esp+=20;}
X86_FUNC(sub_0021AB30){assert(false);}
X86_FUNC(sub_00220200){Put(base,ctx.ecx,Word(base,ctx.esp+4));ctx.esp+=8;}
X86_FUNC(sub_0008BD60){ctx.eax=Word(base,ctx.ecx+0x18);ctx.esp+=4;}
X86_FUNC(sub_002D4360){ctx.esp+=4;} // Matrix inversion is independent of camera admission.
X86_FUNC(sub_00214A50){ctx.eax=Word(base,ctx.esp+4);std::memset(base+ctx.eax,0,64);ctx.esp+=8;}
namespace {
unsigned view_renders=0,null_camera_clears=0,view_callbacks=0,render_ends=0;
unsigned campaign_body_queries=0;
bool painted_world=false;
bool watch_body_views=false;
std::array<std::array<bool,2>,2> body_view_admission{};
bool DrawAdmitted(uint32_t entity);
unsigned native_actions=0,native_action_plays=0,sound_plays=0,body_action_calls=0;
uint32_t last_action_body=0,last_action=0;
bool throw_action=false;
}
X86_FUNC(sub_000C1D60){
    assert(ctx.ecx==owners[0]||ctx.ecx==owners[1]);
    assert(Word(base,ctx.esp+8)==0);++native_actions;
    Put(base,ctx.ecx+0x3f0,Word(base,ctx.esp+4));ctx.eax=0x731010;ctx.esp+=12;
}
X86_FUNC(sub_001FB180){
    assert(ctx.ecx==owners[0]+0x17c||ctx.ecx==owners[1]+0x17c);
    assert(Word(base,ctx.esp+4)==0x731010);++native_action_plays;ctx.esp+=20;
}
X86_FUNC(sub_0028FF80){assert(false);}
X86_FUNC(sub_00287610){assert(false);}
X86_FUNC(sub_00216B10){assert(false);}
X86_FUNC(sub_000E57B0){
    // Native animation resource evaluation is outside this fixture. Exercise
    // its actual body/action ABI and the extracted sound boundary it calls.
    assert(ctx.ecx==objects[0]||ctx.ecx==objects[1]);
    ++body_action_calls;last_action_body=ctx.ecx;last_action=Word(base,ctx.esp+4);
    assert(TrialCoopBodyAnimationSoundSuppressed(ctx,base));
    const uint32_t end_sp=ctx.esp+8;
    for(uint32_t self:{ctx.ecx,owners[0],0x6a8000u}){
        X86Context sound=ctx;sound.ecx=self;
        for(uint32_t value:{0u,0u,0x731020u}){sound.esp-=4;Put(base,sound.esp,value);}
        sound.esp-=4;Put(base,sound.esp,0);
        const uint32_t start_sp=sound.esp;const unsigned before=sound_plays;
        CoopBodyActionSound(sound,base);
        assert(sound.esp==start_sp+16&&sound_plays==before+(self==ctx.ecx?0:1));
    }
    std::memset(&ctx,0xa5,sizeof ctx);ctx.esp=end_sp;_mm_setcsr(0x5f80);
    if(throw_action)throw 17;
}
void x86_icall(X86Context& c,uint8_t* b,uint32_t fn){
    if(!fn&&c.edi==0)throw 99; // Retail FD89A already read the missing parent's vtable.
    if(fn==0x710010){assert(c.ecx==c.edi&&c.edi);++attachment_parent_reads;c.esp+=12;return;}
    // Retail Mainplayer vtable 46FBE8+C0 is 2C2B60: xor eax,eax; ret.
    // Local campaign actors have no native multiplayer body through this getter.
    if(fn==0x2c2b60){assert(c.ecx==owners[0]||c.ecx==owners[1]);++campaign_body_queries;c.eax=0;c.esp+=4;return;}
    if(fn==0x8a090){__imp__sub_0008A090(c,b);return;}
    if(fn==0xb5e90){__imp__sub_000B5E90(c,b);return;}
    if(fn==0x720001){++sound_plays;c.eax=1;c.esp+=16;return;}
    if(fn==0x710001){c.eax=0x6f0000;c.esp+=4;return;} // Platform default fullscreen viewport.
    if(fn==0x710002){c.eax=c.ecx+0x10;c.esp+=4;return;} // Original camera matrix boundary.
    if(fn==0x710003||fn==0x710004){
        const uint32_t out=Word(b,c.esp+4);std::memset(b+out,0,fn==0x710003?64:18);c.eax=out;c.esp+=8;return;
    }
    if(fn==0x710005){
        const uint32_t parameters=Word(b,c.esp+4);++view_renders;
        if(watch_body_views){assert(view_renders<=2);for(unsigned i=0;i<2;++i)body_view_admission[view_renders-1][i]=DrawAdmitted(objects[i]);}
        // Execute the real render-parameter constructor. The device boundary
        // records its fullscreen black/no-scene command rather than invoking D3D.
        if(!Word(b,parameters+0x128)){
            assert(Word(b,parameters+4)==0x6f0000&&Word(b,parameters+0x124)==0xff000000);
            ++null_camera_clears;painted_world=false;
        }else{
            assert(Word(b,parameters+0x128)==0xffffffef&&Word(b,parameters+4)!=0x6f0000);
            painted_world=true;
        }
        c.esp+=8;return;
    }
    if(fn==0x710006){c.eax=0;c.esp+=4;return;} // Optional timing provider absent.
    if(fn==0x710007){++render_ends;c.esp+=4;return;}
    if(fn==0x710008){c.esp+=8;return;} // World category selection.
    if(fn==0x710009){++view_callbacks;c.esp+=4;return;}
    assert(fn==0x700001);c.eax=1;c.esp+=4; // Original render category boundary.
}
namespace {
uint8_t* B(){return kernel::memory.data();}
unsigned creates=0,loads=0,events=0,weapons=0,begins=0,ends=0;
unsigned retires=0;
std::vector<uint32_t> retired_ids;
bool replacement_factory=false,failed_factory=false;
std::array<uint32_t,2> skin{};
uint32_t RenderHandle(uint32_t entity){return Word(B(),Word(B(),entity+0x74));}
void AttachRender(uint32_t entity,uint32_t index,uint8_t visible,bool animated=true){
    const uint32_t wrapper=0x6c0000+index*0x100,render=wrapper+0x20;
    Put(B(),entity+0x74,wrapper);Put(B(),wrapper,render);Put(B(),render,animated?0x457880:0x456a78);
    X86Context c{};c.esp=0x7d0000;c.ecx=render;
    c.esp-=4;Put(B(),c.esp,visible);c.esp-=4;Put(B(),c.esp,0);
    if(animated)__imp__sub_000B5E80(c,B());else __imp__sub_0008A080(c,B());
}
bool DrawAdmitted(uint32_t entity){
    // Read the original model-wrapper chain independently of the repaired helper.
    const uint32_t render=RenderHandle(entity);assert(render);
    const uint32_t queue=0x6e0000,description=0x6e0200;
    Put(B(),queue+4,0);Put(B(),queue+8,0x6e0100);Put(B(),queue+0x1c,1);Put(B(),0x51a00c,8);
    X86Context c{};c.esp=0x7d0000;c.ecx=queue;
    for(uint32_t arg:{0u,description,render}){c.esp-=4;Put(B(),c.esp,arg);}
    c.esp-=4;Put(B(),c.esp,0);__imp__sub_000968A0(c,B());
    return Word(B(),queue+4)==1;
}
void Setup(){
    kernel::memory.fill(0);for(auto& p:bodies)p={};for(auto& p:retiring)p={};trial_coop::Reset();
    creates=loads=events=weapons=begins=ends=0;skin={};turret_mounted[0]=turret_mounted[1]=false;
    retires=0;retired_ids.clear();attachment_parent_reads=flare_cleanup=flare_frees=0;replacement_factory=failed_factory=false;
    native_actions=native_action_plays=sound_plays=body_action_calls=0;
    watch_body_views=false;body_view_admission={};
    last_action_body=last_action=0;throw_action=false;animation_sound_body=0;
    Put(B(),0x51c4dc,0x620000);Put(B(),0x620004,0x624000);
    Put(B(),0x51c398,0x640000);Put(B(),0x640004,0x641000);Put(B(),0x640008,0x642000);
    Put(B(),0x51c3e0,123);Put(B(),0x626044,456);Put(B(),0x626048,789);Put(B(),0x62604c,987);
    Put(B(),0x4fa004,55);
    Put(B(),0x44bf94,0x3f800000);
    Put(B(),0x44c178,0x42c80000);
    for(unsigned id=0;id<27;++id)Put(B(),0x670000+id*0x100,id);
    Put(B(),0x45d040+0x10,0x710010);
    Put(B(),0x462658+0x24,0x20a620);Put(B(),0x470058+0x24,0x20a620);Put(B(),0x472fc8+0x24,0x187af0);
    Put(B(),0x456a88,0x8a080);Put(B(),0x456a8c,0x8a090);Put(B(),0x456ad8,0x700001);
    Put(B(),0x457890,0xb5e80);Put(B(),0x457894,0xb5e90);Put(B(),0x4578e0,0x700001);
    AttachRender(0x6a8000,20,1,false); // Unrelated static scene entity must remain visible.
    for(unsigned i=0;i<2;++i){
        Put(B(),owners[i],0x46fbe8);Put(B(),owners[i]+0x54,100+i);Put(B(),owners[i]+0x368,i);
        Put(B(),owners[i]+0x308,400+i);const uint32_t camera=0x690000+i*0x1000;
        Put(B(),camera+0x54,400+i);Put(B(),camera+0x58,100+i);Put(B(),camera+0x22c,100+i);Put(B(),camera+0x210,2);
        Put(B(),0x660000+i*16+4,3+i);
        Put(B(),owners[i]+0x498+0x64,3+i);
    }
}
uint32_t Lookup(uint32_t id){
    for(uint32_t retired:retired_ids)if(retired==id)return 0;
    for(unsigned i=0;i<2;++i){
        if(id==200+i)return objects[i];
        if(id==400+i)return 0x750000+i*0x2000;
        for(unsigned j=0;j<4;++j)if(id==300+i*10+j)return objects[i]+0x1000+j*0x100;
        for(unsigned j=0;j<4;++j)if(id==500+i*10+j)return 0x751000+i*0x2000+j*0x100;
        for(unsigned j=0;j<2;++j)if(id==700+i*10+j)return 0x700000+i*0x1000+j*0x100;
        for(unsigned j=0;j<2;++j)if(id==800+i*10+j)return 0x760000+i*0x1000+j*0x100;
    }
    return 0;
}
void RemoveEntity(uint32_t id){
    const uint32_t object=Lookup(id);assert(object);
    // Native 207FF0 invalidates the ID and removes its world update node.
    // Every retiring descendant must still have a registered parent here.
    if(Word(B(),object)!=0x45d040u)assert(Lookup(Word(B(),object+0x58)));
    retired_ids.push_back(id);++retires;
}
void Invoke(X86Context& c,uint8_t* base,void(*fn)(X86Context&,uint8_t*),const uint32_t* a,size_t n){
    for(size_t i=n;i;--i){c.esp-=4;Put(base,c.esp,a[i-1]);}
    c.esp-=4;Put(base,c.esp,0);fn(c,base);
}
void SetupViews(bool ready_secondary){
    Setup();
    const uint32_t state=0x624000,list=0x628000,platform=0x6d0000,table=0x6d0200;
    Put(B(),state+4,list);Put(B(),state+8,list+8);Put(B(),state+0x2070,0x6d0400);
    Put(B(),0x6d0400,0x6d0500);Put(B(),0x6d052c,0x710008);
    Put(B(),0x5150b0,platform);Put(B(),platform,table);
    Put(B(),table+0x58,0x710001);Put(B(),table+0x24,0x710005);
    Put(B(),table+0x60,0x710006);Put(B(),table+0xa0,0x710007);
    Put(B(),0x46fbe8+0x4c,0x710009);Put(B(),0x46fbe8+0x50,0x710009);
    for(unsigned i=0;i<2;++i){
        const uint32_t layer=0x629000+i*0x100,chain=0x62a000+i*0x100,camera=0x690000+i*0x1000;
        Put(B(),list+i*4,layer);Put(B(),layer+4,owners[i]);Put(B(),layer+8,chain);Put(B(),layer+0x24,i);
        Put(B(),layer+0x20,0x6f0100+i*0x100);
        Put(B(),chain+8,i==0||ready_secondary?1:0);Put(B(),chain+4,chain+0x10);
        Put(B(),chain+0x14,chain+0x20);Put(B(),chain+0x28,camera);
        Put(B(),camera,0x6d0600);Put(B(),0x6d062c,0x710002);
        Put(B(),camera+0xd0,0x6d0700);Put(B(),0x6d0700,0x710003);Put(B(),0x6d0704,0x710004);
    }
    view_renders=null_camera_clears=view_callbacks=render_ends=0;painted_world=false;
}
void RenderViews(){
    X86Context c{};c.esp=0x7f0000;c.ecx=0x620000;c.fs_base=0x6d1000;
    c.ebx=21;c.edi=22;c.esi=23;c.ebp=24;Put(B(),c.esp,0);Put(B(),c.fs_base,123);
    __imp__sub_00215060(c,B());
    assert(c.esp==0x7f0004&&c.ebx==21&&c.edi==22&&c.esi==23&&c.ebp==24);
    assert(Word(B(),c.fs_base)==123&&render_ends==1);
}
void CheckUnreadyView(){
    // Negative witness: the retail loop/constructor renders the live P1 view,
    // then submits the null P2 camera as a fullscreen black clear.
    SetupViews(false);RenderViews();
    assert(view_renders==2&&null_camera_clears==1&&view_callbacks==4&&!painted_world);
    SetupViews(false);assert(trial_coop::Request(trial_coop::Character::TZero,trial_coop::Character::TZero));
    RenderViews();assert(view_renders==1&&!null_camera_clears&&view_callbacks==2&&painted_world);
    // Requesting co-op must not skip either ready camera or a foreign view.
    SetupViews(true);assert(trial_coop::Request(trial_coop::Character::TZero,trial_coop::Character::TZero));
    trial_coop::SetActive(true);
    RenderViews();assert(view_renders==2&&!null_camera_clears&&view_callbacks==4&&painted_world);
    SetupViews(false);assert(trial_coop::Request(trial_coop::Character::TZero,trial_coop::Character::TZero));
    Put(B(),0x629104,owners[0]);RenderViews();
    assert(view_renders==2&&null_camera_clears==1&&!painted_world);
    SetupViews(false);assert(trial_coop::Request(trial_coop::Character::TZero,trial_coop::Character::TZero));
    Put(B(),0x624008,0x628004);RenderViews();
    assert(view_renders==1&&!null_camera_clears&&view_callbacks==2&&painted_world);
}
}
namespace kernel {
uint32_t CallGuest(X86Context& c,uint32_t fn,const uint32_t* a,size_t n){
    auto* b=B();
    if(fn==0x159c0){assert(n==2);++begins;return c.ecx;}
    if(fn==0x1f8ba0){assert(!n);++ends;return 0;}
    if(fn==0x1f8e30){assert(n==1&&a[0]==1);Put(b,c.ecx+0x2a4,a[0]);return 0;}
    if(fn==0x1f8de0){assert(n==3&&a[0]==1);return 0;}
    if(fn==0x422c0){assert(!n);return c.ecx==owners[0]+0x498?0x660000:0x660010;}
    if(fn==0x41f50){assert(!n);X86Context temp=c;Invoke(temp,b,__imp__sub_00041F50,nullptr,0);return temp.eax;}
    if(fn==0x442d0){assert(n==1);return 0x670000+a[0]*0x100;}
    if(fn==0x207d20){
        assert(n==3&&c.ecx==0x620000&&Word(b,a[0])==0x459a38&&Word(b,a[2]+0x2a4)==1);
        const unsigned i=a[1]-100;assert(i<2);++creates;
        if(failed_factory)return 0xffffffffu;
        const uint32_t id=(replacement_factory?400:200)+i,p=replacement_factory?0x750000+i*0x2000:objects[i];
        Put(b,p,0x45d040);Put(b,p+0x54,id);Put(b,p+0x58,a[1]);Put(b,p+0x60,0x10);
        Put(b,0x51c3e0,999);Put(b,0x626044,0);Put(b,0x626048,0);Put(b,0x62604c,0);
        return id;
    }
    if(fn==0x1f8fc0){assert(n==1);return Lookup(a[0]);}
    if(fn==0xe6110){
        assert(n==2&&!a[1]);const unsigned i=c.ecx==objects[0]||c.ecx==0x750000?0:1;++loads;skin[i]=Word(b,a[0]);
        Put(b,c.ecx+0x158,1);Put(b,c.ecx+0x154,0x670000+(3+i)*0x100);
        AttachRender(c.ecx,i*5,2); // Preserve actual byte, not a normalized bool.
        const uint32_t offsets[]={0x1ac,0x1b0,0x22c,0x230};
        for(unsigned j=0;j<4;++j){
            const uint32_t id=(replacement_factory?500:300)+i*10+j,p=Lookup(id);
            Put(b,c.ecx+offsets[j],id);Put(b,p,j<2?0x462658:0x470058);
            Put(b,p+0x54,id);Put(b,p+0x58,(replacement_factory?400:200)+i);Put(b,p+0x5c,j==1?0x100:0);
            AttachRender(p,(replacement_factory?50:0)+i*5+j+1,j==1?0:1);
            if(j<2){
                const uint32_t flare_id=(replacement_factory?800:700)+i*10+j,flare=Lookup(flare_id);
                Put(b,p+0xac,flare_id);Put(b,flare,0x472fc8);Put(b,flare+0x54,flare_id);Put(b,flare+0x58,id);
                Put(b,flare+0xd0,flare+0x400); // Owned visual-model list; original retirement releases it.
                Put(b,p+0xa8,0x6d0900); // A selected weapon reaches the original parent matrix lookup.
            }
        }
        return 1;
    }
    if(fn==0x1f90a0){assert(n==5&&a[1]==55&&a[3]==1);++events;c.ecx=Lookup(a[0]);const uint32_t args[]={a[2],a[4]};Invoke(c,b,__imp__sub_000E4600,args,2);return 0;}
    if(fn==0xe55f0){assert(n==1);++weapons;Put(b,c.ecx+0x154,a[0]);return 0;}
    if(fn==0xe57b0){assert(n==1);Invoke(c,b,sub_000E57B0,a,n);return c.eax;}
    if(fn==0x218b30||fn==0x20a620){assert(!n);Invoke(c,b,__imp__sub_0020A620,nullptr,0);return 0;}
    if(fn==0x187af0){assert(!n);Invoke(c,b,__imp__sub_00187AF0,nullptr,0);return 0;}
    assert(false);return 0;
}
}
namespace {
void CheckBodyRecreate(){
    for(unsigned slot=0;slot<2;++slot){
        Setup();trial_coop::Request();trial_coop::SetActive(true);trial_coop::ResourcesReady(true);
        X86Context c{};c.esp=0x7f0000;TrialCoopBodiesEnsure(c,B());
        assert(!TrialCoopBodyRecreate(c,B(),owners[slot]));
        B()[objects[slot]+0x198]=1;Put(B(),owners[slot]+0x170,12);
        const auto saved=c;replacement_factory=true;failed_factory=true;
        assert(!TrialCoopBodyRecreate(c,B(),owners[slot])&&!retires);
        assert(TrialCoopBodyForPlayer(B(),owners[slot])==objects[slot]);
        failed_factory=false;assert(TrialCoopBodyRecreate(c,B(),owners[slot]));
        const uint32_t fresh=0x750000+slot*0x2000;
        assert(!retires&&TrialCoopBodyForPlayer(B(),owners[slot])==fresh);
        assert(Lookup(200+slot)==objects[slot]);
        assert(Word(B(),objects[slot]+0x60)&0x40000);
        assert(TrialCoopBodyForPlayer(B(),owners[1-slot])==objects[1-slot]);
        assert(!std::memcmp(&c,&saved,sizeof c));
        assert(Word(B(),0x51c3e0)==123&&Word(B(),0x626044)==456&&Word(B(),0x626048)==789&&Word(B(),0x62604c)==987);
        c.ebp=0x620000;const auto boundary=c;TrialCoopBodiesRetire(c,B());
        assert(!std::memcmp(&c,&boundary,sizeof c));
        assert(retires==7&&!Lookup(200+slot));
        assert(flare_cleanup==2&&flare_frees==2);
        assert(retired_ids.back()==200+slot);
        for(unsigned j=0;j<4;++j)assert(!Lookup(300+slot*10+j)&&Lookup(500+slot*10+j));
        for(unsigned j=0;j<2;++j)assert(!Lookup(700+slot*10+j)&&Lookup(800+slot*10+j));
        c.ecx=objects[slot];TrialCoopBodyDestroyed(c,B()); // Deferred old destructor cannot forget replacement.
        assert(TrialCoopBodyForPlayer(B(),owners[slot])==fresh);
        assert(!TrialCoopBodyRecreate(c,B(),owners[slot]));
    }
    puts("co-op body recreation: guarded dead owner, factory failure retains corpse, deferred native descendant-first retirement and delayed old destruction passed (offline; physics destruction remains live-only)");
}
void CheckReviveAttachmentLifetime(){
    const auto setup=[](unsigned slot){
        Setup();trial_coop::Request();trial_coop::SetActive(true);trial_coop::ResourcesReady(true);
        X86Context c{};c.esp=0x7f0000;TrialCoopBodiesEnsure(c,B());
        B()[objects[slot]+0x198]=1;Put(B(),owners[slot]+0x170,12);replacement_factory=true;
        return c;
    };
    const auto update_gun=[](uint32_t id){
        X86Context c{};c.esp=0x7e0000;c.ecx=Lookup(id);assert(c.ecx);
        OriginalAttachmentParentRead(c,B());
    };
    for(unsigned slot=0;slot<2;++slot){
        // Execute the original FD7F0..FD89F consumer with the previous immediate
        // retirement: the old gun is still scheduled but FD81E returns no body.
        auto c=setup(slot);const uint32_t gun=300+slot*10;
        update_gun(gun);assert(attachment_parent_reads==1);
        c.ecx=objects[slot];Invoke(c,B(),__imp__sub_0020A620,nullptr,0);
        assert(!Lookup(200+slot)&&Lookup(gun));
        bool missing_parent=false;
        try{update_gun(gun);}catch(int fault){assert(fault==99);missing_parent=true;}
        assert(missing_parent&&attachment_parent_reads==1);
    }
    for(unsigned slot=0;slot<2;++slot)for(bool child_already_updated:{false,true})for(bool native_delete:{false,true}){
        auto c=setup(slot);const uint32_t gun=300+slot*10;
        if(child_already_updated)update_gun(gun);
        assert(TrialCoopBodyRecreate(c,B(),owners[slot]));
        if(native_delete)Put(B(),objects[slot]+0x5c,Word(B(),objects[slot]+0x5c)|1);
        // Before the safe boundary, either iteration order still resolves the
        // original parent; its native first-pass admission blocks auto-expiry.
        update_gun(gun);
        Put(B(),0x740004,objects[slot]);X86Context admission{};admission.ecx=0x740000;
        OriginalWorldUpdateAdmission(admission,B());assert(!admission.eax);
        c.ebp=0x620000;TrialCoopBodiesRetire(c,B());
        assert(!Lookup(gun)&&!Lookup(200+slot));
        update_gun(500+slot*10); // Fresh attachment still resolves the fresh body.
        const unsigned retired=retires;TrialCoopBodiesRetire(c,B());assert(retires==retired);
    }
    // Stale/foreign IDs must not retire another player's children or a reused body.
    for(bool reused_body:{false,true}){
        auto c=setup(0);assert(TrialCoopBodyRecreate(c,B(),owners[0]));
        if(reused_body)Put(B(),objects[0]+0x54,999);
        else Put(B(),objects[0]+0x1ac,310);
        c.ebp=0x620000;TrialCoopBodiesRetire(c,B());
        assert(Lookup(310)&&Lookup(710)&&Lookup(201));
        if(reused_body)assert(!retires);
    }
    auto c=setup(0);assert(TrialCoopBodyRecreate(c,B(),owners[0]));
    TrialCoopBodiesReset();c.ebp=0x620000;TrialCoopBodiesRetire(c,B());assert(!retires);
    puts("co-op revive lifetime: original gun-parent crash witness, both players/update orders, pending native deletion, descendant order, fresh/foreign/stale isolation and reset passed (offline)");
}
void CheckBodyActions(){
    Setup();trial_coop::Request();trial_coop::SetActive(true);trial_coop::ResourcesReady(true);
    X86Context c{};c.esp=0x7f0000;TrialCoopBodiesEnsure(c,B());
    Put(B(),0x46fbe8+0xc0,0x2c2b60);Put(B(),0x46fbe8+0xc4,0x2c2b60);
    Put(B(),0x730000,0x730100);Put(B(),0x730104,0x720001);
    for(uint32_t self:{owners[0],owners[1],objects[0],objects[1],0x6a8000u})Put(B(),self+0x78,0x730000);
    for(uint32_t object:objects)Put(B(),object+0x1bc,1);
    const unsigned host_csr=_mm_getcsr();
    for(unsigned slot=0;slot<2;++slot)for(uint32_t action:{0u,3u,4u,7u,10u,16u,17u}){
        c={};c.esp=0x7f0000;c.ecx=owners[slot];c.eax=91;c.edx=92;c.ebx=93;c.edi=94;c.esi=95;c.ebp=96;
        c.cf=1;c.df=1;c.xflags=0x800;c.st[0]=3.25;c.top=2;c.fcw=0x37f;c.fsw=0x400;c.mxcsr=0x3f80;
        Put(B(),c.esp,0);Put(B(),c.esp+4,action);Put(B(),c.esp+8,11);Put(B(),c.esp+12,22);
        const auto start=c;std::array<uint8_t,0x900> owner_before{},owner_after{};
        std::memcpy(owner_before.data(),B()+owners[slot],owner_before.size());
        _mm_setcsr(0x3f80);const unsigned actions_before=body_action_calls;
        OriginalBodyActionCaller(c,B());const auto original=c;
        const unsigned original_csr=_mm_getcsr();
        std::memcpy(owner_after.data(),B()+owners[slot],owner_after.size());
        assert(body_action_calls==actions_before); // Retail campaign C0 getter is null.
        std::memcpy(B()+owners[slot],owner_before.data(),owner_before.size());
        c=start;_mm_setcsr(0x3f80);const unsigned sounds_before=sound_plays;
        CoopBodyActionCaller(c,B());
        assert(body_action_calls==actions_before+1&&last_action_body==objects[slot]&&last_action==action);
        assert(!std::memcmp(&c,&original,sizeof c)&&_mm_getcsr()==original_csr);
        assert(!std::memcmp(B()+owners[slot],owner_after.data(),owner_after.size()));
        assert(sound_plays==sounds_before+2&&!animation_sound_body);
        assert(native_actions==native_action_plays);
    }
    // The exact retail sound function still plays outside the temporary body
    // forwarding scope; suppression must not mute the owner, NPCs or later body sounds.
    for(bool original:{false,true}){
        c={};c.esp=0x7f0000;c.ecx=objects[0];const uint32_t args[]={0x731020,0,0};
        const unsigned before=sound_plays;
        Invoke(c,B(),original?OriginalBodyActionSound:CoopBodyActionSound,args,3);
        assert(sound_plays==before+1&&c.esp==0x7f0000);
        c={};c.esp=0x7f0000;c.ecx=objects[0];c.eax=0x1234abcd;c.esi=71;c.edi=72;
        const uint32_t silent[]={0,11,22};
        Invoke(c,B(),original?OriginalBodyActionSound:CoopBodyActionSound,silent,3);
        assert(sound_plays==before+1&&c.esp==0x7f0000&&c.eax==0x1234ab00&&c.esi==71&&c.edi==72);
    }
    for(unsigned invalid=0;invalid<7;++invalid){
        c={};c.esp=0x7f0000;c.esi=owners[0];c.edi=3;
        if(invalid==0)trial_coop::SetActive(false);
        if(invalid==1)Put(B(),owners[0]+0x54,999);
        if(invalid==2)Put(B(),objects[0]+0x5c,1);
        if(invalid==3)Put(B(),objects[0]+0x158,0);
        if(invalid==4)Put(B(),objects[0]+0x1bc,0);
        if(invalid==5)c.edi=18;
        if(invalid==6)c.esp=0x100;
        const auto before=c;const unsigned calls=body_action_calls;
        TrialCoopBodyAction(c,B());assert(body_action_calls==calls&&!std::memcmp(&c,&before,sizeof c)&&!animation_sound_body);
        trial_coop::SetActive(true);Put(B(),owners[0]+0x54,100);Put(B(),objects[0]+0x5c,0);
        Put(B(),objects[0]+0x158,1);Put(B(),objects[0]+0x1bc,1);
    }
    c={};c.esp=0x7f0000;c.esi=owners[1];c.edi=4;const auto before=c;
    _mm_setcsr(0x3f80);throw_action=true;
    try{TrialCoopBodyAction(c,B());assert(false);}catch(int value){assert(value==17);}
    assert(!animation_sound_body&&!std::memcmp(&c,&before,sizeof c)&&_mm_getcsr()==0x3f80);
    throw_action=false;_mm_setcsr(host_csr);
    puts("co-op action bridge: original null-body caller witness, both owners/actions, unchanged native caller state, scoped sound suppression and stale/inactive/exception restoration passed (offline; native animation evaluation remains live-only)");
}
void CheckBodyWeaponSelection(){
    // Exercise the pinned D6070 selector and 41F50 inventory predicate, rather
    // than reproducing their decisions in mocks. E55F0's resource/model swap is
    // the recorded boundary; live animation/attachment acceptance stays separate.
    unsigned shield_witnesses=0;
    for(unsigned choice=0;choice<5;++choice){
        Setup();trial_coop::Request(static_cast<trial_coop::Character>(choice),trial_coop::Character::TZero);
        trial_coop::SetActive(true);trial_coop::ResourcesReady(true);
        X86Context c{};c.esp=0x7f0000;TrialCoopBodiesEnsure(c,B());
        for(unsigned slot=0;slot<2;++slot)for(bool selected:{false,true})
        for(bool held:{false,true})for(bool lowering:{false,true})
        for(float position:{0.f,99.9f,100.f,101.f})for(bool unavailable:{false,true}){
            const uint32_t owner=owners[slot],primary=3+slot,inventory=owner+0x498;
            Put(B(),inventory+0x64,selected?0:primary);
            B()[inventory+0xf4]=held;B()[inventory+0xf5]=lowering;
            std::memcpy(B()+owner+0x24c,&position,4);B()[owner+0x2ac]=unavailable;
            X86Context oracle{};oracle.esp=0x7e0000;oracle.esi=owner;oracle.ebx=0x12345678;
            Invoke(oracle,B(),OriginalBodyWeaponSelection,nullptr,0);
            assert(oracle.esp==0x7e0000&&oracle.ebx==0x12345678);
            const uint32_t expected=unavailable?0xffffffffu:
                ((selected||held)&&!lowering&&position<100.f?0:primary);
            assert(oracle.eax==expected);
            if(expected==0){assert(primary!=oracle.eax);++shield_witnesses;} // Former firearm-only selection fails.
            c={};c.esp=0x7f0000;c.esi=owner;c.eax=91;c.ecx=92;c.edi=93;
            const auto before=c;const unsigned csr=_mm_getcsr();
            const uint32_t peer=Word(B(),objects[1-slot]+0x154);
            TrialCoopBodyWeapon(c,B());
            assert(Word(B(),objects[slot]+0x154)==(expected==0xffffffffu?0:0x670000+expected*0x100));
            assert(Word(B(),objects[1-slot]+0x154)==peer&&!std::memcmp(&c,&before,sizeof c)&&_mm_getcsr()==csr);
            const unsigned transitions=weapons;TrialCoopBodyWeapon(c,B());assert(weapons==transitions);
        }
        // The unsupported mounted gun remains guarded even if shield flags
        // linger through a mount; selecting a normal weapon resumes the bridge.
        c={};c.esp=0x7f0000;c.esi=owners[0];Put(B(),0x660004,25);
        const unsigned transitions=weapons;const uint32_t prior=Word(B(),objects[0]+0x154);
        TrialCoopBodyWeapon(c,B());assert(weapons==transitions&&Word(B(),objects[0]+0x154)==prior);
        Put(B(),0x660004,3);B()[owners[0]+0x2ac]=0;B()[owners[0]+0x58d]=0;
        Put(B(),owners[0]+0x24c,0);TrialCoopBodyWeapon(c,B());
        assert(Word(B(),objects[0]+0x154)==0x670000);
        trial_coop::SetActive(false);B()[owners[0]+0x58c]=0;Put(B(),owners[0]+0x4fc,3);
        const unsigned inactive=weapons;TrialCoopBodyWeapon(c,B());assert(weapons==inactive);
    }
    assert(shield_witnesses);
    puts("co-op shield: pinned original selection/gate, both owners, all skins, raise/hold/lower, position threshold, unavailable state, unchanged peer/context, stable hold and mounted exclusion passed (offline; visual pose/attachment requires live acceptance)");
}
void CheckBodySync(){
    Setup();trial_coop::Request();trial_coop::SetActive(true);trial_coop::ResourcesReady(true);
    X86Context c{};c.esp=0x7f0000;TrialCoopBodiesEnsure(c,B());assert(creates==2);
    Put(B(),0x46fbe8+0xc0,0x2c2b60);
    // Execute the actual CA847..CA881 lookup/branch/call sequence. Testing the
    // setters alone missed preparation dropping the first two identical hooks.
    for(unsigned frame=0;frame<6;++frame)for(unsigned slot=0;slot<2;++slot){
        c={};c.esp=0x7f0000;c.esi=owners[slot];
        std::array<float,16> transform{};
        for(unsigned j=0;j<16;++j)transform[j]=j%5==0?1.f:0.f;
        transform[12]=float(frame*100+slot*1000);transform[13]=float(frame*7);transform[14]=float(frame*13+slot*30);
        std::memcpy(B()+c.esp+0x20,transform.data(),64);
        const float crouch=float(frame)/5;uint32_t crouch_bits;std::memcpy(&crouch_bits,&crouch,4);Put(B(),c.esp+8,crouch_bits);
        std::array<uint8_t,64> old_matrix{},other_matrix{};
        std::memcpy(old_matrix.data(),B()+objects[slot]+0xd0,64);
        std::memcpy(other_matrix.data(),B()+objects[1-slot]+0xd0,64);
        const uint32_t old_crouch=Word(B(),objects[slot]+0x1d8),other_crouch=Word(B(),objects[1-slot]+0x1d8);
        const auto start=c;campaign_body_queries=0;OriginalBodySyncCaller(c,B());
        assert(campaign_body_queries==1&&c.esp==start.esp&&c.esi==start.esi);
        assert(!std::memcmp(B()+objects[slot]+0xd0,old_matrix.data(),64)&&Word(B(),objects[slot]+0x1d8)==old_crouch);
        c=start;campaign_body_queries=0;CoopBodySyncCaller(c,B());
        assert(campaign_body_queries==3&&c.esp==start.esp&&c.esi==start.esi);
        assert(!std::memcmp(B()+objects[slot]+0xd0,transform.data(),64)&&Word(B(),objects[slot]+0x1d8)==crouch_bits);
        assert(!std::memcmp(B()+objects[1-slot]+0xd0,other_matrix.data(),64)&&Word(B(),objects[1-slot]+0x1d8)==other_crouch);
        assert(!std::memcmp(B()+start.esp+0x20,transform.data(),64));
    }
    for(unsigned slot=0;slot<2;++slot)for(bool stale_owner:{false,true}){
        c={};c.esp=0x7f0000;c.esi=owners[slot];
        std::memset(B()+c.esp+0x20,0x5a,64);Put(B(),c.esp+8,0x3f800000);
        trial_coop::SetActive(stale_owner);if(stale_owner)Put(B(),owners[slot]+0x54,999);
        std::array<uint8_t,64> original{};std::memcpy(original.data(),B()+objects[slot]+0xd0,64);
        const uint32_t crouch=Word(B(),objects[slot]+0x1d8);campaign_body_queries=0;
        CoopBodySyncCaller(c,B());assert(campaign_body_queries==1&&c.esp==0x7f0000);
        assert(!std::memcmp(B()+objects[slot]+0xd0,original.data(),64)&&Word(B(),objects[slot]+0x1d8)==crouch);
        Put(B(),owners[slot]+0x54,100+slot);
    }
    // E6B40 subsequently derives animation/world matrices and submits the skin.
    // Those resource-driven operations still require a live visual comparison.
    puts("co-op body sync: original null-getter witness, both moving owners, crouch and inactive/stale guards passed (native skin animation not simulated)");
}
void CheckRenderLayouts(){
    Setup();trial_coop::Request();trial_coop::SetActive(true);trial_coop::ResourcesReady(true);
    X86Context c{};c.esp=0x7f0000;TrialCoopBodiesEnsure(c,B());assert(creates==2);
    for(bool animated:{false,true})for(unsigned slot=0;slot<2;++slot){
        AttachRender(objects[slot],30+slot,uint8_t(2+slot),animated);
        const uint32_t render=RenderHandle(objects[slot]),offset=animated?0x66:0x50;
        assert(RenderVisibility(B(),objects[slot])==render+offset&&DrawAdmitted(objects[slot]));
        // 215106 has already copied entity visibility before the camera loop.
        // A late entity hide flag (and the former static-only byte) cannot hide
        // an animated render interface from the original 968A0 getter call.
        Put(B(),objects[slot]+0x5c,0x100);
        if(animated)B()[render+0x50]=0;
        assert(DrawAdmitted(objects[slot])&&B()[render+offset]==2+slot);
        Put(B(),objects[slot]+0x5c,0);
        std::array<uint8_t,0x70> original{},hidden{};
        std::memcpy(original.data(),B()+render,original.size());hidden=original;hidden[offset]=0;
        c.edi=owners[slot];c.esi=0x690000+slot*0x1000;
        {TrialCoopBodyView view;view.Select(c,B());
            assert(!DrawAdmitted(objects[slot])&&DrawAdmitted(objects[1-slot]));
            assert(!std::memcmp(B()+render,hidden.data(),hidden.size()));
        }
        assert(DrawAdmitted(objects[slot])&&Word(B(),objects[slot]+0x5c)==0);
        assert(!std::memcmp(B()+render,original.data(),original.size()));
    }
    c.edi=owners[0];c.esi=0x690000;
    // A mismatched virtual pair is an unknown layout, even if its getter happens
    // to be one we know. Do not infer a writable offset from that getter alone.
    const uint32_t unknown_table=0x6d1000,render=RenderHandle(objects[0]);
    Put(B(),unknown_table+0x10,0x8a080);Put(B(),unknown_table+0x14,0xb5e90);
    Put(B(),unknown_table+0x60,0x700001);Put(B(),render,unknown_table);
    std::array<uint8_t,0x70> unknown{};std::memcpy(unknown.data(),B()+render,unknown.size());
    assert(!RenderVisibility(B(),objects[0]));
    {TrialCoopBodyView view;view.Select(c,B());assert(DrawAdmitted(objects[0]));
        assert(!std::memcmp(B()+render,unknown.data(),unknown.size()));}
    assert(!std::memcmp(B()+render,unknown.data(),unknown.size()));
    // If the model is rebound during a view, restoring the old object's byte
    // must not overwrite the replacement or resurrect a detached render object.
    AttachRender(objects[0],30,2);const uint32_t old_render=RenderHandle(objects[0]);
    {TrialCoopBodyView view;view.Select(c,B());assert(B()[old_render+0x66]==0);
        AttachRender(objects[0],40,7);assert(DrawAdmitted(objects[0]));}
    assert(B()[old_render+0x66]==0&&B()[RenderHandle(objects[0])+0x66]==7);
    assert(Word(B(),objects[0]+0x5c)==0);
    for(bool stale_owner:{false,true}){
        trial_coop::SetActive(stale_owner);if(stale_owner)Put(B(),owners[0]+0x54,999);
        {TrialCoopBodyView view;view.Select(c,B());assert(DrawAdmitted(objects[0]));
            assert(Word(B(),objects[0]+0x5c)==0&&B()[RenderHandle(objects[0])+0x66]==7);}
        Put(B(),owners[0]+0x54,100);
    }
    puts("co-op visibility: original animated/static getters, late-flag failure witness, both owners, exact-byte restoration, unknown-layout/rebinding and inactive/stale guards passed (offline)");
}
void CheckAuthoredBodyVisibility(){
    const auto setup=[](uint32_t type,unsigned slot=0){
        SetupViews(true);trial_coop::Request();trial_coop::SetActive(true);trial_coop::ResourcesReady(true);
        X86Context c{};c.esp=0x7f0000;TrialCoopBodiesEnsure(c,B());
        c.ebx=0x620000;c.edi=owners[slot];c.esi=0x690000+slot*0x1000;
        Put(B(),c.esi,type);Put(B(),c.esi+0xd0,0x478d20);
        Put(B(),type+0x2c,0x710002);Put(B(),0x478d20,0x710003);Put(B(),0x478d24,0x710004);
        // Unlike the normal owner camera, authored cameras have their own ID.
        Put(B(),c.esi+0x54,900);Put(B(),c.esi+0x210,0);
        return c;
    };
    for(uint32_t type:{0x46fd70u,0x477da8u})for(unsigned camera_slot=0;camera_slot<2;++camera_slot){
        auto c=setup(type,camera_slot);const auto before=c;const unsigned csr=_mm_getcsr();
        assert(DrawAdmitted(objects[0])&&DrawAdmitted(objects[1]));
        std::array<uint32_t,10> flags{};std::array<uint8_t,10> visible{};
        for(unsigned slot=0;slot<2;++slot)for(unsigned item=0;item<5;++item){
            const uint32_t object=item?Lookup(300+slot*10+item-1):objects[slot];
            flags[slot*5+item]=Word(B(),object+0x5c);visible[slot*5+item]=B()[RenderVisibility(B(),object)];
        }
        for(unsigned repeat=0;repeat<3;++repeat){
            TrialCoopBodyView view;view.Select(c,B());
            assert(!DrawAdmitted(objects[0])&&!DrawAdmitted(objects[1])&&DrawAdmitted(0x6a8000));
            for(unsigned slot=0;slot<2;++slot)for(unsigned item=0;item<4;++item)assert(!DrawAdmitted(Lookup(300+slot*10+item)));
            {TrialCoopBodyView nested;nested.Select(c,B());assert(!DrawAdmitted(objects[0])&&!DrawAdmitted(objects[1]));}
            assert(!DrawAdmitted(objects[0])&&!DrawAdmitted(objects[1]));
            view.Clear();
            for(unsigned slot=0;slot<2;++slot)for(unsigned item=0;item<5;++item){
                const uint32_t object=item?Lookup(300+slot*10+item-1):objects[slot];
                assert(Word(B(),object+0x5c)==flags[slot*5+item]&&B()[RenderVisibility(B(),object)]==visible[slot*5+item]);
            }
        }
        assert(!std::memcmp(&c,&before,sizeof c)&&_mm_getcsr()==csr);
        try{TrialCoopBodyView unwind;unwind.Select(c,B());throw 1;}catch(int){}
        assert(DrawAdmitted(objects[0])&&DrawAdmitted(objects[1]));
        // Exercise the actual 215060 loop and 968A0 admission: authored view
        // hides both, then the ordinary P2 view shows only its partner again.
        watch_body_views=true;RenderViews();watch_body_views=false;
        const std::array<std::array<bool,2>,2> expected=camera_slot==0?
            std::array<std::array<bool,2>,2>{{{false,false},{true,false}}}:
            std::array<std::array<bool,2>,2>{{{false,true},{false,false}}};
        assert(body_view_admission==expected);
        assert(DrawAdmitted(objects[0])&&DrawAdmitted(objects[1]));
    }
    for(unsigned invalid=0;invalid<8;++invalid){
        auto c=setup(0x46fd70);
        if(invalid==0)Put(B(),c.esi,0x477c58); // Original enemy killcam.
        if(invalid==1)Put(B(),c.esi,0x6d0600); // Other/death camera.
        if(invalid==2)Put(B(),c.esi+0x54,0xffffffff);
        if(invalid==3)Put(B(),c.esi+0x5c,1);
        if(invalid==4)Put(B(),c.esi+0xd0,0);
        if(invalid==5)Put(B(),0x62a028,0x691000); // Camera no longer atop this layer.
        if(invalid==6)Put(B(),0x629004,owners[1]);
        if(invalid==7){trial_coop::Reset();}
        TrialCoopBodyView view;view.Select(c,B());assert(DrawAdmitted(objects[0])&&DrawAdmitted(objects[1]));
    }
    for(unsigned dead=0;dead<2;++dead)for(bool ragdoll:{false,true}){
        auto c=setup(0x46fd70);
        if(ragdoll)B()[objects[dead]+0x198]=1;else Put(B(),owners[dead]+0x170,12);
        {TrialCoopBodyView view;view.Select(c,B());assert(DrawAdmitted(objects[dead])&&!DrawAdmitted(objects[1-dead]));}
        assert(DrawAdmitted(objects[0])&&DrawAdmitted(objects[1]));
    }
    auto c=setup(0x46fd70);
    // Admission depends on the current original camera, not readiness of the
    // other camera or a host presentation snapshot. No new body is constructed.
    Put(B(),0x62a108,0);trial_coop::SetActive(false);trial_coop::ResourcesReady(false);
    {TrialCoopBodyView view;view.Select(c,B());assert(!DrawAdmitted(objects[0])&&!DrawAdmitted(objects[1]));}
    assert(DrawAdmitted(objects[0])&&DrawAdmitted(objects[1]));
    puts("co-op authored-camera body visibility: both owned bodies/children, exact restoration, original render-loop admission, reentry/unwind, unready partner and killcam/ragdoll exclusions passed (offline)");
}
}
int main(){
    CheckBodyRecreate();
    CheckReviveAttachmentLifetime();
    CheckUnreadyView();
    CheckBodySync();
    CheckBodyWeaponSelection();
    CheckBodyActions();
    CheckRenderLayouts();
    CheckAuthoredBodyVisibility();
    for(unsigned choice=0;choice<5;++choice){
        Setup();X86Context c{};c.esp=0x7f0000;c.esi=owners[0];c.edi=owners[0];c.eax=1234;
        const auto saved=c;
        TrialCoopBodiesEnsure(c,B());assert(!creates);
        trial_coop::Request(static_cast<trial_coop::Character>(choice),trial_coop::Character::TZero);
        trial_coop::SetActive(true);TrialCoopBodiesEnsure(c,B());assert(!creates);
        trial_coop::ResourcesReady(true);TrialCoopBodiesEnsure(c,B());
        assert(creates==2&&loads==2&&events==2&&begins==2&&ends==2);
        assert(!std::memcmp(&c,&saved,sizeof c));assert(Word(B(),0x51c3e0)==123&&Word(B(),0x626044)==456&&Word(B(),0x626048)==789&&Word(B(),0x62604c)==987);
        const uint32_t skins[]={0x45bcf8,0x45be70,0x45be10,0x45bdb0,0x45bd50};assert(skin[0]==skins[choice]&&skin[1]==skins[0]);
        assert(Word(B(),objects[0]+0x60)==2&&B()[objects[0]+0x234]==1);
        TrialCoopBodiesEnsure(c,B());assert(creates==2);
        for(unsigned i=0;i<2;++i){
            c=saved;c.esi=owners[i];c.eax=0;TrialCoopBodyResolve(c,B());assert(c.eax==objects[i]);
            std::array<float,16> transform{};for(unsigned j=0;j<16;++j)transform[j]=float(i*100+j);
            std::memcpy(B()+0x680000,transform.data(),64);const uint32_t arg=0x680000;c.ecx=c.eax;
            Invoke(c,B(),__imp__sub_000E4300,&arg,1);assert(!std::memcmp(B()+objects[i]+0xd0,transform.data(),64));
            float f=.25f+float(i)*.5f;uint32_t bits;std::memcpy(&bits,&f,4);c.ecx=objects[i];Invoke(c,B(),__imp__sub_000E7800,&bits,1);assert(Word(B(),objects[i]+0x1d8)==bits);
        }
        c=saved;TrialCoopBodyWeapon(c,B());assert(!weapons);Put(B(),0x660004,8);TrialCoopBodyWeapon(c,B());assert(weapons==1&&Word(B(),objects[0]+0x154)==0x670800);
        // ID25 has no retail third-person metadata. Keep the last valid body
        // definition while the partner sees the owner's actual mounted gun.
        turret_mounted[0]=true;Put(B(),0x660004,25);const auto mounted_context=c;
        TrialCoopBodyWeapon(c,B());assert(weapons==1&&Word(B(),objects[0]+0x154)==0x670800);
        assert(!std::memcmp(&c,&mounted_context,sizeof c));
        turret_mounted[0]=false;TrialCoopBodyWeapon(c,B());assert(weapons==1&&Word(B(),objects[0]+0x154)==0x670800);
        Put(B(),0x660004,9);TrialCoopBodyWeapon(c,B());
        assert(weapons==2&&Word(B(),objects[0]+0x154)==0x670900);

        assert(!std::memcmp(&c,&saved,sizeof c));
        c.esi=0x690000;
        turret_mounted[1]=true;
        {TrialCoopBodyView mounted;mounted.Select(c,B());
            assert(DrawAdmitted(objects[1]));
            for(unsigned j=0;j<2;++j)assert(!DrawAdmitted(Lookup(310+j)));
            for(unsigned j=2;j<4;++j)assert(DrawAdmitted(Lookup(310+j)));}
        assert(DrawAdmitted(objects[1])&&DrawAdmitted(Lookup(310)));
        turret_mounted[1]=false;
        assert(DrawAdmitted(objects[0])&&DrawAdmitted(objects[1]));
        {TrialCoopBodyView outer;outer.Select(c,B());assert((Word(B(),objects[0]+0x5c)&0x100)&&!(Word(B(),objects[1]+0x5c)&0x100));
            assert(!DrawAdmitted(objects[0])&&DrawAdmitted(objects[1])&&DrawAdmitted(0x6a8000));
            for(unsigned j=0;j<4;++j){assert(Word(B(),Lookup(300+j)+0x5c)&0x100);assert(!DrawAdmitted(Lookup(300+j)));}
            {TrialCoopBodyView nested;c.edi=owners[1];c.esi=0x691000;nested.Select(c,B());assert(Word(B(),objects[1]+0x5c)&0x100);assert(!DrawAdmitted(objects[1]));}
            assert(!(Word(B(),objects[1]+0x5c)&0x100)&&DrawAdmitted(objects[1]));
            Put(B(),objects[0]+0x5c,Word(B(),objects[0]+0x5c)|0x200); // Other render-time flags survive restoration.
        }
        assert(Word(B(),objects[0]+0x5c)==0x200&&DrawAdmitted(objects[0])&&B()[RenderHandle(objects[0])+0x66]==2);
        for(unsigned j=0;j<4;++j){assert(Word(B(),Lookup(300+j)+0x5c)==(j==1?0x100u:0));assert(DrawAdmitted(Lookup(300+j))==(j!=1));}
        c.edi=owners[0];c.esi=0x690000;Put(B(),c.esi+0x210,1);
        {TrialCoopBodyView cinematic;cinematic.Select(c,B());assert(!(Word(B(),objects[0]+0x5c)&0x100)&&DrawAdmitted(objects[0]));}
        c=saved;Put(B(),owners[0]+0x54,999);c.eax=321;TrialCoopBodyResolve(c,B());assert(c.eax==321);Put(B(),owners[0]+0x54,100);
        c.ecx=objects[0];TrialCoopBodyDestroyed(c,B());c.eax=321;TrialCoopBodyResolve(c,B());assert(c.eax==321);
        c.ecx=owners[1];TrialCoopBodyDestroyed(c,B());c.esi=owners[1];TrialCoopBodyResolve(c,B());assert(c.eax==321);
    }
    puts("co-op body ownership, creation gates, original transform/crouch/visibility, weapon handoff, render admission/restoration and original unready-view black-clear regression passed (offline)");
}

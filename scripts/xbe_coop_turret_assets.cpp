#include "xbe_coop_turret_assets.h"
#include "xbe_coop.h"
#include "cpu/x86_ops.h"
#include "kernel/kernel.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include <array>
#include <cstring>
#include <exception>
#include <filesystem>
#include <xmmintrin.h>

namespace {
constexpr const char* original[]={
    "assets_scene_items\\03-01\\sil_interiorhelicopter\\sil_interiorhelicopter_skin.amf",
    "assets_scene_items\\08-01\\sil_interiorhelicopter\\sil_interiorhelicopter_skin.amf"};
constexpr const char* alias[]={
    "assets_coop\\helicopter\\03-01\\sil_interiorhelicopter_skin.amf",
    "assets_coop\\helicopter\\08-01\\sil_interiorhelicopter_skin.amf"};
constexpr const char* archives[]={"coopturrets03","coopturrets08"};
struct Ready {uint32_t world=0,owner=0,id=0,wrapper=0,render=0;};
std::array<Ready,2> ready{};
uint32_t loaded_world=0;
unsigned loaded_mask=0;
thread_local int loading=-1;
thread_local unsigned seen=0;
uint32_t Word(const uint8_t* b,uint32_t p){uint32_t value;std::memcpy(&value,b+p,4);return value;}
void Put(uint8_t* b,uint32_t p,uint32_t value){std::memcpy(b+p,&value,4);}
bool Name(const uint8_t* b,uint32_t p,const char* expected){
    const auto size=std::strlen(expected);
    if(!TrialGuestReadable(p,uint32_t(size+1))||b[p+size])return false;
    for(size_t i=0;i<size;++i){unsigned char c=b[p+i];if(c>='A'&&c<='Z')c+='a'-'A';if(c=='/')c='\\';if(c!=expected[i])return false;}
    return true;
}
bool Owner(const uint8_t* b,uint32_t p,uint32_t id){
    return TrialGuestReadable(p,0x1c0)&&Word(b,p)==0x4753a0&&
        Word(b,p+0x54)==id&&id!=0xffffffffu&&!(Word(b,p+0x5c)&1);
}
bool Load(X86Context& c,uint8_t* b,unsigned level,uint32_t world,uint32_t storage){
    if(loading>=0||!TrialGuestReadable(c.fs_base,4))return false;
    if(loaded_world!=world){loaded_world=world;loaded_mask=0;ready={};}
    if(loaded_mask&(1u<<level))return true;
    const auto path=kernel::ResolvePath(std::string("D:\\")+archives[level]+".lst",0);
    std::error_code error;const auto size=std::filesystem::file_size(path,error);
    if(error||size<0xc8||size>2*1024*1024)return false;
    // A separate guest stack keeps the active entity/model loader's frame and
    // its original path argument intact during this nested resource load.
    const uint32_t stack=mem::AllocStack(0x10000);if(!stack)return false;
    struct Restore {
        uint32_t stack;uint8_t* base;uint32_t fs,seh;unsigned csr=_mm_getcsr();int previous=loading;unsigned prior_seen=seen;
        ~Restore(){Put(base,fs,seh);loading=previous;seen=prior_seen;_mm_setcsr(csr);mem::FreeStack(stack);}
    } restore{stack,b,c.fs_base,Word(b,c.fs_base)};
    loading=int(level);seen=0;_mm_setcsr(0x1f80);
    std::strcpy(reinterpret_cast<char*>(b+storage+16),archives[level]);
    X86Context call=c;call.esp=stack-16;call.df=0;
    kernel::CallGuest(call,0x54b70,{storage+16});
    // 8DB16 follows the real AMF parser. Support records are raw payloads and
    // may legitimately return null from generic GetResource; the archive
    // builder verifies both, and 21D0E0 consumes them through the normal paths.
    if(seen!=1)return false;
    loaded_mask|=1u<<level;return true;
}
}

void TrialCoopTurretAssetsReset(){ready={};loaded_world=0;loaded_mask=0;}
bool TrialCoopTurretAssetReady(const uint8_t* b,uint32_t helicopter){
    if(!b||!trial_coop::Available()||!trial_coop::Requested()||!TrialGuestReadable(0x51c4dc,4))return false;
    for(const auto& r:ready)if(r.owner==helicopter&&r.world==Word(b,0x51c4dc)&&Owner(b,r.owner,r.id)&&
        Word(b,r.owner+0x74)==r.wrapper&&TrialGuestReadable(r.wrapper,4)&&Word(b,r.wrapper)==r.render&&
        TrialGuestReadable(r.render,0x118)&&Word(b,r.render)==0x457880)return true;
    return false;
}
void TrialCoopTurretResourceLoaded(const X86Context& c,const uint8_t* b){
    if(loading<0||!b||!TrialGuestReadable(c.esp,0x18)||!TrialGuestReadable(c.eax,4))return;
    const uint32_t name=Word(b,c.esp+0x10);
    if(Word(b,c.eax)==0x457008&&Name(b,name,alias[loading]))seen=1;
}
TrialCoopTurretAssetScope::TrialCoopTurretAssetScope(X86Context& c,uint8_t* b):exceptions_(std::uncaught_exceptions()){
    // Only the FirstPersonHelicopter model callback (199810), never arbitrary
    // entities sharing 20BC40. This can precede both MainPlayer constructors.
    if(!b||!trial_coop::Available()||!trial_coop::Requested()||!TrialGuestReadable(c.esp,4)||
       Word(b,c.esp)!=0x199825||!TrialGuestReadable(c.ecx,0x1c0)||!TrialGuestReadable(0x51c4dc,4))return;
    const uint32_t id=Word(b,c.ecx+0x54),world=Word(b,0x51c4dc);
    if(!Owner(b,c.ecx,id)||!TrialGuestReadable(world,0x38))return;
    context_=&c;base_=b;owner_=c.ecx;id_=id;world_=world;
    for(auto& r:ready)if(r.owner==owner_)r={};
}
void TrialCoopTurretAssetScope::Select(X86Context& c){
    if(!base_||selected_||c.esi!=owner_||!Owner(base_,owner_,id_)||
       Word(base_,0x51c4dc)!=world_||!TrialGuestReadable(c.ebp,4))return;
    const uint32_t name=Word(base_,c.ebp);
    unsigned level=0;for(;level<2;++level)if(Name(base_,name,original[level]))break;
    if(level==2)return;
    storage_=mem::AllocSystem(256);if(!storage_)return;
    if(!TrialGuestReadable(storage_,256)||!Load(c,base_,level,world_,storage_))return;
    std::strcpy(reinterpret_cast<char*>(base_+storage_+64),alias[level]);Put(base_,storage_,storage_+64);
    // EBP is a temporary pointer-to-path at 20BC7C; the function's original
    // prologue/epilogue restores the caller's EBP. No authored property changes.
    c.ebp=storage_;selected_=true;
}
TrialCoopTurretAssetScope::~TrialCoopTurretAssetScope(){
    if(selected_&&exceptions_==std::uncaught_exceptions()&&uint8_t(context_->eax)&&
       Word(base_,0x51c4dc)==world_&&Owner(base_,owner_,id_)){
        const uint32_t wrapper=Word(base_,owner_+0x74);
        if(TrialGuestReadable(wrapper,4)){
            const uint32_t render=Word(base_,wrapper);
            if(TrialGuestReadable(render,0x118)&&Word(base_,render)==0x457880)
                for(auto& r:ready)if(!r.owner||r.owner==owner_||r.world!=world_){r={world_,owner_,id_,wrapper,render};break;}
        }
    }
    if(storage_)mem::FreeSystem(storage_);
}

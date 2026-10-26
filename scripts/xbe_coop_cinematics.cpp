#include "xbe_coop_cinematics.h"
#include "xbe_coop.h"
#include "xbe_coop_guest.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include "cpu/x86_ops.h"
#include "kernel/kernel.h"
#include <atomic>
#include <cstring>
#include <exception>
#include <xmmintrin.h>

namespace {
std::atomic<unsigned> presentation{0};
uint32_t Word(const uint8_t* b,uint32_t a){uint32_t v;std::memcpy(&v,b+a,4);return v;}
void Put(uint8_t* b,uint32_t a,uint32_t v){std::memcpy(b+a,&v,4);}
bool PlayerIds(const uint8_t* b,uint32_t (&ids)[2]){
    if(!b||!trial_coop::Available()||!trial_coop::Requested()||!trial_coop::Active()||
       !TrialGuestReadable(0x51c4dc,4))return false;
    const uint32_t world=Word(b,0x51c4dc);
    if(!TrialGuestReadable(world,8))return false;
    const uint32_t state=Word(b,world+4);
    if(!TrialGuestReadable(state,12))return false;
    const uint32_t first=Word(b,state+4);
    if(first>0xfffffff7u||Word(b,state+8)!=first+8||!TrialGuestReadable(first,8))return false;
    for(unsigned slot=0;slot<2;++slot){
        const uint32_t actor=TrialCoopPlayer(slot),layer=Word(b,first+4*slot);
        if(!TrialGuestReadable(actor,0x36c)||Word(b,actor)!=0x46fbe8u||
           (Word(b,actor+0x5c)&1)||Word(b,actor+0x368)!=slot||
           !TrialGuestReadable(layer,0x28)||Word(b,layer+4)!=actor||Word(b,layer+0x24)!=slot)return false;
        ids[slot]=Word(b,actor+0x54);if(ids[slot]==0xffffffffu)return false;
    }
    return ids[0]!=ids[1];
}
bool Cinematic(const uint8_t* b,uint32_t camera){
    if(!TrialGuestReadable(camera,0x104)||Word(b,camera+0x54)==0xffffffffu||
       (Word(b,camera+0x5c)&1)||Word(b,camera+0xd0)!=0x478d20u)return false;
    const uint32_t type=Word(b,camera);
    // Authored bone-camera cutscenes, end-of-level camera, and enemy killcam.
    // Ordinary Camera also owns the first-person view and must not be admitted.
    return type==0x46fd70u||type==0x477da8u||type==0x477c58u;
}
uint32_t Camera(const uint8_t* b,uint32_t layer){
    const uint32_t stack=Word(b,layer+8);
    if(!TrialGuestReadable(stack,12)||!Word(b,stack+8))return 0;
    const uint32_t head=Word(b,stack+4);
    if(!TrialGuestReadable(head,8))return 0;
    const uint32_t node=Word(b,head+4);
    return TrialGuestReadable(node,12)?Word(b,node+8):0;
}
bool SetViewport(X86Context& c,uint8_t* b,uint32_t camera,uint32_t viewport){
    if(c.esp<0x10100||!TrialGuestReadable(c.esp-0x100,0x100))return false;
    const uint32_t scratch=c.esp-0x100;
    struct Restore {
        uint8_t* base;uint32_t address;std::array<uint8_t,256> bytes;unsigned csr;
        ~Restore(){std::memcpy(base+address,bytes.data(),bytes.size());_mm_setcsr(csr);}
    } restore{b,scratch,{},_mm_getcsr()};
    std::memcpy(restore.bytes.data(),b+scratch,restore.bytes.size());
    X86Context temporary=c;temporary.esp=scratch+0xc0;temporary.ecx=camera+0xd0;
    kernel::CallGuest(temporary,0x21e0a0,{viewport});
    return true;
}
}

unsigned TrialCoopPresentation(){
    return trial_coop::Active()?presentation.load(std::memory_order_relaxed):0;
}

void TrialCoopDamagePlayer(X86Context& c,const uint8_t* b){
    uint32_t ids[2];
    // DFE02 resolved the hit's weapon and asked whether its parent is a player.
    // Retail 1726C4/128F0 registers each starting actor through 4A780: the last
    // one owns manager+834. Admit either tracked player without rewriting that
    // shared manager or bypassing the original headshot/medal accounting.
    if(c.eax!=0xffffffffu||!PlayerIds(b,ids)||!TrialGuestReadable(c.ecx,0x838)||
       c.ecx!=Word(b,0x509a60)||
       !TrialGuestReadable(c.edi,0x60)||Word(b,c.edi+0x54)==0xffffffffu||
       (Word(b,c.edi+0x5c)&1))return;
    const uint32_t registered=Word(b,c.ecx+0x834),parent=Word(b,c.edi+0x58);
    if(registered!=ids[0]&&registered!=ids[1])return;
    for(unsigned slot=0;slot<2;++slot)if(parent==ids[slot]){c.eax=slot;return;}
}

void TrialCoopKillcamKiller(X86Context& c,const uint8_t* b){
    uint32_t ids[2];
    // Settings, target eligibility and distance/random/medal gates stay retail;
    // separate spin-latch hooks select this same killer's next-kill reward.
    if(!PlayerIds(b,ids)||(c.eax!=ids[0]&&c.eax!=ids[1])||
       (c.edi!=ids[0]&&c.edi!=ids[1])||
       !TrialGuestReadable(c.esi,0xbc)||Word(b,c.esi+0x54)==0xffffffffu||
       (Word(b,c.esi+0x5c)&1)||Word(b,c.esi+0xb8)!=c.edi)return;
    c.eax=c.edi;
}

TrialCoopCinematicView::TrialCoopCinematicView(X86Context& c,uint8_t* b,bool scene)
    :context_(c),base_(b),exceptions_(std::uncaught_exceptions()){
    if(scene)presentation.store(0,std::memory_order_relaxed);
    if(!b||!trial_coop::Available()||!trial_coop::Requested()||!trial_coop::Active()||
       !TrialGuestReadable(0x51c4dc,4))return;
    const uint32_t world=Word(b,0x51c4dc);
    if(c.ecx!=world||!TrialGuestReadable(world,8))return;
    const uint32_t state=Word(b,world+4);
    if(!TrialGuestReadable(state,12))return;
    const uint32_t first=Word(b,state+4);
    if(first>0xfffffff7u||Word(b,state+8)!=first+8||!TrialGuestReadable(first,8))return;
    for(unsigned slot=0;slot<2;++slot){
        const uint32_t owner=TrialCoopPlayer(slot),layer=Word(b,first+slot*4);
        if(!TrialGuestReadable(owner,0x36c)||Word(b,owner)!=0x46fbe8u||
           Word(b,owner+0x54)==0xffffffffu||(Word(b,owner+0x5c)&1)||
           Word(b,owner+0x368)!=slot||!TrialGuestReadable(layer,0x28)||
           Word(b,layer+4)!=owner||Word(b,layer+0x24)!=slot)return;
        owners_[slot]=owner;cameras_[slot]=Camera(b,layer);
        if(selected_<0&&Cinematic(b,cameras_[slot]))selected_=int(slot);
    }
    world_=world;
    if(selected_>=0){
        camera_=cameras_[selected_];camera_id_=Word(b,camera_+0x54);
        old_viewport_=Word(b,camera_+0xe8);
    }
    if(scene)presentation.store(selected_<0?1:2,std::memory_order_relaxed);
}

bool TrialCoopCinematicView::Select(unsigned slot,uint32_t world,uint32_t camera){
    if(!world_||selected_<0||slot>1||world!=world_||
       Word(base_,0x51c4dc)!=world_||camera!=cameras_[slot])return false;
    if(!installed_&&Cinematic(base_,camera_)&&Word(base_,camera_+0x54)==camera_id_){
        if(SetViewport(context_,base_,camera_,0)){
            full_viewport_=Word(base_,camera_+0xe8);installed_=true;
        }
    }
    return installed_&&slot!=unsigned(selected_);
}

bool TrialCoopCinematicView::Scene(){
    const bool skip=Select(context_.edi,context_.ebx,context_.esi);
    if(!skip&&installed_&&context_.esi==camera_&&context_.edi==unsigned(selected_)&&
       TrialGuestReadable(context_.esp+0x34,4)){
        // 215222 consumes this copied layer viewport immediately through 21E0A0.
        Put(base_,context_.esp+0x34,full_viewport_);
    }
    return skip;
}

bool TrialCoopCinematicView::Hud(){
    if(!TrialGuestReadable(context_.esp+0x10,4))return false;
    const uint32_t slot=Word(base_,context_.esp+0x10);
    if(slot>1||context_.ebp!=owners_[slot])return false;
    return Select(slot,context_.edi,context_.esi);
}

TrialCoopCinematicView::~TrialCoopCinematicView() noexcept(false){
    if(!installed_||Word(base_,0x51c4dc)!=world_||!Cinematic(base_,camera_)||
       Word(base_,camera_+0x54)!=camera_id_||Word(base_,camera_+0xe8)!=full_viewport_)return;
    if(std::uncaught_exceptions()==exceptions_&&SetViewport(context_,base_,camera_,old_viewport_))return;
    // 21E0A0's non-null restore is just pointer publication plus projection dirty.
    // Unwinding must not call back into guest code or leave a half-view camera enlarged.
    Put(base_,camera_+0xe8,old_viewport_);base_[camera_+0x100]=1;
}

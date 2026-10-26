#include "xbe_coop_effects.h"
#include "xbe_coop_guest.h"
#include "xbe_coop.h"
#include "cpu/x86_ops.h"
#include "kernel/kernel.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include <cstring>
#include <exception>
#include <xmmintrin.h>

namespace {
uint32_t Word(const uint8_t* b,uint32_t a){uint32_t v;std::memcpy(&v,b+a,4);return v;}
uint32_t World(const uint8_t* b){
    if(!b||!trial_coop::Available()||!trial_coop::Requested()||!trial_coop::Active()||
       !TrialGuestReadable(0x51c4dc,4))return 0;
    const uint32_t world=Word(b,0x51c4dc);
    if(!TrialGuestReadable(world,8))return 0;
    const uint32_t state=Word(b,world+4);
    if(!TrialGuestReadable(state,12))return 0;
    const uint32_t first=Word(b,state+4);
    if(first>0xfffffff7u||Word(b,state+8)!=first+8||!TrialGuestReadable(first,8))return 0;
    for(unsigned slot=0;slot<2;++slot){
        const uint32_t owner=TrialCoopPlayer(slot),layer=Word(b,first+slot*4);
        if(!TrialGuestReadable(owner,0x36c)||Word(b,owner)!=0x46fbe8u||
           Word(b,owner+0x54)==0xffffffffu||(Word(b,owner+0x5c)&1)||
           Word(b,owner+0x308)==0xffffffffu||Word(b,owner+0x368)!=slot||
           !TrialGuestReadable(layer,0x28)||Word(b,layer+4)!=owner||Word(b,layer+0x24)!=slot)return 0;
    }
    return world;
}
}

TrialCoopEffectsViewportScope::TrialCoopEffectsViewportScope(X86Context& c,uint8_t* b)
    :context_(c),base_(b),exceptions_(std::uncaught_exceptions()){
    world_=World(b);
    if(!world_||!TrialGuestReadable(0x3f4238,4))return;
    const uint32_t device=Word(b,0x3f4238);
    if(!TrialGuestReadable(device,0x1a18))return;
    target_=Word(b,device+0x1a04);depth_=Word(b,device+0x1a08);
    // Other offscreen passes keep their authored viewport, including nested ones.
    if(!target_||target_!=Word(b,device+0x1a14))return;
    std::memcpy(viewport_.data(),b+device+0xee0,24);
    if(viewport_[0]||!viewport_[2]||!viewport_[3]||viewport_[3]>UINT32_MAX/2||
       (viewport_[1]!=0&&viewport_[1]!=viewport_[3])||viewport_[4]||viewport_[5]!=0x3f800000u)return;
    device_=device;
}

TrialCoopEffectsViewportScope::~TrialCoopEffectsViewportScope() noexcept(false){
    if(!device_||std::uncaught_exceptions()!=exceptions_||
       !TrialGuestReadable(0x3f4238,4)||Word(base_,0x3f4238)!=device_||!TrialGuestReadable(device_,0x1a18)||
       Word(base_,device_+0x1a04)!=target_||Word(base_,device_+0x1a08)!=depth_||
       Word(base_,device_+0x1a14)!=target_)return;
    std::array<uint32_t,6> current{};
    std::memcpy(current.data(),base_+device_+0xee0,24);
    // Only the SDK's exact fullscreen reset after a restored scene target is repaired.
    if(current[0]||current[1]||current[2]!=viewport_[2]||current[3]!=viewport_[3]*2||
       current[4]!=viewport_[4]||current[5]!=viewport_[5]||context_.esp<0x10100||
       !TrialGuestReadable(context_.esp-0x100,0x100)||World(base_)!=world_)return;
    const uint32_t scratch=context_.esp-0x100;
    struct Restore {
        uint8_t* base;uint32_t address;std::array<uint8_t,256> bytes;unsigned csr;
        ~Restore(){std::memcpy(base+address,bytes.data(),bytes.size());_mm_setcsr(csr);}
    } restore{base_,scratch,{},_mm_getcsr()};
    std::memcpy(restore.bytes.data(),base_+scratch,restore.bytes.size());
    std::memcpy(base_+scratch,viewport_.data(),24);
    X86Context temporary=context_;
    temporary.esp=scratch+0xc0;
    kernel::CallGuest(temporary,0x3e3610,{scratch});
}

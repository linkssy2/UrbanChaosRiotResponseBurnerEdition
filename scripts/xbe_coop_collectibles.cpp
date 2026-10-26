#include "xbe_coop_collectibles.h"
#include "xbe_coop.h"
#include "xbe_coop_guest.h"
#include "xbe_coop_rewards.h"
#include "cpu/x86_ops.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include <cmath>
#include <cstring>
#include <xmmintrin.h>

namespace evidence_pickups {
thread_local TrialCoopEvidenceScope* current=nullptr;
uint32_t Word(const uint8_t* b,uint32_t p){uint32_t v;std::memcpy(&v,b+p,4);return v;}
float Real(const uint8_t* b,uint32_t p){float v;std::memcpy(&v,b+p,4);return v;}
bool Actor(const uint8_t* b,uint32_t p,unsigned slot){
    return TrialGuestReadable(p,0x890)&&Word(b,p)==0x46fbe8&&Word(b,p+0x368)==slot&&
        Word(b,p+0x54)!=0xffffffffu&&!(Word(b,p+0x5c)&1);
}
bool Alive(const uint8_t* b,uint32_t p){
    const float health=Real(b,p+0x9c);
    return std::isfinite(health)&&health>0&&Word(b,p+0x170)!=12;
}
bool Pickup(const uint8_t* b,uint32_t p){
    return TrialGuestReadable(p,0xdc)&&Word(b,p)==0x46f9a8&&
        Word(b,p+0x54)!=0xffffffffu&&!(Word(b,p+0x5c)&1);
}
bool Within(const uint8_t* b,uint32_t pickup,uint32_t actor){
    // 171E1B..171E75: preserve its scalar SSE order, Y+100 offset and
    // strict 150-unit sphere. The unchanged guest code checks it again.
    float p[3],a[3];
    for(unsigned i=0;i<3;++i){p[i]=Real(b,pickup+0x40+4*i);a[i]=Real(b,actor+0x40+4*i);if(!std::isfinite(p[i])||!std::isfinite(a[i]))return false;}
    const unsigned csr=_mm_getcsr();
    const __m128 x=_mm_sub_ss(_mm_set_ss(p[0]),_mm_set_ss(a[0]));
    const __m128 y=_mm_sub_ss(_mm_set_ss(p[1]),_mm_add_ss(_mm_set_ss(a[1]),_mm_set_ss(100.0f)));
    const __m128 z=_mm_sub_ss(_mm_set_ss(p[2]),_mm_set_ss(a[2]));
    const float square=_mm_cvtss_f32(_mm_add_ss(_mm_add_ss(_mm_mul_ss(z,z),_mm_mul_ss(y,y)),_mm_mul_ss(x,x)));
    _mm_setcsr(csr);
    return square<22500.0f;
}
}

TrialCoopEvidenceScope::TrialCoopEvidenceScope(const X86Context& c,const uint8_t* b)
    :previous_(evidence_pickups::current){
    using namespace evidence_pickups;
    current=this;
    if(b&&trial_coop::Active()&&Pickup(b,c.ecx)&&TrialGuestReadable(0x51c4dc,4)){
        pickup_=c.ecx;pickup_id_=Word(b,pickup_+0x54);world_=Word(b,0x51c4dc);
    }
}
TrialCoopEvidenceScope::~TrialCoopEvidenceScope(){evidence_pickups::current=previous_;}

void TrialCoopEvidenceScope::Select(X86Context& c,const uint8_t* b){
    using namespace evidence_pickups;
    if(!b||!trial_coop::Active()||!pickup_||c.esi!=pickup_||!Pickup(b,pickup_)||
       Word(b,pickup_+0x54)!=pickup_id_||Word(b,0x51c4dc)!=world_||!TrialGuestReadable(world_,8))return;
    const uint32_t players[2]={TrialCoopPlayer(0),TrialCoopPlayer(1)};
    // Keep the authored initialization/collected states and the original commit.
    // Only the resolved proximity candidate changes; no global player alias,
    // profile mask bits, pickup flags, sound or notification queue is rewritten.
    if(!b[pickup_+0xd4]||b[pickup_+0xd5])return;
    // A downed co-op body cannot collect a mask or steal the living partner's
    // notification. A retired partner does not block the remaining actor.
    for(unsigned i=0;i<2;++i)if(Actor(b,players[i],i)&&Alive(b,players[i])&&Within(b,pickup_,players[i])){
        slot_=i;actor_=players[i];actor_id_=Word(b,actor_+0x54);c.eax=actor_;return;
    }
    c.eax=0;
}

void TrialCoopEvidencePopup(X86Context& c,const uint8_t* b){
    using namespace evidence_pickups;
    const auto* s=current;
    if(!b||!s||!s->actor_||!trial_coop::Active()||!TrialGuestReadable(c.esp,24)||
       Word(b,c.esp)!=0x171f1f)return;
    // A recognized collector must never fall back to the other player's HUD.
    // Native 73066 already skips publication when its manager is null.
    c.ecx=0;
    if(!TrialGuestReadable(0x51c4dc,4)||Word(b,0x51c4dc)!=s->world_||
       !Pickup(b,s->pickup_)||Word(b,s->pickup_+0x54)!=s->pickup_id_||
       TrialCoopPlayer(s->slot_)!=s->actor_||!Actor(b,s->actor_,s->slot_)||Word(b,s->actor_+0x54)!=s->actor_id_)return;
    c.ecx=TrialCoopRewardManager(b,s->actor_);
}

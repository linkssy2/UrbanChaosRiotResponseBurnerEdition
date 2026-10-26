#include "xbe_coop_spin.h"
#include "xbe_coop.h"
#include "xbe_coop_guest.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include <cstring>

namespace {
struct Reward {uint32_t actor=0,id=0;uint8_t ready=0;};
Reward rewards[2];
uint32_t reward_world=0;
uint32_t Word(const uint8_t* b,uint32_t a){uint32_t v;std::memcpy(&v,b+a,4);return v;}
bool Scoped(){
    if(trial_coop::Available()&&trial_coop::Requested()&&trial_coop::Active())return true;
    TrialCoopSpinReset();return false;
}
bool Refresh(const uint8_t* b){
    const uint32_t world=b&&TrialGuestReadable(0x51c4dc,4)?Word(b,0x51c4dc):0;
    if(!TrialGuestReadable(world,8)){TrialCoopSpinReset();return false;}
    if(world!=reward_world){TrialCoopSpinReset();reward_world=world;}
    for(unsigned slot=0;slot<2;++slot){
        const uint32_t actor=TrialCoopPlayer(slot);
        auto& reward=rewards[slot];
        if(!TrialGuestReadable(actor,0x36c)||Word(b,actor)!=0x46fbe8u||
           Word(b,actor+0x54)==0xffffffffu||(Word(b,actor+0x5c)&1)||Word(b,actor+0x368)!=slot){
            reward={};continue;
        }
        const uint32_t id=Word(b,actor+0x54);
        if(reward.actor!=actor||reward.id!=id)reward={actor,id,0};
    }
    if(rewards[0].actor&&rewards[1].actor&&
       (rewards[0].actor==rewards[1].actor||rewards[0].id==rewards[1].id)){
        rewards[0]={};rewards[1]={};return false;
    }
    return true;
}
Reward* Actor(const uint8_t* b,uint32_t actor){
    if(!Refresh(b))return nullptr;
    for(auto& reward:rewards)if(reward.actor&&reward.actor==actor)return &reward;
    return nullptr;
}
Reward* Killer(const uint8_t* b,uint32_t controller,uint32_t killer){
    if(!Refresh(b)||!TrialGuestReadable(controller,0xbc)||
       Word(b,controller+0x54)==0xffffffffu||(Word(b,controller+0x5c)&1)||
       Word(b,controller+0xb8)!=killer)return nullptr;
    for(auto& reward:rewards)if(reward.actor&&reward.id==killer)return &reward;
    return nullptr;
}
}

void TrialCoopSpinReset(){rewards[0]={};rewards[1]={};reward_world=0;}
void TrialCoopSpinActorReset(const uint8_t*,uint32_t actor){
    // CC404 also runs during secondary construction; never reset its partner.
    for(auto& reward:rewards)if(reward.actor==actor)reward={};
}
void TrialCoopSpinWrite(uint8_t* b,uint32_t actor,uint8_t value){
    if(!Scoped()){b[0x51bdf4]=value;return;}
    if(auto* reward=Actor(b,actor))reward->ready=value;
}
uint8_t TrialCoopSpinHud(const uint8_t* b,uint32_t reticle){
    if(!Scoped())return b[0x51bdf4];
    if(!TrialGuestReadable(reticle,8))return 0;
    if(auto* reward=Actor(b,Word(b,reticle+4)))return reward->ready;
    return 0;
}
uint8_t TrialCoopSpinReward(const uint8_t* b,uint32_t controller,uint32_t killer){
    if(!Scoped())return b[0x51bdf4];
    if(auto* reward=Killer(b,controller,killer))return reward->ready;
    return 0;
}
void TrialCoopSpinConsume(uint8_t* b,uint32_t controller,uint32_t killer){
    if(!Scoped()){b[0x51bdf4]=0;return;}
    if(auto* reward=Killer(b,controller,killer))reward->ready=0;
}

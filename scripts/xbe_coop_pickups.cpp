#include "xbe_coop_pickups.h"
#include "xbe_coop.h"
#include "xbe_coop_guest.h"
#include "xbe_coop_cinematics.h"
#include <array>
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include "cpu/x86_ops.h"
#include <cmath>
#include <cstring>

bool TrialCoopPickupAllowed(const X86Context& c,const uint8_t* b){
    if(!b||!trial_coop::Active())return true;
    const auto word=[b](uint32_t p){uint32_t v;std::memcpy(&v,b+p,4);return v;};
    const uint32_t actor=c.ecx;
    if(!TrialGuestReadable(actor,0x890)||word(actor)!=0x46fbe8)return true;
    const uint32_t slot=word(actor+0x368);
    if(slot>1||TrialCoopPlayer(slot)!=actor)return true;
    float health;std::memcpy(&health,b+actor+0x9c,4);
    if(word(actor+0x54)==0xffffffffu||(word(actor+0x5c)&1)||
       word(actor+0x170)==12||!std::isfinite(health)||health<=0)return false;
    if(!TrialGuestReadable(c.esp,8)||!TrialGuestReadable(0x51c4dc,4))return false;
    const uint32_t world=word(0x51c4dc);
    if(!TrialGuestReadable(world,8))return false;
    const uint32_t state=word(world+4),id=word(c.esp+4);
    if(id==0xffffffffu||!TrialGuestReadable(state,0x203c))return false;
    // 207D70 keeps pending-deletion actors in this ID table until world cleanup.
    // 20A140 marks +5C immediately, before a second local player can collect.
    const uint32_t entry=state+0x3c+(id&0x3ff)*8;
    if(word(entry)!=id)return false;
    const uint32_t item=word(entry+4);
    return TrialGuestReadable(item,0x60)&&word(item+0x54)==id&&!(word(item+0x5c)&1);
}

namespace {
thread_local uint32_t pickup_actor=0;
struct NoticeOwner {uint32_t actor=0,id=0;unsigned slot=0;};
std::array<NoticeOwner,5> notice_owners;
uint32_t notice_world=0,notice_channel=0;
uint32_t NoticeWord(const uint8_t* b,uint32_t a){uint32_t v;std::memcpy(&v,b+a,4);return v;}
bool NoticeActor(const uint8_t* b,uint32_t actor,unsigned slot){
    return slot<2&&actor==TrialCoopPlayer(slot)&&TrialGuestReadable(actor,0x36c)&&
        NoticeWord(b,actor)==0x46fbe8&&NoticeWord(b,actor+0x368)==slot&&
        NoticeWord(b,actor+0x54)!=0xffffffffu&&!(NoticeWord(b,actor+0x5c)&1);
}
bool NoticeChannel(const uint8_t* b,uint32_t channel){
    if(!b||!trial_coop::Active()||!TrialGuestReadable(0x509a60,4)||!TrialGuestReadable(0x51c4dc,4))return false;
    const uint32_t controller=NoticeWord(b,0x509a60),world=NoticeWord(b,0x51c4dc);
    if(!TrialGuestReadable(world,8)||!TrialGuestReadable(controller,0x7c0)||channel!=controller+0x3ec||
       NoticeWord(b,channel)!=0x45055c||NoticeWord(b,channel+0x3c4)>5)return false;
    if(world!=notice_world||channel!=notice_channel){notice_owners={};notice_world=world;notice_channel=channel;}
    return true;
}
}
TrialCoopPickupScope::TrialCoopPickupScope(uint32_t actor,const uint8_t* b):previous_(pickup_actor){
    pickup_actor=0;
    if(!b||!trial_coop::Active()||!TrialGuestReadable(actor,0x36c))return;
    const unsigned slot=NoticeWord(b,actor+0x368);
    if(NoticeActor(b,actor,slot))pickup_actor=actor;
}
TrialCoopPickupScope::~TrialCoopPickupScope(){pickup_actor=previous_;}
void TrialCoopPickupNotice(const uint8_t* b,uint32_t channel,uint32_t row){
    if(!NoticeChannel(b,channel)||row<channel+4||(row-channel-4)%0xc0)return;
    const unsigned index=(row-channel-4)/0xc0,count=NoticeWord(b,channel+0x3c4);
    if(index>=count||count>notice_owners.size())return;
    // The original urgent path can overwrite row 0/1 and truncate the queue.
    for(unsigned i=count;i<notice_owners.size();++i)notice_owners[i]={};
    notice_owners[index]={};
    if(pickup_actor){
        const unsigned slot=NoticeWord(b,pickup_actor+0x368);
        if(NoticeActor(b,pickup_actor,slot))notice_owners[index]={pickup_actor,NoticeWord(b,pickup_actor+0x54),slot};
    }
}
void TrialCoopPickupNoticeExpired(const uint8_t* b,uint32_t channel){
    if(!NoticeChannel(b,channel))return;
    for(unsigned i=0;i+1<notice_owners.size();++i)notice_owners[i]=notice_owners[i+1];
    notice_owners.back()={};
}
void TrialCoopPickupNoticesReset(uint32_t controller){
    if(controller+0x3ec==notice_channel){notice_owners={};notice_channel=notice_world=0;}
}
TrialCoopPickupTextScope::TrialCoopPickupTextScope(const X86Context& c,uint8_t* b):base_(b){
    if(!NoticeChannel(b,c.ecx)||!NoticeWord(b,c.ecx+0x3c4))return;
    const auto owner=notice_owners.front();if(!owner.actor)return;
    // A personal pickup must not become a fullscreen cinematic or stale-player notice.
    if(!NoticeActor(b,owner.actor,owner.slot)||NoticeWord(b,owner.actor+0x54)!=owner.id||TrialCoopPresentation()!=1){skip_=true;return;}
    float y;std::memcpy(&y,b+c.ecx+0x3c8,4);
    if(!std::isfinite(y)||y<0||y>=240){skip_=true;return;}
    y_=c.ecx+0x3c8;saved_=NoticeWord(b,y_);y+=240.f*owner.slot;
    std::memcpy(b+y_,&y,4);
}
TrialCoopPickupTextScope::~TrialCoopPickupTextScope(){if(y_)std::memcpy(base_+y_,&saved_,4);}

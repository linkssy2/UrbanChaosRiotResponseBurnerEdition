#include "xbe_coop_aim_blur.h"
#include "xbe_coop.h"
#include "xbe_coop_guest.h"
#include "xbe_coop_cinematics.h"
#include "cpu/x86_ops.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <exception>
#include <limits>
#include <vector>
#include <xmmintrin.h>

namespace {
uint32_t Word(const uint8_t* b,uint32_t a){uint32_t v;std::memcpy(&v,b+a,4);return v;}
float Real(const uint8_t* b,uint32_t a){float v;std::memcpy(&v,b+a,4);return v;}
struct PreserveFp {unsigned value=_mm_getcsr();~PreserveFp(){_mm_setcsr(value);}};
struct Session {
    uint32_t world=0,manager=0;
    std::array<uint32_t,2> owners{},ids{},views{};
    bool operator==(const Session& other)const{
        return world==other.world&&manager==other.manager&&owners==other.owners&&ids==other.ids;
    }
};
struct Pulse {uint32_t node,owner,id;unsigned slot;};
struct Request {
    float start=std::numeric_limits<float>::max(),end=std::numeric_limits<float>::max(),amplitude=0;
    void Merge(float a,float z,float value){
        if(!std::isfinite(a)||!std::isfinite(z)||!std::isfinite(value)||value<=0)return;
        start=std::min(start,a);end=std::min(end,z);amplitude=std::max(amplitude,value);
    }
};
thread_local Session session;
thread_local std::vector<Pulse> pulses;
thread_local std::array<Request,2> owned;
thread_local std::array<Request,2> collecting;
thread_local Request shared,collecting_shared;
thread_local int active_view=-1;

Session Current(const uint8_t* b){
    Session result;
    if(!b||!trial_coop::Available()||!trial_coop::Requested()||!trial_coop::Active()||
       !TrialGuestReadable(0x51c4dc,4))return result;
    const auto world=Word(b,0x51c4dc);
    if(!TrialGuestReadable(world,0x938)||Word(b,world)!=0x450c48u)return result;
    const auto state=Word(b,world+4);
    if(!TrialGuestReadable(state,12))return result;
    const auto first=Word(b,state+4);
    if(first>0xfffffff7u||Word(b,state+8)!=first+8||!TrialGuestReadable(first,8))return result;
    for(unsigned slot=0;slot<2;++slot){
        const auto owner=TrialCoopPlayer(slot),layer=Word(b,first+slot*4);
        if(!TrialGuestReadable(owner,0x36c)||Word(b,owner)!=0x46fbe8u||
           Word(b,owner+0x54)==0xffffffffu||(Word(b,owner+0x5c)&1)||
           Word(b,owner+0x368)!=slot||!TrialGuestReadable(layer,0x28)||
           Word(b,layer+4)!=owner||Word(b,layer+0x24)!=slot)return {};
        result.owners[slot]=owner;result.ids[slot]=Word(b,owner+0x54);result.views[slot]=Word(b,layer+0x20);
    }
    const auto manager=Word(b,world+0x934);
    if(result.owners[0]==result.owners[1]||!TrialGuestReadable(manager,16))return {};
    result.world=world;result.manager=manager;return result;
}
bool Refresh(const uint8_t* b){
    const auto current=Current(b);
    if(!(current==session)){pulses.clear();owned={};collecting={};shared={};collecting_shared={};}
    session=current;
    return session.world!=0;
}
bool Split(const uint8_t* b){
    return session.world&&TrialCoopPresentation()==1&&(Word(b,session.world+0x10)&0x880u)==0x80u;
}
}

TrialCoopAimBlurRequestScope::TrialCoopAimBlurRequestScope(const X86Context& c,uint8_t* b)
    :base_(b),exceptions_(std::uncaught_exceptions()){
    if(!Refresh(b)||c.ecx!=session.manager||!TrialGuestReadable(c.esp,4))return;
    const auto caller=Word(b,c.esp);
    if(caller!=0xbef11u&&caller!=0xbef59u)return;
    for(unsigned slot=0;slot<2;++slot)if(c.esi==session.owners[slot]){
        const auto list=Word(b,c.ecx+12);
        if(!TrialGuestReadable(list,12))return;
        const auto sentinel=Word(b,list+4);
        if(!TrialGuestReadable(sentinel,8))return;
        manager_=c.ecx;sentinel_=sentinel;previous_=Word(b,sentinel+4);
        owner_=c.esi;id_=session.ids[slot];slot_=slot;return;
    }
}
TrialCoopAimBlurRequestScope::~TrialCoopAimBlurRequestScope(){
    if(!manager_||std::uncaught_exceptions()!=exceptions_||!Refresh(base_)||
       manager_!=session.manager||owner_!=session.owners[slot_]||id_!=session.ids[slot_]||
       !TrialGuestReadable(sentinel_,8))return;
    const auto node=Word(base_,sentinel_+4);
    if(node==previous_||node==sentinel_||!TrialGuestReadable(node,36)||Word(base_,node)!=sentinel_)return;
    pulses.erase(std::remove_if(pulses.begin(),pulses.end(),[node](const Pulse& p){return p.node==node;}),pulses.end());
    pulses.push_back({node,owner_,id_,slot_});
}

void TrialCoopAimBlurBeginUpdate(const X86Context& c,const uint8_t* b){
    if(Refresh(b)&&c.ecx==session.manager){
        collecting={};collecting_shared={Real(b,c.ecx+4),Real(b,c.ecx+8),Real(b,c.ecx)};
    }
}
void TrialCoopAimBlurNodeBegin(const X86Context& c,uint8_t* b){
    if(!Refresh(b)||c.edi!=session.manager||!TrialGuestReadable(c.ecx,36))return;
    const auto found=std::find_if(pulses.begin(),pulses.end(),[&](const Pulse& p){return p.node==c.ecx;});
    PreserveFp preserve;
    auto& request=found!=pulses.end()&&found->owner==session.owners[found->slot]&&
        found->id==session.ids[found->slot]?collecting[found->slot]:collecting_shared;
    request.Merge(Real(b,c.ecx+16),Real(b,c.ecx+20),Real(b,c.ecx+8));
}
void TrialCoopAimBlurPublish(const X86Context& c,const uint8_t* b){
    // Retain the original aggregate and publication unchanged. Ordinary owned
    // views temporarily select their own pulses; shared cameras use retail data.
    if(Refresh(b)&&c.edi==session.manager){owned=collecting;shared=collecting_shared;}
}
void TrialCoopAimBlurForgetNode(const X86Context& c){
    pulses.erase(std::remove_if(pulses.begin(),pulses.end(),[&](const Pulse& p){return p.node==c.ecx;}),pulses.end());
}
void TrialCoopAimBlurClear(const X86Context& c){
    if(c.ecx==session.manager){pulses.clear();owned={};collecting={};shared={};collecting_shared={};}
}

TrialCoopAimBlurViewScope::TrialCoopAimBlurViewScope(const X86Context& c,uint8_t* b)
    :base_(b),previous_(active_view){
    active_view=-1;
    if(!Refresh(b)||!Split(b)||!TrialGuestReadable(c.ecx,0x24)||Word(b,c.ecx)!=0x456984u)return;
    // 97050 copies the originating 9E670 camera descriptor into queue+18.
    const auto camera=Word(b,c.ecx+0x18);
    if(!TrialGuestReadable(camera,0x12c)||Word(b,camera)!=0x480a6cu||
       Word(b,camera+0x128)!=0xffffffefu)return;
    const auto view=Word(b,camera+4);
    if(!view||session.views[0]==session.views[1])return;
    for(unsigned slot=0;slot<2;++slot)if(view==session.views[slot]){active_view=slot_=int(slot);return;}
}
void TrialCoopAimBlurViewScope::Apply(const X86Context& c){
    if(slot_<0||descriptor_||!Refresh(base_)||!Split(base_)||!TrialGuestReadable(c.eax,0x6c))return;
    const uint32_t descriptor=c.eax+8,table=Word(base_,descriptor);
    if(!TrialGuestReadable(table,0x60)||Word(base_,table+0x48)!=0x94760u||
       Word(base_,table+0x4c)!=0x94770u||Word(base_,table+0x50)!=0x947c0u||
       Word(base_,table+0x54)!=0x94780u||Word(base_,table+0x58)!=0x947a0u||
       Word(base_,table+0x5c)!=0x947b0u)return;
    PreserveFp preserve;
    if(owned[0].amplitude<=0&&owned[1].amplitude<=0)return;
    auto request=shared;
    const auto& local=owned[unsigned(slot_)];
    request.Merge(local.start,local.end,local.amplitude);
    descriptor_=descriptor;table_=table;enabled_=base_[descriptor+0x55];
    std::memcpy(parameters_.data(),base_+descriptor+0x58,12);
    base_[descriptor+0x55]=request.amplitude>0;
    const float parameters[]{request.start,request.end,request.amplitude};
    std::memcpy(base_+descriptor+0x58,parameters,12);
}
TrialCoopAimBlurViewScope::~TrialCoopAimBlurViewScope(){
    if(descriptor_&&TrialGuestReadable(descriptor_,0x64)&&Word(base_,descriptor_)==table_){
        base_[descriptor_+0x55]=enabled_;std::memcpy(base_+descriptor_+0x58,parameters_.data(),12);
    }
    active_view=previous_;
}
int TrialCoopAimBlurActiveView(){return active_view;}

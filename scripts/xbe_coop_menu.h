#pragma once
#include "xbe_coop.h"
#include "xbe_frontend.h"

namespace trial_coop {
inline int CampaignRow(const TrialFrontendFrame& f){
    if(!TrialFrontendSupported(f)||f.source.kind!=UC_HF_LIST||f.list_kind!=1)return -1;
    for(unsigned i=0;i<f.source.count;++i){const auto& r=f.source.rows[i];
        if(r.enabled&&(r.actions&UC_HF_ROW_ACTIVATE)&&r.callback==0x271730&&
           !std::strcmp(r.label,"NewFrontend-Singleplayer-1"))return int(i);
    }
    return -1;
}
inline const char* CharacterName(unsigned value){
    constexpr const char* names[]={"T-Zero","Avenger","Banger","Magnum","Prophet"};
    return value<5?names[value]:"";
}
// A pending mode is created only by a confirmed, still-live campaign row.
// The guest continues to own level selection, sounds, difficulty and deployment.
struct MenuState {
    enum Phase {Idle,Choosing,Entering,Campaign,Leaving} phase=Idle;
    uint64_t epoch=0;uint32_t owner=0,screen=0,row=0;unsigned index=0;
    unsigned choice[2]{};bool ready[2]{};
    bool Matches(const TrialFrontendFrame& f)const{
        const int i=CampaignRow(f);
        return i>=0&&unsigned(i)==index&&f.source.epoch==epoch&&f.source.owner==owner&&
            f.source.screen==screen&&f.source.rows[index].row==row;
    }
    bool Begin(const TrialFrontendFrame& f){
        const int i=CampaignRow(f);
        if(!Available()||Requested()||Active()||i<0)return false;
        *this={};phase=Choosing;epoch=f.source.epoch;owner=f.source.owner;screen=f.source.screen;
        index=unsigned(i);row=f.source.rows[index].row;return true;
    }
    void Choose(unsigned player,unsigned value){
        if(phase==Choosing&&player<2&&value<5){choice[player]=value;ready[player]=false;}
    }
    void Move(unsigned player,int direction){
        if(player<2)Choose(player,(choice[player]+(direction<0?4:1))%5);
    }
    void Confirm(unsigned player){if(phase==Choosing&&player<2)ready[player]=!ready[player];}
    bool CanContinue(bool connected)const{return phase==Choosing&&ready[0]&&ready[1]&&connected;}
    bool Submit(const TrialFrontendFrame& f,bool connected,UcHybridFrontendAction& action){
        if(!CanContinue(connected)||!Matches(f)||!Request(Character(choice[0]),Character(choice[1])))return false;
        action={epoch,screen,owner,row,index,UC_HF_ACTIVATE};phase=Entering;return true;
    }
    void Cancel(){
        const bool retiring=Active();
        if(!retiring)Reset();
        *this={};if(retiring)phase=Leaving;
    }
    // Transient NONE/fade snapshots revoke actions, but do not cancel deployment.
    // Returning to the main menu ends the pending co-op campaign flow.
    void Observe(const TrialFrontendFrame& f){
        // Original world teardown may finish after the first main-menu frame.
        // Keep this cleanup pending without resetting live actors underneath it.
        if(phase==Leaving&&!Active()){Reset();*this={};}
        if(!TrialFrontendSupported(f))return;
        if(phase==Choosing&&!Matches(f)){Cancel();return;}
        if(phase==Entering&&f.source.kind==UC_HF_LEVELS)phase=Campaign;
        else if((phase==Campaign||(phase==Entering&&!Matches(f)))&&CampaignRow(f)>=0)Cancel();
    }
};
struct MenuPad {
    unsigned previous=0;bool released=false,connected=false;
    unsigned Sample(bool present,unsigned mask){
        if(!present){*this={};return 0;}
        connected=true;
        const unsigned events=released?(mask&~previous):0;
        if(!mask)released=true;
        previous=mask;return events;
    }
};
}

#pragma once
#include <cstdint>
#include <filesystem>
struct X86Context;

// Input/UI threads never inspect guest objects. The runtime publishes a small
// snapshot and services each rising edge on the owning player's update.
struct TrialHealthPackStatus {
    unsigned count=0,capacity=3;
    bool visible=false,can_revive=false;
    float bar_extent=120.f;
    float opacity=1.f;
    bool can_heal_partner=false;
};
void TrialHealthPackInput(unsigned slot,bool down,bool allowed);
enum class TrialHealthPackUse : unsigned { None, Context, Self, Partner };
void TrialHealthPackRequest(unsigned slot,TrialHealthPackUse use);
struct TrialHealthPackGesture {
    bool blocked=true,held=false,used=false,revive=false,tap=false;
    uint64_t pressed=0,released=0;
    void Reset(){*this={};}
    TrialHealthPackUse Update(bool down,bool allowed,bool nearby,uint64_t now){
        if(!allowed){Reset();return TrialHealthPackUse::None;}
        if(blocked){if(!down)blocked=false;return TrialHealthPackUse::None;}
        if(!down){
            if(held){tap=!used&&now-pressed<300;released=now;}
            held=used=revive=false;return TrialHealthPackUse::None;
        }
        if(!held){
            held=true;used=false;revive=nearby;pressed=now;
            if(tap&&now-released<=300){tap=false;used=true;return TrialHealthPackUse::Self;}
            tap=false;
        }
        // A cancelled revive stays consumed until release; it never becomes self-heal.
        if(revive&&!nearby){used=true;revive=false;}
        if(revive&&!used&&now-pressed>=650){used=true;return TrialHealthPackUse::Partner;}
        return TrialHealthPackUse::None;
    }
    bool Consumed()const{return held&&(revive||used);}
};
TrialHealthPackStatus TrialHealthPackSnapshot(unsigned slot);
void TrialHealthPackStorage(const std::filesystem::path&);
void TrialHealthPackCheckpointSaved(const X86Context&,const uint8_t*);
void TrialHealthPackCheckpointLoaded(const X86Context&,const uint8_t*);
void TrialHealthPackUpdate(X86Context&,uint8_t*);
// Partner injections reuse the native weapon/animation state. Only their Heal
// contact event is redirected; ordinary self-healing keeps the retail path.
bool TrialHealthPackPartnerAnimation(const uint8_t*,uint32_t owner);
void TrialHealthPackInjectionClip(X86Context&,const uint8_t*);
bool TrialHealthPackInjectionContact(X86Context&,uint8_t*);
void TrialHealthPackCreated(const X86Context&,const uint8_t*);
void TrialHealthPackDestroyed(const X86Context&,const uint8_t*);
struct TrialHealthPackMedicScope {
    TrialHealthPackMedicScope(X86Context&,uint8_t*,bool choosing=false);
    ~TrialHealthPackMedicScope();
    uint32_t previous=0;
};
struct TrialHealthPackDeliveryScope {
    TrialHealthPackDeliveryScope(X86Context&,uint8_t*);
    ~TrialHealthPackDeliveryScope();
    bool intercept=false,accepted=false,received=false;
    uint8_t* base=nullptr;
    uint32_t owner=0,stock=0,before=0,previous=0,root=0;
    int exceptions=0;
};
void TrialHealthPackMedicPlayer(X86Context&,const uint8_t*,bool pointer);
bool TrialHealthPackMedicNeeds(const uint8_t*,bool original);
// Read-only order queries never select a different recipient. A normal collect
// command claims its initiating player before the native asynchronous handoff.
bool TrialHealthPackMedicPrompt(const uint8_t*,uint32_t owner,uint32_t client,uint32_t target,bool offer);
bool TrialHealthPackMedicOfferOwner(const uint8_t*,uint32_t owner,uint32_t client,uint32_t target=0);
bool TrialHealthPackMedicClaim(const uint8_t*,uint32_t owner,uint32_t client,uint32_t target);
void TrialHealthPackMedicEnd(const X86Context&,const uint8_t*);
bool TrialHealthPackMedicReceipt(const X86Context&,const uint8_t*);
bool TrialHealthPackMedicStockBlocked(const X86Context&,const uint8_t*);

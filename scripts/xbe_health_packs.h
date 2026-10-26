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
};
void TrialHealthPackInput(unsigned slot,bool down,bool allowed);
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
bool TrialHealthPackMedicReceipt(const X86Context&,const uint8_t*);
bool TrialHealthPackMedicStockBlocked(const X86Context&,const uint8_t*);

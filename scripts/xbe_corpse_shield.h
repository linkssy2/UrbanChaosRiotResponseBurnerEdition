#pragma once
#include <cstdint>
struct X86Context;
// Native campaign load retires every prior owner and model before new entities.
void TrialCorpseShieldReset();
void TrialCorpseShieldResourceLoaded(const X86Context&,const uint8_t*);
void TrialCorpseShieldBeforePhysics(const X86Context&,uint8_t*,float dt);
void TrialCorpseShieldAfterPhysics(const X86Context&,uint8_t*,float dt);
void TrialCorpseShieldRelease(const X86Context&,uint8_t*);
void TrialCorpseShieldPose(uint8_t*,uint32_t render);
bool TrialCorpseShieldActive(const uint8_t*,uint32_t owner);
// Owner casts pass through only their own held body; other rays still hit it.
bool TrialCorpseShieldIgnoreQuery(const uint8_t*,uint32_t collidable,uint32_t query_filter);
struct TrialCorpseShieldScope {
    TrialCorpseShieldScope(X86Context&,uint8_t*);
    ~TrialCorpseShieldScope();
    void Select(X86Context&);
private:
    X86Context* context_=nullptr;
    uint8_t* base_=nullptr;
    uint32_t owner_=0,entity_=0,id_=0,world_=0,storage_=0;
    unsigned kind_=0,slot_=0;
    int exceptions_=0;
    bool selected_=false;
    const char* fallback_=nullptr;
};

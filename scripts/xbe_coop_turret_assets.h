#pragma once
#include <cstdint>
struct X86Context;
// Reset at campaign preload, before entities/models are constructed.
void TrialCoopTurretAssetsReset();
bool TrialCoopTurretAssetReady(const uint8_t* base,uint32_t helicopter);
void TrialCoopTurretResourceLoaded(const X86Context&,const uint8_t*);
struct TrialCoopTurretAssetScope {
    TrialCoopTurretAssetScope(X86Context&,uint8_t*);
    ~TrialCoopTurretAssetScope();
    void Select(X86Context&);
private:
    X86Context* context_=nullptr;
    uint8_t* base_=nullptr;
    uint32_t owner_=0,id_=0,world_=0,storage_=0;
    int exceptions_=0;
    bool selected_=false;
};

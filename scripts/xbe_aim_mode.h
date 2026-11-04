#pragma once
#include <cstdint>
struct X86Context;

// Host input publishes only availability and the optional magnification key.
// The guest still reads the player's ordinary remapped Zoom action itself.
void TrialAimInput(unsigned slot,bool cycle_down,bool allowed);
void TrialAimUpdate(const X86Context&,uint8_t*);
double TrialAimZoomScale(const uint8_t*,uint32_t owner);
void TrialAimScaleStick(const X86Context&,uint8_t*);
struct TrialAimModeScope {
    TrialAimModeScope(const X86Context&,uint8_t*);
    ~TrialAimModeScope();
    unsigned QueryMode() const {return hold?0u:1u;}
    void Filter(X86Context&);
    void Cycle();
    uint8_t* base=nullptr;
    uint32_t owner=0,id=0,world=0,level=0;
    int slot=-1;
    bool hold=false,queried=false,emitted=false,cycled=false,alternate=false;
};

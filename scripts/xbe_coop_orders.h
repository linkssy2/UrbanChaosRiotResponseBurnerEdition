#pragma once
#include <cstdint>
struct X86Context;

struct TrialCoopOrdersScope {
    uint32_t previous;
    TrialCoopOrdersScope(const X86Context&,uint8_t*);
    ~TrialCoopOrdersScope();
};
struct TrialCoopOrdersHudScope {
    uint32_t previous;
    TrialCoopOrdersHudScope(const X86Context&,const uint8_t*);
    ~TrialCoopOrdersHudScope();
};
struct TrialCoopHelpScope {
    uint32_t previous,previous_selecting;
    TrialCoopHelpScope(const X86Context&,const uint8_t*);
    ~TrialCoopHelpScope();
};
uint32_t TrialCoopHelpHud(const uint8_t*,uint32_t original);
uint32_t TrialCoopOrdersHud(const uint8_t*,uint32_t original);
void TrialCoopOrdersProperty(X86Context&,const uint8_t*);
void TrialCoopOrdersControl(X86Context&,const uint8_t*);
void TrialCoopOrdersPlayer(X86Context&,const uint8_t*);
bool TrialCoopOrdersRegister(X86Context&,uint8_t*);
void TrialCoopOrdersEvent(X86Context&,uint8_t*,uint32_t callback);
bool TrialCoopOrdersChoose(X86Context&,uint8_t*);
bool TrialCoopMedicPrompt(const X86Context&,const uint8_t*,bool offer=false,bool decline=false);

bool TrialPlayerHudVisible(const X86Context&,const uint8_t*);

struct TrialHiddenHudScope {
    uint8_t* base;uint32_t root=0,saved[3]{};bool draw=true;
    TrialHiddenHudScope(const X86Context&,uint8_t*);
    ~TrialHiddenHudScope();
};

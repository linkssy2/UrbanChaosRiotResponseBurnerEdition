#pragma once
#include <atomic>

namespace trial_player_actions {
struct Input {
    bool walk_down=false,hud_down=false,blocked=true,toggled=false;
    unsigned mode=0;
};
inline Input inputs[2];
inline std::atomic<bool> walking[2]{},hidden[2]{},reset[2]{};
inline void Reset(unsigned slot){
    if(slot>1)return;
    walking[slot]=false;hidden[slot]=false;reset[slot]=true;
}
inline void Cancel(unsigned slot){
    if(slot>1)return;
    inputs[slot].blocked=true;inputs[slot].toggled=false;walking[slot]=false;
}
inline void Update(unsigned slot,bool walk,bool hud,unsigned mode,bool allowed){
    if(slot>1)return;
    auto& state=inputs[slot];
    if(reset[slot].exchange(false))Cancel(slot);
    if(!allowed||mode!=state.mode)Cancel(slot);
    state.mode=mode;
    if(state.blocked){if(allowed&&!walk&&!hud)state.blocked=false;}
    else{
        if(mode&&walk&&!state.walk_down)state.toggled=!state.toggled;
        walking[slot]=mode?state.toggled:walk;
        if(hud&&!state.hud_down)hidden[slot]=!hidden[slot].load();
    }
    state.walk_down=walk;state.hud_down=hud;
}
inline bool HudVisible(unsigned slot){return slot>1||!hidden[slot].load();}
inline float MovementScale(unsigned slot){return slot<2&&walking[slot].load()?0.35f:1.f;}
}

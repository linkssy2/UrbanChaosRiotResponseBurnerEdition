#pragma once
#include <atomic>
#include <cstdint>
#include "xbe_coop.h"

namespace trial_coop {
// Only the guest frontend writes the edge/acceptance state. The window and
// rendering threads read the owner after the original pause state opens.
struct PauseState {
    std::atomic<int> owner{-1};
    bool held[2]{};
    int candidate=-1,accepted=-1;
    void Begin(){if(owner.load()<0)candidate=accepted=-1;}
    int Sample(bool first,bool second){
        const bool pressed[2]={!held[0]&&first,!held[1]&&second};
        held[0]=first;held[1]=second;
        candidate=pressed[0]?0:pressed[1]?1:-1;
        return candidate;
    }
    void Consume(bool result){if(result&&owner.load()<0)accepted=candidate;}
    void Enter(){int empty=-1;owner.compare_exchange_strong(empty,accepted==1?1:0);}
    void Reset(){owner=-1;held[0]=held[1]=true;candidate=accepted=-1;}
};
inline PauseState pause_state;
inline int PauseOwner(){return Active()?pause_state.owner.load():-1;}
inline void ResetPauseOwner(){pause_state.Reset();}
inline bool PauseAllowsPlayer(unsigned player){const int owner=PauseOwner();return owner<0||int(player)==owner;}
inline bool PauseBlocksKeyboard(){return PauseOwner()==1;}
// Win32 key/mouse/raw-input messages only; activation, size, paint and close
// keep their normal handling even while the controller owns a paused menu.
inline bool PauseBlocksMessage(uint32_t message){
    return PauseBlocksKeyboard()&&((message>=0x100&&message<=0x109)||
        (message>=0x200&&message<=0x20e)||message==0xff||message==0x2a3||message==0x2a2);
}
}

#pragma once
#include <array>
#include <cstdint>
struct X86Context;

// Published by the guest render thread; the host overlay never follows guest pointers.
// 0: no admitted co-op frame, 1: two gameplay views, 2: shared cinematic.
unsigned TrialCoopPresentation();

// These caller-local corrections retain the original campaign rewards and gates.
void TrialCoopDamagePlayer(X86Context&,const uint8_t*); // DFE07, original 4A6E0 result
void TrialCoopKillcamKiller(X86Context&,const uint8_t*); // 387B7, original 338B0 result

class TrialCoopCinematicView {
public:
    TrialCoopCinematicView(X86Context&,uint8_t*,bool scene);
    ~TrialCoopCinematicView() noexcept(false);
    bool Scene(); // original 215222; true skips this layer
    bool Hud();   // original 2153EB; true skips this layer
private:
    bool Select(unsigned,uint32_t,uint32_t);
    X86Context& context_;
    uint8_t* base_;
    uint32_t world_=0,camera_=0,camera_id_=0,old_viewport_=0,full_viewport_=0;
    std::array<uint32_t,2> owners_{},cameras_{};
    int selected_=-1,exceptions_=0;
    bool installed_=false;
};

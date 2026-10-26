#pragma once
#include <array>
#include <cstdint>
struct X86Context;

void TrialCoopTurretsReset();
// Guest-thread only. The record is tied to this actor, vehicle and current world.
uint32_t TrialCoopTurretMount(const uint8_t*,uint32_t owner);
void TrialCoopTurretRecipient(X86Context&,const uint8_t*);
bool TrialCoopTurretEvent(X86Context&,uint8_t*);
void TrialCoopTurretResume(const X86Context&,uint8_t*);
bool TrialCoopTurretKeepDowned(const X86Context&,const uint8_t*);
void TrialCoopTurretAim(const X86Context&,const uint8_t*);
void TrialCoopTurretCameraSlot(const X86Context&,uint8_t*);

class TrialCoopTurretScope {
public:
    // The vehicle state machine, its mounted update, and its exit callback.
    enum class Phase {State,Update,Exit};
    TrialCoopTurretScope(X86Context&,uint8_t*,Phase);
    ~TrialCoopTurretScope();
    bool Valid(const uint8_t*) const;
    uint32_t world=0,vehicle=0,vehicle_id=0;
    std::array<uint32_t,2> actors{},ids{};
    float pitch=0,yaw=0;
    bool aim=false;
    Phase phase;
private:
    TrialCoopTurretScope* previous_=nullptr;
};

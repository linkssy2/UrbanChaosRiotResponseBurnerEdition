#pragma once
#include <cstdint>
struct X86Context;
void TrialCoopDeathReset();
// Gameplay-thread tick; the UI reads only the published seconds snapshot.
void TrialCoopDeathUpdate(X86Context&,uint8_t*);
unsigned TrialCoopReviveSeconds(unsigned slot);

// Only the two admitted local actors participate; original NPC/single-player
// death and enemy kill cameras remain on their original paths.
void TrialCoopDeathBody(X86Context&,uint8_t*);
unsigned TrialCoopDeathDisposition(const uint8_t*,uint32_t owner);
bool TrialCoopImmediateRagdoll(const uint8_t*,uint32_t body);
bool TrialCoopKeepDownedBody(const uint8_t*,uint32_t body);
bool TrialCoopPlayerDowned(const uint8_t*,uint32_t owner);
bool TrialCoopDownedPosition(const uint8_t*,uint32_t owner,float (&position)[3]);
uint32_t TrialCoopNearbyDownedPlayer(const uint8_t*,uint32_t healer);
bool TrialCoopRevivePlayer(X86Context&,uint8_t*,uint32_t owner);

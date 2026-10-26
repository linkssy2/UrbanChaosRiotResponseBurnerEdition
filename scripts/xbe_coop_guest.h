#pragma once
#include <cstdint>
struct X86Context;
enum class TrialCoopTraceStage:unsigned {Update,CameraEnter,CameraExit,UnreadyView};
#ifdef UC_BURNER_RELEASE
inline void TrialCoopTraceStartup(const X86Context&,const uint8_t*,uint32_t,TrialCoopTraceStage){}
#else
void TrialCoopTraceStartup(const X86Context&,const uint8_t*,uint32_t owner,TrialCoopTraceStage);
#endif

// Experimental local-player admission. Original factories own all guest objects.
void TrialCoopMainplayerCreated(const X86Context&,uint8_t*);
void TrialCoopPlayerSlot(const X86Context&,uint8_t*);
void TrialCoopPlayerLayer(const X86Context&,uint8_t*);
void TrialCoopPlayerAlias(const X86Context&,uint8_t*);
void TrialCoopPlayerStarted(X86Context&,uint8_t*,uint32_t owner);
void TrialCoopPlayerDestroyed(const X86Context&,uint8_t*);
void TrialCoopAudioListenerCamera(X86Context&,const uint8_t*,uint32_t view_count);
void TrialCoopRestoreViewViewport(X86Context&,uint8_t*);
void TrialCoopDeathPlayer(X86Context&,const uint8_t*,uint32_t sequence);
bool TrialCoopDeathSequenceAllowed(const X86Context&,const uint8_t*);
void TrialCoopDeathSequenceAdmitted(const X86Context&,const uint8_t*);
void TrialCoopDeathRumble(const X86Context&,uint8_t*,uint32_t sequence);
uint32_t TrialCoopPlayer(unsigned slot);

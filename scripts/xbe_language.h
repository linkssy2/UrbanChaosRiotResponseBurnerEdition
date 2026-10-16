#pragma once
#include <cstdint>
#include <string>

struct X86Context;
extern uint32_t TrialLanguage;
namespace trial_language {
inline constexpr uint32_t Count=4;
inline constexpr const char* Names[Count]={"English","Fran\xc3\xa7" "ais","Italiano","Deutsch"};
uint32_t Snapshot();
void Initialize(); // after settings load, before original title startup
bool Apply(uint32_t language,std::string& message);
bool PendingRestart();
uint32_t Active();
uint32_t XboxLanguage();
}
// Only the original configuration constructor's first language initialization.
void TrialLanguageStartup(const X86Context& ctx,uint8_t* base);

#pragma once
#include <atomic>
#include <string>

extern bool TrialStoredHealthPacks;
namespace trial_health {
inline std::atomic<bool> enabled{true};
void Initialize();
bool Apply(bool value,std::string& message);
}

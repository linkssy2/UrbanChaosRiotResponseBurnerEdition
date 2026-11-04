#include "xbe_health_settings.h"
#include "app/app.h"
#include <exception>

bool TrialStoredHealthPacks=true;
namespace trial_health {
void Initialize(){enabled=TrialStoredHealthPacks;}
bool Apply(bool value,std::string& message){
    std::lock_guard lock(app::UiLock());
    const bool before=TrialStoredHealthPacks;TrialStoredHealthPacks=value;
    try {app::Save();}
    catch(const std::exception&){TrialStoredHealthPacks=before;message="Could not save gameplay settings.";return false;}
    enabled=value;message.clear();return true;
}
}

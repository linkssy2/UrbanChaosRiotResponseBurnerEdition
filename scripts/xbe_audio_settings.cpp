#include "xbe_audio_output.h"
#include "app/app.h"
#include <exception>

uint32_t TrialSoundOutput=trial_audio::Stereo;
namespace trial_audio {
void Initialize(){TrialSoundOutput=ValidMode(TrialSoundOutput);requested=TrialSoundOutput;}
bool Apply(uint32_t mode,std::string& message){
    if(mode>Surround71){message="Choose Stereo, 5.1 Surround or 7.1 Surround.";return false;}
    std::lock_guard lock(app::UiLock());
    const auto before=TrialSoundOutput;TrialSoundOutput=mode;
    try {app::Save();}
    catch(const std::exception&){TrialSoundOutput=before;message="Could not save audio settings.";return false;}
    requested=mode;message.clear();return true;
}
}

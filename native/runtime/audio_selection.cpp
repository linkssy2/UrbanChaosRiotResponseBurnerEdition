#include "audio.h"
extern "C" {
#include "../audio_event_selection.h"
}
#include <cstring>
#include <stdexcept>
int32_t NativeAudioBankAssets::select_event(const char*name,uint32_t flags,UCAudioRandom&random){
    int32_t index=bank.find(name);if(index<0)return -1;
    if(event_state.size()!=bank.events().size())throw std::runtime_error("Audio event state missing");
    auto&e=event_state.at(index);uint32_t selection;std::memcpy(&selection,e.data()+8,4);
    void*payload=nullptr;size_t size=0;
    if(selection!=UINT32_MAX){
        if(selection>=selectors.size())throw std::runtime_error("Audio selector state missing");
        auto&s=selectors[selection];payload=s.data();size=s.size();
    }
    if(uc_audio_event_select(e.data(),flags,selection,payload,size,&random)!=UC_AUDIO_OK)
        throw std::runtime_error("Audio event selection rejected");
    uint16_t sound=uint16_t(e[2]|uint16_t(e[3])<<8);
    if(sound!=UINT16_MAX&&sound>=bank.sounds().size())throw std::runtime_error("Audio selected sound invalid");
    return sound;
}

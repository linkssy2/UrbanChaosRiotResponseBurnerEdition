#pragma once
#include "mission_world.h"
#include <vector>

// These are completing effects on the SAME live NPC154 dialogue owner. An
// unimplemented voice manager must reject; elapsed time is not completion.
struct NativeCharacterSpeechServices {
    void *owner=nullptr;
    int (*ready)(void *,int32_t,uint8_t *)=nullptr;       //21A4F0
    int (*playing)(void *,int32_t,uint8_t *)=nullptr;     //21A4C0
    int (*request)(void *,int32_t,int32_t,const char *,uint8_t,uint8_t)=nullptr; //B7
    int (*status)(void *,int32_t,uint32_t *)=nullptr;    //actor198
    int (*stop)(void *,int32_t)=nullptr;                //DEEC0
    int (*speaker_owner)(void *,int32_t,int32_t *)=nullptr; //NPC154+8
    int (*cancel)(void *,int32_t)=nullptr;              //21A5D0
};
struct NativeCharacterSpeechState {
    std::vector<uint32_t> targets;
    const char *text="";
    int32_t selected=-1;
    uint8_t look=0,force=0,sent=0;
};
// Full1C7260 selection/wait policy and1C7170 cancellation suffix. Source slots
// and registered identities are queried afresh; absent targets are not created.
int native_character_speech_update(NativeCharacterSpeechState &,int32_t sender,
    const UCLevelEntitySlots &,const UCEntityEventRegistry &,
    const NativeCharacterSpeechServices &,bool *remove);
int native_character_speech_cancel(const NativeCharacterSpeechState &,int32_t sender,
    const UCEntityEventRegistry &,const NativeCharacterSpeechServices &);

// Actual1C7030 authored CharacterSpeech owner, including base-only20BDC0 and
// real retirement. No CharacterSpeechWhenPlayerNear/OrDispatch/controller alias.
class NativeCharacterSpeech {
    NativeMissionWorld *world_=nullptr;
    NativeCharacterSpeechServices services_{};
    NativeCharacterSpeechState state_;
    int failure_=0;
public:
    NativeMissionEntity entity;
    NativeCharacterSpeech(){entity.view.actor=nullptr;entity.view.aux84=nullptr;}
    int construct(NativeMissionWorld &,uint32_t,const NativeCharacterSpeechServices &);
    int update();int remove();int close_detached();
    static int late(void *){return 0;} //20A610 empty base callback.
    const NativeCharacterSpeechState &state()const{return state_;}
};

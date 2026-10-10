#pragma once
#include "mission_world.h"
#include "character_dialogue.h"
class NativeSubtitles;
class NativeMissionPlayer;
//1B2320/1B24E0/1B2410. Shared DISPATCH name queue and one live radio owner.
class NativeDispatch {
    NativeMissionWorld *world_=nullptr;
    NativeCharacterDialogueAudio *audio_=nullptr;NativeDialogueText *text_=nullptr;
    UCGameRandom *random_=nullptr;NativeSubtitles *subtitles_=nullptr;NativeMissionPlayer *player_=nullptr;
    int32_t *active_=nullptr;const uint32_t *world_flags_=nullptr;
    std::unique_ptr<NativeCharacterDialogue> dialogue_;
    std::vector<uint32_t> flags_;
    const char *line_="";
    bool sent_=false,removed_=false,completed_=false;
public:
    NativeMissionEntity entity;
    NativeDispatch(){entity.view.actor=nullptr;entity.view.aux84=nullptr;}
    int construct(NativeMissionWorld &,uint32_t,NativeCharacterDialogueAudio &,NativeDialogueText &,UCGameRandom &,NativeSubtitles &,NativeMissionPlayer &,int32_t &,const uint32_t &);
    int update();int remove();int close_detached();
    bool sent()const{return sent_;}bool completed()const{return completed_;}
    bool playing()const{return dialogue_&&dialogue_->playing();}
};

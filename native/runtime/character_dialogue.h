#pragma once
#include "player_audio.h"
extern "C" {
#include "../archive_lookup.h"
#include "../game_random.h"
#include "../dialogue_emote.h"
}
#include <map>

//17790/17B30: actual language block, level overrides globals. The phoneme
// bytes are opaque here; their consumer receives the same duration/sample span.
struct NativeDialogueVariant {
    std::string text;std::vector<uint8_t> phonemes;uint32_t ticks=0;bool has_phonemes=false;
};
struct NativeDialogueLine {
    std::string name;bool audio=false,always_subtitle=false;
    std::vector<NativeDialogueVariant> variants;
};
class NativeDialogueText {
    std::vector<NativeDialogueLine> global_,level_;
public:
    void load(const void *,size_t,unsigned language,bool level);
    const NativeDialogueLine *find(const char *)const;
};

// Shared speaker group, immutable bank/PCM cache and SAME PC device/audio RNG.
// Resident preload of actual WAVS/SAF data; source streaming and spatial APU
// mixing are not claimed. Immediate direct, weighted and shuffled one-shot commands.
class NativeCharacterDialogueAudio {
    struct Impl;std::unique_ptr<Impl> p_;
    friend class NativeCharacterDialogue;
    std::string text_identity_; // Source51C9B8: shared across all NPC154 owners.
    UCDialogueEmoteSession emotes_{};
public:
    class Source {
        struct Impl;std::unique_ptr<Impl> p_;
    public:
        explicit Source(NativeCharacterDialogueAudio &);~Source();
        bool play(const std::string &bank,const char *event,int32_t &variant);
        bool busy(const std::string &bank,const char *event)const;
        bool started(const std::string &bank,const char *event)const;
        void stop(const std::string &bank,const char *event);void step();
    };
    NativeCharacterDialogueAudio(NativePlayerAudio &,UCArchiveLookup &,
        const std::filesystem::path &stream,uint8_t speech_volume);
    ~NativeCharacterDialogueAudio();
    const NativeAudioBank &bank(const std::string &);
    void advance_emotes(float); //219DF0, after the AI sweep and before world update.
};

struct NativeCharacterDialogueServices {
    void *owner=nullptr;
    //21A180 completing position, including head selection/camera reflection.
    int (*position)(void *,bool select_head,float out[3])=nullptr;
    //7D000 admission/subtitle ownership and DF010's real facial channel.
    int (*subtitle)(void *,const NativeDialogueLine &,int32_t actor,int32_t *handle)=nullptr;
    int (*face)(void *,const NativeDialogueVariant &)=nullptr;
    int (*emote_distance)(void *,double *)=nullptr; //21A380
    int (*emote_actor_flags)(void *,uint32_t *)=nullptr; //1 found,0 absent,-1 failure; actor198
    int (*speech_allowed)(void *,uint8_t *)=nullptr; //DED50 PLAYER/primary gates
    int (*mission_ordinal)(void *,int32_t *)=nullptr; //54BF0
};
class NativeCharacterDialogue {
    NativeCharacterDialogueAudio &audio_;NativeDialogueText &text_;
    NativeCharacterDialogueAudio::Source source_;
    NativeCharacterDialogueServices services_;UCGameRandom &random_;int32_t actor_;
    std::string bank_,emote_bank_,emote_prefix_,event_;int32_t speaker_=-1,subtitle_=0,variant_=0;
    float fallback_=0,timeout_=0;UCDialogueEmoteState emote_{};
    bool owner_known_=false,timeout_known_=false,notified_=false,configured_=false;
    float position_[3]{};
public:
    NativeCharacterDialogue(NativeCharacterDialogueAudio &,NativeDialogueText &,
        UCGameRandom &,int32_t actor,const NativeCharacterDialogueServices &);
    ~NativeCharacterDialogue();
    void configure(uint32_t mode,const char *emotes);
    bool request(const char *text,int32_t speaker,bool subtitle=true);
    bool emote(int32_t code,int32_t priority); //21A730
    bool say(int32_t code,int32_t priority); //DED50
    bool say_pair(int32_t first,int32_t second,float chance,int32_t priority); //DEDD0
    bool busy()const;bool playing()const{return busy()||fallback_>0;}
    bool ready(bool friendly_death_cutscene)const{return !friendly_death_cutscene&&!playing();}
    bool subtitle_ready(int32_t handle)const{return handle!=subtitle_||!playing();}
    int32_t subtitle_type()const{return 1+(fallback_>0);} //21A120, independent of emote mode18.
    void cancel();void step(float delta);
    bool speaker_owner(int32_t &out)const{if(!owner_known_)return false;out=speaker_;return true;}
    float fallback()const{return fallback_;}float timeout()const{return timeout_;}
    float cooldown()const{return emote_.cooldown;}int32_t variant()const{return variant_;}
    const std::string &emote_bank()const{return emote_bank_;}
};

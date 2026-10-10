#include "footstep_audio.h"
#include <cstring>
#include <stdexcept>
extern "C" {
#include "../audio_selector_shuffle.h"
#include "../audio_selector_weighted.h"
#include "../audio_shuffle_event.h"
}
namespace {
uint16_t word(const uint8_t *p){return uint16_t(p[0]|uint16_t(p[1])<<8);}
void require(bool b,const char *s){if(!b)throw std::runtime_error(s);}
struct Play {
    NativeAudioDevice &device;NativeAudioBankAssets &assets;const NativeAudioSound &sound;
    const NativeAudioInstanceMix &mix;UCAudioRandom &random;const UCAudioCommand &command;
    std::vector<uint8_t> &selector;std::shared_ptr<const NativeAudioClip> clip;
    NativeAudioParameters parameters{};bool prepared=false,started=false;
    const UCAudioTrackState *state=nullptr;
};
int admit(void *,const UCAudioTrackState *,const UCAudioCommand *){return 1;}
int voice(void *p){return static_cast<Play*>(p)->prepared;}
void changed(void *p,UCAudioTrackState *){require(!static_cast<Play*>(p)->prepared,"Footstep track already owns a voice");}
void create(void *p,UCAudioTrackState *state,const UCAudioCommand *){
    auto &s=*static_cast<Play*>(p);state->resource=nullptr;state->subsound=-1;UCAudioSelection selected{};
    const auto status=(s.command.flags&0x80)?
        uc_audio_selector_shuffle(s.selector.data(),s.selector.size(),&s.random,&selected):
        uc_audio_selector_weighted(s.selector.data(),s.selector.size(),&s.random,&selected);
    require(status==UC_AUDIO_OK,"Footstep selector failed");
    s.clip=s.assets.clips.at(selected.reference);state->resource=s.clip.get();
    s.parameters=native_audio_parameters(s.sound,*state,s.mix,s.clip->pcm().sample_rate);s.prepared=true;state->flags|=0x1000;s.state=state;
}
void start(void *p){auto &s=*static_cast<Play*>(p);if(s.mix.start)s.mix.start(s.mix.owner,s.sound,*s.state,s.clip,s.mix);
    else s.device.play(s.clip,s.parameters.gain,s.parameters.frequency_ratio);s.started=true;}
void enqueue(void *p,UCAudioTrackState *state,const UCAudioCommand *,uint64_t when){
    auto &s=*static_cast<Play*>(p);require(when==0,"Non-immediate footstep command");
    UCAudioEventServices hooks{p,admit,voice,changed,changed,create,start};
    if(s.command.opcode==1&&(s.command.flags&0x80))require(uc_audio_shuffle_event_execute(state,&s.command,&s.random,&hooks)==UC_AUDIO_OK,"Footstep dispatch failed");
    else {auto command=s.command;command.flags=0x20;require(uc_audio_event_execute(state,&command,&s.random,&hooks)==UC_AUDIO_OK,"Footstep dispatch failed");}
}
}
static NativeAudioPlayResult play_footstep(NativeAudioDevice &device,NativeAudioBankAssets &assets,
    const char *name,const NativeAudioInstanceMix &mix,UCAudioRandom &random,bool character){
    require(name!=nullptr,"Null footstep event");const auto index=assets.bank.find(name);
    if(index<0)return NativeAudioPlayResult::MissingEvent;
    const auto &e=assets.bank.events().at(size_t(index));require((word(e.authored.data())&~0x80)==0,"Unsupported footstep event flags");
    if(e.sound_index()==65535)return NativeAudioPlayResult::NoDirectSound;
    const auto &sound=assets.bank.sounds().at(e.sound_index());const auto *a=sound.authored.data();
    require(sound.track_count==1&&(word(a+4)&0xfe00)==0x9000&&word(a+6)==1&&word(a+8)==255&&
        word(a+12)==0&&word(a+14)==65535&&word(a+16)==0&&word(a+20)==(character?0x280:0x200)&&
        (character?word(a+22)<=3:word(a+22)==65535)&&
        word(a+24)==0&&word(a+26)==247,"Unsupported footstep sound policy");
    // sound+18 names are observer-only (24C650); this owner has no observer.
    const auto &track=assets.bank.tracks().at(sound.first_track).authored;static const uint8_t zero[8]={};
    require((track.size()==36||track.size()==28)&&word(track.data())==1&&!std::memcmp(track.data()+4,zero,8),"Unsupported footstep track");
    uint8_t direct[24]{};std::memcpy(direct,track.data()+12,track.size()-12);UCAudioCommand command{};
    if(track.size()==36){
        if(word(direct+6)==0xa4)require(uc_audio_shuffle_command_read(direct,24,&command)==UC_AUDIO_OK,"Unsupported footstep shuffle command");
        else {
            require(direct[4]==1&&direct[5]==12&&word(direct+6)==0x24&&!word(direct+14),"Unsupported footstep weighted command");
            direct[6]=0x20;direct[14]=direct[15]=255;
            require(uc_audio_command_read(direct,24,&command)==UC_AUDIO_OK,"Unsupported footstep command");command.flags=0x24;
        }
    }
    else {
        const uint16_t flags=word(direct+6);
        require(direct[4]==3&&(flags==0x84||(character&&flags==4))&&!word(direct+14),"Unsupported footstep resource command");
        direct[6]=0x20;direct[14]=direct[15]=255;
        require(uc_audio_command_read(direct,16,&command)==UC_AUDIO_OK,"Unsupported footstep command");command.flags=flags;
    }
    require(
        command.time==0&&command.pitch[0]<=command.pitch[1]&&command.reference<assets.selectors.size(),"Unsupported footstep command");
    auto &selector=assets.selectors[command.reference];auto preview=selector;auto probe=random;UCAudioSelection selected{};
    const auto status=(command.flags&0x80)?uc_audio_selector_shuffle(preview.data(),preview.size(),&probe,&selected):
        uc_audio_selector_weighted(preview.data(),preview.size(),&probe,&selected);
    require(status==UC_AUDIO_OK,"Invalid footstep selector");
    const uint32_t rows=word(selector.data())&32767u;
    for(uint32_t i=0;i<rows;i++){
        const auto *row=selector.data()+4+i*12;const uint16_t ref=word(row);
        require(word(row+2)==65535&&ref<assets.clips.size()&&assets.clips[ref]&&!assets.clips[ref]->pcm().loop_frame_count,"Unsupported footstep resource");
        UCAudioTrackState state{};state.pitch=command.pitch[0];auto low=native_audio_parameters(sound,state,mix,assets.clips[ref]->pcm().sample_rate);
        state.pitch=command.pitch[1];auto high=native_audio_parameters(sound,state,mix,assets.clips[ref]->pcm().sample_rate);
        require(int(command.pitch[0])+low.sample_pitch>=-32768&&int(command.pitch[1])+high.sample_pitch<=32767,"Footstep pitch wrap unsupported");
    }
    require(device.collect()<64,"Native audio voice capacity reached");
    // Flags are opaque to24E010 in this single command domain. Feed its existing
    // validated scheduler a projection; enqueue receives the ORIGINAL command.
    direct[6]=0x20;direct[14]=direct[15]=255;
    UCAudioTrackCommands commands{direct,size_t(command.bytes),1};UCAudioTrackState state{};state.subsound=-1;
    Play playback{device,assets,sound,mix,random,command,selector,{}};
    require(uc_audio_track_schedule(&state,&commands,0,1,enqueue,&playback)==UC_AUDIO_OK&&playback.started,"Footstep start incomplete");
    return NativeAudioPlayResult::Started;
}
NativeAudioPlayResult native_audio_play_footstep(NativeAudioDevice&d,NativeAudioBankAssets&a,
    const char*n,const NativeAudioInstanceMix&m,UCAudioRandom&r){return play_footstep(d,a,n,m,r,false);}
NativeAudioPlayResult native_audio_play_character_footstep(NativeAudioDevice&d,NativeAudioBankAssets&a,
    const char*n,const NativeAudioInstanceMix&m,UCAudioRandom&r){return play_footstep(d,a,n,m,r,true);}

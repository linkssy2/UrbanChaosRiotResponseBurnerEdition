#include "audio_events.h"
#include <cfenv>
#include <cmath>
#include <cstring>
#include <stdexcept>
namespace {
uint16_t word(const uint8_t *p){return (uint16_t)(p[0]|p[1]<<8);}
void require(bool b,const char *why){if(!b)throw std::runtime_error(why);}
int16_t signed_word(uint16_t n){return (int16_t)(n<32768?n:(int32_t)n-65536);}
}
NativeAudioParameters native_audio_parameters(const NativeAudioSound &sound,
    const UCAudioTrackState &state,const NativeAudioInstanceMix &input,uint32_t rate){
    require(std::fegetround()==FE_TONEAREST,"Audio requires round-to-nearest");
    require(rate>=188&&rate<=191983,"Unsupported audio sample rate");
    const float relative=(float)((double)rate/48000.0);
    const int16_t compensation=rate==48000?0:(int16_t)std::llrint(4096.0*std::log2((double)relative));
    UCAudioMix mix{};mix.sound_gain=word(sound.authored.data()+4);mix.sound_pitch=signed_word(word(sound.authored.data()+10));
    mix.instance_gain=input.gain;mix.fade_gain=input.fade_gain;mix.group_gain=input.group_gain;
    mix.instance_pitch=input.pitch;mix.group_pitch=input.group_pitch;mix.mute=(uint8_t)input.mute;mix.sample_pitch=compensation;
    mix.track_gain=input.track_gain;mix.track_pitch=input.track_pitch;
    NativeAudioParameters p{};p.sample_pitch=compensation;p.quantized_gain=uc_audio_event_gain(&mix,&state);p.quantized_pitch=uc_audio_event_pitch(&mix,&state);
    p.gain=p.quantized_gain>=0?1.0f:p.quantized_gain<=-10000?0.0f:
        (float)std::pow(10.0,(double)p.quantized_gain*(double)0.0005f);
    p.frequency_ratio=(float)(std::pow(2.0,(double)p.quantized_pitch/4096.0)*48000.0/(double)rate);
    require(std::isfinite(p.frequency_ratio)&&p.frequency_ratio>=.25f&&p.frequency_ratio<=4.0f,"Audio pitch outside native voice range");
    return p;
}
namespace {
struct Playback {
    NativeAudioDevice *device;
    const NativeAudioBankAssets *assets;
    const NativeAudioSound *sound;
    const NativeAudioInstanceMix *mix;
    std::shared_ptr<const NativeAudioClip> clip;
    NativeAudioParameters parameters{};
    UCAudioRandom *random;
    bool prepared=false,started=false;
    std::vector<uint8_t> *selector=nullptr;
    const UCAudioTrackState *state=nullptr;
};
int admit(void *,const UCAudioTrackState *,const UCAudioCommand *){return 1;}
int voice(void *p){return ((Playback*)p)->prepared;}
void unchanged(void *p,UCAudioTrackState *){
    /* Actual248C50/2495C0 return before mixing when the fresh track has no voice. */
    require(!((Playback*)p)->prepared,"Unexpected preexisting immediate voice");
}
const void *resolve(void *p,uint16_t index){auto &s=*(Playback*)p;s.clip=s.assets->clips.at(index);return s.clip.get();}
void create(void *p,UCAudioTrackState *track,const UCAudioCommand *command){
    auto &s=*(Playback*)p;
    if(s.selector){
        track->resource=nullptr;track->subsound=-1;UCAudioSelection selected{};
        require(uc_audio_selector_cycle(s.selector->data(),s.selector->size(),&selected)==UC_AUDIO_OK&&
            selected.subsound==65535,"Audio cycle selector rejected");
        track->resource=resolve(p,selected.reference);
    }else require(uc_audio_event_resolve(track,command,(uint32_t)s.assets->clips.size(),resolve,p)==UC_AUDIO_OK,"Audio reference rejected");
    require(track->resource!=nullptr,"Missing resident audio resource");
    s.parameters=native_audio_parameters(*s.sound,*track,*s.mix,s.clip->pcm().sample_rate);
    s.prepared=true;track->flags|=0x1000;s.state=track;
}
void start(void *p){auto &s=*(Playback*)p;
    if(s.mix->start)s.mix->start(s.mix->owner,*s.sound,*s.state,s.clip,*s.mix);
    else s.device->play(s.clip,s.parameters.gain,s.parameters.frequency_ratio);s.started=true;}
void enqueue(void *p,UCAudioTrackState *track,const UCAudioCommand *command,uint64_t when){
    auto &s=*(Playback*)p;require(when==0,"Non-immediate audio command");
    UCAudioEventServices h{p,admit,voice,unchanged,unchanged,create,start};
    require(uc_audio_event_execute(track,command,s.random,&h)==UC_AUDIO_OK,"Audio execution rejected");
}
}
static NativeAudioPlayResult play_immediate(NativeAudioDevice &device,
    const NativeAudioBankAssets &assets,const char *name,const NativeAudioInstanceMix &mix,UCAudioRandom &random,
    NativeAudioBankAssets *mutable_assets,bool heartbeat=false){
    require(name!=nullptr,"Null audio event name");
    const int32_t event=assets.bank.find(name);if(event<0)return NativeAudioPlayResult::MissingEvent;
    const auto &e=assets.bank.events().at((size_t)event);const uint16_t index=e.sound_index();
    require(word(e.authored.data())==(heartbeat?0:0x80),"Unsupported audio event flags");
    if(index==65535)return NativeAudioPlayResult::NoDirectSound;
    const auto &sound=assets.bank.sounds().at(index);
    require(sound.track_count==1,"Audio event requires unsupported multiple tracks");
    static const uint8_t ordinary_tail[]={0,0,255,255,0,0,255,255,0,2,255,255,0,0,247,0};
    static const uint8_t panel_sound[]={0x6c,0,0,0,0x25,0x90,1,0,255,0,0,0,0,0xe7,255,255,1,0,255,255,0,2,255,255,0,0,0xf7,0};
    if(heartbeat)require(index==3&&!std::memcmp(sound.authored.data(),panel_sound,sizeof panel_sound),"Unsupported HeartBeep sound policy");
    else require((word(sound.authored.data()+4)&0xfe00)==0x9000&&word(sound.authored.data()+6)==1&&
        word(sound.authored.data()+8)==255&&!std::memcmp(sound.authored.data()+12,ordinary_tail,16),"Unsupported audio sound policy");
    const auto &track=assets.bank.tracks().at(sound.first_track).authored;
    static const uint8_t zero[8]={};
    require(track.size()>=12&&word(track.data())==1&&!std::memcmp(track.data()+4,zero,8),"Unsupported audio track metadata");
    UCAudioCommand command{};
    require(uc_audio_command_read(track.data()+12,track.size()-12,&command)==UC_AUDIO_OK&&command.bytes==track.size()-12&&command.time==0&&command.repeat==1,"Unsupported non-immediate audio command");
    if(heartbeat)require(command.opcode==3&&command.flags==0&&command.reference==0,"Unsupported HeartBeep command");
    uint16_t reference=command.reference;std::vector<uint8_t> *selector=nullptr;
    if(command.flags&4){
        require(mutable_assets&&command.opcode==3&&command.flags==0x0104&&
            command.reference<mutable_assets->selectors.size(),"Unsupported audio selector request");
        selector=&mutable_assets->selectors[command.reference];auto preview=*selector;UCAudioSelection selected{};
        require(uc_audio_selector_cycle(preview.data(),preview.size(),&selected)==UC_AUDIO_OK&&
            selected.reference!=65535&&selected.subsound==65535,"Unsupported audio selector result");
        reference=selected.reference;
    }
    require(reference<assets.clips.size()&&assets.clips[reference],"Missing audio GUID reference");
    const auto &pcm=assets.clips[reference]->pcm();require(!pcm.loop_frame_count,"Looping audio is unsupported");
    /* Bound every possible direct pitch result before consuming the shared RNG.
     * Ordinary authored ranges are ordered and stay inside the signed-word sum.
     * Endpoint checks then also bound all intermediate backend ratios. Only
     * the track-local sum wraps16; sound/instance/group terms stay32-bit before
     * the final clamp (2495C0), so they cannot introduce another discontinuity. */
    require(command.opcode==3||(command.pitch[0]<=command.pitch[1]),"Reversed immediate pitch range");
    UCAudioTrackState state{};state.subsound=-1;
    int32_t lo=command.opcode==1?command.pitch[0]:0,hi=command.opcode==1?command.pitch[1]:0;
    state.pitch=(int16_t)lo;auto low=native_audio_parameters(sound,state,mix,pcm.sample_rate);
    state.pitch=(int16_t)hi;auto high=native_audio_parameters(sound,state,mix,pcm.sample_rate);
    require(lo+low.sample_pitch>=-32768&&hi+high.sample_pitch<=32767,"Immediate audio pitch would wrap");
    require(device.collect()<64,"Native audio voice capacity reached");
    state={};state.subsound=-1;
    Playback playback{&device,&assets,&sound,&mix,{}, {},&random};playback.selector=selector;
    UCAudioTrackCommands commands{track.data()+12,track.size()-12,1};
    require(uc_audio_track_schedule(&state,&commands,0,1,enqueue,&playback)==UC_AUDIO_OK&&playback.started,"Audio start did not complete");
    return NativeAudioPlayResult::Started;
}

NativeAudioPlayResult native_audio_play_immediate(NativeAudioDevice &device,
    const NativeAudioBankAssets &assets,const char *name,const NativeAudioInstanceMix &mix,UCAudioRandom &random){
    return play_immediate(device,assets,name,mix,random,nullptr);
}
NativeAudioPlayResult native_audio_play_cycle(NativeAudioDevice &device,
    NativeAudioBankAssets &assets,const char *name,const NativeAudioInstanceMix &mix,UCAudioRandom &random){
    return play_immediate(device,assets,name,mix,random,&assets);
}
NativeAudioPlayResult native_audio_play_heartbeat(NativeAudioDevice &device,
    const NativeAudioBankAssets &assets,const NativeAudioInstanceMix &mix,UCAudioRandom &random){
    return play_immediate(device,assets,"HeartBeep",mix,random,nullptr,true);
}

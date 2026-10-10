#include "character_dialogue.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
extern "C" {
#include "../audio_selector_shuffle.h"
#include "../audio_selector_weighted.h"
}
namespace {
void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
uint16_t u16(const uint8_t*p){return p[0]|uint16_t(p[1])<<8;}
uint32_t u32(const uint8_t*p){return u16(p)|uint32_t(u16(p+2))<<16;}
std::string path(const char *text,std::string &event){std::string n=text;auto cut=n.rfind('-');need(cut!=std::string::npos&&cut+1<n.size(),"Dialogue text bank delimiter");event=n.substr(cut+1);need(event.size()<16,"Dialogue event exceeds retained source name");return "banks//dialogue//"+n.substr(0,cut)+".amp";}
}
struct NativeCharacterDialogueAudio::Impl {
    NativePlayerAudio &session;UCArchiveLookup &lookup;std::filesystem::path stream;
    UCAudioGroup speaker{};bool group_ready=false;uint8_t volume;
    std::map<std::string,std::unique_ptr<NativeAudioBankAssets>> banks;
    std::map<const NativeAudioBankAssets*,uint16_t> speaker_indices;
    Impl(NativePlayerAudio&s,UCArchiveLookup&l,const std::filesystem::path&p,uint8_t v):session(s),lookup(l),stream(p),volume(v){}
    NativeAudioBankAssets &get(const std::string&name){
        std::string key;for(char c:name){if(c=='/')c='\\';else if(c>='A'&&c<='Z')c+=32;if(c!='\\'||key.empty()||key.back()!='\\')key+=c;}
        auto found=banks.find(key);if(found!=banks.end())return *found->second;
        size_t index=SIZE_MAX;need(!uc_archive_lookup(&lookup,key.c_str(),&index)&&index!=SIZE_MAX,"Dialogue bank selection");
        auto assets=std::make_unique<NativeAudioBankAssets>();assets->load(lookup.archive,index,{lookup.archive},{},stream);
        UCArchiveBytes raw{};need(uc_archive_read(lookup.archive,index,UC_AR_LOCAL_PAYLOAD,&raw)==UC_AR_OK,"Dialogue bank bytes");
        std::unique_ptr<UCArchiveBytes,decltype(&uc_archive_bytes_free)> cleanup(&raw,uc_archive_bytes_free);
        const uint8_t*groups=nullptr,*names=nullptr,*properties=nullptr;size_t names_size=0,property_size=0;uint32_t count=0;
        for(size_t at=12,end=size_t(u32(raw.data+4))+8;at<end;){auto*p=raw.data+at+8;size_t n=u32(raw.data+at+4);
            if(!std::memcmp(raw.data+at,"amct",4)){need(n>=4&&(u32(p)==1||u32(p)==2)&&n==4+u32(p)*20,"Dialogue group count");count=u32(p);groups=p+4;}
            if(!std::memcmp(raw.data+at,"ampz",4)){names=p;names_size=n;}
            if(!std::memcmp(raw.data+at,"LIST",4)&&n>=4&&!std::memcmp(p,"ampk",4)){properties=p;property_size=n;}
            at+=8+n+(n&1);
        }
        need(groups&&names&&properties&&property_size==4+count*12,"Dialogue group property extent");
        UCAudioGroupSeed seed{};uint16_t selected=UINT16_MAX;
        for(uint32_t j=0;j<count;j++){
            need(!std::memcmp(properties+4+j*12,"ampk",4)&&u32(properties+8+j*12)==4&&u32(properties+12+j*12)==j,"Dialogue group property program");
            UCAudioGroupSeed current{};const auto*group=groups+j*20;need(!uc_audio_group_seed(group,20,names,names_size,&current),"Dialogue group metadata");
            for(size_t i=8;i<20;i++)need(!group[i],"Dialogue group nondefault metadata");
            if(!std::strcmp(current.name,"speaker")){need(selected==UINT16_MAX,"Duplicate dialogue speaker group");seed=current;selected=(uint16_t)j;}
            else need(!std::strcmp(current.name,"<none>")&&!current.gain&&!current.pitch&&!current.secondary_gain,"Unconnected dialogue group");
        }
        need(selected!=UINT16_MAX,"Dialogue speaker group");
        if(!group_ready){need(!uc_audio_group_initialize(&speaker,&seed,nullptr)&&!uc_audio_group_volume(&speaker,float(volume)*.0625f),"Dialogue speaker group construction");group_ready=true;}
        auto*result=assets.get();speaker_indices.emplace(result,selected);banks.emplace(key,std::move(assets));return *result;
    }
};
struct NativeCharacterDialogueAudio::Source::Impl {
    NativeCharacterDialogueAudio &owner;
    struct Voice {NativeAudioBankAssets*bank;std::string event;NativeAudioDevice::VoiceHandle handle;};
    std::vector<Voice> voices;
    explicit Impl(NativeCharacterDialogueAudio&o):owner(o){}
};
NativeCharacterDialogueAudio::NativeCharacterDialogueAudio(NativePlayerAudio&s,UCArchiveLookup&lookup,const std::filesystem::path&stream,uint8_t volume):p_(new Impl(s,lookup,stream,volume)){need(volume<=16&&lookup.archive,"Dialogue session owners");}
NativeCharacterDialogueAudio::~NativeCharacterDialogueAudio()=default;
const NativeAudioBank &NativeCharacterDialogueAudio::bank(const std::string&name){return p_->get(name).bank;}
NativeCharacterDialogueAudio::Source::Source(NativeCharacterDialogueAudio&o):p_(new Impl(o)){}
NativeCharacterDialogueAudio::Source::~Source(){auto&s=*p_->owner.p_;for(auto&v:p_->voices)s.session.device_->destroy_owned_voice(v.handle);}
bool NativeCharacterDialogueAudio::Source::play(const std::string&bank,const char*name,int32_t&variant){
    auto&s=*p_->owner.p_;auto&a=s.get(bank);const auto index=a.bank.find(name);if(index<0)return false;
    const auto&e=a.bank.events().at(index);need(!(u16(e.authored.data())&~0x80)&&u32(e.authored.data()+8)==UINT32_MAX,"Dialogue event selection policy");
    const int32_t sound_index=a.select_event(name,0,s.session.random_);if(sound_index==65535)return false;
    const auto&sound=a.bank.sounds().at(sound_index);const auto*r=sound.authored.data();
    //uc_audio_source_request: radio has no spatial descriptor. Its +12
    //aux attenuation is ignored for a nonspatial buffer by instance_prepare.
    const bool radio=u16(r+20)==0x200&&u16(r+22)==65535;
    const bool spatial=u16(r+20)==0x280&&u16(r+22)<=4&&!u16(r+12);
    need(sound.track_count==1&&(u16(r+4)&0xfe00)==0x9000&&u16(r+6)==1&&u16(r+8)==255&&(radio||spatial)&&u16(r+14)==65535&&u16(r+16)==s.speaker_indices.at(&a)&&u16(r+18)==65535&&!u16(r+24)&&u16(r+26)==247,"Dialogue sound policy");
    const auto&t=a.bank.tracks().at(sound.first_track).authored;need(t.size()==28&&u16(t.data())==1&&std::all_of(t.begin()+4,t.begin()+12,[](uint8_t v){return !v;}),"Dialogue immediate track");
    uint8_t raw[16];std::memcpy(raw,t.data()+12,16);const uint16_t flags=u16(raw+6);
    need(raw[4]==3&&(flags==0||flags==0x80||flags==4||flags==0x84),"Dialogue resource command flags");
    const bool selection=(flags&4)!=0;need(u16(raw+14)==(selection?0:65535),"Dialogue resource command suffix");
    raw[6]=0;raw[7]=0;raw[14]=raw[15]=255;UCAudioCommand command{};
    need(!uc_audio_command_read(raw,16,&command)&&!command.time&&command.repeat==1,"Dialogue immediate command");
    auto validate=[&](uint16_t reference){need(reference<a.clips.size()&&a.clips[reference]&&!a.clips[reference]->pcm().loop_frame_count,"Dialogue resident one-shot resource");};
    std::vector<uint8_t>*table=nullptr;
    if(selection){need(command.reference<a.selectors.size(),"Dialogue selector reference");table=&a.selectors[command.reference];auto preview=*table;auto rng=s.session.random_;UCAudioSelection chosen{};
        need(((flags&0x80)?uc_audio_selector_shuffle(preview.data(),preview.size(),&rng,&chosen):uc_audio_selector_weighted(preview.data(),preview.size(),&rng,&chosen))==UC_AUDIO_OK,"Dialogue selector shape");
        const unsigned rows=u16(table->data())&32767;for(unsigned i=0;i<rows;i++){const auto*row=table->data()+4+12*i;need(u16(row+2)==65535,"Dialogue selector subbank");validate(u16(row));}
    }else validate(command.reference);
    UCAudioGroupMix group{};need(!uc_audio_group_mix(&s.speaker,&group),"Dialogue live speaker group");
    NativeAudioInstanceMix mix{0,0,group.gain,0,group.pitch,s.session.mute_};need(p_->voices.size()<16,"Dialogue source capacity");
    struct Play {Impl*source;NativeAudioBankAssets*assets;const NativeAudioSound*sound;NativeAudioInstanceMix mix;std::vector<uint8_t>*table;uint16_t flags;int32_t*variant;NativeAudioDevice::VoiceHandle handle{};} context{p_.get(),&a,&sound,mix,table,flags,&variant};
    auto enqueue=[](void*p,UCAudioTrackState*state,const UCAudioCommand*c,uint64_t time){
        auto&v=*(Play*)p;auto&s=*v.source->owner.p_;need(time==0,"Dialogue immediate schedule");
        UCAudioEventServices h{p,[](void*,const UCAudioTrackState*,const UCAudioCommand*){return 1;},[](void*p)->int{return ((Play*)p)->handle.value!=0;},
            [](void*,UCAudioTrackState*){throw std::runtime_error("Dialogue unexpected pitch command");},[](void*,UCAudioTrackState*){throw std::runtime_error("Dialogue unexpected volume command");},
            [](void*p,UCAudioTrackState*state,const UCAudioCommand*c){auto&v=*(Play*)p;auto&s=*v.source->owner.p_;uint16_t reference=c->reference;
                if(v.table){UCAudioSelection choice{};need(((v.flags&0x80)?uc_audio_selector_shuffle(v.table->data(),v.table->size(),&s.session.random_,&choice):uc_audio_selector_weighted(v.table->data(),v.table->size(),&s.session.random_,&choice))==UC_AUDIO_OK,"Dialogue resource selection");reference=choice.reference;*v.variant=(int32_t)choice.index;}
                //216730 observes the selected ROW. Direct resources preserve the
                //previous variant; source requests reset it separately when needed.
                auto clip=v.assets->clips.at(reference);state->resource=clip.get();state->subsound=-1;auto params=native_audio_parameters(*v.sound,*state,v.mix,clip->pcm().sample_rate);
                v.handle=s.session.device_->create_owned_voice();s.session.device_->prepare_owned_voice(v.handle,clip,params.gain,params.frequency_ratio,0,clip->pcm().frame_count);state->flags|=0x1000;},
            [](void*p){auto&v=*(Play*)p;auto&s=*v.source->owner.p_;s.session.device_->start_owned_voice(v.handle);++s.session.started_;}};
        need(!uc_audio_event_execute(state,c,&s.session.random_,&h),"Dialogue event execution");
    };
    UCAudioTrackState state{};state.subsound=-1;UCAudioTrackCommands commands{raw,16,1};++s.session.requests_;
    need(!uc_audio_track_schedule(&state,&commands,0,1,enqueue,&context)&&context.handle.value,"Dialogue audio start");
    p_->voices.push_back({&a,name,context.handle});return true;
}
bool NativeCharacterDialogueAudio::Source::busy(const std::string&bank,const char*event)const{
    if(bank.empty()||!event)return false;auto&a=p_->owner.p_->get(bank);for(const auto&v:p_->voices)if(v.bank==&a&&!_stricmp(v.event.c_str(),event))return true;return false;
}
bool NativeCharacterDialogueAudio::Source::started(const std::string&bank,const char*event)const{
    if(bank.empty()||!event)return false;auto&s=*p_->owner.p_;auto&a=s.get(bank);for(const auto&v:p_->voices)if(v.bank==&a&&!_stricmp(v.event.c_str(),event)){auto state=s.session.device_->owned_voice_state(v.handle);if(state.started&&state.buffers_queued)return true;}return false;
}
void NativeCharacterDialogueAudio::Source::stop(const std::string&bank,const char*event){
    if(bank.empty()||!event)return;auto&s=*p_->owner.p_;auto&a=s.get(bank);for(auto it=p_->voices.begin();it!=p_->voices.end();)if(it->bank==&a&&!_stricmp(it->event.c_str(),event)){s.session.device_->destroy_owned_voice(it->handle);it=p_->voices.erase(it);}else ++it;
}
void NativeCharacterDialogueAudio::Source::step(){auto&s=*p_->owner.p_;for(auto it=p_->voices.begin();it!=p_->voices.end();)if(!s.session.device_->owned_voice_state(it->handle).buffers_queued){s.session.device_->destroy_owned_voice(it->handle);it=p_->voices.erase(it);}else ++it;}

NativeCharacterDialogue::NativeCharacterDialogue(NativeCharacterDialogueAudio&a,NativeDialogueText&t,UCGameRandom&r,int32_t actor,const NativeCharacterDialogueServices&services):audio_(a),text_(t),source_(a),services_(services),random_(r),actor_(actor){
    need(services.position&&services.subtitle&&services.face&&actor!=-1,"NPC dialogue services");int32_t draw;need(!uc_game_random_next(&r,&draw),"NPC dialogue cooldown random");emote_.cooldown=float(double(draw)*4.0*0x1p-31+1.0);
}
void NativeCharacterDialogue::configure(uint32_t mode,const char*emotes){
    need(!busy()&&mode<=3,"NPC dialogue configure scope");emote_.mode=mode;emote_bank_.clear();emote_prefix_=emotes?emotes:"";
    if(emotes){std::string n=emotes;if(n.find('*')!=std::string::npos){auto cut=n.rfind('-');need(cut!=std::string::npos,"NPC emote wildcard delimiter");n.resize(cut);emote_prefix_=n;emote_bank_="banks//dialogue//"+n+".AMP";audio_.bank(emote_bank_);}}
    if(emote_bank_.empty()&&!mode){int32_t draw;need(!uc_game_random_range(&random_,0,3,&draw),"NPC dialogue bank random");const char*names[]={"Generic-GangA","Generic-GangB","Generic-GangC"};emote_bank_=std::string("banks//dialogue//")+names[draw]+".AMP";audio_.bank(emote_bank_);}emote_.has_bank=!emote_bank_.empty();configured_=true;
}
bool NativeCharacterDialogue::request(const char*text,int32_t speaker,bool subtitle){
    //21AAE0 scripted requests do not require the optional21A0xx emote setup.
    need(text,"Dialogue request text");if(speaker!=-1){speaker_=speaker;owner_known_=true;}
    const auto*line=text_.find(text);fallback_=float(line?line->variants.front().text.size():std::strlen(text))*.08f;
    if(!line)return false;
    if(line->audio){if(emote_.active)need(!uc_dialogue_emote_release(&audio_.emotes_,&emote_,reinterpret_cast<uintptr_t>(this)),"NPC scripted request releases emote slot");variant_=0;bank_=path(text,event_);audio_.text_identity_=text;emote_.priority=100;
        need(!services_.position(services_.owner,true,position_),"NPC dialogue head position");notified_=false;
        if(source_.play(bank_,event_.c_str(),variant_))fallback_=0;
        timeout_=float(double(line->variants.front().ticks)*double(.05f)*2.0);timeout_known_=true;
    }
    if(subtitle)need(!services_.subtitle(services_.owner,*line,actor_,&subtitle_),"NPC dialogue subtitle owner");return true;
}
bool NativeCharacterDialogue::busy()const{return (emote_.active&&source_.busy(emote_bank_,event_.c_str()))||source_.busy(bank_,event_.c_str());}
void NativeCharacterDialogue::cancel(){if(busy())source_.stop(bank_,event_.c_str());}
void NativeCharacterDialogue::step(float delta){
    need(std::isfinite(delta)&&delta>=0&&delta<=1,"NPC dialogue delta");need(!services_.position(services_.owner,false,position_),"NPC dialogue current position");source_.step();
    if(busy()&&!notified_&&source_.started(emote_.active?emote_bank_:bank_,event_.c_str())){
        const auto*line=text_.find(audio_.text_identity_.c_str());if(line){size_t index=variant_>0?size_t(variant_):0;if(index>=line->variants.size())index=0;
        const auto&v=line->variants[index];if(v.has_phonemes)need(!services_.face(services_.owner,v),"NPC dialogue facial animation owner");}notified_=true;
    }
    if(emote_.active&&!busy())need(!uc_dialogue_emote_release(&audio_.emotes_,&emote_,reinterpret_cast<uintptr_t>(this)),"NPC dialogue emote release");
    emote_.cooldown-=delta;fallback_-=delta;
    //219E80 leaves+68 unwritten. Before the first scripted audio request no
    //voice exists, so its unknown value cannot cause an observable stop.
    if(timeout_known_&&timeout_>0){timeout_-=delta;if(timeout_<=0&&busy())source_.stop(bank_,event_.c_str());}
}

NativeCharacterDialogue::~NativeCharacterDialogue(){uc_dialogue_emote_release(&audio_.emotes_,&emote_,reinterpret_cast<uintptr_t>(this));}
void NativeCharacterDialogueAudio::advance_emotes(float delta){need(!uc_dialogue_emote_advance(&emotes_,delta),"Dialogue shared cooldown step");}
bool NativeCharacterDialogue::emote(int32_t code,int32_t priority){
 need(configured_,"NPC emote before configuration");
 UCDialogueEmoteServices h{this,
  [](void*p){auto&s=*(NativeCharacterDialogue*)p;double value=0;need(s.services_.emote_distance&&!s.services_.emote_distance(s.services_.owner,&value),"NPC emote distance owner");return value;},
  [](void*p,uint32_t*out){auto&s=*(NativeCharacterDialogue*)p;need(s.services_.emote_actor_flags,"NPC emote actor flags owner");return s.services_.emote_actor_flags(s.services_.owner,out);},
  [](void*p)->uint8_t{return ((NativeCharacterDialogue*)p)->busy();},
  [](void*p,uint8_t active){auto&s=*(NativeCharacterDialogue*)p;s.source_.stop(active?s.emote_bank_:s.bank_,s.event_.c_str());},
  [](void*p,int32_t line){auto&s=*(NativeCharacterDialogue*)p;s.event_=std::to_string(line);need(!s.services_.position(s.services_.owner,true,s.position_),"NPC emote head position");s.notified_=false;s.source_.play(s.emote_bank_,s.event_.c_str(),s.variant_);},
  [](void*p,int32_t id,uint8_t found)->uint8_t{auto&s=*(NativeCharacterDialogue*)p;s.audio_.text_identity_=s.emote_prefix_+"-"+std::to_string(id);const auto*line=s.text_.find(s.audio_.text_identity_.c_str());if(line&&found&&line->always_subtitle)need(!s.services_.subtitle(s.services_.owner,*line,s.actor_,&s.subtitle_),"NPC emote subtitle owner");return line&&line->audio;}};
 int result=uc_dialogue_emote_select(&audio_.emotes_,&emote_,reinterpret_cast<uintptr_t>(this),emote_prefix_.c_str(),code,priority,&h);need(result>=0,"NPC emote selection policy");return result!=0;
}
bool NativeCharacterDialogue::say(int32_t code,int32_t priority){uint8_t allowed=0;need(services_.speech_allowed&&!services_.speech_allowed(services_.owner,&allowed),"NPC speech admission owner");return allowed&&emote(code,priority);}
bool NativeCharacterDialogue::say_pair(int32_t first,int32_t second,float chance,int32_t priority){int32_t ordinal=0,selected=0;need(services_.mission_ordinal&&!services_.mission_ordinal(services_.owner,&ordinal),"NPC speech mission ordinal");need(!uc_dialogue_emote_pair(&random_,ordinal,first,second,chance,&selected),"NPC paired emote selection");return say(selected,priority);}

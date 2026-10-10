#include "player_audio.h"
#include "footstep_audio.h"
#include "audio_device_bridge.h"
#include <map>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <cmath>
extern "C" {
#include "../audio_request.h"
#include "../audio_material_route.h"
#include "../audio_timed_event.h"
#include "../audio_track_resource.h"
#include "../audio_zero_policy.h"
#include "../audio_fade.h"
#include "../audio_source_busy.h"
#include "../audio_impact_policy.h"
#include "../audio_default_policy.h"
#include "../audio_bus_effect.h"
#include "../audio_parameter.h"
#include "../audio_live_mix.h"
}
namespace {
void require(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
uint32_t dword(const uint8_t *p){return uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24;}
struct Archive {
    UCArchive *value=nullptr;
    explicit Archive(const std::filesystem::path &path){require(uc_archive_open(path.string().c_str(),&value)==UC_AR_OK,"Player audio archive");}
    ~Archive(){uc_archive_close(value);}
};
size_t entry(UCArchive *a,const char *name){
    for(size_t i=0;i<uc_archive_count(a);i++)if(!std::strcmp(reinterpret_cast<const char*>(uc_archive_entry(a,i)->name),name))return i;
    throw std::runtime_error("Missing authored player soundbank");
}
void group_seed(const UCArchive *archive,size_t index,UCAudioGroup &group,bool first,uint32_t groups=1,uint32_t selected=0,const char *name="game3d"){
    UCArchiveBytes bytes{};require(uc_archive_read(archive,index,UC_AR_LOCAL_PAYLOAD,&bytes)==UC_AR_OK,"Player group bytes");
    std::unique_ptr<UCArchiveBytes,decltype(&uc_archive_bytes_free)> cleanup(&bytes,uc_archive_bytes_free);
    // NativeAudioBank::load already validated this immutable RIFF's extents.
    const uint8_t *record=nullptr,*names=nullptr;size_t record_size=0,names_size=0;
    for(size_t at=12,end=size_t(dword(bytes.data+4))+8;at<end;){
        size_t n=dword(bytes.data+at+4);const uint8_t *p=bytes.data+at+8;
        require(std::memcmp(bytes.data+at,"ampk",4)!=0,"Top-level player audio property program unsupported");
        if(!std::memcmp(bytes.data+at,"LIST",4)&&n>=4&&!std::memcmp(p,"ampk",4)){
            require(n==4+groups*12,"Player audio group properties unsupported");
            for(uint32_t j=0;j<groups;j++)require(!std::memcmp(p+4+j*12,"ampk",4)&&
                dword(p+8+j*12)==4&&dword(p+12+j*12)==j,"Player audio property program unsupported");
        }
        if(!std::memcmp(bytes.data+at,"amct",4)){require(selected<groups&&n==4+groups*20&&dword(p)==groups,"Player sound group count");record=p+4+selected*20;record_size=20;}
        if(!std::memcmp(bytes.data+at,"ampz",4)){names=p;names_size=n;}
        at+=8+n+(n&1);
    }
    UCAudioGroupSeed seed{};
    require(uc_audio_group_seed(record,record_size,names,names_size,&seed)==UC_AUDIO_OK&&
        !std::strcmp(seed.name,name),"Unsupported player sound group");
    // The weapon banks use the same registry name;240D80 reuses the first
    // group. These exact records have no additional property program.
    for(size_t i=8;i<record_size;i++)require(record[i]==0,"Unsupported player group metadata");
    if(first)require(uc_audio_group_initialize(&group,&seed,nullptr)==UC_AUDIO_OK,"Player sound group construction");
}
struct Dispatch {
    NativePlayerAudio *audio;const void *actor;uint32_t state;int32_t active,previous;
};
struct HeartRequest {const NativeAudioBankAssets *bank;const UCAudioGroup *panel;};
const uint8_t *heart_sound(void *p,uint16_t index){
    return static_cast<HeartRequest*>(p)->bank->bank.sounds().at(index).authored.data();
}
const void *heart_group(void *p,uint16_t index){
    require(index==1,"Unsupported HeartBeep cached group");return static_cast<HeartRequest*>(p)->panel;
}
const void *selected_bank(void *p,const void *){
    auto &d=*static_cast<Dispatch*>(p);uint32_t item=0;
    require(uc_animation_sound_item(d.state,d.active,d.previous,&item)==UC_ANIMATION_OK,"Animation sound item");
    return &d.audio->bank(item);
}
void play_selected(void *p,const void *,const void *bank,const char *name){
    auto &d=*static_cast<Dispatch*>(p);
    uint32_t item=bank==&d.audio->bank(2)?2:17;
    require(bank==&d.audio->bank(item),"Animation sound bank identity");
    d.audio->play(item,name);
}
uint8_t sound(UCAnimationEventRef ref,UCAnimationDispatch *dispatch){
    auto &d=*static_cast<Dispatch*>(dispatch->user);uint8_t handled=0;
    UCAnimationSoundServices services{&d,selected_bank,play_selected};
    require(uc_animation_sound(&ref.owner->events[ref.index],d.actor,&services,&handled)==UC_ANIMATION_OK,"Animation sound dispatch");
    return handled;
}
}
struct NativePlayerAudio::LiveVoices {
    struct Voice {NativeAudioSound sound;UCAudioTrackState state{};NativeAudioInstanceMix mix{};
        std::shared_ptr<const NativeAudioClip> clip;NativeAudioDevice::VoiceHandle handle{};};
    std::vector<Voice> voices;
};
NativePlayerAudio::NativePlayerAudio(const std::filesystem::path &game,uint8_t volume,bool mute):mute_(mute){
    require(volume<=16,"Player audio profile volume outside0..16");
    Archive pistol(game/"pistol.lst"),level(game/"1-01.ent");
    size_t p=entry(pistol.value,"banks\\player\\pistol.amp"),s=entry(level.value,"banks\\player\\policeshotgun.amp"),w=entry(level.value,"banks\\player.amp");
    banks_[0].load(pistol.value,p,{pistol.value});banks_[1].load(level.value,s,{level.value});banks_[2].load(level.value,w,{level.value},{0,1,2,3});
    const size_t h=entry(level.value,"banks\\levelglobals.amp");banks_[3].load(level.value,h,{level.value},{0});
    const size_t f=entry(level.value,"banks\\playerfootsteps.amp");banks_[4].load(level.value,f,{level.value});
    require(banks_[0].bank.find("SecondaryFire")<0&&banks_[1].bank.find("SecondaryFire")<0,"Unsupported secondary sound policy");
    group_seed(pistol.value,p,group_,true);group_seed(level.value,s,group_,false);group_seed(level.value,w,group_,false,3);
    group_seed(level.value,h,panel_,true,3,1,"panel");
    group_seed(level.value,f,group_,false);
    require(uc_audio_group_volume(&group_,float(volume)*.0625f)==UC_AUDIO_OK,"Player group profile volume");
    require(uc_audio_group_volume(&panel_,float(volume)*.0625f)==UC_AUDIO_OK,"Panel group profile volume");
    device_=std::make_unique<NativeAudioDevice>();live_=std::make_unique<LiveVoices>();
}
const NativeAudioBankAssets &NativePlayerAudio::bank(uint32_t item) const {
    require(item==2||item==17,"Player sound item unsupported");return banks_[item==2?0:1];
}
NativeAudioInstanceMix NativePlayerAudio::live_mix(const UCAudioGroup&identity)const{
    UCAudioGroupMix group{};require(uc_audio_group_mix(&identity,&group)==UC_AUDIO_OK,"Player sound live group");
    NativeAudioInstanceMix result{0,0,group.gain,0,group.pitch,mute_};result.owner=const_cast<NativePlayerAudio*>(this);result.group=&identity;
    result.start=[](void*p,const NativeAudioSound&sound,const UCAudioTrackState&state,std::shared_ptr<const NativeAudioClip>clip,const NativeAudioInstanceMix&mix){
        auto&s=*(NativePlayerAudio*)p;LiveVoices::Voice voice{sound,state,mix,std::move(clip),{}};
        auto parameters=native_audio_parameters(sound,state,mix,voice.clip->pcm().sample_rate);voice.handle=s.device_->create_owned_voice();
        try{s.device_->prepare_owned_voice(voice.handle,voice.clip,parameters.gain,parameters.frequency_ratio,0,voice.clip->pcm().frame_count);
            s.filter_voice(voice.handle,(const UCAudioGroup*)mix.group,8000);s.device_->start_owned_voice(voice.handle);s.live_->voices.push_back(std::move(voice));
        }catch(...){s.device_->destroy_owned_voice(voice.handle);throw;}
    };return result;
}
NativeAudioInstanceMix NativePlayerAudio::mix() const {return live_mix(group_);}

NativeAudioPlayResult NativePlayerAudio::play(uint32_t item,const char *name){
    auto result=native_audio_play_immediate(*device_,bank(item),name,mix(),random_);
    ++requests_;started_+=result==NativeAudioPlayResult::Started;return result;
}
NativeAudioPlayResult NativePlayerAudio::fire(uint32_t item,uint8_t alternate){
    if(alternate){auto result=play(item,"SecondaryFire");if(result==NativeAudioPlayResult::Started)return result;}
    return play(item,"Fire");
}
NativeAudioPlayResult NativePlayerAudio::switch_weapon(bool shield){
    auto result=native_audio_play_cycle(*device_,banks_[2],shield?"ShieldEquip":"SwitchWeapon",mix(),random_);
    ++requests_;started_+=result==NativeAudioPlayResult::Started;return result;
}
NativeAudioPlayResult NativePlayerAudio::shield(bool active){
    auto result=native_audio_play_immediate(*device_,banks_[2],active?"ShieldActivate":"ShieldDeactivate",mix(),random_);
    ++requests_;started_+=result==NativeAudioPlayResult::Started;return result;
}
NativeAudioPlayResult NativePlayerAudio::heartbeat(){
    const auto &assets=banks_[3];const auto index=assets.bank.find("HeartBeep");
    HeartRequest context{&assets,&panel_};UCAudioRequestServices h{&context,heart_sound,heart_group};
    UCAudioRequestSelection selected{};uint8_t found=0;
    require(uc_audio_request_prepare(index<0?nullptr:assets.bank.events().at((size_t)index).authored.data(),&h,&selected,&found)==UC_AUDIO_OK&&
        found&&selected.group==&panel_,"HeartBeep event/group preparation");
    UCAudioGroupMix group{};require(uc_audio_group_mix(&panel_,&group)==UC_AUDIO_OK,"Panel current mix");
    NativeAudioInstanceMix input=live_mix(panel_);
    const auto result=native_audio_play_heartbeat(*device_,assets,input,random_);
    require(result==NativeAudioPlayResult::Started,"HeartBeep did not start");++requests_;++started_;return result;
}
std::string NativePlayerAudio::footstep(uint32_t material){
    return material_footstep(banks_[4],material,false);
}
void NativePlayerAudio::prepare_character_footsteps(const std::filesystem::path&game){
    if(character_steps_)return;Archive level(game/"1-01.ent");auto index=entry(level.value,"banks\\footsteps.amp");
    auto assets=std::make_unique<NativeAudioBankAssets>();assets->load(level.value,index,{level.value});
    group_seed(level.value,index,group_,false,2);character_steps_=std::move(assets);
    index=entry(level.value,"banks\\npc\\burners\\blazer.amp");
    assets=std::make_unique<NativeAudioBankAssets>();assets->load(level.value,index,{level.value},{2,3,4,5});
    group_seed(level.value,index,group_,false);character_foley_=std::move(assets);
}
std::string NativePlayerAudio::character_footstep(uint32_t material){
    require(character_steps_!=nullptr,"NPC footsteps bank not prepared");return material_footstep(*character_steps_,material,true);
}
NativeAudioPlayResult NativePlayerAudio::character_foley(const NativeAudioBank&bank){
    require(character_foley_!=nullptr,"NPC equipment sound bank not prepared");
    auto&a=*character_foley_;const auto incoming=bank.find("RunFoley"),loaded=a.bank.find("RunFoley");
    require(incoming>=0&&loaded>=0&&bank.events().at(incoming).authored==a.bank.events().at(loaded).authored&&
        bank.sounds().at(bank.events().at(incoming).sound_index()).authored==a.bank.sounds().at(a.bank.events().at(loaded).sound_index()).authored,"NPC equipment sound owner mismatch");
    const auto result=native_audio_play_character_footstep(*device_,a,"RunFoley",mix(),random_);
    ++requests_;started_+=result==NativeAudioPlayResult::Started;return result;
}
std::string NativePlayerAudio::material_footstep(NativeAudioBankAssets&assets,uint32_t material,bool character){
    if(material>=36)throw std::runtime_error("Invalid footstep material "+std::to_string(material));
    //ponytail: WoodenHoles multi-track arbitration is not ported. Match the
    //player-footstep path: omit this optional sound, never terminate a mission.
    if(material==30)return {};
    UCAudioMaterialActor actor{this};char name[64];
    struct Context {NativePlayerAudio*self;NativeAudioBankAssets*assets;bool character;} material_context{this,&assets,character};
    UCAudioMaterialServices hooks{&material_context,
        [](void *,const UCAudioMaterialActor *a,const void *)->int{return a->controller!=nullptr;},
        [](void *,UCAudioMaterialActor *)->void *{throw std::runtime_error("Existing player audio controller required");},
        [](void *context,void *controller,const void *bank,const char *event)->uint8_t{
            auto&c=*static_cast<Context*>(context);auto&self=*c.self;require(controller==&self&&bank==c.assets,"Footstep owner identity");
            const auto index=c.assets->bank.find(event);UCAudioRequestServices prepare{&c,
                [](void *p,uint16_t i){return static_cast<Context*>(p)->assets->bank.sounds().at(i).authored.data();},
                [](void *p,uint16_t i)->const void *{require(i==0,"Footstep shared group index");return &static_cast<Context*>(p)->self->group_;}};
            UCAudioRequestSelection selected{};uint8_t found=0;
            require(uc_audio_request_prepare(index<0?nullptr:c.assets->bank.events().at(size_t(index)).authored.data(),&prepare,&selected,&found)==UC_AUDIO_OK&&
                (!found||selected.group==&self.group_),"Footstep event/group preparation");
            const auto result=(c.character?native_audio_play_character_footstep:native_audio_play_footstep)(*self.device_,*c.assets,event,self.mix(),self.random_);
            ++self.requests_;self.started_+=result==NativeAudioPlayResult::Started;return result==NativeAudioPlayResult::Started;
        }};
    require(uc_audio_material_request(&actor,&assets,material,nullptr,name,sizeof name,&hooks)==UC_AUDIO_OK,"Footstep material routing failed");
    return name;
}
void NativePlayerAudio::dispatch(UCAnimationEventQueue &queue,const void *actor,uint32_t state,int32_t active,int32_t previous){
    Dispatch context{this,actor,state,active,previous};UCAnimationHandlers local{},handlers{1,{sound}};
    UCAnimationDispatch dispatch{queue,&local,&handlers,&context};
    require(uc_animation_dispatch_events(&dispatch)==UC_ANIMATION_OK,"Player global sound queue");queue=dispatch.queue;
}

struct NativePlayerAudio::MissionState {
    struct Group {std::string name;UCAudioGroup value{},*scalar=nullptr;float cutoff=1,resonance=0;};
    std::vector<std::unique_ptr<Group>> groups;
    std::map<std::string,UCAudioBusEnvelope> envelopes;
    Group* group(const char*name){for(auto&g:groups)if(!_stricmp(g->name.c_str(),name))return g.get();return nullptr;}
    Group* group(const UCAudioGroup*value){for(auto&g:groups)if(g->scalar==value)return g.get();return nullptr;}
    explicit MissionState(NativePlayerAudio&s){
        for(auto pair:{std::pair<const char*,UCAudioGroup*>{"game3d",&s.group_},{"panel",&s.panel_}}){auto g=std::make_unique<Group>();g->name=pair.first;g->scalar=pair.second;groups.push_back(std::move(g));}
    }
    struct Bank {
        std::string name;std::vector<uint8_t> bytes;NativeAudioBankAssets assets;
        std::vector<UCAudioSelectorPayload> selectors;UCAudioPolicyBank policy{};std::vector<Group*> groups;
    };
    struct Instance {
        std::shared_ptr<Bank> bank;const NativeAudioEvent*event=nullptr;
        const NativeAudioSound*sound=nullptr;void*source=nullptr;UCAudioFadeState state{0,2,0};
        void(*stop)(Instance*)=nullptr;void(*fade)(Instance*)=nullptr;void(*refresh)(Instance*)=nullptr;
    };
    std::vector<std::shared_ptr<Bank>> banks;
    std::vector<std::weak_ptr<Instance>> instances;
    std::shared_ptr<Bank> load(const UCArchive*a,size_t index,const std::filesystem::path&stream){
        UCArchiveBytes bytes{};require(uc_archive_read(a,index,UC_AR_LOCAL_PAYLOAD,&bytes)==UC_AR_OK,"Mission sound bank bytes");
        std::unique_ptr<UCArchiveBytes,decltype(&uc_archive_bytes_free)> cleanup(&bytes,uc_archive_bytes_free);
        const char*name=(const char*)uc_archive_entry(a,index)->name;
        for(const auto&b:banks)if(!_stricmp(b->name.c_str(),name)&&b->bytes.size()==bytes.size&&
            !std::memcmp(b->bytes.data(),bytes.data,bytes.size))return b;
        auto b=std::make_shared<Bank>();b->name=name;b->bytes.assign(bytes.data,bytes.data+bytes.size);
        b->assets.load(a,index,{a},{},stream);
        for(auto&selector:b->assets.selectors)b->selectors.push_back({selector.data(),selector.size()});
        for(size_t at=12,end=size_t(dword(b->bytes.data()+4))+8;at<end;){
            const size_t size=dword(b->bytes.data()+at+4);
            if(!std::memcmp(b->bytes.data()+at,"ampz",4))b->policy={b->bytes.data()+at+8,size,104};
            at+=8+size+(size&1);
        }
        require(b->policy.strings,"Mission sound policy strings");
        const uint8_t*records=nullptr;size_t record_bytes=0;const uint8_t*properties=nullptr;size_t property_bytes=0;
        for(size_t at=12,end=size_t(dword(b->bytes.data()+4))+8;at<end;){const size_t n=dword(b->bytes.data()+at+4);const auto*p=b->bytes.data()+at+8;
            if(!std::memcmp(b->bytes.data()+at,"amct",4)){records=p;record_bytes=n;}
            if(!std::memcmp(b->bytes.data()+at,"LIST",4)&&n>=4&&!std::memcmp(p,"ampk",4)){properties=p;property_bytes=n;}at+=8+n+(n&1);}
        require(records&&record_bytes>=4&&record_bytes==4+size_t(dword(records))*20,"Mission sound group table");
        const uint32_t count=dword(records);require(!properties||property_bytes==4+size_t(count)*12,"Mission group property program");
        for(uint32_t i=0;i<count;i++){const auto*r=records+4+size_t(i)*20;UCAudioGroupSeed seed{};
            require(uc_audio_group_seed(r,20,b->policy.strings,b->policy.string_bytes,&seed)==UC_AUDIO_OK,"Mission group descriptor");
            require(std::all_of(r+8,r+20,[](uint8_t n){return !n;}),"Mission group metadata");
            if(properties)require(!std::memcmp(properties+4+size_t(i)*12,"ampk",4)&&dword(properties+8+size_t(i)*12)==4&&dword(properties+12+size_t(i)*12)==i,"Mission group property program");
            auto*g=group(seed.name);if(!g){auto created=std::make_unique<Group>();created->name=seed.name;created->scalar=&created->value;
                require(uc_audio_group_initialize(created->scalar,&seed,nullptr)==UC_AUDIO_OK,"Mission group initialize");g=created.get();groups.push_back(std::move(created));}b->groups.push_back(g);
        }
        banks.push_back(b);return b;
    }
    bool admit(const std::shared_ptr<Instance>&request){
        const auto*sound=request->sound->authored.data();const auto*event=request->event->authored.data();
        const uint16_t policy=uint16_t(sound[14]|uint16_t(sound[15])<<8);if(policy==65535)return true;
        std::vector<std::shared_ptr<Instance>> live;std::vector<UCAudioPolicyInstance> views;
        std::vector<const uint32_t*> flags;std::vector<uint8_t> matches;
        for(auto i=instances.begin();i!=instances.end();){auto p=i->lock();if(!p)i=instances.erase(i);else{
            live.push_back(p);views.push_back({104,&p->bank->policy,p->sound->authored.data(),p->event->authored.data()});
            flags.push_back(&p->state.flags);matches.push_back(p->source==request->source);++i;}}
        auto stop=[](void*p,uint32_t index){auto&v=*(std::vector<std::shared_ptr<Instance>>*)p;auto&victim=*v.at(index);victim.stop(&victim);return 0;};
        auto fade=[](void*p,uint32_t index){auto&v=*(std::vector<std::shared_ptr<Instance>>*)p;auto&victim=*v.at(index);victim.fade(&victim);return 0;};
        uint8_t accepted=0;int result=0;
        if(!(policy>>14))result=uc_audio_zero_policy(&request->bank->policy,event,sound,views.data(),uint32_t(views.size()),1,matches.data(),flags.data(),&request->state.flags,fade,stop,&live,&accepted);
        else if(event[0]&128)result=uc_audio_impact_policy(&request->bank->policy,event,sound,views.data(),uint32_t(views.size()),&accepted);
        else result=uc_audio_default_policy(&request->bank->policy,event,sound,views.data(),uint32_t(views.size()),flags.data(),request->state.flags,stop,&live,&accepted);
        if(result)throw std::runtime_error(std::string("Unsupported mission sound arbitration: ")+request->bank->name+" / "+request->event->name+" event="+std::to_string(event[0])+" policy="+std::to_string(policy)+" result="+std::to_string(result));return accepted!=0;
    }
};
struct NativePlayerAudio::MissionSource::Impl {
    struct Track {
        std::string event;const NativeAudioSound *sound=nullptr;
        std::shared_ptr<MissionState::Instance> instance;
        UCAudioTimedCommand command{};UCAudioTrackState state{};
        std::shared_ptr<const NativeAudioClip> clip;uint64_t when=0;
        NativeAudioDevice::VoiceHandle voice{};double age=0;float volume=1;
        bool started=false,executed=false;
        std::array<uint8_t,44> fade{};uint64_t fade_when=0;bool fade_pending=false;int16_t parameter_gain=0,parameter_pitch=0;uint16_t cutoff=8000;
    };
    NativePlayerAudio &session;std::shared_ptr<MissionState::Bank> bank;bool panel=false;uint32_t panel_group=0;
    std::vector<std::unique_ptr<Track>> tracks;double clock=0;
    explicit Impl(NativePlayerAudio&s):session(s){}
    const UCAudioGroup*group(const NativeAudioSound&sound)const{uint16_t index=uint16_t(sound.authored[16]|uint16_t(sound.authored[17])<<8);require(index<bank->groups.size(),"Mission selected sound group");return bank->groups[index]->scalar;}
    NativeAudioInstanceMix mix(const Track*t=nullptr)const{
        if(!t)return session.mix();UCAudioGroupMix group_mix{};
        require(uc_audio_group_mix(group(*t->sound),&group_mix)==UC_AUDIO_OK,"Mission sound live group");
        NativeAudioInstanceMix result{0,t->instance->state.gain,group_mix.gain,0,group_mix.pitch,session.mute_};
        result.track_gain=t->parameter_gain;result.track_pitch=t->parameter_pitch;return result;
    }
    ~Impl(){for(auto&t:tracks)if(t->voice.value)session.device_->destroy_owned_voice(t->voice);}
    void parameters(Track&t){
        require(t.clip!=nullptr,"Mission sound resolved voice resource");
        auto p=native_audio_parameters(*t.sound,t.state,mix(&t),t.clip->pcm().sample_rate);
        session.device_->set_owned_parameters(t.voice,p.gain*t.volume,p.frequency_ratio);session.filter_voice(t.voice,group(*t.sound),t.cutoff);
    }
    struct Dispatch {Impl *source;Track *track;};
    static const void *resolve(void*p,uint16_t i){auto&d=*(Dispatch*)p;d.track->clip=d.source->bank->assets.clips.at(i);return d.track->clip.get();}
    UCAudioImpactBank resources(){UCAudioImpactBank result{};result.selectors=bank->selectors.data();result.selector_count=(uint32_t)bank->selectors.size();return result;}
    void begin(Track&t){
        Dispatch d{this,&t};
        UCAudioEventServices h{&d,
            [](void*,const UCAudioTrackState*,const UCAudioCommand*){return 1;},
            [](void*p){return ((Dispatch*)p)->track->voice.value?1:0;},
            [](void*p,UCAudioTrackState*){auto&d=*(Dispatch*)p;if(d.track->voice.value)d.source->parameters(*d.track);},
            [](void*p,UCAudioTrackState*){auto&d=*(Dispatch*)p;if(d.track->voice.value)d.source->parameters(*d.track);},
            [](void*p,UCAudioTrackState*s,const UCAudioCommand*c){auto&d=*(Dispatch*)p;auto&o=*d.source;auto&t=*d.track;
                auto bank=o.resources();require(!uc_audio_track_resource(s,c,&bank,&o.session.random_,resolve,p),"Mission sound reference/selector");
                if(!s->resource)return;auto clip=t.clip;require(clip&&s->resource==clip.get(),"Mission selected PCM owner");
                auto mix=native_audio_parameters(*t.sound,*s,o.mix(&t),clip->pcm().sample_rate);
                t.voice=o.session.device_->create_owned_voice();
                o.session.device_->prepare_owned_voice(t.voice,clip,mix.gain*t.volume,mix.frequency_ratio,0,clip->pcm().frame_count,c->repeat==0);
                o.session.filter_voice(t.voice,o.group(*t.sound),t.cutoff);s->flags|=0x1000;
            },
            [](void*p){auto&d=*(Dispatch*)p;d.source->session.device_->start_owned_voice(d.track->voice);d.track->started=true;++d.source->session.started_;}};
        require(!uc_audio_timed_execute(&t.state,&t.command,&session.random_,&h),"Mission sound command");t.executed=true;
    }
    void fade(MissionState::Instance*p,uint32_t flags){
        const auto*event=p->event->authored.data();uint16_t index=uint16_t(event[12]|uint16_t(event[13])<<8);const uint8_t*row=nullptr;
        if(index!=65535){for(const auto&v:p->bank->assets.bank.auxiliary())if(v.tag=="xfde"){
            require(v.authored.size()>=4&&index<dword(v.authored.data())&&v.authored.size()==4+size_t(dword(v.authored.data()))*12,"Mission authored fade row");row=v.authored.data()+4+size_t(index)*12;}
            require(row,"Mission fade metadata missing");}
        std::vector<Impl::Track*> selected;for(auto&t:tracks)if(t->instance.get()==p)selected.push_back(t.get());
        require(!uc_audio_fade_begin(&p->state,flags,row,uint32_t(selected.size()),uint64_t(clock),
            [](void*q,uint32_t index,uint64_t when,const uint8_t*command){auto&v=*(std::vector<Impl::Track*>*)q;auto&t=*v.at(index);
                std::memcpy(t.fade.data(),command,44);t.fade_when=when;t.fade_pending=true;return 0;},&selected),"Mission fade schedule");

    }
};
NativePlayerAudio::MissionSource::MissionSource(NativePlayerAudio&s,const UCArchive*a,size_t index,const std::filesystem::path&stream,bool panel,uint32_t panel_groups):impl_(new Impl(s)){
    if(!s.mission_state_)s.mission_state_=std::make_shared<MissionState>(s);
    impl_->bank=s.mission_state_->load(a,index,stream);
    // Panel.amp owns group0; LevelGlobals.amp uses group1 in its three groups.
    if(panel){require(panel_groups==1||panel_groups==3,"Mission Panel group table");
        impl_->panel_group=panel_groups==3?1:0;
        group_seed(a,index,s.panel_,false,panel_groups,impl_->panel_group,"panel");impl_->panel=true;}
}
NativePlayerAudio::MissionSource::~MissionSource()=default;
bool NativePlayerAudio::MissionSource::play(const char*name){
    require(name,"Mission sound name");auto&s=*impl_;int32_t selected=s.bank->assets.select_event(name,0,s.session.random_);++s.session.requests_;
    if(selected<0||selected==65535)return false;
    const auto&sound=s.bank->assets.bank.sounds().at(selected);
    auto instance=std::make_shared<MissionState::Instance>();instance->bank=s.bank;
    instance->event=&s.bank->assets.bank.events().at(size_t(s.bank->assets.bank.find(name)));
    instance->sound=&sound;instance->source=&s;
    instance->stop=[](MissionState::Instance*p){auto&s=*(Impl*)p->source;p->state.phase=7;
        for(auto i=s.tracks.begin();i!=s.tracks.end();)if((*i)->instance.get()==p){
            if((*i)->voice.value)s.session.device_->destroy_owned_voice((*i)->voice);i=s.tracks.erase(i);
        }else ++i;};
    instance->fade=[](MissionState::Instance*p){((Impl*)p->source)->fade(p,0x4000);};
    instance->refresh=[](MissionState::Instance*p){auto&s=*(Impl*)p->source;for(auto&t:s.tracks)if(t->instance.get()==p&&t->voice.value)s.parameters(*t);};
    (void)s.group(sound);
    if(s.panel)require((uint32_t(sound.authored[16])|(uint32_t(sound.authored[17])<<8))==s.panel_group,"Mission Panel sound group");
    require(s.tracks.size()+sound.track_count<=64,"Mission sound track capacity");
    // A bank can contain unrelated programs outside this owner's domain.
    // Validate the entire selected sound before creating its first voice.
    for(uint32_t i=0;i<sound.track_count;i++){
        const auto&b=s.bank->assets.bank.tracks().at(sound.first_track+i).authored;UCAudioTimedCommand decoded{};
        if(!(b.size()>=12&&b[0]==1&&b[1]==0&&std::all_of(b.begin()+4,b.begin()+12,[](uint8_t n){return !n;})&&
            !uc_audio_timed_command_read(b.data()+12,b.size()-12,&decoded)&&decoded.command.bytes==b.size()-12&&
            decoded.command.time<600000&&(decoded.command.opcode!=3||decoded.command.repeat<=1))){
            std::string hex;static constexpr char digits[]="0123456789ABCDEF";for(uint8_t byte:b){hex+=digits[byte>>4];hex+=digits[byte&15];}
            throw std::runtime_error(std::string("Unsupported mission sound track: ")+s.bank->name+" / "+name+" track="+std::to_string(i)+" bytes="+hex);}
        const auto&c=decoded.command;
        auto resident=[&](uint16_t ref){require(ref==65535||(ref<s.bank->assets.clips.size()&&s.bank->assets.clips[ref]),"Mission sound resident reference");};
        if(c.flags&4){
            require(c.reference<s.bank->assets.selectors.size(),"Mission sound selector index");auto payload=s.bank->assets.selectors[c.reference];
            require(payload.size()>=4,"Mission sound selector payload");uint32_t count=dword(payload.data())&32767u;
            require(count&&payload.size()==4+size_t(count)*12,"Mission sound selector rows");
            for(uint32_t row=0;row<count;row++){const auto*p=payload.data()+4+row*12;require(p[2]==255&&p[3]==255,"Mission sound subbank reference");resident(uint16_t(p[0]|uint16_t(p[1])<<8));}
            UCAudioSelectorPayload entry{payload.data(),payload.size()};auto bank=s.resources();auto selectors=s.bank->selectors;selectors[c.reference]=entry;bank.selectors=selectors.data();
            UCAudioTrackState state{};auto rng=s.session.random_;Impl::Track preview;Impl::Dispatch dispatch{&s,&preview};
            require(!uc_audio_track_resource(&state,&c,&bank,&rng,Impl::resolve,&dispatch),"Mission sound selector admission");
        }else resident(c.reference);
    }
    if(!s.session.mission_state_->admit(instance))return false;
    if(instance->state.flags&0x8000)return false;
    s.session.mission_state_->instances.push_back(instance);
    for(uint32_t i=0;i<sound.track_count;i++){
        auto t=std::make_unique<Impl::Track>();t->event=name;t->sound=&sound;t->instance=instance;t->state.subsound=-1;t->state.repeat=1;
        const auto&b=s.bank->assets.bank.tracks().at(sound.first_track+i).authored;UCAudioTrackCommands commands{b.data()+12,b.size()-12,1};
        //250280 schedules admitted tracks once with horizon600000. The existing
        //PC source clock maps these units to milliseconds; spatial output and
        //the shared original engine heap remain separate integration work.
        require(!uc_audio_timed_schedule(&t->state,&commands,0,600000,&s.session.random_,
            [](void*p,UCAudioTrackState*,const UCAudioTimedCommand*c,uint64_t when){auto&t=*(Impl::Track*)p;t.command=*c;t.when=when;},t.get()),"Mission sound schedule");
        if(!t->when&&!(instance->state.flags&0x2000))s.begin(*t);s.tracks.push_back(std::move(t));
    }
    if(instance->state.flags&0x2000){s.fade(instance.get(),0x2000);
        for(auto&t:s.tracks)if(t->instance==instance&&!t->when)s.begin(*t);
    }
    return true;
}
void NativePlayerAudio::MissionSource::stop(const char*name){
    if(!name)return;auto&s=*impl_;
    for(auto it=s.tracks.begin();it!=s.tracks.end();)if(!_stricmp((*it)->event.c_str(),name)){
        if((*it)->voice.value)s.session.device_->destroy_owned_voice((*it)->voice);it=s.tracks.erase(it);
    }else ++it;
}
void NativePlayerAudio::MissionSource::volume(const char*name,float value){
    require(name&&std::isfinite(value)&&value>=0&&value<=1,"Mission sound volume");auto&s=*impl_;
    for(auto&t:s.tracks)if(!_stricmp(t->event.c_str(),name)){t->volume=value;if(t->voice.value)s.parameters(*t);}
}
void NativePlayerAudio::MissionSource::step(float delta){
    require(std::isfinite(delta)&&delta>=0&&delta<=1,"Mission sound delta");auto&s=*impl_;
    s.clock+=(double)delta*1000.;for(auto&t:s.tracks)t->age+=(double)delta*1000.;
    for(;;){Impl::Track*next=nullptr;double earliest=0;
        for(auto&t:s.tracks)if(!t->executed&&double(t->when)<=t->age){double due=double(t->when)-t->age;if(!next||due<earliest){next=t.get();earliest=due;}}
        if(!next)break;s.begin(*next);
    }
    for(;;){Impl::Track*next=nullptr;
        for(auto&t:s.tracks)if(t->fade_pending&&double(t->fade_when)<=s.clock&&(!next||t->fade_when<next->fade_when))next=t.get();
        if(!next)break;struct Fade{Impl*source;Impl::Track*track;}context{&s,next};uint8_t repeat=0;
        require(!uc_audio_fade_tick(&next->instance->state,next->fade.data(),uint64_t(s.clock),
            [](void*p,uint64_t when){auto&c=*(Fade*)p;c.track->fade_when=when;return 0;},
            [](void*p){auto&c=*(Fade*)p;for(auto&t:c.source->tracks)if(t->instance==c.track->instance&&t->voice.value)c.source->parameters(*t);return 0;},
            &context,&repeat),"Mission fade command");next->fade_pending=repeat!=0;
    }
    for(size_t index=0;index<s.tracks.size();){auto instance=s.tracks[index]->instance;
        require(!uc_audio_fade_transition(&instance->state,s.tracks[index]->state.flags,
            [](void*p){auto&v=*(MissionState::Instance*)p;v.stop(&v);return 0;},instance.get()),"Mission fade completion");
        if(instance->state.phase!=7)++index;
    }
    for(auto it=s.tracks.begin();it!=s.tracks.end();){auto&t=**it;
        if(t.executed&&(!t.voice.value||!s.session.device_->owned_voice_state(t.voice).buffers_queued)){
            if(t.voice.value)s.session.device_->destroy_owned_voice(t.voice);it=s.tracks.erase(it);
        }else ++it;
    }
}
size_t NativePlayerAudio::MissionSource::voices()const{return impl_->tracks.size();}


bool NativePlayerAudio::MissionSource::busy()const{
    std::vector<UCAudioBusyInstance> instances;const MissionState::Instance*previous=nullptr;
    for(const auto&t:impl_->tracks)if(t->instance.get()!=previous){previous=t->instance.get();instances.push_back({previous->state.flags,previous->state.phase});}
    uint8_t busy=0;require(!uc_audio_source_busy(instances.data(),uint32_t(instances.size()),&busy),"Mission source busy query");return busy!=0;
}
NativePlayerAudio::~NativePlayerAudio(){stop_all();}
size_t NativePlayerAudio::collect(){
    if(live_)for(auto i=live_->voices.begin();i!=live_->voices.end();)if(!device_->owned_voice_state(i->handle).buffers_queued){device_->destroy_owned_voice(i->handle);i=live_->voices.erase(i);}else ++i;
    return device_->collect();
}
void NativePlayerAudio::stop_all(){
    if(mission_state_){std::vector<std::shared_ptr<MissionState::Instance>> live;for(auto&w:mission_state_->instances)if(auto p=w.lock())live.push_back(p);
        for(auto&p:live)p->stop(p.get());}
    if(live_)live_->voices.clear();if(device_)device_->stop_all();
}
void NativePlayerAudio::filter_voice(NativeAudioDevice::VoiceHandle handle,const UCAudioGroup*group,uint16_t cutoff){
    UCAudioInstance instance{};UCAudioInstanceTrack track{};UCAudioBuffer voice{};UCAudioDeviceBuffer device{};
    instance.group=const_cast<UCAudioGroup*>(group);track.voice=&voice;track.parameters[6]=cutoff;voice.device=&device;device.buffer=&handle;
    struct Context{NativePlayerAudio*self;float one=1,zero=0;}context{this};
    UCAudioLiveMixServices h{};h.context=&context;
    h.preflight=[](void*,const UCAudioInstance*,const UCAudioInstanceTrack*,unsigned){return 0;};
    h.group=[](void*,const void*p){return static_cast<const UCAudioGroup*>(p);};
    h.filter_fields=[](void*p,const UCAudioGroup*g,const float**cut,const float**res){auto&c=*(Context*)p;
        auto*group=c.self->mission_state_?c.self->mission_state_->group(g):nullptr;
        if(group){*cut=&group->cutoff;*res=&group->resonance;return 0;}
        if(g!=&c.self->group_&&g!=&c.self->panel_)return 1;*cut=&c.one;*res=&c.zero;return 0;};
    h.gain=[](void*,void*,uint8_t,int32_t){throw std::runtime_error("Filter requested gain");};
    h.pitch=[](void*,void*,uint8_t,int32_t){throw std::runtime_error("Filter requested pitch");};
    h.filter=[](void*p,void*backend,uint8_t stream,const uint32_t*packet){require(!stream,"Mission filter stream owner");auto&c=*(Context*)p;
        c.self->device_->set_owned_filter(*(NativeAudioDevice::VoiceHandle*)backend,native_audio_device_filter(packet));};
    require(!uc_audio_live_filter(&instance,&track,&h),"Mission live group filter");
}
void NativePlayerAudio::update_live(){
    collect();for(auto&v:live_->voices){auto next=v.mix;UCAudioGroupMix group{};
        require(uc_audio_group_mix((const UCAudioGroup*)next.group,&group)==UC_AUDIO_OK,"Live resident group");next.group_gain=group.gain;next.group_pitch=group.pitch;
        auto parameters=native_audio_parameters(v.sound,v.state,next,v.clip->pcm().sample_rate);device_->set_owned_parameters(v.handle,parameters.gain,parameters.frequency_ratio);
        filter_voice(v.handle,(const UCAudioGroup*)next.group,8000);}
    if(mission_state_)for(auto&w:mission_state_->instances)if(auto p=w.lock())p->refresh(p.get());
}
void NativePlayerAudio::group_pitch(const char*name,float ratio){
    require(name&&std::isfinite(ratio),"Audio group pitch request");if(!mission_state_)mission_state_=std::make_shared<MissionState>(*this);
    if(auto*g=mission_state_->group(name)){require(!uc_audio_bus_pitch(g->scalar,ratio),"Audio group pitch");update_live();}
}
void NativePlayerAudio::group_filter(const char*name,float cutoff,float attack,float hold,float release,float clock){
    require(name&&*name,"Audio group filter name");if(!mission_state_)mission_state_=std::make_shared<MissionState>(*this);
    auto&map=mission_state_->envelopes;auto i=map.find(name);UCAudioBusEnvelope value=i==map.end()?UCAudioBusEnvelope{}:i->second;
    require(!uc_audio_bus_envelope_begin(&value,i==map.end(),clock,cutoff,attack,hold,release),"Audio group filter request");map[name]=value;
}
void NativePlayerAudio::step_groups(float clock){
    require(std::isfinite(clock),"Audio group clock");if(!mission_state_)return;
    for(auto i=mission_state_->envelopes.begin();i!=mission_state_->envelopes.end();){int remove=0;
        require(!uc_audio_bus_envelope_step(&i->second,clock,&remove),"Audio group filter envelope");
        if(auto*g=mission_state_->group(i->first.c_str())){g->cutoff=i->second.current;g->scalar->dirty|=4;}
        if(remove)i=mission_state_->envelopes.erase(i);else ++i;}
    update_live();
}
void NativePlayerAudio::MissionSource::parameter(const char*event,const char*name,float value){
    require(event&&name&&std::isfinite(value),"Mission parameter request");auto&s=*impl_;
    std::shared_ptr<MissionState::Instance> selected;
    for(const auto&t:s.tracks)if(!_stricmp(t->event.c_str(),event)){selected=t->instance;break;}
    if(!selected)return; //216350 finds the first live source-owned matching event.
    const auto&aux=s.bank->assets.bank.auxiliary();auto program=std::find_if(aux.begin(),aux.end(),[](const auto&v){return v.tag=="ampp";});if(program==aux.end())return;
    UCAudioParameterCurve curve{};int found=0;
    require(!uc_audio_parameter_find(program->authored.data(),program->authored.size(),s.bank->policy.strings,s.bank->policy.string_bytes,
        selected->event->sound_index(),name,&curve,&found),"Mission sound parameter metadata");if(!found)return;
    //2161B0 publishes one sample: count0, increment1, value clamped0..1.
    UCAudioParameterResult r{};require(!uc_audio_parameter_curve(&curve,0,1,std::clamp(value,0.f,1.f),&r),"Mission sound parameter curve");
    uint32_t index=0;for(auto&t:s.tracks)if(t->instance==selected){if(index++!=r.track)continue;
        if(r.property==0)t->parameter_gain=r.value;else if(r.property==1)t->parameter_pitch=r.value;else t->cutoff=(uint16_t)r.value;
        if(t->voice.value)s.parameters(*t);break;}
}
std::vector<NativePlayerAudio::VoiceSnapshot> NativePlayerAudio::inspect_live()const{
    std::vector<VoiceSnapshot> out;for(const auto&v:live_->voices){const auto*g=mission_state_?mission_state_->group((const UCAudioGroup*)v.mix.group):nullptr;
        std::string name=g?g->name:v.mix.group==&group_?"game3d":"panel";
        out.push_back({name,device_->owned_voice_parameters(v.handle),device_->owned_voice_filter(v.handle)});}return out;
}
std::vector<NativePlayerAudio::VoiceSnapshot> NativePlayerAudio::MissionSource::inspect()const{
    const auto&s=*impl_;std::vector<VoiceSnapshot> out;for(const auto&t:s.tracks)if(t->voice.value){auto*g=s.session.mission_state_->group(s.group(*t->sound));
        require(g,"Mission group inspection");out.push_back({g->name,s.session.device_->owned_voice_parameters(t->voice),s.session.device_->owned_voice_filter(t->voice)});}return out;
}

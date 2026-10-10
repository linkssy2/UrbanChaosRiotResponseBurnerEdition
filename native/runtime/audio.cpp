#include "audio.h"
#include "../audio_device_bus.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <xaudio2.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <fstream>
#include <map>
#include <stdexcept>

namespace {
uint16_t u16(const uint8_t *p){return uint16_t(p[0]|uint16_t(p[1])<<8);}
uint32_t u32(const uint8_t *p){return u16(p)|uint32_t(u16(p+2))<<16;}
uint8_t lower(uint8_t c){return c>='A'&&c<='Z'?uint8_t(c+32):c;}
bool equal(const char *a,const char *b){
    while(lower(uint8_t(*a))==lower(uint8_t(*b))){if(!*a)return true;++a;++b;}
    return false;
}
void need(bool yes,const char *why){if(!yes)throw std::runtime_error(why);}
void hr(HRESULT result,const char *what){if(FAILED(result))throw std::runtime_error(std::string(what)+" HRESULT="+std::to_string(uint32_t(result)));}
struct Span {const uint8_t *data=nullptr;size_t size=0;};
using Chunks=std::map<std::string,Span>;
size_t riff(const void *source,size_t size,const char *form){
    auto p=static_cast<const uint8_t*>(source);
    need(p&&size>=12&&size<=64u*1024u*1024u,"audio RIFF input/limit");
    need(!memcmp(p,"RIFF",4)&&!memcmp(p+8,form,4),"unsupported audio RIFF form");
    size_t end=size_t(u32(p+4))+8;
    need(end>=12&&end<=size&&size-end<=15,"audio RIFF extent");
    for(size_t i=end;i<size;i++)need(!p[i],"nonzero audio archive padding");
    return end;
}
Chunks chunks(const uint8_t *p,size_t end){
    Chunks result;
    for(size_t at=12;at<end;){
        need(end-at>=8,"audio chunk header");size_t n=u32(p+at+4);
        need(n<=end-at-8&&(n&1)<=end-at-8-n,"audio chunk extent");
        std::string key(reinterpret_cast<const char*>(p+at),4);Span v{p+at+8,n};
        if(key=="LIST"){need(n>=4,"audio LIST header");key.assign(reinterpret_cast<const char*>(v.data),4);}
        need(result.emplace(key,v).second&&result.size()<=64,"duplicate audio chunk/limit");
        at+=8+n+(n&1);
    }
    return result;
}
Span required(const Chunks &c,const char *name){auto i=c.find(name);if(i==c.end())throw std::runtime_error(std::string("missing audio chunk: ")+name);return i->second;}
uint32_t records(Span s,size_t stride){
    need(s.size>=4,"audio record header");uint32_t n=u32(s.data);
    need(n<=65535&&s.size==4+size_t(n)*stride,"audio record count/extent");return n;
}
std::string string_at(Span s,uint32_t at){
    need(at<s.size,"audio string offset");auto p=s.data+at;
    auto end=static_cast<const uint8_t*>(memchr(p,0,s.size-at));
    need(end!=nullptr,"unterminated audio string");return {reinterpret_cast<const char*>(p),size_t(end-p)};
}
uint32_t hash(const std::string &name){uint32_t h=0;for(uint8_t c:name){uint32_t x=lower(c);h+=x*x^(h>>1)^(h*2);}return h;}
struct ArchiveBytes {
    UCArchiveBytes bytes{};
    ArchiveBytes(const UCArchive *a,size_t i){need(uc_archive_read(a,i,UC_AR_LOCAL_PAYLOAD,&bytes)==UC_AR_OK,"audio archive read");}
    ~ArchiveBytes(){uc_archive_bytes_free(&bytes);}
};
}

uint16_t NativeAudioEvent::sound_index() const {return u16(authored.data()+2);}
void NativeAudioBank::load(const void *source,size_t size){
    auto p=static_cast<const uint8_t*>(source);auto c=chunks(p,riff(source,size,"AMPC"));
    for(const auto &v:c)need(v.first=="name"||v.first=="ampz"||v.first=="amct"||v.first=="ampk"||
        v.first=="trkl"||v.first=="ampv"||v.first=="amps"||v.first=="amp3"||v.first=="amph"||
        v.first=="guid"||v.first=="date"||v.first=="amrf"||v.first=="ampw"||v.first=="ampp"||v.first=="xfde","unsupported AMPC chunk");
    Span header=required(c,"amph");
    // Actual Default.amp:24FE60 leaves the event hash absent when ampv is
    // absent;24BCA0/24D2E0 then return an event miss. Admit only this already
    // qualified metadata profile, never a truncated populated bank.
    if(c.size()==5&&c.count("name")&&c.count("ampz")&&c.count("guid")&&c.count("date")){
        const uint8_t empty[16]={0x20,1,0,0,0,0,0,0,0,0,0,0,0,0,12,0};
        need(header.size==sizeof empty&&!memcmp(header.data,empty,sizeof empty)&&
            c.at("guid").size==16&&c.at("date").size==8,"unsupported empty AMPC metadata");
        *this=NativeAudioBank{};return;
    }
    Span names=required(c,"ampz"),events=required(c,"ampv"),
         sounds=required(c,"amps"),tracks=required(c,"trkl");
    auto reference_chunk=c.find("amrf");
    need(header.size==16&&u16(header.data)==0x120,"unsupported AMPC version/header");
    uint32_t buckets=u32(header.data+8);need(buckets&&buckets<=65535,"AMPC hash bucket limit");
    NativeAudioBank next;
    for(const char *name:{"amp3","ampk","ampp","xfde"}){
        auto found=c.find(name);if(found!=c.end())next.auxiliary_.push_back({name,{found->second.data,found->second.data+found->second.size}});
    }
    auto selectors=c.find("ampw");
    if(selectors!=c.end()){
        Span list=selectors->second;
        for(size_t at=4;at<list.size;){
            need(list.size-at>=12&&!memcmp(list.data+at,"ampw",4),"AMPC selector header");
            size_t bytes=u32(list.data+at+4);need(bytes>=4&&bytes<=list.size-at-8,"AMPC selector extent");
            const uint8_t *payload=list.data+at+8;uint32_t packed=u32(payload),count=packed&32767;
            need(count&&bytes==4+size_t(count)*12&&(count==1||(packed>>17)<count),"AMPC selector rows/index");
            next.selectors_.emplace_back(payload,payload+bytes);need(next.selectors_.size()<=65535,"AMPC selector count");at+=8+bytes;
        }
    }
    uint32_t n=records(events,16);
    for(uint32_t i=0;i<n;i++){
        NativeAudioEvent e;memcpy(e.authored.data(),events.data+4+16*i,16);
        e.name=string_at(names,u32(e.authored.data()+4));
        uint32_t selection=u32(e.authored.data()+8);
        need(selection==UINT32_MAX||selection<next.selectors_.size(),"AMPC event selector index");
        need(u16(e.authored.data()+14)==hash(e.name)%buckets,"AMPC authored hash bucket mismatch");
        next.events_.push_back(std::move(e));
    }
    for(size_t at=4;at<tracks.size;){
        need(tracks.size-at>=8&&!memcmp(tracks.data+at,"ampt",4),"AMPC track header");
        uint32_t bytes=u32(tracks.data+at+4);
        need(bytes>=16&&bytes<=tracks.size-at-8,"AMPC track extent");
        NativeAudioTrack t;t.offset=uint32_t(at-4);
        t.authored.assign(tracks.data+at+8,tracks.data+at+8+bytes);next.tracks_.push_back(std::move(t));
        need(next.tracks_.size()<=65535,"AMPC track limit");at+=8+size_t(bytes);
    }
    n=records(sounds,28);
    for(uint32_t i=0;i<n;i++){
        NativeAudioSound s;memcpy(s.authored.data(),sounds.data+4+28*i,28);
        uint32_t offset=u32(s.authored.data());
        auto it=std::find_if(next.tracks_.begin(),next.tracks_.end(),[offset](const NativeAudioTrack &t){return t.offset==offset;});
        s.track_count=s.authored[6]; // 2492F7/249351: original unsigned byte.
        need(it!=next.tracks_.end(),"AMPC sound track offset");
        s.first_track=uint32_t(it-next.tracks_.begin());
        need(s.track_count<=next.tracks_.size()-s.first_track,"AMPC sound track count");
        next.sounds_.push_back(s);
    }
    for(const auto &e:next.events_){
        need(e.sound_index()==UINT16_MAX||e.sound_index()<n,"AMPC event sound index");
        uint32_t selection=u32(e.authored.data()+8);
        if(selection!=UINT32_MAX){const auto&t=next.selectors_[selection];
            for(size_t at=4;at<t.size();at+=12)need(u16(t.data()+at)==UINT16_MAX||u16(t.data()+at)<n,"AMPC selected sound index");
        }
    }
    // 24B760 permits an absent amrf and then visits the bank's resource list.
    // Immutable Medic.amp has an empty list and authored FFFF resource misses.
    // Keep other missing-reference programs outside this admitted profile.
    if(reference_chunk==c.end()){
        need(next.selectors_.empty(),"AMPC absent reference selector profile");
        for(const auto&t:next.tracks_){const auto&r=t.authored;
            need(r.size()==28&&u16(r.data())==1&&r[16]==3&&u16(r.data()+18)==0&&
                u16(r.data()+20)==1&&u16(r.data()+22)==65535&&u16(r.data()+24)==65535&&
                u16(r.data()+26)==65535,"AMPC absent reference track profile");
        }
    }
    Span refs=reference_chunk==c.end()?Span{}:reference_chunk->second;
    n=reference_chunk==c.end()?0:records(refs,20);
    for(uint32_t i=0;i<n;i++){
        NativeAudioGuid g;memcpy(g.data(),refs.data+4+20*i,16);
        need(!u32(refs.data+20+20*i),"AMPC serialized reference cache not empty");next.references_.push_back(g);
    }
    *this=std::move(next);
}
int32_t NativeAudioBank::find(const char *name) const {
    if(name)for(size_t i=0;i<events_.size();i++)if(equal(events_[i].name.c_str(),name))return int32_t(i);
    return -1;
}
NativeAudioClip::NativeAudioClip(const void *source,size_t size){
    auto p=static_cast<const uint8_t*>(source);size_t end=riff(source,size,"WAVE");auto c=chunks(p,end);
    Span id=required(c,"guid");need(id.size==guid_.size(),"resident audio GUID extent");memcpy(guid_.data(),id.data,id.size);
    Span format=required(c,"fmt "),data=required(c,"data");
    need(format.size==format_.size(),"resident audio format extent");
    memcpy(format_.data(),format.data,format.size);encoded_.assign(data.data,data.data+data.size);
    need(uc_audio_asset_load(source,end,&pcm_)==UC_AUDIO_OK,"unsupported/invalid resident audio codec");
}
NativeAudioClip::NativeAudioClip(const void*descriptor,size_t size,const void*span,size_t bytes){
    auto*p=static_cast<const uint8_t*>(span);need(p&&bytes>=12&&bytes<=64u*1024u*1024u,"Audio stream span");
    size_t end=size_t(u32(p+4))+8;need(end>=12&&end<=bytes,"Audio stream RIFF extent");
    auto c=chunks(p,end);Span id=required(c,"guid"),format=required(c,"fmt "),data=required(c,"data");
    need(id.size==guid_.size()&&format.size==format_.size(),"Audio stream metadata extent");
    memcpy(guid_.data(),id.data,id.size);memcpy(format_.data(),format.data,format.size);
    encoded_.assign(data.data,data.data+data.size);
    need(uc_audio_stream_asset_load(descriptor,size,span,bytes,&pcm_)==UC_AUDIO_OK,"Audio stream descriptor/span mismatch");
}
void NativeAudioBankAssets::load(const UCArchive *a,size_t index,const std::vector<const UCArchive*> &archives,const std::vector<uint16_t> &requested,const std::filesystem::path&stream_backing){
    need(a&&!archives.empty(),"audio archive owners required");ArchiveBytes bytes(a,index);NativeAudioBankAssets next;
    next.bank.load(bytes.bytes.data,bytes.bytes.size);next.clips.resize(next.bank.references().size());next.selectors=next.bank.selectors();
    for(const auto&e:next.bank.events())next.event_state.push_back(e.authored);
    std::vector<uint8_t> needed(next.clips.size(),uint8_t(requested.empty()));
    for(uint16_t ref:requested){need(ref<needed.size(),"Requested audio reference out of bounds");needed[ref]=1;}
    std::map<NativeAudioGuid,std::vector<uint8_t>> found;
    std::ifstream backing;uint64_t backing_size=0;
    if(!stream_backing.empty()){
        backing.open(stream_backing,std::ios::binary|std::ios::ate);need(backing.good(),"Audio stream backing unavailable");
        auto end=backing.tellg();need(end>=0,"Audio stream backing size");backing_size=uint64_t(end);
    }
    for(const UCArchive *archive:archives){
        need(archive!=nullptr,"null audio archive owner");
        for(size_t i=0;i<uc_archive_count(archive);i++){
            const auto *e=uc_archive_entry(archive,i);std::string name(reinterpret_cast<const char*>(e->name),e->name_bytes-1);
            if(name.size()<4||!equal(name.c_str()+name.size()-4,".wav"))continue;
            ArchiveBytes wav(archive,i);auto p=wav.bytes.data;
            if(wav.bytes.size<12||memcmp(p,"RIFF",4))continue;
            bool stream=!memcmp(p+8,"WAVS",4);
            if((stream&&stream_backing.empty())||(!stream&&memcmp(p+8,"WAVE",4)))continue;
            size_t end=riff(p,wav.bytes.size,stream?"WAVS":"WAVE");auto c=chunks(p,end);auto id=c.find("guid");
            if(id==c.end())continue;need(id->second.size==16,"resident audio GUID extent");
            NativeAudioGuid g;memcpy(g.data(),id->second.data,16);
            bool wanted=false;for(size_t k=0;k<needed.size();k++)wanted|=needed[k]&&next.bank.references()[k]==g;
            if(!wanted)continue;
            std::vector<uint8_t> raw(p,p+end);auto prior=found.find(g);
            if(prior!=found.end()){need(prior->second==raw,"ambiguous resident audio GUID");continue;}
            std::shared_ptr<NativeAudioClip> clip;
            if(stream){UCAudioStreamInfo info{};
                need(uc_audio_stream_info(p,end,&info)==UC_AUDIO_OK,"Invalid stream descriptor");
                need(uint64_t(info.file_offset)+info.file_bytes<=backing_size,"Audio stream backing extent");
                std::vector<uint8_t> span(info.file_bytes);backing.seekg(info.file_offset);backing.read(reinterpret_cast<char*>(span.data()),info.file_bytes);
                need(backing.good(),"Audio stream backing read");clip=std::make_shared<NativeAudioClip>(p,end,span.data(),span.size());
            }else clip=std::make_shared<NativeAudioClip>(p,wav.bytes.size);
            found.emplace(g,std::move(raw));
            for(size_t k=0;k<next.clips.size();k++)if(needed[k]&&next.bank.references()[k]==g)next.clips[k]=clip;
        }
    }
    for(size_t k=0;k<next.clips.size();k++)need(!needed[k]||next.clips[k]!=nullptr,"unresolved resident audio GUID");
    *this=std::move(next);
}

static_assert(XAUDIO2_MIN_FREQ_RATIO==1.f/1024.f&&XAUDIO2_MAX_FREQ_RATIO==1024.f,"qualified owned pitch range");
struct NativeAudioDevice::Impl:IXAudio2EngineCallback {
    struct Bus {IXAudio2SubmixVoice*voice=nullptr;uint64_t token=0;uint32_t references=0;bool owner=false;};
    struct Voice {IXAudio2SourceVoice *voice=nullptr;std::shared_ptr<const NativeAudioClip> clip;
        uint64_t token=0;bool owned=false,started=false;float maximum_ratio=4.f;
        Bus*output=nullptr;uint32_t headroom=0;std::vector<float> master_matrix;
        XAUDIO2_FILTER_PARAMETERS filter{LowPassFilter,1.f,1.f};bool filter_set=false;};
    IXAudio2 *engine=nullptr;IXAudio2MasteringVoice *master=nullptr;
    IXAudio2SubmixVoice *speaker_bed=nullptr;std::array<Bus,64>buses{};
    std::array<Voice,64> voices{};DWORD thread=GetCurrentThreadId();bool com=false;
    std::atomic<HRESULT> error{S_OK};bool registered=false;
    uint64_t next_token=1;
    void STDMETHODCALLTYPE OnProcessingPassStart() override {}
    void STDMETHODCALLTYPE OnProcessingPassEnd() override {}
    void STDMETHODCALLTYPE OnCriticalError(HRESULT result) override {error.store(result);}
    void check_thread() const {need(thread==GetCurrentThreadId(),"audio device called on another thread");hr(error.load(),"XAudio2 asynchronous device failure");}
    Voice &owned_voice(VoiceHandle token){check_thread();need(token.device==this,"owned audio voice belongs to another device");
        auto it=std::find_if(voices.begin(),voices.end(),[&](const Voice&v){return v.owned&&v.token==token.value;});
        need(token.value&&it!=voices.end()&&it->voice,"invalid native audio owned voice");return *it;}
    Bus&bus(BusHandle token){check_thread();need(token.device==this,"audio bus belongs to another device");auto it=std::find_if(buses.begin(),buses.end(),[&](const Bus&b){return b.token==token.value;});need(token.value&&it!=buses.end()&&it->voice,"invalid native audio bus");return *it;}
    static void unref(Bus*b){if(b&&!--b->references){b->voice->DestroyVoice();*b=Bus{};}}
    uint32_t master_channels()const{XAUDIO2_VOICE_DETAILS d{};master->GetVoiceDetails(&d);return d.InputChannels;}
    uint32_t master_rate()const{XAUDIO2_VOICE_DETAILS d{};master->GetVoiceDetails(&d);return d.InputSampleRate;}
    static uint32_t channels(IXAudio2SourceVoice*v){XAUDIO2_VOICE_DETAILS d{};v->GetVoiceDetails(&d);return d.InputChannels;}
    void matrix(IXAudio2SourceVoice*v,Bus*b,uint32_t headroom,std::vector<float>&base){
        auto n=channels(v);need(n==1||n==2,"audio bus source channel count");
        if(b){float level=float(std::pow(10.,-(double(headroom)+(n==2?602.:0.))/2000.));float values[2]={level,level};hr(v->SetOutputMatrix(b->voice,n,1,values),"Set bus input matrix");}
        else {auto outputs=master_channels();if(base.empty()){base.resize(n*outputs);v->GetOutputMatrix(master,n,outputs,base.data());}auto values=base;float level=float(std::pow(10.,-double(headroom)/2000.));for(auto&x:values)x*=level;hr(v->SetOutputMatrix(master,n,outputs,values.data()),"Set master headroom matrix");}
    }
    void route(Voice&v,Bus*next){
        Bus*old=v.output;if(next==old)return;need(!v.started,"stop voice before changing output");
        need(!v.filter_set||next||master_rate()==48000,"filtered voice requires48k output");
        if(next)++next->references;XAUDIO2_SEND_DESCRIPTOR send{0,next?next->voice:nullptr};XAUDIO2_VOICE_SENDS sends{1,&send};
        try{hr(v.voice->SetOutputVoices(next?&sends:nullptr),"Set owned output voice");}catch(...){unref(next);throw;}
        v.output=next;v.master_matrix.clear();unref(old);matrix(v.voice,next,v.headroom,v.master_matrix);
    }
    ~Impl(){for(auto &v:voices)if(v.voice)v.voice->DestroyVoice();for(auto&b:buses)if(b.voice)b.voice->DestroyVoice();if(speaker_bed)speaker_bed->DestroyVoice();if(master)master->DestroyVoice();if(engine){if(registered)engine->UnregisterForCallbacks(this);engine->Release();}if(com)CoUninitialize();}
};
NativeAudioDevice::NativeAudioDevice():impl_(std::make_unique<Impl>()){
    HRESULT r=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
    if(r!=RPC_E_CHANGED_MODE){hr(r,"COM audio initialization");impl_->com=true;}
    hr(XAudio2Create(&impl_->engine,0,XAUDIO2_DEFAULT_PROCESSOR),"XAudio2Create");
    hr(impl_->engine->RegisterForCallbacks(impl_.get()),"RegisterForCallbacks");impl_->registered=true;
    hr(impl_->engine->CreateMasteringVoice(&impl_->master),"CreateMasteringVoice");
}
NativeAudioDevice::~NativeAudioDevice(){if(impl_->thread!=GetCurrentThreadId())std::terminate();}
void NativeAudioDevice::play(std::shared_ptr<const NativeAudioClip> clip,float gain,float ratio){
    impl_->check_thread();need(clip&&std::isfinite(gain)&&gain>=0&&gain<=1&&std::isfinite(ratio)&&ratio>=.25f&&ratio<=4,"audio one-shot parameters");
    const auto &pcm=clip->pcm();need(!pcm.loop_frame_count,"looped resource needs authored loop policy");
    collect();auto it=std::find_if(impl_->voices.begin(),impl_->voices.end(),[](const Impl::Voice &v){return !v.voice;});
    need(it!=impl_->voices.end(),"native audio voice capacity");
    WAVEFORMATEX f{};f.wFormatTag=WAVE_FORMAT_PCM;f.nChannels=WORD(pcm.channels);f.nSamplesPerSec=pcm.sample_rate;
    f.wBitsPerSample=16;f.nBlockAlign=WORD(pcm.channels*2);f.nAvgBytesPerSec=pcm.sample_rate*f.nBlockAlign;
    IXAudio2SourceVoice *voice=nullptr;hr(impl_->engine->CreateSourceVoice(&voice,&f,0,4),"CreateSourceVoice");
    try{
        hr(voice->SetVolume(gain),"SetVolume");hr(voice->SetFrequencyRatio(ratio),"SetFrequencyRatio");
        XAUDIO2_BUFFER b{};b.Flags=XAUDIO2_END_OF_STREAM;b.AudioBytes=pcm.frame_count*pcm.channels*2;
        b.pAudioData=reinterpret_cast<const BYTE*>(pcm.samples);
        hr(voice->SubmitSourceBuffer(&b),"SubmitSourceBuffer");hr(voice->Start(),"Start source voice");
    }catch(...){voice->DestroyVoice();throw;}
    it->voice=voice;it->clip=std::move(clip);
}
size_t NativeAudioDevice::collect(){
    impl_->check_thread();for(auto &v:impl_->voices)if(v.voice&&!v.owned){XAUDIO2_VOICE_STATE state{};v.voice->GetState(&state,XAUDIO2_VOICE_NOSAMPLESPLAYED);
        if(!state.BuffersQueued){v.voice->DestroyVoice();v.voice=nullptr;v.clip.reset();}}
    return active();
}
void NativeAudioDevice::stop_all(){impl_->check_thread();for(auto &v:impl_->voices)if(v.voice){v.voice->DestroyVoice();Impl::unref(v.output);v=Impl::Voice{};}}
size_t NativeAudioDevice::active() const {impl_->check_thread();return size_t(std::count_if(impl_->voices.begin(),impl_->voices.end(),[](const Impl::Voice &v){return v.voice!=nullptr;}));}
NativeAudioDevice::VoiceHandle NativeAudioDevice::create_owned_voice(float maximum){
    impl_->check_thread();need(std::isfinite(maximum)&&maximum>=1&&maximum<=XAUDIO2_MAX_FREQ_RATIO,"owned voice maximum frequency");collect();auto it=std::find_if(impl_->voices.begin(),impl_->voices.end(),[](const Impl::Voice&v){return !v.voice;});
    need(it!=impl_->voices.end()&&impl_->next_token,"native audio owned voice capacity");
    WAVEFORMATEX f{};f.wFormatTag=WAVE_FORMAT_PCM;f.nChannels=2;f.nSamplesPerSec=48000;f.wBitsPerSample=16;f.nBlockAlign=4;f.nAvgBytesPerSec=192000;
    IXAudio2SourceVoice*voice=nullptr;hr(impl_->engine->CreateSourceVoice(&voice,&f,XAUDIO2_VOICE_USEFILTER,maximum),"Create owned source voice");
    it->voice=voice;it->owned=true;it->maximum_ratio=maximum;it->token=impl_->next_token++;return {it->token,impl_.get()};
}
void NativeAudioDevice::prepare_owned_voice(VoiceHandle token,std::shared_ptr<const NativeAudioClip>clip,float gain,float ratio,uint32_t first,uint32_t frames,bool loop){
    auto&v=impl_->owned_voice(token);need(clip&&std::isfinite(gain)&&gain>=0&&gain<=1&&std::isfinite(ratio)&&ratio>=XAUDIO2_MIN_FREQ_RATIO&&ratio<=v.maximum_ratio,"owned audio parameters");
    const auto&p=clip->pcm();need((loop||!p.loop_frame_count)&&p.channels>=1&&p.channels<=2&&frames&&first<=p.frame_count&&frames<=p.frame_count-first,"owned audio resident region");
    const uint32_t loop_first=p.loop_frame_count?p.loop_start_frame:first,loop_frames=p.loop_frame_count?p.loop_frame_count:frames;
    need(!loop||(loop_first>=first&&loop_first-first<frames&&loop_frames<=frames-(loop_first-first)),"owned audio loop outside play region");
    XAUDIO2_VOICE_STATE state{};v.voice->GetState(&state,XAUDIO2_VOICE_NOSAMPLESPLAYED);need(!state.BuffersQueued,"owned audio voice still queued");
    WAVEFORMATEX f{};f.wFormatTag=WAVE_FORMAT_PCM;f.nChannels=WORD(p.channels);f.nSamplesPerSec=p.sample_rate;f.wBitsPerSample=16;f.nBlockAlign=WORD(p.channels*2);f.nAvgBytesPerSec=p.sample_rate*f.nBlockAlign;
    IXAudio2SourceVoice*voice=nullptr;hr(impl_->engine->CreateSourceVoice(&voice,&f,XAUDIO2_VOICE_USEFILTER,v.maximum_ratio),"Prepare owned source voice");
    std::vector<float> master_matrix;
    try{hr(voice->SetVolume(gain),"Set owned volume");hr(voice->SetFrequencyRatio(ratio),"Set owned frequency");
        if(v.output){XAUDIO2_SEND_DESCRIPTOR send{0,v.output->voice};XAUDIO2_VOICE_SENDS sends{1,&send};hr(voice->SetOutputVoices(&sends),"Restore owned output voice");}
        if(v.output||v.headroom)impl_->matrix(voice,v.output,v.headroom,master_matrix);
        if(v.filter_set)hr(voice->SetFilterParameters(&v.filter),"Restore owned filter");
        XAUDIO2_BUFFER b{};b.Flags=XAUDIO2_END_OF_STREAM;b.AudioBytes=p.frame_count*p.channels*2;b.pAudioData=reinterpret_cast<const BYTE*>(p.samples);b.PlayBegin=first;b.PlayLength=frames;
        if(loop){b.LoopBegin=loop_first;b.LoopLength=loop_frames;b.LoopCount=XAUDIO2_LOOP_INFINITE;}
        hr(voice->SubmitSourceBuffer(&b),"Submit owned source buffer");
    }catch(...){voice->DestroyVoice();throw;}
    v.voice->DestroyVoice();v.voice=voice;v.clip=std::move(clip);v.started=false;v.master_matrix=std::move(master_matrix);
}
void NativeAudioDevice::set_owned_parameters(VoiceHandle token,float gain,float ratio){
    auto&v=impl_->owned_voice(token);need(std::isfinite(gain)&&gain>=0&&gain<=1&&std::isfinite(ratio)&&ratio>=XAUDIO2_MIN_FREQ_RATIO&&ratio<=v.maximum_ratio,"owned audio parameters");
    hr(v.voice->SetVolume(gain),"Set owned volume");hr(v.voice->SetFrequencyRatio(ratio),"Set owned frequency");
}
NativeAudioDevice::VoiceParameters NativeAudioDevice::owned_voice_parameters(VoiceHandle token)const{
    auto&v=impl_->owned_voice(token);VoiceParameters out{};v.voice->GetVolume(&out.gain);v.voice->GetFrequencyRatio(&out.frequency_ratio);return out;
}
void NativeAudioDevice::start_owned_voice(VoiceHandle token){auto&v=impl_->owned_voice(token);XAUDIO2_VOICE_STATE s{};v.voice->GetState(&s,XAUDIO2_VOICE_NOSAMPLESPLAYED);
    need(v.clip&&s.BuffersQueued,"owned audio voice has no queued PCM");hr(v.voice->Start(),"Start owned source voice");v.started=true;}
void NativeAudioDevice::stop_owned_voice(VoiceHandle token){auto&v=impl_->owned_voice(token);hr(v.voice->Stop(),"Stop owned source voice");hr(v.voice->FlushSourceBuffers(),"Flush owned source voice");v.started=false;}
NativeAudioDevice::VoiceState NativeAudioDevice::owned_voice_state(VoiceHandle token)const{auto&v=impl_->owned_voice(token);XAUDIO2_VOICE_STATE s{};v.voice->GetState(&s);return{s.BuffersQueued,s.SamplesPlayed,v.started};}
void NativeAudioDevice::destroy_owned_voice(VoiceHandle token){auto&v=impl_->owned_voice(token);v.voice->DestroyVoice();Impl::unref(v.output);v=Impl::Voice{};}
NativeAudioDevice::BusHandle NativeAudioDevice::create_owned_bus(const UCAudioDeviceBusDescriptor&d){
    impl_->check_thread();static const uint32_t bins[6]={0,4,1,5,2,3};
    need(!d.size&&d.flags==0x2000&&!d.buffer_bytes&&!d.format&&!d.input_mix_bin&&d.mix_bins&&d.mix_bins->count==6&&d.mix_bins->pairs,"unsupported nonspatial audio bus descriptor");
    for(unsigned i=0;i<6;i++)need(d.mix_bins->pairs[i].index==bins[i]&&d.mix_bins->pairs[i].volume==(i==5?-1200:0),"unsupported audio bus send level");
    auto it=std::find_if(impl_->buses.begin(),impl_->buses.end(),[](const Impl::Bus&b){return !b.voice;});need(it!=impl_->buses.end()&&impl_->next_token,"native audio bus capacity");
    if(!impl_->speaker_bed)hr(impl_->engine->CreateSubmixVoice(&impl_->speaker_bed,6,48000,0,1),"Create PC speaker bed");
    XAUDIO2_SEND_DESCRIPTOR send{0,impl_->speaker_bed};XAUDIO2_VOICE_SENDS sends{1,&send};IXAudio2SubmixVoice*voice=nullptr;
    hr(impl_->engine->CreateSubmixVoice(&voice,1,48000,0,0,&sends),"Create mono48k audio bus");
    float levels[6]={1,1,1,float(std::pow(10.,-.6)),1,1};
    try{hr(voice->SetOutputMatrix(impl_->speaker_bed,1,6,levels),"Set six speaker bus levels");}catch(...){voice->DestroyVoice();throw;}
    it->voice=voice;it->token=impl_->next_token++;it->references=1;it->owner=true;return{it->token,impl_.get()};
}
void NativeAudioDevice::set_owned_filter(VoiceHandle token,VoiceFilter f){
    auto&v=impl_->owned_voice(token);need(!f.type&&std::isfinite(f.frequency)&&f.frequency>=1.f/256.f&&f.frequency<=XAUDIO2_MAX_FILTER_FREQUENCY&&f.one_over_q==1.f,"owned filter profile");
    need(v.output||impl_->master_rate()==48000,"owned filter requires48k output");
    XAUDIO2_FILTER_PARAMETERS p{LowPassFilter,f.frequency,f.one_over_q};hr(v.voice->SetFilterParameters(&p),"Set owned filter");v.filter=p;v.filter_set=true;
}
NativeAudioDevice::VoiceFilter NativeAudioDevice::owned_voice_filter(VoiceHandle token)const{
    auto&v=impl_->owned_voice(token);XAUDIO2_FILTER_PARAMETERS p{};v.voice->GetFilterParameters(&p);return{p.Frequency,p.OneOverQ,(uint32_t)p.Type};
}
void NativeAudioDevice::release_owned_bus(BusHandle token){auto&b=impl_->bus(token);need(b.owner,"audio bus owner already released");b.owner=false;Impl::unref(&b);}
NativeAudioDevice::BusState NativeAudioDevice::owned_bus_state(BusHandle token)const{auto&b=impl_->bus(token);XAUDIO2_VOICE_DETAILS d{};b.voice->GetVoiceDetails(&d);BusState out{b.references,d.InputChannels,d.InputSampleRate,b.owner,{}};b.voice->GetOutputMatrix(impl_->speaker_bed,1,6,out.speaker_matrix.data());return out;}
void NativeAudioDevice::route_owned_voice(VoiceHandle token,BusHandle destination){auto&v=impl_->owned_voice(token);need(destination.value||!destination.device,"invalid null audio bus handle");impl_->route(v,destination.value?&impl_->bus(destination):nullptr);}
void NativeAudioDevice::set_owned_headroom(VoiceHandle token,uint32_t level){auto&v=impl_->owned_voice(token);need(level<=10000,"audio headroom range");impl_->matrix(v.voice,v.output,level,v.master_matrix);v.headroom=level;}
NativeAudioDevice::VoiceRouting NativeAudioDevice::owned_voice_routing(VoiceHandle token)const{auto&v=impl_->owned_voice(token);auto n=Impl::channels(v.voice);auto outputs=v.output?1:impl_->master_channels();VoiceRouting out{{v.output?v.output->token:0,v.output?impl_.get():nullptr},v.headroom,n,std::vector<float>(n*outputs)};v.voice->GetOutputMatrix(v.output?static_cast<IXAudio2Voice*>(v.output->voice):impl_->master,n,outputs,out.matrix.data());return out;}

void NativeAudioDevice::output_gain(float value){impl_->check_thread();need(std::isfinite(value)&&value>=0&&value<=1,"PC output gain range");hr(impl_->master->SetVolume(value),"Set PC output gain");}
float NativeAudioDevice::output_gain()const{impl_->check_thread();float value;impl_->master->GetVolume(&value);return value;}
void NativeAudioDevice::pause(bool paused){impl_->check_thread();if(paused)impl_->engine->StopEngine();else hr(impl_->engine->StartEngine(),"Resume PC audio");}

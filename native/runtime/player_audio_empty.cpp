#include "player_audio.h"
extern "C" {
#include "../audio_source_controller.h"
#include "../audio_source_engine.h"
}
#include <algorithm>
#include <cstring>
#include <cmath>
#include <stdexcept>
#include <xmmintrin.h>

namespace {
void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
uint32_t word(const uint8_t *p){uint32_t value;std::memcpy(&value,p,4);return value;}
bool empty_bank(const uint8_t *p,size_t size){
    // The actual Default.amp metadata profile. No ampv means24FE60 never
    // creates the event hash;24BCA0/24D2E0 consequently returnFFFFFFFF.
    const uint8_t header[16]={0x20,1,0,0,0,0,0,0,0,0,0,0,0,0,12,0};
    if(!p||size<12||std::memcmp(p,"RIFF",4)||std::memcmp(p+8,"AMPC",4)||size_t(word(p+4))+8!=size)return false;
    unsigned seen=0;
    for(size_t at=12;at<size;){
        if(size-at<8)return false;const size_t bytes=word(p+at+4);if(bytes>size-at-8)return false;
        const unsigned kind=!std::memcmp(p+at,"name",4)?1:!std::memcmp(p+at,"ampz",4)?2:
            !std::memcmp(p+at,"amph",4)?4:!std::memcmp(p+at,"guid",4)?8:!std::memcmp(p+at,"date",4)?16:0;
        if(!kind||(seen&kind))return false;seen|=kind;
        if(kind==4&&(bytes!=16||std::memcmp(p+at+8,header,16)))return false;
        if((kind==8&&bytes!=16)||(kind==16&&bytes!=8))return false;
        at+=8+bytes+(bytes&1);if(at>size)return false;
    }
    return seen==31;
}
bool isolated(const UCAudioLink &p){return p.next==&p&&p.previous==&p;}
}
struct NativePlayerAudio::EmptySource {
    UCAudioSourceController controller{};
    std::unique_ptr<UCAudioSourceEngine> engine;
};
struct NativePlayerAudio::EmptyState {
    NativePlayerAudio *audio;
    std::vector<uint8_t> bank;
    UCAudioLink registry{};
    std::vector<std::unique_ptr<EmptySource>> sources;
    explicit EmptyState(NativePlayerAudio *owner):audio(owner){registry.next=registry.previous=&registry;}
    EmptySource &get(const EmptySource *p)const{
        const auto i=std::find_if(sources.begin(),sources.end(),[p](const auto &s){return s.get()==p;});
        require(i!=sources.end(),"Empty audio helper is not owned by this session");return **i;
    }
    UCAudioSourceEngineServices engine_services(){
        return {this,[](void *,const UCAudioSourceEngine *,unsigned){return 0;},
            [](void *p)->void*{return static_cast<EmptyState*>(p)->audio;},
            [](void *,void *)->void*{return nullptr;},
            [](void *,void *)->double{throw std::runtime_error("No listener interface in nonspatial player audio");},
            [](void *p,void *engine)->UCAudioLink*{auto &s=*static_cast<EmptyState*>(p);require(engine==s.audio,"Audio registry owner");return &s.registry;},
            nullptr,[](void *,const UCAudioBuffer *)->uint32_t{throw std::runtime_error("Empty bank unexpectedly owns a spatial buffer");},
            nullptr,[](void *,UCAudioBuffer *,uint32_t,const void *)->int{throw std::runtime_error("Empty bank unexpectedly updates a buffer");}};
    }
    UCAudioSourceServices services(){
        return {this,[](void *p,const UCAudioSourceController *c,const UCAudioSourceBank *b)->int{
                auto &s=*static_cast<EmptyState*>(p);bool found=false;for(const auto &v:s.sources)found|=&v->controller==c;
                return !found||(b&&(b->identity!=s.bank.data()||b->type!=104));},
            [](void *p)->void*{return static_cast<EmptyState*>(p)->audio;},
            [](void *p,void *audio)->void*{auto &s=*static_cast<EmptyState*>(p);require(audio==s.audio,"Audio factory owner");return p;},
            [](void *p,void *factory)->void*{
                auto &s=*static_cast<EmptyState*>(p);require(factory==p,"Audio source factory identity");
                EmptySource *target=nullptr;for(auto &v:s.sources)if(v->controller.busy&&!v->controller.engine){require(!target,"Reentrant empty audio source creation");target=v.get();}
                require(target&&!target->engine,"Audio source creation owner");auto engine=std::make_unique<UCAudioSourceEngine>();auto h=s.engine_services();
                require(!uc_audio_source_engine_begin(engine.get(),&h),"Audio source registration failed");target->engine=std::move(engine);return target->engine.get();},
            [](void *p,void *engine,const float *point){auto &s=*static_cast<EmptyState*>(p);auto h=s.engine_services();return uc_audio_source_engine_position(static_cast<UCAudioSourceEngine*>(engine),point,&h);},
            [](void *,void *engine){return uc_audio_source_engine_enable(static_cast<UCAudioSourceEngine*>(engine));},
            [](void *p,const UCAudioSourceBank *b,const char *)->uint32_t{auto &s=*static_cast<EmptyState*>(p);require(b->identity==s.bank.data()&&b->type==104,"Audio empty resource identity");return UINT32_MAX;},
            [](void *,void *,const UCAudioSourceBank *,uint32_t)->int{throw std::runtime_error("Empty audio bank unexpectedly issued a sound request");},
            [](void *p,void *engine){
                auto &s=*static_cast<EmptyState*>(p);EmptySource *target=nullptr;for(auto &v:s.sources)if(v->engine.get()==engine)target=v.get();
                require(target&&target->engine->references==1&&isolated(target->engine->buffers)&&isolated(target->engine->pending),"Audio source is not quiescent");
                //245A80 decrements the one real reference before247050 deletes.
                --target->engine->references;auto h=s.engine_services();if(uc_audio_source_engine_end(target->engine.get(),&h))return 2;
                target->engine.reset();return 0;}};
    }
    void close(EmptySource *p){
        auto &s=get(p);auto h=services();require(!uc_audio_source_end(&s.controller,&h),"Empty source cleanup failed");
        sources.erase(std::find_if(sources.begin(),sources.end(),[p](const auto &v){return v.get()==p;}));
    }
    ~EmptyState(){while(!sources.empty())close(sources.back().get());}
};
NativePlayerAudio::EmptySource *NativePlayerAudio::create_empty_source(const void *bytes,size_t size,const float point[3],bool attached){
    const auto *data=static_cast<const uint8_t*>(bytes);require(point&&empty_bank(data,size),"Unsupported empty audio bank");
    if(!empty_state_)empty_state_=std::make_shared<EmptyState>(this);
    auto &s=*empty_state_;require(s.sources.size()<4096,"Empty source capacity");
    if(s.bank.empty())s.bank.assign(data,data+size);else require(s.bank.size()==size&&!std::memcmp(s.bank.data(),data,size),"Distinct empty bank resource not admitted");
    auto source=std::make_unique<EmptySource>();require(!uc_audio_source_begin(&source->controller,point),"Empty helper construction");
    if(attached)source->controller.position.attached_position=point; //216100 actor+40, same borrowed position owner.
    auto *result=source.get();s.sources.push_back(std::move(source));return result;
}
bool NativePlayerAudio::play_empty_source(EmptySource *source,const char *event){
    require(empty_state_&&event&&!(_mm_getcsr()&0xe040),"Empty helper request/FP mode");auto &s=*empty_state_;auto &v=s.get(source);
    UCAudioSourceBank bank{s.bank.data(),104};auto h=s.services();uint8_t accepted=255;
    int status;try{status=uc_audio_source_play(&v.controller,&bank,event,&h,&accepted);}catch(...){v.controller.busy=0;throw;}
    require(!status,"Empty helper request failed");
    ++requests_;return accepted!=0;
}
bool NativePlayerAudio::empty_source_busy(const EmptySource *source)const{
    require(empty_state_!=nullptr,"Empty helper state");const auto &s=empty_state_->get(source);
    if(!s.controller.engine)return false;
    require(s.engine.get()==s.controller.engine&&isolated(s.engine->pending)&&isolated(s.engine->buffers),"Empty helper has unadmitted live instances");
    //2167C0→245E00 enumerates actual pending then each buffer's instance ring.
    // Both authoritative rings are empty, so there is no first entry.
    return false;
}
void NativePlayerAudio::close_empty_source(EmptySource *source){require(empty_state_!=nullptr,"Empty helper state");empty_state_->close(source);}
void NativePlayerAudio::tick_empty_source(EmptySource *source,float delta){
    require(empty_state_&&std::isfinite(delta)&&delta>0&&!(_mm_getcsr()&0xe040),"Empty helper tick/FP domain");
    auto &state=*empty_state_;auto &s=state.get(source);
    require(s.controller.active&&!s.controller.busy&&!s.controller.flag25,"Empty helper tick lifetime/velocity mode");
    float point[3];const auto&position=s.controller.position;
    std::memcpy(point,position.attached_position?position.attached_position:position.position,12);
    for(unsigned i=0;i<3;i++)require(std::isfinite(point[i]),"Finite empty helper position");
    if(s.controller.engine){
        require(s.engine.get()==s.controller.engine&&isolated(s.engine->buffers)&&isolated(s.engine->pending),"Empty helper tick rings");
        auto h=state.engine_services();require(!uc_audio_source_engine_position(s.engine.get(),point,&h),"Empty source tick position");
        //216850 computes(point-old)/delta, then245CD0 visits the SAME buffer
        //ring. It is actually empty: no velocity property receiver is called.
        //FP sticky status and callback inspection of discarded temporary
        //velocity are excluded; no buffer exists to receive that value.
    }
    std::memcpy(s.controller.position.position,point,12);
}
size_t NativePlayerAudio::empty_source_count()const{return empty_state_?empty_state_->sources.size():0;}
const UCAudioSourceEngine *NativePlayerAudio::empty_source_engine(const EmptySource *source)const{
    require(empty_state_!=nullptr,"Empty helper state");return empty_state_->get(source).engine.get();
}

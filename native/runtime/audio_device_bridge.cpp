#include "audio_device_bridge.h"
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <xmmintrin.h>
static void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
float native_audio_device_gain(int32_t value){
    require(!(_mm_getcsr()&0xe040)&&value>=-10000&&value<=0,"resident gain domain");
    return value==0?1.f:value==-10000?0.f:(float)std::pow(10.,(double)value*(double)0.0005f);
}
float native_audio_device_ratio(int32_t value,uint32_t rate){
    require(!(_mm_getcsr()&0xe040)&&value>=-32767&&value<=8191&&rate>=188&&rate<=191983,"resident pitch domain");
    float ratio=(float)(std::pow(2.,(double)value/4096.)*48000./(double)rate);
    require(std::isfinite(ratio)&&ratio>=1.f/1024.f&&ratio<=1024.f,"resident pitch exceeds PC voice range");return ratio;
}
NativeAudioDevice::VoiceFilter native_audio_device_filter(const uint32_t*p){
    require(!(_mm_getcsr()&0xe040)&&p&&p[0]==1&&!p[1]&&p[2]<=65535&&p[2]==p[4]&&p[3]==32768&&p[5]==32768,"resident filter profile");
    int32_t coefficient=p[2]<32768?(int32_t)p[2]:(int32_t)p[2]-65536;
    require(coefficient<=0,"resident filter exceeds PC frequency range");
    return{(float)std::exp2((double)coefficient/4096.),1.f};
}
NativeAudioDeviceBridge::NativeAudioDeviceBridge(NativeAudioDevice&d,std::shared_ptr<const NativeAudioClip>clip,float gain,float ratio)
 :device_(d),resident_(std::move(clip)),gain_(gain),ratio_(ratio){
    require(resident_.clip()->pcm().sample_rate>=1000,"resident clip below PC format rate");
    float maximum=std::max(4.f,native_audio_device_ratio(8191,resident_.clip()->pcm().sample_rate));
    require(std::isfinite(gain)&&gain>=0&&gain<=1&&std::isfinite(ratio)&&ratio>=1.f/1024.f&&ratio<=maximum,"resident bridge mix domain");
    require(!resident_.clip()->pcm().loop_frame_count,"resident bridge loop policy required");
    handle_=device_.create_owned_voice(maximum);
}
NativeAudioDeviceBridge::~NativeAudioDeviceBridge(){device_.destroy_owned_voice(handle_);}
void NativeAudioDeviceBridge::format(const UCAudioDeviceFormat*f){const auto&a=*wave().format;
    require(f&&f->tag==a.tag&&f->channels==a.channels&&f->rate==a.rate&&f->bytes_per_second==a.bytes_per_second&&f->alignment==a.alignment&&f->bits==a.bits&&f->extra==a.extra,"resident bridge format identity");format_=true;
}
void NativeAudioDeviceBridge::data(const void*p,uint32_t n){require(p==wave().data&&n==wave().bytes,"resident bridge encoded data identity");data_=true;}
void NativeAudioDeviceBridge::region(uint32_t start,uint32_t n){require(format_&&data_,"resident bridge unbound metadata");auto f=resident_.frames(start,n);device_.prepare_owned_voice(handle_,resident_.clip(),gain_,ratio_,f.first,f.count);}
void NativeAudioDeviceBridge::gain(int32_t value){float next=native_audio_device_gain(value);device_.set_owned_parameters(handle_,next,ratio_);gain_=next;}
void NativeAudioDeviceBridge::pitch(int32_t value){float next=native_audio_device_ratio(value,resident_.clip()->pcm().sample_rate);device_.set_owned_parameters(handle_,gain_,next);ratio_=next;}
void NativeAudioDeviceBridge::start(uint32_t repeat){require(!repeat,"resident bridge repeat policy required");device_.start_owned_voice(handle_);}
void NativeAudioDeviceBridge::stop(){device_.stop_owned_voice(handle_);}
uint32_t NativeAudioDeviceBridge::status()const{auto s=device_.owned_voice_state(handle_);return s.started&&s.buffers_queued?1u:0u;}

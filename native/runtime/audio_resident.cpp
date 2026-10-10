#include "audio_resident.h"
#include <stdexcept>
namespace {
uint16_t word(const uint8_t*p){return(uint16_t)(p[0]|p[1]<<8);}
uint32_t dword(const uint8_t*p){return(uint32_t)word(p)|(uint32_t)word(p+2)<<16;}
void require(bool p,const char*why){if(!p)throw std::runtime_error(why);}
}
NativeAudioResidentView::NativeAudioResidentView(std::shared_ptr<const NativeAudioClip>p):clip_(std::move(p)){
    require(clip_!=nullptr,"Missing resident audio owner");
    const auto&encoded=clip_->encoded();const auto&a=clip_->pcm();const auto*f=clip_->authored_format().data();
    format_={word(f),word(f+2),dword(f+4),dword(f+8),word(f+12),word(f+14),word(f+16)};
    require(format_.tag==0x69&&format_.channels==a.channels&&format_.rate==a.sample_rate&&
        format_.alignment==36*a.channels&&encoded.size()==a.encoded_bytes,"Resident audio metadata mismatch");
    wave_={encoded.data(),a.encoded_bytes,&format_,0,a.encoded_bytes,a.loop_start_bytes,a.loop_length_bytes,{0,0}};
}
NativeAudioResidentView::Frames NativeAudioResidentView::frames(uint32_t start,uint32_t length)const{
    const auto&a=clip_->pcm();const uint32_t block=36*a.channels;
    require(length&&start<=a.encoded_bytes&&length<=a.encoded_bytes-start&&
        !(start%block)&&!(length%block),"Unaligned/out-of-range encoded audio region");
    return{start/block*64,length/block*64};
}

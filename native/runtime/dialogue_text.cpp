#include "character_dialogue.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>
namespace {
void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
uint16_t u16(const uint8_t*p){return p[0]|uint16_t(p[1])<<8;}
uint32_t u32(const uint8_t*p){return u16(p)|uint32_t(u16(p+2))<<16;}
std::string string(const uint8_t*p,size_t size,size_t at){need(at<size,"Dialogue text offset");auto*end=(const uint8_t*)std::memchr(p+at,0,size-at);need(end,"Dialogue string extent");return {(const char*)p+at,size_t(end-p-at)};}
}
void NativeDialogueText::load(const void*data,size_t size,unsigned language,bool level){
    auto*p=(const uint8_t*)data;need(p&&size>=24&&u16(p)==4&&u16(p+2)==32,"Dialogue text header");
    const size_t records=u32(p+4),rows=u32(p+8),strings=u32(p+12),faces=u32(p+16),count=u32(p+20);
    const size_t block=records+rows+strings+faces;
    need(count<=65535&&records==count*56&&block&&language<4&&size==24+block*4,"Dialogue language block extent");
    p+=24+language*block;std::vector<NativeDialogueLine> result;result.reserve(count);
    for(size_t i=0;i<count;i++){
        const auto*r=p+i*56;NativeDialogueLine line;
        //17B30 compares all40 fixed name bytes. One real globals entry fills
        //the field completely; termination is required only for text payloads.
        line.name.assign((const char*)r,size_t(std::find(r,r+40,uint8_t(0))-r));line.audio=r[50]!=0;line.always_subtitle=r[51]!=0;
        const size_t at=u32(r+52),n=r[49];need(n&&at<=rows&&n*12<=rows-at,"Dialogue variant extent");
        for(size_t j=0;j<n;j++){
            const auto*v=p+records+at+j*12;NativeDialogueVariant variant;
            variant.text=string(p+records+rows,strings,u32(v));variant.ticks=u32(v+8);
            const size_t face=u32(v+4);variant.has_phonemes=face!=UINT32_MAX;
            if(variant.has_phonemes){need(face<=faces&&variant.ticks<=faces-face,"Dialogue phoneme extent");variant.phonemes.assign(p+records+rows+strings+face,p+records+rows+strings+face+variant.ticks);}
            line.variants.push_back(std::move(variant));
        }result.push_back(std::move(line));
    }(level?level_:global_)=std::move(result);
}
const NativeDialogueLine *NativeDialogueText::find(const char*name)const{
    if(!name)return nullptr;for(const auto*v:{&level_,&global_})for(const auto&line:*v)if(!_strnicmp(line.name.c_str(),name,40))return &line;return nullptr;
}


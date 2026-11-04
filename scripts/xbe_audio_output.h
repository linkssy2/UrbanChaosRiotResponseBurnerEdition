#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <string>

extern uint32_t TrialSoundOutput; // persisted preference, independent of device capacity
namespace trial_audio {
enum Mode : uint32_t { Stereo, Surround51, Surround71 };
inline std::atomic<uint32_t> requested{Stereo}, output_channels{2};
inline uint32_t ValidMode(uint32_t mode){return mode<=Surround71?mode:Stereo;}
inline uint32_t Capacity(uint32_t mask){
    if((mask&0xF)!=0xF||((mask&0x30)!=0x30&&(mask&0x600)!=0x600))return 2;
    return (mask&0x63F)==0x63F?8:6;
}
inline uint32_t Active(uint32_t mode,uint32_t channels){
    return channels<6?Stereo:mode==Surround71&&channels<8?Surround51:ValidMode(mode);
}
inline uint32_t Active(){return Active(requested.load(),output_channels.load());}
inline const char* Name(uint32_t mode){
    return mode==Surround71?"7.1 Surround":mode==Surround51?"5.1 Surround":"Stereo";
}
// Darkwatch's host enhancement redistributes rear energy, never duplicates it.
inline std::array<float,2> RearSpread(float azimuth,float elevation){
    if(!std::isfinite(azimuth)||!std::isfinite(elevation)||std::abs(elevation)>90)return {1,0};
    constexpr float radians=0.017453292519943295f;
    const float angle=std::abs(std::remainder(azimuth,360.f));
    const float t=std::clamp((angle-90)/90,0.f,1.f);
    const float side=std::clamp(std::cos(t*90*radians),0.f,1.f)*std::max(0.f,std::cos(elevation*radians));
    return {std::sqrt(std::max(0.f,1-side*side)),side};
}
void Initialize();
bool Apply(uint32_t mode,std::string& message);
}

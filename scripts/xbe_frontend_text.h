#pragma once
#include <cstddef>
#include <cstdint>

// The original European text banks/font tables use single-byte Windows-1252.
// ImGui consumes UTF-8; keep this conversion at the copied host-menu boundary.
inline uint32_t TrialFrontendCodepoint(unsigned byte){
    static constexpr uint16_t extended[]={
        0x20ac,0,0x201a,0x0192,0x201e,0x2026,0x2020,0x2021,
        0x02c6,0x2030,0x0160,0x2039,0x0152,0,0x017d,0,
        0,0x2018,0x2019,0x201c,0x201d,0x2022,0x2013,0x2014,
        0x02dc,0x2122,0x0161,0x203a,0x0153,0,0x017e,0x0178};
    return byte>=128&&byte<160?(extended[byte-128]?extended[byte-128]:'?'):byte;
}
template<size_t N> inline void TrialFrontendUtf8(const char* source,char (&out)[N]){
    static_assert(N>0);size_t used=0;
    for(const auto* p=reinterpret_cast<const unsigned char*>(source);*p;++p){
        const uint32_t c=TrialFrontendCodepoint(*p);const size_t count=c<128?1:c<2048?2:3;
        if(count>=N-used)break; // Never leave a partial UTF-8 character.
        if(count==1)out[used++]=char(c);
        else{
            if(count==3){out[used++]=char(0xe0|(c>>12));out[used++]=char(0x80|((c>>6)&63));}
            else out[used++]=char(0xc0|(c>>6));
            out[used++]=char(0x80|(c&63));
        }
    }
    out[used]=0;
}

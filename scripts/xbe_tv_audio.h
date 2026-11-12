#pragma once
#include <cstdint>
#include <cstring>

// Apply the accepted scene timing to loaded bank data, never to disc archives.
template<class Readable>
bool TrialTvAudio(uint8_t* base,uint32_t bank,Readable readable){
    if(!base||!readable(bank,0x9c))return false;
    const uint8_t guid[]={0x44,0xcb,0x88,0xb6,0x61,0xf5,0xe9,0x40,0x8a,0x42,0x36,0x6e,0xcd,0xc7,0x96,0xd2};
    if(std::memcmp(base+bank+0x34,guid,16))return false;
    auto word=[&](uint32_t p){uint32_t v;std::memcpy(&v,base+p,4);return v;};
    const uint32_t tracks=word(bank+0x98),refs=word(bank+0x90);
    if(!readable(tracks,228)||!readable(refs,92))return false;
    if(word(tracks)!=0x5453494c||word(tracks+4)!=220||word(tracks+8)!=0x6c6b7274||
       word(refs)!=0x66726d61||word(refs+4)!=84||word(refs+8)!=4)return false;
    const uint8_t tv[]={0x16,0xf3,0xb3,0x30,0xcf,0xbc,0x36,0x4a,0xbd,0xf4,0x39,0xa9,0xdf,0x23,0x7d,0xa4};
    if(std::memcmp(base+refs+32,tv,16))return false;
    const uint8_t command[]={0x4e,0x0e,0,0,3,4,0,0,1,0,0xff,0xff,1,0,0xff,0xff};
    if(word(tracks+48)!=0x74706d61||word(tracks+52)!=28||
       std::memcmp(base+tracks+68,command,sizeof(command)))return false;
    const uint32_t corrected=3362;
    std::memcpy(base+tracks+68,&corrected,4);
    return true;
}


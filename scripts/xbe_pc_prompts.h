#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

struct TrialPromptInput {
    unsigned bindings[13]{};
    char labels[13][64]{};
    long x=0,y=0;
    unsigned direction=0;
    bool held=false;
    bool controller=false;
};
bool TrialPromptSnapshot(unsigned id,TrialPromptInput& out,unsigned variant=0);
bool TrialPromptArtwork(unsigned id,const uint8_t* source,const TrialPromptInput& input,
                        std::vector<uint8_t>& pixels,unsigned& width,unsigned& height);
int xbox_KeyboardMode();

namespace uc_pc_prompts {
inline uint64_t Hash(const uint8_t* p,size_t size) {
    uint64_t h=1469598103934665603ull;
    for(size_t i=0;i<size;++i)h=(h^p[i])*1099511628211ull;
    return h;
}
// These are the accepted CONTROL source textures, not arbitrary matching sizes.
template<class Desc>
unsigned Match(const Desc& d,const uint8_t* p,size_t bytes) {
    if(!p||d.cube||d.depth!=1||d.linear)return 0;
    if(d.width==32&&d.height==1024&&d.format==0x0f&&bytes>=32768&&Hash(p,32768)==0xdd89ab98a03df6b3ull)return 1;
    if(d.width==64&&d.height==64&&bytes>=4096&&Hash(p,4096)==0x775d4d8634377583ull)return 2;
    return 0;
}
template<class Pad> bool ControllerActive(const Pad& pad) {
    if(pad.buttons||pad.lx>7849||pad.lx< -7849||pad.ly>7849||pad.ly< -7849||
       pad.rx>8689||pad.rx< -8689||pad.ry>8689||pad.ry< -8689)return true;
    for(auto value:pad.analog)if(value>30)return true;
    return false;
}
struct PointerActivity {
    bool valid=false;long x=0,y=0;
    bool Move(long next_x,long next_y){
        bool moved=valid&&(next_x!=x||next_y!=y);
        valid=true;x=next_x;y=next_y;return moved;
    }
    void Reset(){valid=false;}
};
inline bool Equal(const TrialPromptInput& a,const TrialPromptInput& b) {
    return !std::memcmp(a.bindings,b.bindings,sizeof(a.bindings))&&
           !std::memcmp(a.labels,b.labels,sizeof(a.labels))&&a.x==b.x&&a.y==b.y&&
           a.direction==b.direction&&a.held==b.held&&a.controller==b.controller;
}
// Same BC3 byte conversion as launcher/burner/hud_import.py, performed once
// from the title's own verified live texture. Unmapped atlas rows stay original.
inline void DecodeBc3(const uint8_t* source,unsigned width,unsigned height,uint32_t* pixels) {
    for(unsigned by=0;by<height/4;++by)for(unsigned bx=0;bx<width/4;++bx) {
        const uint8_t* b=source+(by*(width/4)+bx)*16;
        unsigned alpha[8]={b[0],b[1]};
        if(alpha[0]>alpha[1])for(unsigned i=1;i<7;++i)alpha[i+1]=((7-i)*alpha[0]+i*alpha[1])/7;
        else {for(unsigned i=1;i<5;++i)alpha[i+1]=((5-i)*alpha[0]+i*alpha[1])/5;alpha[6]=0;alpha[7]=255;}
        uint64_t bits=0;for(unsigned i=0;i<6;++i)bits|=uint64_t(b[2+i])<<(i*8);
        unsigned colors[4][3]{};
        for(unsigned i=0;i<2;++i) {
            unsigned c=b[8+i*2]|unsigned(b[9+i*2])<<8;
            unsigned r=c>>11,g=c>>5&63,bl=c&31;
            colors[i][0]=(bl<<3)|(bl>>2);colors[i][1]=(g<<2)|(g>>4);colors[i][2]=(r<<3)|(r>>2);
        }
        for(unsigned c=0;c<3;++c){colors[2][c]=(2*colors[0][c]+colors[1][c])/3;colors[3][c]=(colors[0][c]+2*colors[1][c])/3;}
        uint32_t indices;std::memcpy(&indices,b+12,4);
        for(unsigned i=0;i<16;++i){auto* c=colors[indices>>(2*i)&3];
            pixels[(by*4+i/4)*width+bx*4+i%4]=c[0]|c[1]<<8|c[2]<<16|alpha[bits>>(3*i)&7]<<24;}
    }
}
inline void DecodeButtons(const uint8_t* source,uint32_t* pixels) {
    DecodeBc3(source,32,1024,pixels);
}
}

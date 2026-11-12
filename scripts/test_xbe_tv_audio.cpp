#undef NDEBUG
#include "xbe_tv_audio.h"
#include <cassert>
#include <fstream>
#include <iterator>
#include <vector>
#include <cstdio>
int main(int argc,char** argv){
    assert(argc==2);
    std::ifstream f(argv[1],std::ios::binary);
    std::vector<uint8_t> source((std::istreambuf_iterator<char>(f)),{});
    assert(source.size()==720);
    std::vector<uint8_t> memory(0x20000),saved;
    auto put=[&](unsigned p,unsigned n){std::memcpy(memory.data()+p,&n,4);};
    unsigned tracks=0,refs=0,guid=0;
    for(unsigned at=12;at+8<=source.size();){
        uint32_t n;std::memcpy(&n,source.data()+at+4,4);
        assert(n<=source.size()-at-8);
        if(!std::memcmp(source.data()+at,"LIST",4)&&!std::memcmp(source.data()+at+8,"trkl",4))tracks=at;
        if(!std::memcmp(source.data()+at,"amrf",4))refs=at;
        if(!std::memcmp(source.data()+at,"guid",4))guid=at+8;
        at+=8+n+(n&1);
    }
    assert(tracks&&refs&&guid);
    std::memcpy(memory.data()+0x12000,source.data(),source.size());
    std::memcpy(memory.data()+0x10034,source.data()+guid,16);
    put(0x10098,0x12000+tracks);put(0x10090,0x12000+refs);saved=memory;
    auto readable=[&](uint32_t p,uint32_t n){return p>=0x10000&&uint64_t(p)+n<=memory.size();};
    assert(TrialTvAudio(memory.data(),0x10000,readable));
    uint32_t time;std::memcpy(&time,memory.data()+0x12000+228,4);assert(time==3362);
    for(unsigned i=0;i<memory.size();++i)if(i<0x12000+228||i>=0x12000+232)assert(memory[i]==saved[i]);
    auto changed=memory;assert(!TrialTvAudio(memory.data(),0x10000,readable)&&memory==changed);
    for(unsigned bad:{0x10034u,0x12000+refs+32,0x12000+tracks+4,0x12000u+228}){
        memory=saved;memory[bad]^=1;changed=memory;
        assert(!TrialTvAudio(memory.data(),0x10000,readable)&&memory==changed);
    }
    memory=saved;put(0x10098,0xfffffff0);changed=memory;
    assert(!TrialTvAudio(memory.data(),0x10000,readable)&&memory==changed);
    assert(!TrialTvAudio(memory.data(),0xfffffff0,readable));
    puts("PASS original bank fixture: only TV timestamp changes, unrelated/malformed/repeated input stays unchanged");
}



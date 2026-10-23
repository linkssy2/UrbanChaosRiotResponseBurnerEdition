// Compare the extracted production decoder/sample path with its prior version.
#undef NDEBUG
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

static std::array<uint8_t, 65536> memory{};
static void* HostPtr(uint32_t at) { assert(at < memory.size()); return memory.data() + at; }
static uint16_t R16(uint32_t at) { uint16_t x;std::memcpy(&x,HostPtr(at),2);return x; }
static uint32_t R32(uint32_t at) { uint32_t x;std::memcpy(&x,HostPtr(at),4);return x; }
static void W16(uint32_t at,uint16_t x) { std::memcpy(HostPtr(at),&x,2); }
namespace mem {
static void FreeContiguous(uint32_t) { assert(false); }
static uint32_t AllocContiguous(uint32_t,uint32_t,uint32_t,uint32_t) { assert(false);return 0; }
}
#define LOG_FIRST(...) ((void)0)
struct Fields {
    uint16_t tag=0x69,channels=1,bits=16,align=2;
    uint32_t data=0,bytes=0,play_len=0,loop_len=0,play_start=0,loop_start=0,owned=0,output=0,rate=48000,freq=0;
    double pos=0;bool mixin=false;
};
namespace before {
struct Voice : Fields { /* BEFORE_FIELDS */ };
static unsigned decodes;
/* BEFORE_CODE */
static void FlushStream(Voice& v) { v.pos=0; }
static void UpdateAudioRouteFormat(Voice& v) { assert(!v.output); }
/* BEFORE_SETTERS */
}
namespace after {
struct Voice : Fields { /* AFTER_FIELDS */ };
static unsigned decodes;
/* AFTER_CODE */
static void BeginMix(Voice& v) { /* BEGIN_MIX */ }
static void FlushStream(Voice& v) { v.pos=0; }
static void UpdateAudioRouteFormat(Voice& v) { assert(!v.output); }
/* AFTER_SETTERS */
}

static void Same(float a, float b) { assert(std::memcmp(&a, &b, sizeof(a)) == 0); }

static void Fill(uint32_t at, unsigned channels, unsigned blocks, uint32_t seed) {
    for (unsigned i=0; i<36*channels*blocks; ++i) {
        seed=seed*1664525u+1013904223u;
        memory.at(at+i)=uint8_t(seed>>24);
    }
}

static void SamplePair(before::Voice& b, after::Voice& a, uint32_t base,
                       unsigned frame, uint32_t next_base, unsigned next, float t) {
    // The existing mixer reads both current channels, then both next channels.
    float old[4], now[4];
    for (unsigned i=0; i<4; ++i) {
        old[i]=before::Sample(b,i<2?base:next_base,i<2?frame:next,int(i&1));
        now[i]=after::Sample(a,i<2?base:next_base,i<2?frame:next,int(i&1));
        Same(old[i],now[i]);
    }
    for(unsigned c=0;c<2;++c) Same(old[c]+(old[c+2]-old[c])*t, now[c]+(now[c+2]-now[c])*t);
}

int main() {
    constexpr uint32_t base=4096, packet=16384;
    unsigned total_old=0,total_new=0;
    for (unsigned channels : {1u,2u,8u}) {
        Fill(base,channels,32,channels);
        Fill(packet,channels,8,channels+17);
        for (double step : {1.0/256,0.125,0.5,1.0,1.5,4.0}) {
            before::Voice b; after::Voice a;
            b.channels=a.channels=uint16_t(channels);
            before::decodes=after::decodes=0;
            double pos=62.0;
            for (unsigned mix=0;mix<8;++mix) {
                after::BeginMix(a);
                for(unsigned i=0;i<480;++i) {
                    unsigned f=unsigned(pos)%512,next=(f+1)%512;
                    SamplePair(b,a,base,f,base,next,float(pos-unsigned(pos)));
                    pos+=step;
                }
            }
            assert(after::decodes<=before::decodes);
            if (step<=0.5) assert(after::decodes<before::decodes);
            total_old+=before::decodes; total_new+=after::decodes;
            // A stream interpolation can read the first frame of another packet.
            after::BeginMix(a);
            SamplePair(b,a,base,511,packet,0,0.75f);
        }
        // Guest rewrites the older block between mixes: the new extra slot must
        // expire, retaining only the same current block as the previous decoder.
        before::Voice b; after::Voice a;
        b.channels=a.channels=uint16_t(channels);
        SamplePair(b,a,base,63,base,64,0.5f);
        Fill(base,channels,1,313+channels);
        after::BeginMix(a);
        SamplePair(b,a,base,63,base,64,0.5f);
        // The carried B must refresh when A is requested first in the next mix.
        Fill(base+36*channels,channels,1,717+channels);
        after::BeginMix(a);
        SamplePair(b,a,base,63,base,64,0.5f);
        // If B is requested first, its old sample stays observable just as before;
        // after leaving B, returning to it must decode the rewritten contents.
        Fill(base+36*channels,channels,1,818+channels);
        after::BeginMix(a);
        Same(before::Sample(b,base,64,0),after::Sample(a,base,64,0));
        Same(before::Sample(b,base,0,0),after::Sample(a,base,0,0));
        Same(before::Sample(b,base,64,0),after::Sample(a,base,64,0));
        // Existing buffer data/region/format resets invalidate both entries.
        Fill(base,channels,2,424+channels);
        b.block=-1; after::ForgetBlocks(a);
        assert(a.block[0]==-1 && a.block[1]==-1);
        SamplePair(b,a,base,63,base,64,0.75f);
        // Execute each production reset path, including rejection/unchanged data.
        b.bytes=a.bytes=36*channels*8;
        b.data=a.data=base;
        W16(512,0x69);W16(514,uint16_t(channels));W16(524,uint16_t(36*channels));W16(526,4);
        for(unsigned setter=0;setter<4;++setter) {
            SamplePair(b,a,base,63,base,64,0.5f);
            if(setter==0) {
                assert(before::BufferData(b,{0,base,b.bytes})==0);
                assert(after::BufferData(a,{0,base,a.bytes})==0);
                assert(a.block[a.last]==b.block);
                assert(before::BufferData(b,{0,packet,b.bytes})==0);
                assert(after::BufferData(a,{0,packet,a.bytes})==0);
            } else if(setter==1) {
                assert(before::PlayRegion(b,{0,b.bytes+1,0})==0x88780032u);
                assert(after::PlayRegion(a,{0,a.bytes+1,0})==0x88780032u);
                assert(a.block[a.last]==b.block);
                assert(before::PlayRegion(b,{0,0,0})==0);
                assert(after::PlayRegion(a,{0,0,0})==0);
            } else if(setter==2) {
                assert(before::BufferFormat(b,{0,512})==0);
                assert(after::BufferFormat(a,{0,512})==0);
            } else {
                assert(before::StreamFormat(b,{0,512})==0);
                assert(after::StreamFormat(a,{0,512})==0);
            }
            assert(b.block==-1 && a.block[0]==-1 && a.block[1]==-1 && !a.carried_block);
            SamplePair(b,a,base,63,base,64,0.25f);
        }
    }
    // PCM has no decode cache; samples and clamped mono channel lookup are exact.
    for (unsigned bits : {8u,16u}) for(unsigned channels : {1u,2u}) {
        before::Voice b; after::Voice a;
        b.tag=a.tag=1; b.bits=a.bits=uint16_t(bits); b.channels=a.channels=uint16_t(channels);
        b.align=a.align=uint16_t(channels*bits/8);
        for(unsigned f=0;f<1024;++f) SamplePair(b,a,base,f,base,f+1,0.375f);
    }
    assert(total_new<total_old);
    std::printf("ADPCM samples/interpolation bit-identical; block decodes %u -> %u across fixture workloads.\n",total_old,total_new);
    return 0;
}

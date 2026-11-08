#undef NDEBUG
#include <array>
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "cpu/x86_ops.h"
#include "xbe_lighting_bounds.h"

// The preparer inserts the CURRENT generated leaf and its real wrapper twice.
// No mocked math or reimplemented expected predicate is used for equivalence.
// NATIVE_BODIES
namespace {
alignas(64) std::array<uint8_t,0x500000> memory{};
constexpr uint32_t REGION=0x10000,EXTRA=0x44d600,SP=0x10140;
uint32_t seed=0x87abcdu;
uint32_t Bits(){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
void Put(uint32_t p,uint32_t value){std::memcpy(memory.data()+p,&value,4);}
void Float(uint32_t p,float value){std::memcpy(memory.data()+p,&value,4);}
void Randomize(uint8_t* p,size_t n){for(size_t i=0;i<n;++i)p[i]=uint8_t(Bits());}
struct Ram {
    std::array<uint8_t,512> main;
    std::array<uint8_t,256> extra;
    void Save(){std::memcpy(main.data(),memory.data()+REGION,main.size());std::memcpy(extra.data(),memory.data()+EXTRA,extra.size());}
    void Restore()const{std::memcpy(memory.data()+REGION,main.data(),main.size());std::memcpy(memory.data()+EXTRA,extra.data(),extra.size());}
    bool Equal()const{return !std::memcmp(main.data(),memory.data()+REGION,main.size())&&!std::memcmp(extra.data(),memory.data()+EXTRA,extra.size());}
};
X86Context Context(){X86Context c;Randomize(reinterpret_cast<uint8_t*>(&c),sizeof c);c.eax=REGION;c.ecx=REGION+32;c.edx=REGION+64;c.esp=SP;return c;}
void Check(bool condition,unsigned test,const char* label){if(!condition){std::fprintf(stderr,"FAIL trial=%u %s\n",test,label);std::abort();}}
uint64_t fast=0,fallback=0;
void Compare(X86Context original,unsigned mode,unsigned trial,bool wrapper){
    if(wrapper){original.ecx-=0x1b0;Put(original.esp+4,original.edx);}
    Ram initial;initial.Save();X86Context probe=original;
    if(wrapper){probe.edx=trial_lighting_bounds::Word(memory.data(),probe.esp+4);probe.eax=probe.ecx+0x1bc;probe.ecx+=0x1b0;probe.esp-=4;Put(probe.esp,0xb2bb5);}
    Ram before;before.Save();X86Context before_context=probe;
    _mm_setcsr(mode);const bool supported=trial_lighting_bounds::Try(probe,memory.data());
    if(supported)++fast;
    else{++fallback;Check(!std::memcmp(&probe,&before_context,sizeof probe),trial,"decline changed context");Check(before.Equal(),trial,"decline changed RAM");Check(_mm_getcsr()==mode,trial,"decline changed MXCSR");}
    initial.Restore();X86Context expected=original;
    _mm_setcsr(mode);(wrapper?OriginalWrapper:OriginalLeaf)(expected,memory.data());
    const unsigned expected_mxcsr=_mm_getcsr();Ram expected_ram;expected_ram.Save();
    initial.Restore();X86Context actual=original;
    _mm_setcsr(mode);(wrapper?FixedWrapper:FixedLeaf)(actual,memory.data());
    const unsigned actual_mxcsr=_mm_getcsr();
    if(std::memcmp(&expected,&actual,sizeof actual)){
        const auto* a=reinterpret_cast<const uint8_t*>(&actual);const auto* e=reinterpret_cast<const uint8_t*>(&expected);
        for(size_t i=0;i<sizeof actual;++i)if(a[i]!=e[i])std::fprintf(stderr,"context byte %zu expected %02x actual %02x\n",i,e[i],a[i]);
    }
    Check(!std::memcmp(&expected,&actual,sizeof actual),trial,"context/all XMM lanes");
    Check(expected_ram.Equal(),trial,"guest RAM/scratch");
    Check(expected_mxcsr==actual_mxcsr,trial,"host MXCSR");
}
void Correctness(){
    const uint32_t special[]={0,0x80000000,1,0x80000001,0x007fffff,0x807fffff,0x00800000,0x80800000,
        0x3f800000,0xbf800000,0x4e800000,0xce800000,0x4e800001,0x7f7fffff,0xff7fffff,
        0x7f800000,0xff800000,0x7fc12345,0xffc12345,0x7f812345,0xff812345};
    unsigned trial=0;
    for(unsigned rounding=0;rounding<4;++rounding)for(unsigned denormals=0;denormals<4;++denormals){
        const unsigned mode=0x1f80u|(rounding<<13)|((denormals&1)?0x8000u:0)|((denormals&2)?0x40u:0);
        for(unsigned n=0;n<12000;++n,++trial){
            Randomize(memory.data()+REGION,512);Randomize(memory.data()+EXTRA,256);
            auto c=Context();
            for(uint32_t p:{c.eax,c.ecx,c.edx})for(unsigned i=0;i<(p==c.edx?6u:3u);++i){
                if(n%3==0)Put(p+4*i,special[Bits()%std::size(special)]);
                else if(n%3==1)Float(p+4*i,float(int(Bits()%2001)-1000)/128.f);
            }
            Put(0x44d660,n%3==0?special[Bits()%std::size(special)]:0x3f800000);
            if(n%7==0)c.eax=c.esp-28;
            if(n%11==0)c.edx=c.esp-32;
            if(n%13==0)c.ecx=c.edx;
            if(n%17==0)c.esp=0x44d67c; // scratch aliases the threshold
            if(n%19==0){++c.eax;++c.ecx;++c.edx;} // generated memory is unaligned
            Compare(c,mode|(n&63u),trial,false);
            // Wrapper's offset parameters have A=light+1BC, C=light+1B0.
            c=Context();c.eax=c.ecx+12;
            Compare(c,mode|(n&63u),trial,true);
        }
    }
    // Independent inside/boundary/outside witnesses and all early-return paths.
    for(unsigned axis=0;axis<3;++axis)for(float distance:{0.f,1.f,2.f,-2.f}){
        std::memset(memory.data()+REGION,0,512);auto c=Context();
        for(unsigned i=0;i<3;++i)Float(c.ecx+4*i,1);
        Float(c.eax+4*axis,distance);Float(0x44d660,1);
        auto result=c;_mm_setcsr(0x1f80);FixedLeaf(result,memory.data());
        Check((result.eax&255u)==(distance==0),trial,"independent unit-sphere witness");
        Compare(c,0x1f80,trial++,false);
    }
    for(uint32_t bad:{0u,0xfff0u,0xfffffff8u}){
        auto c=Context();c.eax=bad;auto copy=c;const unsigned csr=_mm_getcsr();
        Check(!trial_lighting_bounds::Try(c,memory.data()),trial,"range declined");
        Check(!std::memcmp(&c,&copy,sizeof c)&&_mm_getcsr()==csr,trial++,"range decline unchanged");
    }
    // No exceptional arithmetic is executed with traps enabled. Every mode
    // declines before reading operands or publishing context/scratch state.
    for(unsigned masks=0;masks<63;++masks)for(unsigned rounding=0;rounding<4;++rounding)for(unsigned denormals=0;denormals<4;++denormals){
        auto c=Context();const auto copy=c;Ram ram;ram.Save();
        const unsigned mode=(masks<<7)|(rounding<<13)|((denormals&1)?0x8000u:0)|((denormals&2)?0x40u:0);
        _mm_setcsr(mode);const bool accepted=trial_lighting_bounds::Try(c,memory.data());
        const unsigned after=_mm_getcsr();_mm_setcsr(0x1f80);
        Check(!accepted&&!std::memcmp(&c,&copy,sizeof c)&&ram.Equal()&&after==mode,trial++,"unmasked mode declined unchanged");
    }
    Check(fast&&fallback,trial,"both paths covered");
    std::printf("PASS %u iterations; %llu direct/wrapper comparisons; accepted=%llu fallback=%llu; all context/RAM/MXCSR exact\n",trial,
        static_cast<unsigned long long>(fast+fallback),
        static_cast<unsigned long long>(fast),static_cast<unsigned long long>(fallback));
}
struct Record{std::array<float,3> offset,scale;std::array<float,6> box;};
void Load(const Record& r){
    // Shared layout matches the live B2BA0 point-light wrapper.
    std::memcpy(memory.data()+REGION+32,r.scale.data(),12);std::memcpy(memory.data()+REGION+44,r.offset.data(),12);
    std::memcpy(memory.data()+REGION+64,r.box.data(),24);
}
void Benchmark(){
    constexpr unsigned iterations=1000000;
    for(unsigned workload=0;workload<4;++workload){
        std::array<Record,256> records{};
        for(unsigned i=0;i<records.size();++i){
            const bool hit=workload==0||(workload>=2&&(i%4)!=0);
            records[i]={{hit?.1f:2.f,float(i%11)*.01f,float(i%17)*.01f},{.02f,.02f,.02f},{-5,-8,-3,5,8,3}};
            if(workload==3&&i%16==0)records[i].offset[0]=std::numeric_limits<float>::quiet_NaN();
        }
        unsigned accepted=0;
        for(const auto& record:records){Load(record);auto c=Context();c.eax=c.ecx+12;Put(0x44d660,0x3f800000);accepted+=trial_lighting_bounds::Try(c,memory.data());}
        std::printf("workload=%s accepts=%u/%zu\n",workload==0?"hit":workload==1?"miss":workload==2?"mixed":"mixed-fallback",accepted,records.size());
        // Alternating original/candidate, full helper guards and context
        // publication included. Loading identical data is timed for both.
        for(unsigned pass=0;pass<6;++pass){
            uint64_t checksum=0;const bool fixed=pass%2;auto c=Context();_mm_setcsr(0x1f80);
            auto fn=fixed?FixedWrapper:OriginalWrapper;
            const auto start=std::chrono::steady_clock::now();
            for(unsigned i=0;i<iterations;++i){
                Load(records[i%records.size()]);c.eax=REGION+44;c.ecx=REGION+32-0x1b0;c.edx=REGION+64;c.esp=SP;
                Put(SP+4,REGION+64);fn(c,memory.data());checksum+=c.eax;
            }
            const auto finish=std::chrono::steady_clock::now();
            std::printf("%s %s pass=%u calls=%u ms=%.3f checksum=%llu\n",workload==0?"hit":workload==1?"miss":workload==2?"mixed":"mixed-fallback",
                fixed?"candidate":"original",pass,iterations,std::chrono::duration<double,std::milli>(finish-start).count(),static_cast<unsigned long long>(checksum));
        }
    }
}
}
int main(int argc,char** argv){
    const unsigned csr=_mm_getcsr();Correctness();
    if(argc==2&&!std::strcmp(argv[1],"--benchmark"))Benchmark();
    _mm_setcsr(csr);return 0;
}

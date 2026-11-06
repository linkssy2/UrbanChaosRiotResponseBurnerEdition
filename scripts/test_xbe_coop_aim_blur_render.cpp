// Execute the original retail quad builders before testing view isolation.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include "kernel/kernel.h"
namespace kernel {static std::array<uint8_t,0x800000> memory{};}
namespace mem {bool IsCommitted(uint32_t p){return p<kernel::memory.size();}}
#include "../common/xbe_coop_aim_blur_render.cpp"
namespace fixture {
constexpr uint32_t device=0x600000,target=0x610000,depth=0x620000,owner=0x630000,lookup=0x640000,stack=0x7f0000;
int active=-1;unsigned restores=0;
uint8_t* Bytes(){return kernel::memory.data();}
void Put(uint32_t a,uint32_t v){std::memcpy(Bytes()+a,&v,4);}
void Float(uint32_t a,float v){std::memcpy(Bytes()+a,&v,4);}
uint32_t Get(uint32_t a){uint32_t v;std::memcpy(&v,Bytes()+a,4);return v;}
std::array<uint32_t,6> Half(unsigned slot,unsigned w=640,unsigned h=480){return {0,slot*(h/2),w,h/2,0,0x3f800000u};}
std::array<uint32_t,6> Full(unsigned w=640,unsigned h=480){return {0,0,w,h,0,0x3f800000u};}
void View(std::array<uint32_t,6> v){std::memcpy(Bytes()+device+0xee0,v.data(),24);}
constexpr std::array<uint32_t,6> full{0,0,640,480,0,0x3f800000u};
X86Context Setup(int slot,unsigned w=640,unsigned h=480,bool fullscreen=false){
    kernel::memory.fill(0);active=slot;restores=0;
    Put(0x3f4238,device);Put(device+0x1a04,target);Put(device+0x1a08,depth);Put(device+0x1a14,target);
    View(slot<0||fullscreen?Full(w,h):Half(slot,w,h));Put(stack,0x97c0b);Float(0x44bf94,1);
    Put(owner+0x20,lookup);Float(lookup+0x24,float(w));Float(lookup+0x28,float(h));
    Put(stack+0x9c,w);Put(stack+0xa0,h);Put(stack+0xb8,w/2);Put(stack+0xbc,h/2);
    X86Context c{};c.esp=stack;c.esi=owner;c.edi=3;c.fcw=0x37f;c.mxcsr=0x1f80;return c;
}
}
int TrialCoopAimBlurActiveView(){return fixture::active;}
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
X86_FUNC(sub_003E55B0){ctx.esp+=8;}
X86_FUNC(sub_003E3840){ctx.esp+=12;}
X86_FUNC(sub_003E7080){ctx.esp+=4;}
X86_FUNC(sub_003E82D0){ctx.esp+=8;}
X86_FUNC(sub_003E7450){ctx.esp+=8;}
X86_FUNC(sub_0009C7E0){ctx.esp+=4;}
X86_FUNC(sub_0009C7A0){ctx.esp+=4;}
X86_FUNC(sub_0009C830){ctx.esp+=4;}
// ORIGINAL_AIM_BLUR_QUADS
namespace kernel {
uint32_t CallGuest(X86Context& c,uint32_t target,const uint32_t* args,size_t count){
    assert(target==0x3e3610&&count==1);++fixture::restores;
    std::memcpy(fixture::Bytes()+fixture::device+0xee0,fixture::Bytes()+args[0],24);
    c.eax=0xdeadcafe;c.esp-=32;_mm_setcsr(0x3f80);return 0;
}
}
int main(){
    using namespace fixture;
    unsigned cases=0;
    // The recorded failure enters BOTH cameras with a full640x480 SDK viewport.
    // Execute the native builders with that state as well as correct half views,
    // widescreen sizes and half viewports with an odd number of rows.
    for(auto size:{std::array<unsigned,2>{640,480},{854,480},{1280,720},{640,482}})
    for(bool fullscreen:{false,true})for(int slot:{0,1}){
        const auto w=size[0],h=size[1];
        auto c=Setup(slot,w,h,fullscreen);const unsigned csr=_mm_getcsr();
        {
            TrialCoopAimBlurRenderScope scope(c,Bytes());assert(scope.view==slot);
            OriginalDownsample(c,Bytes());const auto saved=c;
            const uint32_t source=Get(c.esp+8);std::array<float,20> old{};std::memcpy(old.data(),Bytes()+source,80);
            TrialCoopAimBlurDownsample(c,Bytes());assert(scope.copied&&!std::memcmp(&saved,&c,sizeof c));
            auto* v=reinterpret_cast<float*>(Bytes()+source);
            for(unsigned i=0;i<4;++i)for(unsigned j=0;j<5;++j)
                assert(v[i*5+j]==(j==4?float(slot*(h/2))+old[i*5+j]*.5f:old[i*5+j]));
            assert(TrialCoopAimBlurSourceView(0x923d1)==unsigned(slot+1));
            assert(!TrialCoopAimBlurSourceView(0xa7e87));
            c.esp=stack;c.ebp=stack-0x100;Put(c.ebp+4,0x923ea);
            const uint32_t gaussian=stack-0x200;
            float taps[4][11]{};
            for(unsigned i=0;i<4;++i){taps[i][0]=float((i&1)*(w/2));taps[i][1]=float((i/2)*(h/2));
                for(unsigned j=0;j<4;++j){taps[i][3+j*2]=taps[i][0];taps[i][4+j*2]=taps[i][1]+float(j)-1.5f;}}
            std::memcpy(Bytes()+gaussian,taps,sizeof(taps));Put(c.esp,6);Put(c.esp+4,4);Put(c.esp+8,gaussian);Put(c.esp+12,44);
            Put(c.ebp+4,0x9fbbe);TrialCoopAimBlurVertical(c,Bytes());
            assert(!std::memcmp(Bytes()+gaussian,taps,sizeof(taps)));Put(c.ebp+4,0x923ea);
            const unsigned before_fp=_mm_getcsr();
            TrialCoopAimBlurVertical(c,Bytes());
            assert(_mm_getcsr()==before_fp);
            auto* filtered=reinterpret_cast<float*>(Bytes()+gaussian);
            for(unsigned i=0;i<4;++i)for(unsigned j=0;j<11;++j)
                assert(filtered[i*11+j]==(j>=4&&!(j&1)?taps[i][1]+2*(taps[i][j]-taps[i][1]):taps[i][j]));
            c.esp=stack;c.esi=owner;c.ebx=0;c.edi=3;
            OriginalComposite(c,Bytes());const uint32_t composite=Get(c.esp+8);
            float before[4][8];std::memcpy(before,Bytes()+composite,sizeof(before));
            TrialCoopAimBlurComposite(c,Bytes());auto* final=reinterpret_cast<float*>(Bytes()+composite);
            for(unsigned i=0;i<4;++i)for(unsigned j=0;j<8;++j)
                assert(final[i*8+j]==((j==1||j==5)?float(slot*(h/2))+before[i][j]*.5f:before[i][j]));
            View(Full(w,h));c.esp=stack;
        }
        assert(restores==unsigned(!fullscreen)&&_mm_getcsr()==csr&&!TrialCoopAimBlurSourceView(0x923d1));
        auto expected=fullscreen?Full(w,h):Half(slot,w,h);
        assert(!std::memcmp(Bytes()+device+0xee0,expected.data(),24));++cases;
    }
    for(unsigned invalid=0;invalid<8;++invalid){
        auto c=Setup(1);
        switch(invalid){case 0:active=-1;break;case 1:active=2;break;case 2:Put(stack,0x12345);break;
            case 3:View(Full(800,600));break;case 4:View(Half(0));break;case 5:Put(device+0x1a14,depth);break;
            case 6:Put(0x3f4238,0);break;case 7:Put(device+0xee0+16,0x3e800000);break;}
        {TrialCoopAimBlurRenderScope scope(c,Bytes());OriginalDownsample(c,Bytes());
            const auto address=Get(c.esp+8);std::array<uint8_t,80> before{};std::memcpy(before.data(),Bytes()+address,80);
            TrialCoopAimBlurDownsample(c,Bytes());assert(!scope.copied&&!std::memcmp(before.data(),Bytes()+address,80));}
        assert(!restores&&!TrialCoopAimBlurSourceView(0x923d1));++cases;
    }
    // Unknown native layouts and malformed source extents cannot guess a crop.
    for(unsigned invalid=0;invalid<7;++invalid)for(int slot:{0,1}){
        auto c=Setup(slot,invalid==0?641:640,invalid==1?481:480,true);
        {TrialCoopAimBlurRenderScope scope(c,Bytes());OriginalDownsample(c,Bytes());
            const auto address=Get(c.esp+8);
            if(invalid==2)Put(address+8,0); // native source quad z is one
            if(invalid==3)Put(address+20+12,0x7fc00000u);
            if(invalid==4)Put(address+20+12,0x7f800000u);
            if(invalid==5)Float(address+20+12,640.5f);
            if(invalid==6)Float(address+40+16,16777218.f);
            std::array<uint8_t,80> before{};std::memcpy(before.data(),Bytes()+address,80);
            const auto csr=_mm_getcsr();TrialCoopAimBlurDownsample(c,Bytes());
            assert(_mm_getcsr()==csr&&!scope.copied&&!TrialCoopAimBlurSourceView(0x923d1));
            assert(!std::memcmp(before.data(),Bytes()+address,80));}
        assert(!restores);++cases;
    }
    // Fullscreen/nested helpers cannot inherit an outer player's crop.
    {auto c=Setup(0);TrialCoopAimBlurRenderScope outer(c,Bytes());OriginalDownsample(c,Bytes());TrialCoopAimBlurDownsample(c,Bytes());
        assert(TrialCoopAimBlurSourceView(0x923d1)==1);Put(c.esp,0);
        {TrialCoopAimBlurRenderScope inner(c,Bytes());assert(!TrialCoopAimBlurSourceView(0x923d1));}
        assert(TrialCoopAimBlurSourceView(0x923d1)==1);}
    try {auto c=Setup(0);TrialCoopAimBlurRenderScope scope(c,Bytes());
        OriginalDownsample(c,Bytes());TrialCoopAimBlurDownsample(c,Bytes());View(full);throw 7;
    }catch(int value){assert(value==7&&!restores&&!TrialCoopAimBlurSourceView(0x923d1));}
    std::printf("co-op aim blur: %u retail quad/guard cases, Gaussian radius and viewport restoration passed\n",cases);
}

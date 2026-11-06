#include "xbe_coop_aim_blur_render.h"
#include "xbe_coop_aim_blur.h"
#include "cpu/x86_ops.h"
#include "kernel/kernel.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include <cmath>
#include <cstring>
#include <exception>
#include <xmmintrin.h>

namespace {
thread_local TrialCoopAimBlurRenderScope* current=nullptr;
struct PreserveFp {unsigned csr=_mm_getcsr();~PreserveFp(){_mm_setcsr(csr);}};
uint32_t Word(const uint8_t* b,uint32_t a){uint32_t v;std::memcpy(&v,b+a,4);return v;}
bool Draw(const X86Context& c,const uint8_t* b,unsigned primitive,unsigned stride,uint32_t& address){
    if(!b||!TrialGuestReadable(c.esp,16)||Word(b,c.esp)!=primitive||
       Word(b,c.esp+4)!=4||Word(b,c.esp+12)!=stride)return false;
    address=Word(b,c.esp+8);return TrialGuestReadable(address,4*stride);
}
}

TrialCoopAimBlurRenderScope::TrialCoopAimBlurRenderScope(X86Context& c,uint8_t* b)
    :context(c),base(b),previous(current),exceptions(std::uncaught_exceptions()){
    current=this;
    const int owner=TrialCoopAimBlurActiveView();
    if(!b||owner<0||owner>1||!TrialGuestReadable(c.esp,4)||Word(b,c.esp)!=0x97c0bu||
       !TrialGuestReadable(0x3f4238,4))return;
    const uint32_t d=Word(b,0x3f4238);
    if(!TrialGuestReadable(d,0x1a18))return;
    target=Word(b,d+0x1a04);depth=Word(b,d+0x1a08);
    if(!target||target!=Word(b,d+0x1a14))return;
    std::memcpy(incoming.data(),b+d+0xee0,24);
    if(incoming[0]||!incoming[2]||!incoming[3]||incoming[4]||incoming[5]!=0x3f800000u)return;
    device=d;view=owner;
}

TrialCoopAimBlurRenderScope::~TrialCoopAimBlurRenderScope() noexcept(false){
    current=previous;
    if(view<0||!copied||std::uncaught_exceptions()!=exceptions||
       !TrialGuestReadable(0x3f4238,4)||Word(base,0x3f4238)!=device||
       !TrialGuestReadable(device,0x1a18)||Word(base,device+0x1a04)!=target||
       Word(base,device+0x1a08)!=depth||Word(base,device+0x1a14)!=target)return;
    std::array<uint32_t,6> full{0,0,viewport[2],viewport[3]*2,0,0x3f800000u};
    if(incoming==full||std::memcmp(base+device+0xee0,full.data(),24)||context.esp<0x10100u||
       !TrialGuestReadable(context.esp-0x100,0x100))return;
    const uint32_t scratch=context.esp-0x100;
    struct Restore {
        uint8_t* b;uint32_t a;std::array<uint8_t,256> bytes;unsigned csr;
        ~Restore(){std::memcpy(b+a,bytes.data(),bytes.size());_mm_setcsr(csr);}
    } restore{base,scratch,{},_mm_getcsr()};
    std::memcpy(restore.bytes.data(),base+scratch,restore.bytes.size());
    std::memcpy(base+scratch,incoming.data(),24);
    X86Context temporary=context;temporary.esp=scratch+0xc0;
    kernel::CallGuest(temporary,0x3e3610,{scratch});
}

void TrialCoopAimBlurDownsample(const X86Context& c,uint8_t* b){
    PreserveFp fp;
    auto* s=current;uint32_t address=0;
    if(!s||s->base!=b||s->view<0||s->copied||!Draw(c,b,6,20,address))return;
    float v[4][5];std::memcpy(v,b+address,sizeof(v));
    // 920D8 obtains the full backbuffer description. The original quad's UV
    // extents come from that description, while its positions use the half-size
    // scratch. Earlier effects can leave the device viewport fullscreen for
    // either player, so it cannot identify this camera's scene rectangle.
    const float width=v[1][3],height=v[2][4];
    if(!std::isfinite(width)||!std::isfinite(height)||width<2||height<2||
       width>16777216.f||height>16777216.f)return;
    const auto w=uint32_t(width),h=uint32_t(height);
    if(float(w)!=width||float(h)!=height||(w&1)||(h&1))return;
    const float x[4]{0,width*.5f,0,width*.5f},y[4]{0,0,height*.5f,height*.5f};
    for(unsigned i=0;i<4;++i)
        if(v[i][0]!=x[i]||v[i][1]!=y[i]||v[i][2]!=1||v[i][3]!=x[i]*2||v[i][4]!=y[i]*2)return;
    const std::array<uint32_t,6> full{0,0,w,h,0,0x3f800000u};
    const std::array<uint32_t,6> owned{0,unsigned(s->view)*(h/2),w,h/2,0,0x3f800000u};
    if(s->incoming!=full&&s->incoming!=owned)return;
    s->viewport=owned;
    for(auto& vertex:v)vertex[4]=float(s->viewport[1])+vertex[4]*.5f;
    std::memcpy(b+address,v,sizeof(v));s->copied=true;
}

void TrialCoopAimBlurVertical(const X86Context& c,uint8_t* b){
    PreserveFp fp;
    auto* s=current;uint32_t address=0;
    if(!s||s->base!=b||!s->copied||!TrialGuestReadable(c.ebp+4,4)||
       Word(b,c.ebp+4)!=0x923eau||!Draw(c,b,6,44,address))return;
    float v[4][11];std::memcpy(v,b+address,sizeof(v));
    for(const auto& vertex:v)for(float value:vertex)if(!std::isfinite(value))return;
    // Each player's half-height source occupies the original full-height
    // scratch. Preserve the original vertical footprint without shrinking or
    // reinterpreting live GPU resources. The retained scratch has more rows.
    for(auto& vertex:v)for(unsigned i=4;i<11;i+=2)vertex[i]=vertex[1]+2*(vertex[i]-vertex[1]);
    std::memcpy(b+address,v,sizeof(v));
}

void TrialCoopAimBlurComposite(const X86Context& c,uint8_t* b){
    PreserveFp fp;
    auto* s=current;uint32_t address=0;
    if(!s||s->base!=b||!s->copied||!Draw(c,b,8,32,address))return;
    float v[4][8];std::memcpy(v,b+address,sizeof(v));
    const float w=float(s->viewport[2]),h=float(s->viewport[3]*2);
    const float x[4]{0,w,w,0},y[4]{0,0,h,h};
    for(unsigned i=0;i<4;++i)
        if(v[i][0]!=x[i]||v[i][1]!=y[i]||v[i][4]!=x[i]||v[i][5]!=y[i]||
           v[i][6]!=x[i]*.5f||v[i][7]!=y[i]*.5f)return;
    // The packed-depth lookup stays in the player's part of the full scene;
    // colour spans the whole isolated scratch, so neither input mixes views.
    for(auto& vertex:v){vertex[1]=float(s->viewport[1])+vertex[1]*.5f;vertex[5]=vertex[1];}
    std::memcpy(b+address,v,sizeof(v));
}

unsigned TrialCoopAimBlurSourceView(uint32_t caller){
    return caller==0x923d1u&&current&&current->copied?unsigned(current->view+1):0;
}

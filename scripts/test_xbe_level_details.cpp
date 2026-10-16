// Execute the original detail-list lifecycle and both render owners. Only the
// easing curve and final font/image sinks are fixture boundaries; the same
// curve is used for original/adapted runs to compare every updated field.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "kernel/kernel.h"
#include "xbe_frontend.h"
namespace kernel {
static std::array<uint8_t,0x800000> memory{};
uint8_t* g_base=memory.data();
}
using namespace kernel;
namespace mem {
static uint32_t denied=0xffffffff;
bool IsCommitted(uint32_t page){return page<kernel::memory.size()&&page!=denied;}
}
static constexpr uint32_t owner=0x600000,list=owner+0xf4,head=0x601000;
static constexpr uint32_t node=0x601020,widget=0x602000,font=0x603000,stack=0x700000;
static TrialFrontendPresentation presentation;
bool TrialFrontendPresents(uint32_t p,uint32_t k){return presentation.Presents(p,k);}
#include "../common/xbe_level_details.cpp"
X86_EXTERN_FUNC(sub_00273730);
X86_EXTERN_FUNC(sub_00273860);
X86_EXTERN_FUNC(sub_002739A0);
X86_EXTERN_FUNC(sub_00274320);
struct TextDraw {uint32_t prefix,suffix,flags,scale,color;float x,y,blur,left,right;};
static std::vector<TextDraw> text_draws;
static unsigned image_draws,destructions,node_frees;
static float clip_left,clip_right;
static uint32_t color;

X86_FUNC(sub_00273270){
    assert(X86_MEM32(ctx.esp+4)==1);
    const float t=X86_MEMF32(ctx.esp+8);
    const uint32_t derivative=X86_MEM32(ctx.esp+16);
    X86_MEMF32(derivative)=1.f;
    x87_push(ctx,1.f-t);ctx.esp+=4;
}
X86_FUNC(sub_00017060){assert(ctx.ecx==font);ctx.esp+=8;}
X86_FUNC(sub_00017160){assert(ctx.ecx==font);color=X86_MEM32(ctx.esp+4);ctx.esp+=12;}
X86_FUNC(sub_00017180){
    assert(ctx.ecx==font);clip_left=X86_MEMF32(ctx.esp+4);clip_right=X86_MEMF32(ctx.esp+8);ctx.esp+=12;
}
X86_FUNC(sub_00087100){
    const uint32_t r=X86_MEM32(ctx.esp+4),g=X86_MEM32(ctx.esp+8),b=X86_MEM32(ctx.esp+12),a=X86_MEM32(ctx.esp+16);
    X86_MEM32(ctx.ecx)=(a<<24)|(r<<16)|(g<<8)|b;ctx.eax=ctx.ecx;ctx.esp+=20;
}
X86_FUNC(sub_00018CE0){
    assert(ctx.ecx==font&&X86_MEM32(ctx.esp+28)==0);
    text_draws.push_back({X86_MEM32(ctx.esp+4),X86_MEM32(ctx.esp+24),X86_MEM32(ctx.esp+16),
        X86_MEM32(ctx.esp+20),color,X86_MEMF32(ctx.esp+8),X86_MEMF32(ctx.esp+12),
        G<float>(font+0x44),clip_left,clip_right});ctx.esp+=32;
}
X86_FUNC(sub_00065760){assert(X86_MEM32(ctx.esp+4)==0x604000);++image_draws;ctx.esp+=4;}
X86_FUNC(sub_00065550){assert(X86_MEM32(ctx.esp+4)==0x604000);++image_draws;ctx.esp+=4;}
X86_FUNC(sub_002D5CCD){assert(X86_MEM32(ctx.esp+4)==node);++node_frees;ctx.esp+=4;}
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
void x86_icall(X86Context& ctx,uint8_t* base,uint32_t target){
    switch(target){
    case 0x273730:sub_00273730(ctx,base);return;
    case 0x273860:sub_00273860(ctx,base);return;
    case 0x2739a0:sub_002739A0(ctx,base);return;
    case 0x25d1b0:assert(ctx.ecx==widget&&X86_MEM32(ctx.esp+4)==1);++destructions;ctx.esp+=8;return;
    default:std::abort();
    }
}
// GENERATED_ORIGINAL

static void Reset(bool active,bool outgoing,bool icon=false){
    memory.fill(0);mem::denied=0xffffffff;
    TrialFrontendFrame frame{};frame.source.epoch=1;frame.source.owner=owner;
    frame.source.screen=0x600200;frame.source.kind=UC_HF_LEVELS;frame.source.count=3;
    presentation.Publish(frame,active);
    text_draws.clear();image_draws=destructions=node_frees=0;clip_left=clip_right=0;color=0;
    W32(owner,0x4885fc);W32(owner+0xec,3);W32(owner+0x1c,0);
    W32(list+4,head);W32(list+8,1);W32(head,node);W32(head+4,node);
    W32(node,head);W32(node+4,head);W32(node+8,widget);
    W32(0x48905c,0x25d1b0);W32(0x489060,0x273860);
    W32(0x452f1c,0x25d1b0);W32(0x452f20,0x2739a0);
    W32(widget,icon?0x452f1c:0x48905c);
    G<float>(widget+4)=100;G<float>(widget+8)=.6f;
    G<float>(widget+0xc)=G<float>(widget+0x10)=300;
    G<float>(widget+0x14)=.1f;G<float>(widget+0x18)=1;
    G<uint8_t>(widget+0x1c)=outgoing;G<float>(widget+0x24)=100;G<float>(widget+0x38)=1;
    if(icon){
        W32(widget+0x3c,0x604000);G<float>(widget+0x40)=G<float>(widget+0x44)=24;
        W32(widget+0x48,0xffffffff);
    }else{
        std::strcpy(reinterpret_cast<char*>(g_base+widget+0x3c),"Original mission details");
        std::strcpy(reinterpret_cast<char*>(g_base+widget+0x6c),"original suffix");
        W32(widget+0x5c,1);W32(widget+0x60,0xffffffff);W32(widget+0x64,4);
        G<float>(widget+0x68)=1;G<float>(widget+0xac)=50;G<float>(widget+0xb0)=590;
    }
    W32(0x509704,font);
    // GENERATED_CONSTANTS
}
static void Step(float dt=1.f/60,uint32_t caller=0x2704df){
    X86Context c{};c.ecx=list;c.esp=stack;c.esi=0x123456;c.edi=0x234567;c.ebx=0x345678;c.ebp=0x456789;
    W32(stack,caller);G<float>(stack+4)=dt;
    sub_00274320(c,g_base);
    assert(c.esp==stack+8&&c.esi==0x123456&&c.edi==0x234567&&c.ebx==0x345678&&c.ebp==0x456789);
}
static std::array<uint8_t,0xb4> Widget(){std::array<uint8_t,0xb4> result;std::memcpy(result.data(),g_base+widget,result.size());return result;}
static void SameText(const TextDraw& a,const TextDraw& b){
    assert(a.prefix==b.prefix&&a.suffix==b.suffix&&a.flags==b.flags&&a.scale==b.scale&&a.color==b.color);
    assert(a.x==b.x&&a.y==b.y&&a.left==b.left&&a.right==b.right);
}
static X86Context TargetContext(){
    X86Context c{};c.ecx=widget;c.edi=list;c.esi=node;c.esp=stack;
    W32(stack+20,0x2704df);return c;
}
static X86Context BlurContext(){
    X86Context c{};c.esi=widget;c.ecx=font;c.esp=stack;
    W32(stack,0x2738f7);G<float>(stack+4)=60;W32(stack+8,list);W32(stack+12,node);
    W32(stack+28,0x274398);W32(stack+52,0x2704df);return c;
}
int main(){
    // Real outgoing owners update identically, but cannot draw obsolete text
    // or medals over the newly selected assignment. Original disposal survives.
    for(bool icon:{false,true}){
        Reset(false,true,icon);Step();const auto reference=Widget();
        assert(icon?image_draws==1:text_draws.size()==1);
        Reset(true,true,icon);Step();assert(Widget()==reference&&text_draws.empty()&&!image_draws);
        // Source input ownership ends during its fade, but the same validated
        // detail-list render owner must not regain obsolete text/medal draws.
        Reset(true,true,icon);TrialFrontendFrame moving{};moving.source.source_kind=UC_HF_TRANSITION;
        presentation.Publish(moving,false);assert(presentation.transition&&!TrialFrontendSupported(moving));
        Step();assert(Widget()==reference&&text_draws.empty()&&!image_draws);
        Step(1);assert(G<float>(widget+8)<=0);Step();
        assert(destructions==1&&node_frees==1&&R32(list+8)==0&&R32(head)==head&&R32(head+4)==head);
        Step();assert(destructions==1&&node_frees==1);
    }
    // New details keep all original text, transform, alpha, clipping and
    // source update fields. Only the scoped motion-trail argument is removed.
    Reset(false,false);Step();const auto reference=Widget();assert(text_draws.size()==1);
    const auto original=text_draws.front();assert(original.blur>0);
    Reset(true,false);Step();assert(Widget()==reference&&text_draws.size()==1);
    SameText(original,text_draws.front());assert(text_draws.front().blur==0&&G<float>(font+0x44)==0);
    assert(clip_left==50&&clip_right==590);
    Reset(true,false);TrialFrontendFrame moving{};moving.source.source_kind=UC_HF_TRANSITION;
    presentation.Publish(moving,false);Step();assert(text_draws.size()==1&&text_draws.front().blur==0);
    Reset(true,false,true);Step();assert(image_draws==1&&!destructions);
    Reset(true,true);Step(1.f/60,0x2704eb);assert(text_draws.size()==1&&text_draws[0].blur>0);

    // Reject other menus, containers, ownership, widget classes, corrupted
    // iterator relationships and uncommitted pages. No global font change.
    Reset(true,true);auto c=TargetContext();assert(TrialLevelDetailTarget(c,g_base,0x273860)==0x273730);
    auto unchanged=[&]{assert(TrialLevelDetailTarget(c,g_base,0x273860)==0x273860);};
    const auto stable=presentation;presentation.Publish({},false);unchanged();presentation=stable;
    presentation.frame.source.owner=owner+4;unchanged();presentation=stable;
    presentation.frame.source.kind=UC_HF_PROFILES;unchanged();presentation=stable;
    W32(stack+20,0x2704eb);unchanged();W32(stack+20,0x2704df);
    W32(owner,0xdeadbeef);unchanged();W32(owner,0x4885fc);
    W32(owner+0x1c,3);unchanged();W32(owner+0x1c,0);
    W32(widget,0x4873a4);unchanged();W32(widget,0x48905c);
    W32(head,node+4);unchanged();W32(head,node);
    W32(node+8,widget+4);unchanged();W32(node+8,widget);
    mem::denied=widget;unchanged();mem::denied=0xffffffff;
    assert(TrialLevelDetailTarget(c,g_base,0x2739a0)==0x2739a0);
    G<uint8_t>(widget+0x1c)=0;unchanged();
    Reset(true,false);c=BlurContext();TrialLevelDetailBlur(c,g_base);assert(G<float>(stack+4)==0);
    for(unsigned field:{0u,8u,12u,28u,52u}){
        c=BlurContext();W32(stack+field,0);TrialLevelDetailBlur(c,g_base);assert(G<float>(stack+4)==60);
    }
    c=BlurContext();c.ecx=font+4;TrialLevelDetailBlur(c,g_base);assert(G<float>(stack+4)==60);
    c=BlurContext();presentation.Publish({},false);TrialLevelDetailBlur(c,g_base);assert(G<float>(stack+4)==60);
    std::puts("Level detail original update/destruction, scoped draw suppression and blur contracts passed");
}

// Real localized dispatch, queue admission/packing and per-record immediate
// emitter. Font layout is a completing one-glyph service; final GPU submission
// copies the production state just as the queued backend does.
namespace d3d {
struct PromptFixtureState {unsigned uc_prompt_variant=0;};
static PromptFixtureState prompt_state;
static std::recursive_mutex prompt_d3d_lock;
static PromptFixtureState& State(){return prompt_state;}
static std::recursive_mutex& Lock(){return prompt_d3d_lock;}
}
static constexpr uint32_t kPromptRam=0x600000,kPromptOwner=0x60000,kPromptBuffer=0x61000;
static bool TrialGuestReadable(uint32_t p,uint32_t n){return p>=0x10000&&uint64_t(p)+n<=kPromptRam;}
/* PROMPT_CONTEXT_DECLARATIONS */
/* PROMPT_CONTEXT_IMPLEMENTATION */
static std::vector<unsigned> prompt_submissions;
static float prompt_input_value=0;
void x86_icall(X86Context& ctx,uint8_t* base,uint32_t target){
    CHECK(target==0xf0001&&X86_MEM32(ctx.esp+4)==0&&
          X86_MEM32(ctx.esp+8)==22&&X86_MEM32(ctx.esp+12)==2);
    x87_push(ctx,prompt_input_value);ctx.esp+=16;
}
X86_FUNC(sub_003EF700){ctx.esp+=8;}
X86_FUNC(sub_003EF600){ctx.esp+=24;}
X86_FUNC(sub_003EF5C0){ctx.esp+=12;}
X86_FUNC(sub_003EF740){prompt_submissions.push_back(d3d::State().uc_prompt_variant);ctx.esp+=4;}
X86_EXTERN_FUNC(sub_0009D9B0);
X86_FUNC(sub_00017B30){ctx.eax=0x65000;ctx.esp+=8;}
extern "C" uint32_t TrialFrontendWarningText(uint8_t*,uint32_t,uint32_t text,uint32_t,uint32_t){return text;}
static void PromptEnqueue(X86Context& ctx,uint8_t* base){
    auto saved=ctx;uint32_t stack=ctx.esp-128;ctx.esp=stack;ctx.ecx=kPromptOwner;
    X86_MEM32(stack)=0x65203;
    const uint32_t args[]={0x66000,0x66008,0xff00ff00,0x123450,0x66010,0x66018,0,0};
    for(unsigned i=0;i<8;++i)X86_MEM32(stack+4+i*4)=args[i];
    sub_0009D9B0(ctx,base);CHECK(ctx.esp==stack+36);ctx=saved;
}
X86_FUNC(sub_00018A70){
    const auto first=X86_MEM8(X86_MEM32(ctx.esp+4));
    CHECK(first=='O'||first==2||first==3);PromptEnqueue(ctx,base);ctx.esp+=28;
}
X86_FUNC(sub_00017E40){x87_push(ctx,20);ctx.esp+=20;}
X86_FUNC(sub_002745F0){CHECK(X86_MEM32(ctx.esp+4)==22);ctx.eax=prompt_input_value!=0;ctx.esp+=4;}
// The actual animated Skip text object retains its localization key. Its
// animation/font-state services complete here; its constructor, draw argument
// packing, localized dispatcher and quad queue are the production bodies.
X86_FUNC(sub_00273730){ctx.esp+=8;}
X86_FUNC(sub_00017060){ctx.esp+=8;}
X86_FUNC(sub_00017160){ctx.esp+=12;}
X86_FUNC(sub_00017730){ctx.esp+=8;}
X86_FUNC(sub_00017180){ctx.esp+=12;}
X86_FUNC(sub_00087100){X86_MEM32(ctx.ecx)=0xffffffff;ctx.eax=ctx.ecx;ctx.esp+=20;}
/* PROMPT_ORIGINAL_FUNCTIONS */
static void PromptContextChecks(){
    std::vector<uint8_t> memory(kPromptRam);uint8_t* base=memory.data();
    /* PROMPT_ORIGINAL_CONSTANTS */
    X86_MEM32(kPromptOwner)=0x456bb8;X86_MEM32(kPromptOwner+0x10)=kPromptBuffer;
    X86_MEM32(kPromptOwner+8)=0x456b44;
    X86_MEM32(kPromptOwner+0x18)=8;
    X86_MEM32(0x65000+0x34)=0x65100;X86_MEM8(0x65000+0x31)=1;X86_MEM32(0x65100)=0x65200;
    std::memcpy(base+0x65200,"Open \x02",7);
    for(unsigned i=0;i<8;++i)X86_MEMF32(0x66000+i*4)=i<4?float(20+i*10):float(i&1)*.03125f;
    auto text=[&](const char* key,uint32_t flags=0){
        std::strcpy(reinterpret_cast<char*>(base+0x64000),key);X86Context ctx{};
        ctx.esp=0x50000;ctx.ecx=0x68000;X86_MEM32(ctx.esp)=0x12345;
        X86_MEM32(ctx.esp+4)=0x64000;X86_MEM32(ctx.esp+8)=0x66000;
        X86_MEMF32(ctx.esp+12)=1;X86_MEM32(ctx.esp+16)=flags;
        X86_MEM32(ctx.esp+20)=0;X86_MEM32(ctx.esp+24)=0;X86_MEM32(ctx.esp+28)=0;
        sub_00018CE0(ctx,base);CHECK(ctx.esp==0x50020);
        CHECK(!prompt_text_variant&&d3d::State().uc_prompt_variant==0);
        CHECK(!std::strcmp(reinterpret_cast<char*>(base+0x64000),key));
    };
    auto draw=[&](unsigned index){
        X86Context ctx{};ctx.ebx=kPromptOwner+4;ctx.edi=7-index;ctx.esp=0x50000;
        PromptOriginalSelectRecord(ctx,base);
        CHECK(ctx.esp==0x4fff8&&X86_MEM32(ctx.esp)==kPromptOwner+8);
        CHECK(X86_MEM32(kPromptOwner+0x14)==kPromptBuffer+index*28);
        ctx.ecx=X86_MEM32(ctx.esp);ctx.esp=0x50000;
        sub_0009DB90(ctx,base);CHECK(ctx.esp==0x50004);
        CHECK(d3d::State().uc_prompt_variant==0);
    };
    conversation_prompt=false;text("HUD-Operate-1");text("HUD-Help-50");text("HUD-Operate-2");
    CHECK(X86_MEM32(kPromptOwner+0x1c)==3);
    // Draw later, in a different order. Each immutable GPU snapshot follows its
    // own glyph, not whichever hint or input device was most recently active.
    draw(6);draw(7);draw(5);CHECK((prompt_submissions==std::vector<unsigned>{0,1,1}));
    conversation_prompt=true;text("HUD-Help-120");conversation_prompt=false;text("HUD-Help-120");
    draw(3);draw(4);CHECK(prompt_submissions[3]==1&&prompt_submissions[4]==1);
    // Queue capacity and viewport rejection must not overwrite an earlier tag.
    const auto prior=prompt_records;X86_MEM32(kPromptOwner+0x1c)=8;
    text("HUD-Operate-4");CHECK(prompt_records==prior);
    X86_MEM32(kPromptOwner+0x1c)=5;
    X86_MEMF32(0x66000)=-40;X86_MEMF32(0x66008)=-20;
    text("HUD-Operate-8");CHECK(prompt_records==prior&&X86_MEM32(kPromptOwner+0x1c)==5);
    X86_MEMF32(0x66000)=20;X86_MEMF32(0x66008)=40;
    // Normal same-slot reuse clears the old interaction even before reset.
    X86_MEM32(kPromptOwner+0x1c)=0;text("HUD-Help-50");draw(7);CHECK(prompt_submissions.back()==0);
    TrialPromptQueueReset(kPromptOwner);CHECK(!PromptRecordVariant(base,kPromptOwner));
    X86_MEM32(kPromptOwner+0x1c)=0;text("hud-operate-1");draw(7);CHECK(prompt_submissions.back()==1);
    X86_MEM32(kPromptOwner+0x10)=kPromptBuffer+0x1000;CHECK(!PromptRecordVariant(base,kPromptOwner));
    X86_MEM32(kPromptOwner+0x10)=kPromptBuffer;
    X86_MEM32(kPromptOwner)=0;CHECK(!PromptRecordVariant(base,kPromptOwner));
    X86_MEM32(kPromptOwner)=0x456bb8;
    for(const char* key:{"HUD-Help-50","HUD-Help-400","HUD-Help-40-extra","HUD-Operate-","HUD-Operate-7","HUD-Operate-9","HUD-Operate-10",
                         "HUD-Operate-11","HUD-Operate-1-extra","newfrontend-friends-111"}){
        std::strcpy(reinterpret_cast<char*>(base+0x64000),key);
        TrialPromptTextScope scope(base,0x64000,0);CHECK(!prompt_text_variant);
    }
    std::strcpy(reinterpret_cast<char*>(base+0x64000),"HUD-Operate-1");
    base[kPromptRam-2]='x';base[kPromptRam-1]='y';
    {TrialPromptTextScope raw(base,0x64000,0x80);CHECK(!prompt_text_variant);}
    {TrialPromptTextScope outer(base,0x64000,0);CHECK(prompt_text_variant==1);
     {TrialPromptTextScope invalid(base,kPromptRam-2,0);CHECK(!prompt_text_variant);}
     CHECK(prompt_text_variant==1);}
    CHECK(!prompt_text_variant);TrialPromptQueueReset(kPromptOwner);
    X86_MEM32(kPromptOwner+0x1c)=0;text("HUD-Help-40");text("HUD-Help-50");
    draw(6);draw(7);CHECK(prompt_submissions[prompt_submissions.size()-2]==0&&prompt_submissions.back()==2);
    TrialPromptQueueReset(kPromptOwner);
    // Negative witness for the former conversation-only qualification: the
    // same original Fade text draw is used by a cinematic without that flag.
    // It must keep F through the delayed queue, while a subsequent Reload is R.
    // PlayerHelpPrompt140680 uses this same constructor with font2/flags0x11;
    // its Orders key must also survive the later draw and ordinary Reload.
    for(bool orders:{false,true}){
        conversation_prompt=false;X86_MEM32(kPromptOwner+0x1c)=0;
        const char* key=orders?"HUD-Help-40":"HUD-Help-120";
        std::strcpy(reinterpret_cast<char*>(base+0x64000),key);
        X86Context ctor{};ctor.ecx=0x69000;ctor.esp=0x50000;
        const uint32_t args[]={0x66000,orders?0x3ecccccdu:0x3e4ccccdu,3,0x64000,
                               orders?2u:1u,0xffffffff,orders?0x11u:1u,0,0x64100};
        X86_MEM8(0x64100)=0;
        for(unsigned i=0;i<9;++i)X86_MEM32(ctor.esp+4+i*4)=args[i];
        sub_00273590(ctor,base);CHECK(ctor.esp==0x50028&&ctor.eax==0x69000);
        CHECK(!std::strcmp(reinterpret_cast<char*>(base+0x6903c),key));
        CHECK(X86_MEM32(0x6905c)==args[4]&&X86_MEM32(0x69064)==args[6]);
        auto object=std::array<uint8_t,0xb4>{};std::memcpy(object.data(),base+0x69000,object.size());
        X86Context render{};render.ecx=0x69000;render.esp=0x50000;
        X86_MEM32(render.esp+4)=0;sub_00273860(render,base);
        CHECK(render.esp==0x50008&&!prompt_text_variant);
        CHECK(!std::memcmp(object.data(),base+0x69000,object.size()));
        text("HUD-Help-50");draw(6);draw(7);
        CHECK(prompt_submissions[prompt_submissions.size()-2]==0&&prompt_submissions.back()==(orders?2u:1u));
        TrialPromptQueueReset(kPromptOwner);
    }
    // Both non-conversation Skip producers poll the original A action. This
    // result is deliberately untouched; physical bindings are checked below.
    X86_MEM32(0x51c4dc)=0x6a000;X86_MEM32(0x6a000)=0x6a100;X86_MEM32(0x6a108)=0xf0001;
    for(float value:{0.f,1.f})for(bool simple:{false,true}){
        X86Context ctx{};ctx.ecx=0x6a000;ctx.esp=0x50000;prompt_input_value=value;
        if(simple)PromptSimpleCutsceneQuery(ctx,base);else PromptCinematicQuery(ctx,base);
        CHECK(ctx.esp==0x50000&&X87_ST(0)==value&&ctx.top==7);
    }
    // Actual reward publication and owned-screen footer emit localized text
    // followed by a separate literal A icon. The latter was always variant0.
    X86_MEM32(0x4847e8)=2;
    std::strcpy(reinterpret_cast<char*>(base+0x485c58),"NewFrontEnd-Continue-1");
    for(bool is_continue:{true,false}){
        TrialPromptQueueReset(kPromptOwner);X86_MEM32(kPromptOwner+0x1c)=0;
        X86Context ctx{};ctx.eax=0x6a200;PromptRewardLabel(ctx,base);
        CHECK(X86_MEM32(0x6a21c)==0x485c58);
        ctx.edi=is_continue?X86_MEM32(0x6a21c):0x64000;ctx.esi=0x4847e8;ctx.esp=0x50000;
        X86_MEMF32(ctx.esp+0x80)=500;X86_MEM32(ctx.esp+0x10)=1;
        const auto edi=ctx.edi;PromptOwnedFooter(ctx,base);
        CHECK(ctx.esp==0x50000&&ctx.edi==edi&&ctx.esi==0x4847e8&&!prompt_text_variant);
        draw(6);draw(7);CHECK(prompt_submissions[prompt_submissions.size()-2]==unsigned(is_continue));
        CHECK(prompt_submissions.back()==0);
    }
    for(unsigned wrong:{0u,1u,2u,3u}){
        TrialPromptTextScope scope(base,wrong==1?0x4847e9:0x4847e8,wrong==2?0x80:0x81,
                                  wrong==0?0x25496a:0x254969,wrong==3?0x485c59:0x485c58);
        CHECK(!prompt_text_variant);
    }
    for(float value:{0.f,1.f}){
        X86Context ctx{};ctx.esp=0x50000;prompt_input_value=value;PromptRewardQuery(ctx,base);
        CHECK(ctx.esp==0x50000&&ctx.eax==unsigned(value!=0));
    }
    TrialPromptQueueReset(kPromptOwner);
}

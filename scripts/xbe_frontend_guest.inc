#include "xbe_frontend_text.h"
extern int xbox_KeyboardMode();
// Native presentation submits identities to the original handlers. Profile
// activation and name validation retain their original callbacks/save policy.
extern "C" uint32_t TrialFrontendWarningText(uint8_t* base,uint32_t key,uint32_t text,
                                           uint32_t scratch,uint32_t capacity){
    auto key_is=[&](const char* name){
        const size_t size=std::strlen(name)+1;
        if(!base||!TrialGuestReadable(key,uint32_t(size)))return false;
        for(size_t i=0;i<size;++i){
            unsigned c=base[key+i];if(c>='A'&&c<='Z')c+='a'-'A';
            if(c!=unsigned(name[i]))return false;
        }
        return true;
    };
    if(xbox_KeyboardMode()&&(key_is("newfrontend-pressstart-1")||key_is("newfrontend-pressstart-2"))){
        if(!TrialGuestReadable(text,128)||!std::memchr(base+text,0,128))return text;
        const char* source=reinterpret_cast<const char*>(base+text);
        const char* label=nullptr;
        if(!std::strcmp(source,"PRESS START BUTTON"))label="PRESS ANY KEY";
        else if(!std::strncmp(source,"APPUYER SUR",11))label="APPUYEZ SUR UNE TOUCHE";
        else if(!std::strncmp(source,"PREMI ",6))label="PREMI UN TASTO";
        else if(!std::strncmp(source,"START",5))label="BELIEBIGE TASTE DR\xDC" "CKEN";
        if(label&&std::strlen(label)+1<=capacity&&TrialGuestReadable(scratch,uint32_t(std::strlen(label)+1))){
            std::memcpy(base+scratch,label,std::strlen(label)+1);return scratch;
        }
        return text;
    }
    // Only the two localized profile I/O warnings. The original render/measure
    // functions already reserve scratch strings; keep the bank and save logic
    // untouched, and use the same replacement in both presentation paths.
    constexpr char prefix[]="xboxtrc-memcard-";
    constexpr unsigned n=sizeof(prefix)-1;
    if(!base||!TrialGuestReadable(key,n+2))return text;
    for(unsigned i=0;i<n;++i){
        unsigned c=base[key+i];if(c>='A'&&c<='Z')c+='a'-'A';
        if(c!=unsigned(prefix[i]))return text;
    }
    if((base[key+n]!='1'&&base[key+n]!='2')||base[key+n+1])return text;
    if(!TrialGuestReadable(text,512))return text;
    const char* source=reinterpret_cast<const char*>(base+text);
    const char* end=static_cast<const char*>(std::memchr(source,0,512));
    if(!end)return text;
    // The supported text bank has English, French, Italian and German blocks.
    // Retain all translated wording except the console noun and its article.
    const char* from=nullptr;const char* to=nullptr;const char* at=nullptr;
    struct Phrase{const char* from;const char* to;};
    constexpr Phrase phrases[]={{"Xbox console","PC"},{"la console Xbox","le PC"},
                                {"die Xbox","den PC"},{"Xbox","il PC"}};
    for(const auto& phrase:phrases){
        if((at=std::strstr(source,phrase.from))){from=phrase.from;to=phrase.to;break;}
    }
    if(!at)return text;
    const size_t before=at-source,removed=std::strlen(from),added=std::strlen(to);
    const size_t length=(end-source)-removed+added;
    if(length+1>capacity||!TrialGuestReadable(scratch,uint32_t(length+1)))return text;
    std::memcpy(base+scratch,source,before);
    std::memcpy(base+scratch+before,to,added);
    std::memcpy(base+scratch+before+added,at+removed,end-(at+removed)+1);
    return scratch;
}
static uint32_t frontend_name_owner,frontend_name_cancel;
static uint64_t frontend_name_cancel_epoch;
// The title's delete callback uses the global selected slot. Retain the exact
// slot record while its original confirmation child is active, so a stale
// click can never target a newly selected or replaced profile.
static struct {
    uint32_t owner=0,screen=0,index=0,dialog=0;
    bool requested=false;
    uint8_t slot[0x19C]{};
    char name[20]{};
} frontend_delete;
static bool FrontendProfileCanDelete(uint32_t owner,unsigned index){
    return menu_profile_valid(owner)&&MEM32(owner+4)==0x51D710&&
        MEM32(owner+0x10)==0x284580&&index<MEM32(owner+0x688);
}
static bool FrontendProfileDeleteValid(){
    const auto& d=frontend_delete;
    return d.owner&&d.screen==menu_screen&&FrontendProfileCanDelete(d.owner,d.index)&&
        MEM32(0x51D710)==d.index&&
        !std::memcmp(ram+d.owner+0x18+d.index*0x19C,d.slot,sizeof(d.slot));
}
static bool FrontendProfileDelete(uint32_t owner,unsigned index){
    if(!FrontendProfileCanDelete(owner,index))return false;
    frontend_delete={};auto& d=frontend_delete;
    d.owner=owner;d.screen=menu_screen;d.index=index;d.requested=true;
    std::memcpy(d.slot,ram+owner+0x18+index*0x19C,sizeof(d.slot));
    if(!hf_inline(owner+0x1C+index*0x19C,d.name,sizeof(d.name))||!d.name[0]){d={};return false;}
    return true;
}
static bool FrontendProfileDeleteQuery(){
    auto& d=frontend_delete;
    if(!d.requested||!profile_running||profile_owner!=d.owner||g_esi!=d.owner||
       !hf_settled()||!menu_pointer_readable(g_esp,8)||
       MEM32(g_esp)!=0x2842AF||MEM32(g_esp+4)!=0x18)return false;
    d.requested=false;
    if(FrontendProfileDeleteValid())return true;
    d={};return false;
}
static bool FrontendDeleteDialog(uint32_t owner){
    auto& d=frontend_delete;
    if(!d.owner||d.requested||d.screen!=menu_screen||!menu_dialog_valid(owner)||
       MEM32(owner+4)!=0x48CB3C||MEM32(owner+0xC)!=2||
       MEM32(owner+0x30)!=0x284580||MEM32(owner+0x34)!=0x284310||
       MEM32(owner+0x50)!=0x284310)return false;
    // Bind once. Keep an invalidated slot's dialog visible with Yes disabled;
    // dropping native ownership would expose the stale original Yes instead.
    if(!d.dialog)d.dialog=owner;
    return d.dialog==owner;
}
static bool FrontendDeleteChoice(uint32_t owner,unsigned index){
    return !FrontendDeleteDialog(owner)||index!=0||FrontendProfileDeleteValid();
}
static bool FrontendDeleteParent(){
    // The profile update draws its parent before the active confirmation child.
    // Do not revoke that same child's input/epoch on every parent draw.
    return profile_running&&profile_owner==frontend_delete.owner&&frontend_delete.dialog&&
        hf_view.kind==UC_HF_DIALOG&&hf_view.owner==frontend_delete.dialog&&
        FrontendDeleteDialog(frontend_delete.dialog);
}
static bool FrontendPauseDialog(uint32_t owner){
    // Pause restart/checkpoint/quit rows share 263520 -> 265D9E/258DF0.
    // Its No and Back both return through the original 284310 callback.
    return menu_manager&&hf_pause_manager==menu_manager&&
        menu_pointer_readable(menu_manager,0x15)&&MEM32(menu_manager+4)==menu_screen&&
        menu_dialog_valid(owner)&&MEM32(owner+4)==0x473368&&MEM32(owner+0xC)==2&&
        MEM32(owner+0x10)==0x48573C&&MEM32(owner+0x14)==0x485750&&
        TrialFrontendPauseTitle(MEM32(owner+0x30))&&MEM32(owner+0x34)==0x284310&&
        MEM32(owner+0x50)==0x284310;
}
static bool FrontendDifficultyDialog(uint32_t owner){
    if(!menu_dialog_valid(owner))return false;
    UcHybridFrontendSnapshot s{};s.kind=UC_HF_DIALOG;s.count=MEM32(owner+0xC);
    if((s.count!=3&&s.count!=4)||!hf_text(MEM32(owner+4),s.title))return false;
    for(unsigned i=0;i<s.count;++i){
        s.rows[i].callback=MEM32(owner+0x30+4*i);
        if(!hf_text(MEM32(owner+0x10+4*i),s.rows[i].label))return false;
    }
    return TrialFrontendDifficulty(s);
}
static void FrontendPauseDialogEnter(){
    // Reuse the original modal capture before its first glyph submission, so
    // presentation must accept the replacement before any source text hides.
    if(!xbox_KeyboardMode()||!hf_settled()||!(FrontendPauseDialog(dialog_owner)||FrontendDifficultyDialog(dialog_owner))||
       (hf_view.kind==UC_HF_DIALOG&&hf_view.owner==dialog_owner&&
        hf_view.screen==menu_screen&&TrialFrontendPresents(dialog_owner,UC_HF_DIALOG)))return;
    menu_dialog_capture();
    if(hf_pending.kind==UC_HF_DIALOG&&hf_pending.owner==dialog_owner)hf_publish();
}
bool TrialFrontendHidePauseDialogText(uint32_t owner){
    // Suppress at the three original text calls, before glyphs are queued.
    // The shade/backdrop and original layout, sounds and callbacks still run.
    return xbox_KeyboardMode()&&dialog_running&&dialog_owner==owner&&
        (FrontendPauseDialog(owner)||FrontendDifficultyDialog(owner))&&TrialFrontendPresents(owner,UC_HF_DIALOG);
}
static bool FrontendNameValid(uint32_t owner){
    return menu_pointer_readable(owner,0x58)&&MEM32(owner)==0x48CD08&&!MEM8(owner+4)&&
        MEM32(owner+8)==0x51D71C&&MEM32(owner+0xC)==15&&MEM32(owner+0x10)==0x283250&&
        (MEM32(owner+0x18)&~8u)==0x52&&menu_pointer_readable(MEM32(owner+0x14),0x678)&&
        MEM32(MEM32(owner+0x14))==0x48CAD8&&MEM32(MEM32(owner+0x14)+0x674)<=4&&
        menu_pointer_readable(0x51D71C,16)&&std::memchr(ram+0x51D71C,0,16);
}
extern "C" int ucrr_native_frontend_enabled(){return 1;}
extern "C" int ucrr_native_frontend_take_action(UcHybridFrontendAction* action){
    return action&&TrialFrontendTakeAction(*action);
}
static void FrontendText(const char* key,char (&out)[128]){
    TrialFrontendUtf8(key,out);
    if(!*key||!regs||g_esp<0x10200)return;
    const uint32_t font=MEM32(0x509704);if(!menu_pointer_readable(font,16))return;
    const X86Context saved=*regs;const uint32_t scratch=(g_esp-160)&~15u;
    if(!menu_pointer_readable(scratch-16,176))return;
    std::memcpy(ram+scratch,key,128);g_esp=scratch;g_ecx=font;
    // 17B30 is the original read-only localized record lookup used by17E40.
    const uint32_t record=kernel::CallGuest(*regs,0x17B30,{scratch});
    if(menu_pointer_readable(record,0x38)&&MEM8(record+0x31)){
        const uint32_t variants=MEM32(record+0x34);
        char text[128]{};
        if(menu_pointer_readable(variants,12)&&hf_text(MEM32(variants),text))TrialFrontendUtf8(text,out);
    }
    *regs=saved;
}
extern "C" void ucrr_native_frontend_publish(const UcHybridFrontendSnapshot* source){
    TrialFrontendFrame frame{};
    if(source){
        frame.source=*source;
        if(source->kind==UC_HF_LIST)frame.list_kind=GraphicsMenuKind(source->owner);
        if(source->kind==UC_HF_PROFILES){
            if(frontend_delete.dialog)frontend_delete={};
            for(unsigned i=0;i<4;++i)if(FrontendProfileCanDelete(source->owner,i))frame.profile_delete_mask|=1u<<i;
            for(unsigned i=0;i<profile_count;++i){const auto& t=profile_targets[i];
                if(t.rect.owner==source->owner&&t.rect.index<4&&t.frame==special_frame)
                    frame.profile_bounds[t.rect.index]={t.rect.x,t.rect.y,t.rect.w,t.rect.h};
            }
        }
        if(source->kind==UC_HF_DIALOG&&FrontendDeleteDialog(source->owner)){
            frame.delete_dialog=true;
            std::memcpy(frame.delete_name,frontend_delete.name,sizeof(frame.delete_name));
            if(!FrontendProfileDeleteValid())frame.source.rows[0].enabled=0;
        }
        if(source->kind==UC_HF_DIALOG&&FrontendPauseDialog(source->owner)){
            frame.pause_dialog=true;
            FrontendText(source->title,frame.dialog_message);
        }
        if(source->kind==TRIAL_HF_NAME&&FrontendNameValid(source->owner)){
            frame.name_capacity=15;
            hf_inline(0x51D71C,frame.name,sizeof(frame.name));
            char error[128]{};
            if(hf_text(MEM32(source->owner+0x34),error))FrontendText(error,frame.name_error);
        }
        if(TrialFrontendSupported(frame)){
            static TrialFrontendFrame cached{};
            if(source->kind!=UC_HF_PROFILES&&source->kind!=TRIAL_HF_NAME&&source->kind!=UC_HF_DIALOG&&
               cached.source.epoch==source->epoch&&cached.source.owner==source->owner){
                frame=cached;frame.source=*source;
            }else{
                for(unsigned i=0;i<source->count;++i)FrontendText(source->rows[i].label,frame.labels[i]);
                frame.row_top=480;
                for(unsigned i=0;i<menu_target_count;++i)if(menu_targets[i].owner==source->owner)
                    frame.row_top=std::min(frame.row_top,menu_targets[i].y);
                frame.row_top=std::clamp(frame.row_top-4.f,72.f,390.f);
                cached=frame;
            }
            if(frame.list_kind>=4&&frame.list_kind<=6)
                for(unsigned i=0;i<source->count;++i)FrontendText(source->rows[i].value,frame.values[i]);
        }
    }
    TrialFrontendPublish(frame);
}
static bool FrontendGuestOwns(uint32_t owner,uint32_t kind){
    return (kind==UC_HF_LEVELS||kind==UC_HF_REWARD_BROWSER||kind==UC_HF_PROFILES||kind==TRIAL_HF_NAME||kind==UC_HF_OBJECTIVES||
        (kind==UC_HF_DIALOG&&(FrontendDeleteDialog(owner)||FrontendPauseDialog(owner)||FrontendDifficultyDialog(owner)))||
        GraphicsMenuKind(owner))&&TrialFrontendOwns(owner,kind);
}

static uint32_t frontend_objectives_owner,frontend_objectives_action;
static bool FrontendObjectivesValid(uint32_t owner){
    return menu_pointer_readable(owner,0xFC)&&MEM32(owner)==0x487588&&
        MEM32(owner+4)==0x51CF9C&&MEM32(owner+0x58)<=20&&MEM32(owner+0xAC)&&
        MEM32(owner+0xAC)!=0xffffffffu;
}
static void FrontendObjectivesEnter(){
    frontend_objectives_action=0;
    frontend_objectives_owner=FrontendObjectivesValid(g_ecx)?g_ecx:0;
    if(frontend_objectives_owner){menu_seen=1;menu_render_count=1;}
}
static void FrontendObjectivesLeave(){
    const auto owner=frontend_objectives_owner;
    frontend_objectives_owner=frontend_objectives_action=0;
    if(!hf_settled()||!FrontendObjectivesValid(owner)||MEM8(owner+0xB0))return;
    UcHybridFrontendSnapshot s{};s.kind=UC_HF_OBJECTIVES;s.owner=owner;s.count=1;
    s.back_action=MEM32(0x485A18);s.rows[0].row=owner;s.rows[0].enabled=1;
    // Keep the original grouped objectives, progress counters and medal art.
    std::strcpy(s.title,"NewFrontEnd-Objectives-1");hf_pause_manager=menu_manager;hf_pending=s;
}
static void FrontendObjectivesInput(){
    const auto owner=frontend_objectives_owner;
    if(owner!=g_edi||!hf_settled()||!FrontendObjectivesValid(owner)||
       !FrontendGuestOwns(owner,UC_HF_OBJECTIVES)||hf_view.screen!=menu_screen)return;
    UcHybridFrontendAction a{};
    if(!TrialFrontendTakeAction(a)||a.epoch!=hf_view.epoch||a.owner!=owner||
       a.screen!=menu_screen||a.row!=owner||a.index)return;
    if(a.kind==UC_HF_BACK||a.kind==UC_HF_LEFT||a.kind==UC_HF_RIGHT)frontend_objectives_action=a.kind;
}
static bool FrontendObjectivesQuery(){
    const auto owner=frontend_objectives_owner;
    if(!frontend_objectives_action||g_edi!=owner||!hf_settled()||
       !FrontendObjectivesValid(owner)||!FrontendGuestOwns(owner,UC_HF_OBJECTIVES)||
       !menu_pointer_readable(g_esp,8))return false;
    const uint32_t caller=MEM32(g_esp),action=MEM32(g_esp+4);
    return (frontend_objectives_action==UC_HF_BACK&&caller==0x26316A&&action==hf_view.back_action)||
        (frontend_objectives_action==UC_HF_LEFT&&caller==0x26319C&&action==0x1E)||
        (frontend_objectives_action==UC_HF_RIGHT&&caller==0x2631DE&&action==0x1F);
}

static void FrontendNameEnter(){
    frontend_name_owner=FrontendNameValid(g_ecx)?g_ecx:0;
    if(frontend_name_owner){menu_seen=1;menu_render_count=1;}
}
static bool FrontendNameInput(){
    const uint32_t owner=frontend_name_owner;
    if(owner!=g_esi||!hf_settled()||!FrontendNameValid(owner)||
       !FrontendGuestOwns(owner,TRIAL_HF_NAME)||hf_view.screen!=menu_screen)return false;
    TrialFrontendNameActionData request{};
    if(!TrialFrontendTakeName(request))return true;
    const auto& a=request.identity;
    if(a.epoch!=hf_view.epoch||a.screen!=menu_screen||a.owner!=owner||
       a.row!=MEM32(owner+8)||a.index||!TrialFrontendNameText(request.text))return true;
    if(a.kind==UC_HF_BACK){frontend_name_cancel=owner;frontend_name_cancel_epoch=a.epoch;return true;}
    if(a.kind!=UC_HF_SELECT&&a.kind!=UC_HF_ACTIVATE)return true;
    std::memcpy(ram+MEM32(owner+8),request.text,sizeof(request.text));
    MEM32(owner+0x18)&=~8u;MEM32(owner+0x34)=0;
    if(a.kind==UC_HF_ACTIVATE&&menu_pointer_readable(g_esp,0x28)&&g_esp>=0x10200){
        // Exact 284C60/284DCF contract: validate the name through2858A0.
        // Its real validator chooses errors/duplicates/default names. The
        // original284EF6..285690 then renders, frees the validator on success,
        // and returns the original callback to the original screen manager.
        const X86Context saved=*regs;g_ecx=owner;
        const uint32_t callback=kernel::CallGuest(*regs,0x2858A0,{MEM32(owner+0x10),owner+0x34,0});
        *regs=saved;MEM32(g_esp+0x24)=callback;
    }
    return true;
}
static void FrontendNameLeave(){
    const uint32_t owner=frontend_name_owner;
    if(!hf_settled()||!FrontendNameValid(owner))return;
    UcHybridFrontendSnapshot s{};s.kind=TRIAL_HF_NAME;s.owner=owner;s.count=1;s.back_action=0x17;
    std::strcpy(s.title,"New profile");auto& r=s.rows[0];r.row=MEM32(owner+8);r.index=0;
    r.callback=MEM32(owner+0x10);r.enabled=1;r.actions=UC_HF_ROW_ACTIVATE;
    std::strcpy(r.label,"Name");hf_pending=s;
}
static bool FrontendNameQuery(){
    if(!frontend_name_cancel||hf_view.kind!=TRIAL_HF_NAME||hf_view.owner!=frontend_name_cancel||
       hf_view.epoch!=frontend_name_cancel_epoch||!hf_settled()||!FrontendNameValid(frontend_name_cancel)||
       !menu_pointer_readable(g_esp,8)||MEM32(g_esp)!=0x276723||MEM32(g_esp+4)!=0x17||
       !menu_pointer_readable(g_edi,0x20)||MEM32(g_edi)!=0x489974||MEM32(g_edi+4)!=0x17||
       MEM32(g_edi+8)!=0x271810)return false;
    frontend_name_cancel=0;return true;
}
static void FrontendListEnter(){
    if(!GraphicsMenuKind(menu_owner))TrialFrontendPublish({});
}
thread_local uint32_t TrialFrontendProfileDrawOwner=0;
thread_local bool TrialFrontendLevelDrawHidden=false;
bool TrialFrontendLevelDrawBegin(uint32_t owner){
    if(!xbox_KeyboardMode()||!hf_settled()||!menu_pointer_readable(owner,0x118)||
       MEM32(owner)!=0x4885FC)return false;
    const uint32_t wrapper=MEM32(owner+0x10C);
    if(!menu_pointer_readable(wrapper,0x14)||MEM32(wrapper)!=0x485C84)return false;
    // 258D40 wraps the actual modal in253FF0's shade object before25A150
    // installs it in the campaign wrapper.2546B0 delegates to its +4 child.
    const uint32_t shade=MEM32(wrapper+4);
    if(!menu_pointer_readable(shade,0x10)||MEM32(shade)!=0x4846A4)return false;
    const uint32_t child=MEM32(shade+4);
    if(!FrontendDifficultyDialog(child))return false;
    // Accept the actual modal before its parent's sprites are submitted.
    const uint32_t saved_owner=dialog_owner;const int saved_running=dialog_running;
    dialog_owner=child;dialog_running=1;FrontendPauseDialogEnter();
    dialog_owner=saved_owner;dialog_running=saved_running;
    return hf_view.screen==menu_screen&&hf_view.owner==child&&
        TrialFrontendDifficulty(hf_view)&&TrialFrontendPresents(child,UC_HF_DIALOG);
}
bool TrialFrontendHideListSprites(){
    // Modal children share the parent's draw scope; only their own accepted replacement may hide them.
    if(dialog_running)return false;
    // Incoming/outgoing lists can draw before host ownership changes. Hide their
    // submissions throughout mouse mode; the parent backdrop is outside this scope.
    return xbox_KeyboardMode()&&(TrialFrontendLevelDrawHidden||(menu_owner&&GraphicsMenuKind(menu_owner))||
        (TrialFrontendProfileDrawOwner&&TrialFrontendPresents(TrialFrontendProfileDrawOwner,UC_HF_PROFILES)));
}
static void FrontendRewardSidebar(){
    const uint32_t owner=g_esi;
    if(!xbox_KeyboardMode()||!hf_settled()||!menu_pointer_readable(owner,0x34)||
       MEM32(owner)!=0x485F14||!TrialFrontendPresents(owner,UC_HF_REWARD_BROWSER))return;
    const unsigned count=MEM32(owner+0x18),selected=MEM32(owner+4);
    const uint32_t records=MEM32(owner+0x2C);
    if(!count||count>55||selected>=count||!menu_pointer_readable(records,count*44)||g_esp<0x10200)return;
    const X86Context saved=*regs;const uint32_t scratch=(g_esp-128)&~15u;
    if(!menu_pointer_readable(scratch-32,160))return;
    auto draw=[&](uint32_t texture,float x,float y,float w,float h,uint32_t color){
        MEMF(scratch)=x;MEMF(scratch+4)=y;MEMF(scratch+8)=x+w;MEMF(scratch+12)=y+h;MEM32(scratch+16)=color;
        g_esp=scratch;
        if(texture)kernel::CallGuest(*regs,0x65760,{texture,scratch,scratch+8,scratch+16,0});
        else kernel::CallGuest(*regs,0x656D0,{scratch,scratch+8,scratch+16});
    };
    // Same opaque sidebar as level select; keep the original detail panel intact.
    draw(0,0,73,47,306,0xff000000);draw(0,46,73,1,306,0xff858b89);
    const unsigned first=selected/TrialRewardsPerPage*TrialRewardsPerPage;
    for(unsigned i=first;i<std::min(count,first+TrialRewardsPerPage);++i){
        const uint32_t record=records+i*44;
        const uint32_t art=MEM8(record+0x28)?MEM32(record+0x14):0x4FF4BC;
        if(!menu_pointer_readable(art,24))continue;
        const uint32_t texture=MEM32(art+16);if(!texture)continue;
        const auto r=TrialLevelBounds(i-first,TrialRewardsPerPage);const float size=std::min(24.f,r.h-2);
        if(i==selected)draw(0,r.x,r.y,r.w,r.h,0xff3d4444);
        draw(texture,r.x+2,r.y+1,size,size,0xffffffff);
    }
    *regs=saved;
}
static bool FrontendLevelReady(){
    return xbox_KeyboardMode()&&hf_settled()&&level_owner&&menu_pointer_readable(level_owner,0x118)&&
        MEM32(level_owner)==0x4885FC&&MEM32(level_owner+0xEC)>0&&MEM32(level_owner+0xEC)<=17;
}
static bool FrontendBadge(){
    if(!FrontendLevelReady()||!menu_pointer_readable(g_esp,32)||MEM32(g_esp)!=0x27092C)return false;
    const unsigned count=MEM32(level_owner+0xEC),index=level_draw_index;
    if(index>=count||!MEM8(level_owner+0x28+12*index))return false;
    const uint32_t mission=MEM32(level_owner+0x20+12*index),bonus=MEM32(level_owner+0x24+12*index);
    const uint32_t art=mission<11?0x500A98+24*mission:mission==0xffffffffu&&bonus<6?0x500BA0:0;
    if(!art||MEM32(g_esp+4)!=art||!menu_pointer_readable(art,24)||g_esp<0x10200)return false;
    const uint32_t completion=MEM32(g_esp+28);
    if(completion!=0xffffffffu&&completion>3)return false;
    const uint32_t texture=MEM32(art+(completion<4?20:16));if(!texture)return false;
    uint32_t check=0;
    if(completion<4){
        const uint32_t branch=MEM32(0x26F688+completion*4);
        const uint32_t slot=branch==0x26F621?0x51D0DC:branch==0x26F653?0x51D0E0:branch==0x26F5EE?0x51D0E4:0;
        if(!slot||!(check=MEM32(slot)))return false;
    }
    const auto r=TrialLevelBounds(index,count);const float size=std::min(24.f,r.h-2);
    const X86Context saved=*regs;const uint32_t scratch=(g_esp-128)&~15u;
    if(!menu_pointer_readable(scratch-32,160))return false;
    auto draw=[&](uint32_t texture,float x,float y,float w,float h,uint32_t color){
        MEMF(scratch)=x;MEMF(scratch+4)=y;MEMF(scratch+8)=x+w;MEMF(scratch+12)=y+h;MEM32(scratch+16)=color;
        g_esp=scratch;
        if(texture)kernel::CallGuest(*regs,0x65760,{texture,scratch,scratch+8,scratch+16,0});
        else kernel::CallGuest(*regs,0x656D0,{scratch,scratch+8,scratch+16});
    };
    // Cover the entire panel height, including the gaps above/below the badges
    // where the original sliding detail text otherwise remains visible.
    // Details slide all the way to guest x=0. An inset backing leaves their
    // letters visible at the left edge (three guest pixels = twelve at 1440p).
    if(index==0){draw(0,0,73,47,306,0xff000000);draw(0,46,73,1,306,0xff858b89);}
    if(MEM32(level_owner+0x1C)==index)draw(0,r.x,r.y,r.w,r.h,0xff3d4444);
    draw(texture,r.x+2,r.y+1,size,size,0xffffffff);
    if(completion<4){
        //26F500's actual switch chooses completion artwork by difficulty.
        draw(check,r.x+size*.65f,r.y+size*.35f,size*.7f,size*.7f,0xffffffff);
    }
    *regs=saved;
    if(!level_render_count++)level_target_count=0;
    if(level_target_count<32){UCRR_MENU_TARGET t={level_owner,mission,index,r.x,r.y,r.w,r.h};level_targets[level_target_count++]=t;}
    hf_capture_levels(level_owner);return true;
}

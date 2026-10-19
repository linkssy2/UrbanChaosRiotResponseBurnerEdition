// Offline UI contract: actual production frame drawing and ImGui input, with
// original user fonts. No window, GPU, guest VM, save file, or game launch.
#define NOMINMAX
#include <windows.h>
#include "imgui.h"
#include "imgui_internal.h"
#include "kernel/disc.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>
#include "xbe_graphics_settings.h"
#include "xbe_controls.h"
#include "xbe_language.h"
#include "xbe_controls_capture.h"
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)

struct HitRecord {std::string name;ImGuiID id;ImVec2 a,b;};
static std::vector<HitRecord> hits;
static unsigned input_changes=0,applied=0,frames=0;
static app::Mode output{640,480};
static trial_graphics::Draft live;
static unsigned sample_support=2|(1u<<2)|(1u<<4);
static bool fail_apply=false;
static unsigned controls_applied=0;
static unsigned quit_posts=0;
static bool fail_quit_post=false;
static BOOL WINAPI RecordQuitRequest(HWND window,UINT message,WPARAM wp,LPARAM lp){
    CHECK(window==nullptr&&message==WM_CLOSE&&wp==0&&lp==0);
    if(fail_quit_post)return FALSE;
    ++quit_posts;return TRUE;
}
static void RecordMenuHit(const char* name,ImVec2 p,ImVec2 size){
    hits.push_back({name,ImGui::GetID(name),p,{p.x+size.x,p.y+size.y}});
}
bool TrialShowFps=true;
static int keyboard_mode=1;
int xbox_KeyboardMode(){return keyboard_mode;}
void TrialGraphicsInputChanged(){++input_changes;}
namespace app {
void* Window(){return nullptr;}
Mode Output(){return output;}
}
namespace trial_graphics {
Draft Snapshot(){return live;}
bool Apply(const Draft& next,std::string& status){
    if(fail_apply){status="Could not save settings.";return false;}
    live=next;++applied;status="Changes applied and saved.";return true;
}
bool ScalePending(){return false;}
bool HdUi(){return live.hd_ui;}
bool MsaaSupported(uint32_t samples){return samples<32&&(sample_support&(1u<<samples));}
}
namespace trial_language {
static uint32_t selected=0,active=0;static unsigned saves=0;
uint32_t Snapshot(){return selected;}
bool PendingRestart(){return selected!=active;}
bool Apply(uint32_t value,std::string& status){
    CHECK(value<Count);if(fail_apply){status="Could not save language.";return false;}
    selected=value;++saves;status.clear();return true;
}
}
namespace trial_controls {
static Draft live_controls;
static CaptureGate capture;
Draft Defaults(){
    Draft result;const unsigned first[25]={'R','X','C','V','T','E',VK_LCONTROL,VK_LBUTTON,VK_RETURN,VK_BACK,VK_SHIFT,VK_RBUTTON,
        VK_UP,VK_DOWN,VK_LEFT,VK_RIGHT,'A','D','W','S','J','L','I','K',VK_MBUTTON};
    for(unsigned i=0;i<25;++i)result.keyboard[i][0]=first[i];result.keyboard[0][1]='F';
    for(unsigned i=0;i<24;++i)result.controller[i][0]=i+1;
    return result;
}
Draft Snapshot(){return live_controls;}
bool Apply(const Draft& next,std::string& status){
    if(fail_apply){status="Could not save controls.";return false;}
    live_controls=next;++controls_applied;status="Controls applied and saved.";return true;
}
void BeginCapture(unsigned device){capture.Begin(device);}
void CancelCapture(){capture.Cancel();}
unsigned TakeCapture(){return capture.Take();}
bool CaptureWaiting(){return capture.active;}
}
const char* xbox_KeyboardSourceName(unsigned source){
    static char text[64];if(!source)return "Unbound";
    if(source==VK_MBUTTON)return "Mouse Middle";if(source==256)return "Wheel Up";if(source==257)return "Wheel Down";
    std::snprintf(text,sizeof(text),"Key %u",source);return text;
}
const char* xbox_ControllerSourceName(unsigned source){
    static char text[64];if(!source)return "Unbound";std::snprintf(text,sizeof(text),"Button %u",source);return text;
}
namespace kernel {
const Disc* MountedDisc(){return nullptr;}
const Disc::Entry* Disc::Find(std::string_view)const{return nullptr;}
bool Disc::Load(std::string_view,std::vector<uint8_t>&)const{return false;}
}

#define PostMessageW RecordQuitRequest
// PRODUCTION_MENU_UI
#undef PostMessageW

static void DrawFrame(){
    hits.clear();auto& io=ImGui::GetIO();io.DisplaySize={float(output.width),float(output.height)};io.DeltaTime=1.f/60;
    ImGui::NewFrame();TrialGraphicsUi();ImGui::Render();++frames;
    for(size_t i=0;i<hits.size();++i){
        const auto& h=hits[i];CHECK(h.a.x>=0&&h.a.y>=0&&h.b.x<=output.width+.1f&&h.b.y<=output.height+.1f);
        CHECK(h.b.x>h.a.x&&h.b.y>h.a.y);
        for(size_t j=0;j<i;++j)CHECK(h.id!=hits[j].id);
    }
    const auto* data=ImGui::GetDrawData();CHECK(data&&data->Valid);
    for(const auto* list:data->CmdLists)for(const auto& v:list->VtxBuffer){
        CHECK(std::isfinite(v.pos.x)&&std::isfinite(v.pos.y)&&std::isfinite(v.uv.x)&&std::isfinite(v.uv.y));
        CHECK(v.pos.x>=-2&&v.pos.x<=output.width+2&&v.pos.y>=-2&&v.pos.y<=output.height+2);
    }
}
static void Settle(){DrawFrame();DrawFrame();DrawFrame();}
static void Drain(){UcHybridFrontendAction a{};while(TrialFrontendTakeAction(a)){};}
static UcHybridFrontendAction Take(){UcHybridFrontendAction a{};CHECK(TrialFrontendTakeAction(a));return a;}
static void CheckEmptyAction(unsigned caller){
    UcHybridFrontendAction a{};if(!TrialFrontendTakeAction(a))return;
    std::fprintf(stderr,"unexpected action at caller %u: kind=%u index=%u row=%08x frame=%u size=%ux%u\n",
        caller,a.kind,a.index,a.row,frames,output.width,output.height);CHECK(false);
}
#define EmptyAction() CheckEmptyAction(__LINE__)
static TrialFrontendNameActionData TakeName(){TrialFrontendNameActionData a{};CHECK(TrialFrontendTakeName(a));return a;}
static void EmptyName(){TrialFrontendNameActionData a{};CHECK(!TrialFrontendTakeName(a));}
static void Type(const char* text){ImGui::GetIO().AddInputCharactersUTF8(text);DrawFrame();}
static void Move(float x,float y){ImGui::GetIO().AddMousePosEvent(x,y);DrawFrame();}
static void ClickPoint(float x,float y,bool settle=true){
    Move(x,y);ImGui::GetIO().AddMouseButtonEvent(0,true);DrawFrame();
    ImGui::GetIO().AddMouseButtonEvent(0,false);DrawFrame();if(settle)Settle();
}
static void Click(unsigned index,bool settle=true){
    CHECK(index<hits.size());auto r=hits[index];ClickPoint((r.a.x+r.b.x)*.5f,(r.a.y+r.b.y)*.5f,settle);
}
static bool KeyEvent(ImGuiKey key,bool down,bool repeat=false){
    // Match WndProc: ImGui receives the event before the native ownership hook.
    ImGui::GetIO().AddKeyEvent(key,down);
    const unsigned vk=key==ImGuiKey_Enter?VK_RETURN:key==ImGuiKey_Space?VK_SPACE:0;
    return vk&&TrialGraphicsMessage(down?WM_KEYDOWN:WM_KEYUP,vk,repeat?intptr_t(1ull<<30):0);
}
static void Key(ImGuiKey key){
    KeyEvent(key,true);DrawFrame();KeyEvent(key,false);DrawFrame();
}
static void RightClick(){
    ImGui::GetIO().AddMouseButtonEvent(1,true);DrawFrame();
    ImGui::GetIO().AddMouseButtonEvent(1,false);DrawFrame();Settle();
}
static TrialFrontendFrame MakeList(unsigned kind){
    static uint64_t next=0;TrialFrontendFrame f{};f.list_kind=kind;f.row_top=390;
    auto& s=f.source;s.epoch=++next;s.owner=0x2000+kind*0x100;s.screen=0x1000+kind*0x100;
    s.kind=UC_HF_LIST;s.count=kind==2?5:kind==3?3:4;s.selected=0;
    if(kind==2)s.source_kind=0x80000010u;
    const char* main[] ={"Single player","Multiplayer","Options","Credits"};
    const char* pause[]={"Return to game","Restart mission","Options","Objectives","Quit"};
    const char* options[]={"Controls","Audio","Back"};
    for(unsigned i=0;i<s.count;++i){
        s.rows[i].enabled=1;s.rows[i].row=0x3000+i*0x100;s.rows[i].actions=UC_HF_ROW_ACTIVATE;
        std::snprintf(f.labels[i],sizeof(f.labels[i]),"%s",kind==2?pause[i]:kind==3?options[i]:main[i]);
    }
    if(kind==3)std::strcpy(s.rows[0].label,"NewFrontEnd-ControllerOptions-1");
    return f;
}
static void Publish(const TrialFrontendFrame& f){
    TrialGraphicsMenuContext(0x800,f.source.screen,f.source.owner,f.list_kind);TrialFrontendPublish(f);Settle();Drain();
}
static void CheckNativeConfirmTransitions(){
    auto main=MakeList(1),levels=MakeList(1);
    levels.list_kind=0;levels.source.kind=UC_HF_LEVELS;
    levels.source.owner+=0x1000;levels.source.screen+=0x1000;
    auto transition=main;transition.source.source_kind|=UC_HF_TRANSITION;
    auto no_action=[](){for(unsigned i=0;i<8;++i){DrawFrame();EmptyAction();}};

    Publish(main);Move(0,0);Drain();
    // Both taps arrive before the first render. ImGui intentionally retains
    // their down/up/down/up sequence, but native confirm coalesces this page.
    CHECK(KeyEvent(ImGuiKey_Enter,true));CHECK(KeyEvent(ImGuiKey_Enter,false));
    CHECK(KeyEvent(ImGuiKey_Enter,true));CHECK(KeyEvent(ImGuiKey_Enter,false));
    DrawFrame();auto action=Take();
    CHECK(action.kind==UC_HF_ACTIVATE&&action.owner==main.source.owner);
    TrialFrontendPublish(transition);
    TrialGraphicsMenuContext(0x800,levels.source.screen,levels.source.owner,0);
    TrialFrontendPublish(levels);no_action();
    Key(ImGuiKey_Enter);action=Take();
    CHECK(action.kind==UC_HF_ACTIVATE&&action.owner==levels.source.owner);no_action();

    // A native transition must consume confirms before the original keyboard
    // adapter can retain them, even though no interactive overlay is open.
    TrialFrontendPublish(transition);CHECK(!TrialGraphicsOpen());
    CHECK(KeyEvent(ImGuiKey_Enter,true));CHECK(KeyEvent(ImGuiKey_Enter,false));
    CHECK(KeyEvent(ImGuiKey_Space,true));CHECK(KeyEvent(ImGuiKey_Space,false));
    TrialFrontendPublish(levels);no_action();
    Key(ImGuiKey_Space);action=Take();CHECK(action.kind==UC_HF_ACTIVATE);no_action();

    // A queued edge is invalid after replacement, and repeats/focus loss do
    // not create a fresh activation. No global ImGui key queue is cleared.
    Publish(main);CHECK(KeyEvent(ImGuiKey_Enter,true));CHECK(KeyEvent(ImGuiKey_Enter,false));
    TrialFrontendPublish(levels);no_action();
    KeyEvent(ImGuiKey_Enter,true,true);DrawFrame();EmptyAction();
    KeyEvent(ImGuiKey_Enter,false);no_action();
    KeyEvent(ImGuiKey_Enter,true);TrialGraphicsMessage(WM_KILLFOCUS,0,0);
    DrawFrame();EmptyAction();KeyEvent(ImGuiKey_Enter,false);no_action();

    // Rebinding still receives Enter as a binding candidate, not a menu
    // activation. Original/title screens remain available to the guest path.
    trial_controls::BeginCapture(0);CHECK(KeyEvent(ImGuiKey_Enter,true));
    CHECK(native_confirm_generation.load()==0);
    trial_controls::capture.Sample(0,0,true);
    trial_controls::capture.Sample(VK_RETURN,0,true);
    CHECK(trial_controls::TakeCapture()==VK_RETURN);
    KeyEvent(ImGuiKey_Enter,false);trial_controls::CancelCapture();no_action();
    TrialFrontendPublish({});
    CHECK(!KeyEvent(ImGuiKey_Enter,true));CHECK(!KeyEvent(ImGuiKey_Enter,false));
    Publish(levels);no_action();
}
static void OpenGraphics(const TrialFrontendFrame& f){
    CHECK(f.list_kind==3);
    Publish(f);CHECK(hits.size()==f.source.count+2);
    if(keyboard_mode)for(unsigned i=0;i<hits.size();++i){
        CHECK(hits[i].a.x==48.f*output.width/640.f);
        CHECK(hits[i].b.x==432.f*output.width/640.f);
        if(i)CHECK(hits[i].a.y>=hits[i-1].b.y-.1f);
    }
    Click(f.source.count);CHECK(PanelOpen());CHECK(hits.size()==21);
    for(unsigned i=0;i<18;++i){
        CHECK(hits[i].name=="setting");
        CHECK(hits[i].b.y<=364*output.height/480.f+.1f);
        if(i)CHECK(hits[i-1].b.y<=hits[i].a.y+.1f);
    }
}
static void ChangeSetting(unsigned index){
    CHECK(index<18&&hits.size()==21);auto r=hits[index];
    // Click the actual row's right-arrow side, using recorded production bounds.
    ClickPoint(r.b.x-15*float(output.width)/640,(r.a.y+r.b.y)*.5f);
}
static void CheckStrip(unsigned count){
    CHECK(hits.size()==count);
    for(unsigned i=0;i<count;++i){
        CHECK(hits[i].name=="row");
        CHECK(hits[i].b.y-hits[i].a.y>=26*output.height/480.f);
        if(i){
            if(hits[i].a.y==hits[0].a.y)CHECK(hits[i-1].b.x<=hits[i].a.x+.1f);
            else {CHECK(hits[i].a.x==hits[0].a.x);CHECK(hits[i-1].b.y<=hits[i].a.y+.1f);}
        }
    }
    CHECK(hits.front().a.x>=17*output.width/640.f);
    CHECK(hits.back().b.x<=623*output.width/640.f);
}
static void CheckVisualHandoff(const TrialFrontendFrame& f){
    Publish(f);const auto count=hits.size();CHECK(count&&ImGui::GetDrawData()->TotalVtxCount>100);
    FrontendAction(f,UC_HF_ACTIVATE,0); // Must be discarded by the real publisher.
    TrialFrontendFrame transition{};transition.source.epoch=f.source.epoch+1;
    transition.source.source_kind=UC_HF_TRANSITION;
    TrialGraphicsMenuContext(0,0,0,0);TrialFrontendPublish(transition);DrawFrame();
    CHECK(hits.size()==count&&ImGui::GetDrawData()->TotalVtxCount>100);
    if(f.source.kind==TRIAL_HF_NAME){
        // The transition retains the entire original-keyboard cover, not just
        // its footer. Inspect the actual production draw-list quad at all
        // three fixture resolutions, with input disabled independently.
        const float sx=output.width/640.f,sy=output.height/480.f;bool covered=false;
        for(const auto* list:ImGui::GetDrawData()->CmdLists)for(int i=0;i+3<list->VtxBuffer.Size;++i){
            const auto* v=&list->VtxBuffer[i];const auto black=IM_COL32(0,0,0,255);
            if(v[0].col==black&&v[1].col==black&&v[2].col==black&&v[3].col==black&&
               std::abs(v[0].pos.x-3*sx)<.01f&&std::abs(v[0].pos.y-73*sy)<.01f&&
               std::abs(v[2].pos.x-637*sx)<.01f&&std::abs(v[2].pos.y-379*sy)<.01f)covered=true;
        }
        CHECK(covered);
    }
    CHECK(frontend_presentation.visible&&frontend_presentation.transition&&!TrialGraphicsOpen());
    CHECK(!TrialFrontendOwns(f.source.owner,f.source.kind));EmptyAction();EmptyName();
    Click(0);Click(unsigned(count-1));Key(ImGuiKey_Enter);Key(ImGuiKey_Escape);Key(ImGuiKey_Q);RightClick();
    TrialFrontendPad(0,0,0,0,0);TrialFrontendPad(0,255,255,0,0);DrawFrame();
    TrialFrontendPad(0,0,0,0,0);EmptyAction();EmptyName();
    CHECK(hits.size()==count&&!PanelOpen());
    // Every supported page shares this lifecycle. A settled original menu
    // (Rewards, Objectives, confirmation or a legacy settings child) must
    // reveal its own pixels rather than retain an unrelated native footer.
    for(unsigned kind:{unsigned(UC_HF_NONE),unsigned(UC_HF_DIALOG),unsigned(UC_HF_LIST)}){
        TrialFrontendPublish(f);TrialFrontendPublish(transition);
        auto original=f;original.source.kind=kind;original.list_kind=0;original.delete_dialog=false;original.pause_dialog=false;
        TrialFrontendPublish(original);DrawFrame();
        CHECK(hits.empty()&&ImGui::GetDrawData()->TotalVtxCount==0&&!frontend_presentation.visible);
        EmptyAction();EmptyName();
    }
    TrialFrontendPublish({});DrawFrame();CHECK(hits.empty());
}
static void CheckProfilesAndName(){
    // Original profile rectangles stay authoritative; the host overlays only
    // matching mouse targets and a footer, never a replacement save callback.
    auto profiles=MakeList(1);profiles.list_kind=0;profiles.source.kind=UC_HF_PROFILES;
    for(unsigned i=0;i<4;++i)profiles.profile_bounds[i]={100,100+55.f*i,440,42};
    profiles.profile_delete_mask=3;
    Publish(profiles);CHECK(hits.size()==7);
    for(unsigned i=0;i<4;++i){
        CHECK(hits[i].name=="profile");
        CHECK(std::abs(hits[i].a.x-profiles.profile_bounds[i].x*output.width/640.f)<.1f);
    }
    Click(2);auto ordinary=Take();CHECK(ordinary.kind==UC_HF_ACTIVATE&&ordinary.index==2);
    RightClick();ordinary=Take();CHECK(ordinary.kind==UC_HF_BACK);
    profiles.source.rows[1].enabled=0;++profiles.source.epoch;Publish(profiles);
    Click(1);EmptyAction();
    // Delete is a request for the original dialog, not an immediate save action.
    profiles.source.selected=0;++profiles.source.epoch;Publish(profiles);
    Click(5);ordinary=Take();CHECK(ordinary.kind==TRIAL_HF_DELETE&&ordinary.index==0);
    Key(ImGuiKey_Delete);ordinary=Take();CHECK(ordinary.kind==TRIAL_HF_DELETE&&ordinary.index==0);
    profiles.source.selected=3;++profiles.source.epoch;Publish(profiles);
    Click(5);Key(ImGuiKey_Delete);EmptyAction(); // empty slot
    // Controller directions can reach the original-profile footer's Delete.
    profiles.source.selected=0;++profiles.source.epoch;Publish(profiles);
    TrialFrontendPad(0,0,0,0,0);TrialFrontendPad(0,0,0,0,0);
    for(unsigned i=0;i<5;++i){TrialFrontendPad(2,0,0,0,0);DrawFrame();TrialFrontendPad(0,0,0,0,0);Drain();}
    // Traversing rows chose empty slot3, so Delete correctly does nothing.
    TrialFrontendPad(0,255,0,0,0);DrawFrame();EmptyAction();TrialFrontendPad(0,0,0,0,0);
    profiles.source.selected=0;++profiles.source.epoch;Publish(profiles);
    TrialFrontendPad(0,0,0,0,0);TrialFrontendPad(0,0,0,0,0);
    for(unsigned i=0;i<2;++i){TrialFrontendPad(4,0,0,0,0);DrawFrame();TrialFrontendPad(0,0,0,0,0);}
    TrialFrontendPad(0,255,0,0,0);DrawFrame();ordinary=Take();
    CHECK(ordinary.kind==TRIAL_HF_DELETE&&ordinary.index==0);TrialFrontendPad(0,0,0,0,0);

    auto deletion=MakeList(1);deletion.list_kind=0;deletion.source.kind=UC_HF_DIALOG;
    deletion.source.count=2;deletion.source.selected=1;deletion.delete_dialog=true;
    deletion.source.owner=0x7400;deletion.source.screen=profiles.source.screen;
    deletion.source.rows[0].callback=0x284580;deletion.source.rows[1].callback=0x284310;
    std::strcpy(deletion.delete_name,"Alice");
    KeyEvent(ImGuiKey_Enter,true);Publish(deletion);CHECK(hits.size()==2);EmptyAction();
    DrawFrame();EmptyAction();KeyEvent(ImGuiKey_Enter,false);Settle();
    Key(ImGuiKey_Enter);ordinary=Take();CHECK(ordinary.kind==UC_HF_ACTIVATE&&ordinary.index==1); // default No
    for(auto key:{ImGuiKey_Escape,ImGuiKey_Q}){Key(key);ordinary=Take();CHECK(ordinary.kind==UC_HF_BACK);}
    RightClick();ordinary=Take();CHECK(ordinary.kind==UC_HF_BACK);
    Click(0);ordinary=Take();CHECK(ordinary.kind==UC_HF_ACTIVATE&&ordinary.index==0);
    ++deletion.source.epoch;Publish(deletion);Key(ImGuiKey_LeftArrow);Key(ImGuiKey_Space);
    ordinary=Take();CHECK(ordinary.kind==UC_HF_ACTIVATE&&ordinary.index==0);
    ++deletion.source.epoch;Publish(deletion);
    TrialFrontendPad(0,0,0,0,0);TrialFrontendPad(0,0,0,0,0);
    TrialFrontendPad(4,0,0,0,0);DrawFrame();TrialFrontendPad(0,0,0,0,0);
    TrialFrontendPad(0,255,0,0,0);DrawFrame();ordinary=Take();CHECK(ordinary.kind==UC_HF_ACTIVATE&&ordinary.index==0);
    TrialFrontendPad(0,0,0,0,0);TrialFrontendPad(0,0,255,0,0);DrawFrame();ordinary=Take();CHECK(ordinary.kind==UC_HF_BACK);
    TrialFrontendPad(0,0,0,0,0);
    deletion.source.rows[0].enabled=0;++deletion.source.epoch;Publish(deletion);
    Click(0);EmptyAction();Key(ImGuiKey_LeftArrow);Key(ImGuiKey_Enter);EmptyAction();
    Click(1);ordinary=Take();CHECK(ordinary.kind==UC_HF_ACTIVATE&&ordinary.index==1);
    auto stale_deletion=deletion;--stale_deletion.source.epoch;FrontendAction(stale_deletion,UC_HF_ACTIVATE,0);EmptyAction();
    // Closing or changing the original screen invalidates pending acceptance.
    FrontendAction(deletion,UC_HF_ACTIVATE,1);TrialFrontendPublish({});DrawFrame();EmptyAction();
    CheckVisualHandoff(deletion);

    auto name=MakeList(1);name.list_kind=0;name.source.kind=TRIAL_HF_NAME;
    name.source.count=1;name.source.owner=0x7100;name.source.screen=0x7200;
    name.source.rows[0].row=0x51d71c;name.name_capacity=15;
    std::strcpy(name.name,"Unit");
    // ImGui may still contain a second old-page Enter when the first one opens
    // the text editor. It must neither submit the name nor steal text focus.
    profiles.source.selected=3;++profiles.source.epoch;Publish(profiles);Move(0,0);Drain();
    CHECK(KeyEvent(ImGuiKey_Enter,true));CHECK(KeyEvent(ImGuiKey_Enter,false));
    CHECK(KeyEvent(ImGuiKey_Enter,true));CHECK(KeyEvent(ImGuiKey_Enter,false));
    DrawFrame();ordinary=Take();CHECK(ordinary.kind==UC_HF_ACTIVATE&&ordinary.index==3);
    TrialGraphicsMenuContext(0x800,name.source.screen,name.source.owner,0);TrialFrontendPublish(name);
    for(unsigned i=0;i<8;++i){DrawFrame();EmptyName();EmptyAction();}
    CHECK(native_confirm_generation.load()==0);
    CHECK(KeyEvent(ImGuiKey_Space,true));CHECK(native_confirm_generation.load()==0);
    KeyEvent(ImGuiKey_Space,false);Settle();EmptyName();
    CHECK(hits.size()==2);
    CHECK(TrialFrontendOwns(name.source.owner,TRIAL_HF_NAME));
    // InputText uses real ImGui character/filter events, including Q as text.
    Key(ImGuiKey_End);Type(" Q7!_");auto edit=TakeName();
    CHECK(edit.identity.kind==UC_HF_SELECT&&!std::strcmp(edit.text,"Unit Q7"));
    CHECK(edit.identity.row==0x51d71c&&edit.identity.owner==name.source.owner);
    EmptyAction();Key(ImGuiKey_Backspace);edit=TakeName();CHECK(!std::strcmp(edit.text,"Unit Q"));
    Type("123456789012345");edit=TakeName();CHECK(std::strlen(edit.text)==15);
    Key(ImGuiKey_Enter);auto accept=TakeName();CHECK(accept.identity.kind==UC_HF_ACTIVATE&&std::strlen(accept.text)==15);
    CHECK(native_confirm_generation.load()==0); // InputText owns name submission.
    // An original validation error preserves the edited draft and can be
    // corrected without losing focus or interpreting Q as the Back command.
    std::strcpy(name.name_error,"This profile name already exists.");
    TrialFrontendPublish(name);Settle();Key(ImGuiKey_End);Key(ImGuiKey_Backspace);edit=TakeName();CHECK(std::strlen(edit.text)==14);
    Key(ImGuiKey_Escape);auto cancel=TakeName();CHECK(cancel.identity.kind==UC_HF_BACK);EmptyAction();
    RightClick();cancel=TakeName();CHECK(cancel.identity.kind==UC_HF_BACK);EmptyAction();

    // The original controller keyboard remains usable. Returning to keyboard
    // mode acquires its current text, not a stale earlier PC draft.
    keyboard_mode=0;TrialFrontendPublish(name);Settle();CHECK(hits.empty());
    CHECK(!TrialFrontendOwns(name.source.owner,TRIAL_HF_NAME));EmptyName();
    std::strcpy(name.name,"Pad Name");name.name_error[0]=0;keyboard_mode=1;
    TrialFrontendPublish(name);Settle();Key(ImGuiKey_End);Type("Q");edit=TakeName();
    CHECK(!std::strcmp(edit.text,"Pad NameQ"));

    // Blank input is deliberately forwarded to the retail default-name
    // validator. Identity and terminal action checks protect stale submissions.
    TrialFrontendNameAction(name,UC_HF_ACTIVATE,"");
    TrialFrontendNameAction(name,UC_HF_SELECT,"overwrite");
    accept=TakeName();CHECK(accept.identity.kind==UC_HF_ACTIVATE&&!accept.text[0]);
    auto stale=name;++stale.source.epoch;TrialFrontendNameAction(stale,UC_HF_ACTIVATE,"stale");EmptyName();
    TrialFrontendNameAction(name,UC_HF_ACTIVATE,"bad!");EmptyName();
    TrialFrontendNameAction(name,UC_HF_ACTIVATE,"1234567890123456");EmptyName();
    TrialFrontendPublish({});Settle();EmptyName();
    CheckVisualHandoff(profiles);CheckVisualHandoff(name);
}
static void CheckLegacyFogSettings(){
    // All prior combinations still load without becoming dirty. Inactive flags
    // are preserved when saving an unrelated setting, including Off+Smooth and
    // Distant+Smooth profiles made by the previous two-control menu.
    for(bool distance:{false,true})for(unsigned mode:{0u,1u})for(bool smooth:{false,true}){
        TrialDistanceFog=distance;TrialFogDistanceMode=mode;TrialSmoothFog=smooth;
        const unsigned expected=!distance?0:mode==1?3:smooth?2:1;
        ReadDraft();CHECK(draft_fog_mode==expected&&!Dirty());
        draft_fps=!draft_fps;CHECK(Dirty());CHECK(Apply());
        CHECK(TrialDistanceFog==distance&&TrialFogDistanceMode==mode&&TrialSmoothFog==smooth&&!Dirty());
        draft_fog_mode=(expected+1)%4;fail_apply=true;CHECK(!Apply());fail_apply=false;
        CHECK(TrialDistanceFog==distance&&TrialFogDistanceMode==mode&&TrialSmoothFog==smooth&&Dirty());
        ReadDraft();CHECK(draft_fog_mode==expected&&!Dirty()); // Discard restores the effective mode.
    }
    TrialDistanceFog=true;TrialFogDistanceMode=0;TrialSmoothFog=false;ReadDraft();
}
static void CheckControls(const TrialFrontendFrame& options){
    auto open=[&](){
        Publish(options);Click(0);CHECK(PanelOpen()&&settings_page==1);
        // Hovering the existing original row legitimately publishes SELECT.
        // The host intercepts activation, not that guest selection update.
        UcHybridFrontendAction selected{};
        if(TrialFrontendTakeAction(selected))CHECK(selected.kind==UC_HF_SELECT&&selected.index==0&&selected.row==options.source.rows[0].row);
        EmptyAction();
    };
    trial_controls::live_controls=trial_controls::Defaults();controls_device=0;
    open();CHECK(hits.size()==24);
    CHECK(!ControlsDirty());
    for(const auto* description:control_help)CHECK(std::strlen(description)>20);
    auto source=[](unsigned value){
        trial_controls::capture.Sample(0,0,true);
        trial_controls::capture.Sample(controls_device?0:value,controls_device?value:0,true);
        DrawFrame();Settle();
    };
    // The first held opening input is ignored. A later fresh key edits only
    // the selected slot, with explicit conflict decisions before publication.
    Click(1);CHECK(controls_capture_action==0);
    trial_controls::capture.Sample(VK_LBUTTON,0,true);DrawFrame();CHECK(controls_capture_action==0);
    source('S');CHECK(controls_conflict_action==19&&hits.size()==3);
    Click(2);CHECK(controls_capture_action<0&&!ControlsDirty());
    Click(1);source('Z');CHECK(controls_draft.keyboard[0][0]=='Z'&&ControlsDirty());
    CHECK(trial_controls::live_controls.keyboard[0][0]=='R');
    Click(2);source(VK_MBUTTON);CHECK(controls_conflict_action==24);Click(1);
    CHECK(controls_draft.keyboard[0][1]==VK_MBUTTON&&controls_draft.keyboard[24][0]==VK_MBUTTON);
    const unsigned before=controls_applied;Click(19);CHECK(controls_applied==before+1&&!ControlsDirty());
    // Wheel capture is an event, not a held virtual key; it can bind normal
    // actions but not the hold-and-drag orders action.
    Click(1);trial_controls::capture.Sample(0,0,true);trial_controls::capture.Wheel(256);DrawFrame();Settle();
    CHECK(controls_draft.keyboard[0][0]==256&&ControlsDirty());Click(20);CHECK(!ControlsDirty());
    for(unsigned page=0;page<3;++page)Click(18);
    CHECK(controls_page==3&&hits.size()==10);Click(1);CHECK(controls_capture_action==24);
    trial_controls::capture.Sample(0,0,true);trial_controls::capture.Wheel(257);DrawFrame();Settle();
    CHECK(controls_capture_action==24&&trial_controls::CaptureWaiting()&&!ControlsDirty());
    Key(ImGuiKey_Escape);Settle();CHECK(controls_capture_action<0&&PanelOpen());
    // Mouse-down on Cancel wins over that same raw left-button candidate.
    Click(1);auto cancel=hits[1];Move((cancel.a.x+cancel.b.x)/2,(cancel.a.y+cancel.b.y)/2);
    trial_controls::capture.Sample(0,0,true);trial_controls::capture.Sample(VK_LBUTTON,0,true);
    ImGui::GetIO().AddMouseButtonEvent(0,true);DrawFrame();
    CHECK(controls_capture_action<0&&!ControlsDirty());
    ImGui::GetIO().AddMouseButtonEvent(0,false);Settle();
    // Device selection returns to page one. Controller sources are raw, and
    // Replace clears an overlapping source instead of silently invoking both.
    auto device=hits[0];ClickPoint(device.b.x-15*float(output.width)/640,(device.a.y+device.b.y)/2);
    CHECK(controls_device==1&&controls_page==0&&hits.size()==24);
    Click(1);source(8);CHECK(controls_conflict_action==7);Click(0);
    CHECK(controls_draft.controller[0][0]==8&&!controls_draft.controller[7][0]);
    fail_apply=true;Click(19);fail_apply=false;CHECK(ControlsDirty()&&trial_controls::live_controls.controller[0][0]==1);
    Click(19);CHECK(!ControlsDirty()&&trial_controls::live_controls.controller[0][0]==8);
    Click(21);CHECK(ControlsDirty()&&controls_draft.controller[0][0]==1);
    Click(20);CHECK(!ControlsDirty()&&controls_draft.controller[0][0]==8);
    Click(21);RightClick();CHECK(confirm_back&&PanelOpen());
    RightClick();CHECK(!confirm_back&&ControlsDirty());
    Click(22);CHECK(confirm_back);Click(1);CHECK(!PanelOpen());EmptyAction();
    // The original controller options (inversion, crouch, vibration and
    // sensitivity) remain reachable by their original callback. Dirty drafts
    // receive the same save prompt before that transition.
    open();Click(21);CHECK(ControlsDirty());
    Click(23);CHECK(confirm_back&&controls_original_requested);Click(2);
    CHECK(PanelOpen()&&!confirm_back&&!controls_original_requested);EmptyAction();
    Click(23);CHECK(confirm_back);Click(1);CHECK(!PanelOpen());
    auto original_options=Take();CHECK(original_options.kind==UC_HF_ACTIVATE&&original_options.index==0&&original_options.row==options.source.rows[0].row);
    // Controller left/right reaches the pager and every footer action. It
    // never silently reuses a queued Confirm from the capture dialog.
    open();auto apply=hits[19];Move((apply.a.x+apply.b.x)/2,(apply.a.y+apply.b.y)/2);
    frontend_events=32;DrawFrame();frontend_events=4;DrawFrame();Settle();CHECK(message=="Changes discarded.");
    auto previous=hits[17];Move((previous.a.x+previous.b.x)/2,(previous.a.y+previous.b.y)/2);
    frontend_events=32;DrawFrame();frontend_events=4;DrawFrame();Settle();CHECK(controls_page==1);
    Click(22);CHECK(!PanelOpen());EmptyAction();
    // Original Options Controls is intercepted only within the qualified hub.
    auto unrelated=options;unrelated.list_kind=1;++unrelated.source.epoch;
    Publish(unrelated);Click(0);auto action=Take();CHECK(action.kind==UC_HF_ACTIVATE&&!PanelOpen());
}
static void CheckLanguage(const TrialFrontendFrame& options){
    trial_language::selected=trial_language::active=0;
    auto open=[&](){Publish(options);CheckStrip(options.source.count+2);Click(options.source.count+1);
        CHECK(PanelOpen()&&settings_page==2&&hits.size()==4);EmptyAction();};
    auto change=[&](){const auto r=hits[0];ClickPoint(r.b.x-15*float(output.width)/640,(r.a.y+r.b.y)/2);};
    open();CHECK(!LanguageDirty());change();CHECK(language_draft==1&&LanguageDirty()&&trial_language::Snapshot()==0);
    Click(2);CHECK(!LanguageDirty()&&language_draft==0);change();
    fail_apply=true;Click(1);fail_apply=false;
    CHECK(LanguageDirty()&&trial_language::Snapshot()==0&&message=="Could not save language.");
    Click(1);CHECK(!LanguageDirty()&&trial_language::Snapshot()==1&&trial_language::active==0&&trial_language::PendingRestart());
    CHECK(message.find("Restart")!=std::string::npos);
    change();Click(3);CHECK(confirm_back&&hits.size()==3);Click(2);CHECK(!confirm_back&&LanguageDirty());
    RightClick();CHECK(confirm_back);Click(1);CHECK(!PanelOpen()&&trial_language::Snapshot()==1);EmptyAction();
    // A held opening Enter must not cycle the language or save it.
    Publish(options);auto row=hits[options.source.count+1];Move((row.a.x+row.b.x)/2,(row.a.y+row.b.y)/2);
    const auto saves=trial_language::saves;
    KeyEvent(ImGuiKey_Enter,true);DrawFrame();Settle();
    CHECK(PanelOpen()&&language_draft==1&&trial_language::saves==saves&&!LanguageDirty());
    KeyEvent(ImGuiKey_Enter,false);Settle();
    // Actual controller producer: separate directions/confirm navigate the
    // language choice and Apply; the saved language is still next-boot only.
    auto pad=[&](uint32_t buttons,uint8_t a=0,uint8_t b=0){
        TrialFrontendPad(0,0,0,0,0);DrawFrame();TrialFrontendPad(buttons,a,b,0,0);DrawFrame();
        TrialFrontendPad(0,0,0,0,0);Settle();};
    pad(8);CHECK(language_draft==2&&LanguageDirty());pad(2);pad(0,255);
    CHECK(trial_language::Snapshot()==2&&trial_language::active==0&&!LanguageDirty());
    pad(0,0,255);CHECK(!PanelOpen());EmptyAction();
    // Restart notice survives reopening; switching back to the boot language
    // clears it after Apply. Paused Options uses this same ownership path.
    auto paused=options;paused.source.source_kind|=0x80000000u;++paused.source.epoch;
    Publish(paused);Click(paused.source.count+1);CHECK(PanelOpen()&&settings_page==2&&trial_language::PendingRestart());
    change();change();CHECK(language_draft==0);Click(1);CHECK(!trial_language::PendingRestart());
    change();RightClick();CHECK(confirm_back);Click(0);CHECK(!PanelOpen()&&trial_language::Snapshot()==1);
    EmptyAction();
}
static void CheckQuit(const TrialFrontendFrame& main){
    const auto before=quit_posts;
    auto open=[&]{Publish(main);Click(main.source.count);CHECK(quit_open&&!quit_submitted&&quit_selection==1);CheckStrip(2);EmptyAction();};
    auto cancelled=[&]{Settle();CHECK(!quit_open&&quit_posts==before);CheckStrip(main.source.count+1);EmptyAction();
        CHECK(frontend.source.owner==main.source.owner&&frontend.source.screen==main.source.screen&&frontend.source.epoch==main.source.epoch);};
    open();Click(1);cancelled();
    for(auto key:{ImGuiKey_Escape,ImGuiKey_Q}){open();Key(key);cancelled();}
    open();ImGui::GetIO().AddMouseButtonEvent(1,true);DrawFrame();cancelled();
    // A held closing button cannot also back out of the underlying main menu.
    ImGui::GetIO().AddMouseButtonEvent(1,false);Settle();EmptyAction();
    open();TrialFrontendPad(0,0,0,0,0);TrialFrontendPad(0,0,255,0,0);DrawFrame();cancelled();
    TrialFrontendPad(0,0,255,0,0);Settle();EmptyAction();TrialFrontendPad(0,0,0,0,0);

    // Keyboard activation reaches the extra row; its opening Enter is held
    // through multiple modal frames without accepting the default No or Yes.
    Publish(main);auto quit=hits.back();Move((quit.a.x+quit.b.x)/2,(quit.a.y+quit.b.y)/2);
    KeyEvent(ImGuiKey_Enter,true);DrawFrame();CHECK(quit_open);Settle();
    CHECK(quit_open&&quit_selection==1&&quit_posts==before);EmptyAction();
    KeyEvent(ImGuiKey_Enter,false);Settle();Key(ImGuiKey_Enter);cancelled();

    // Production posts the app's clean WM_CLOSE request once, only after Yes.
    open();fail_quit_post=true;Click(0);fail_quit_post=false;CHECK(quit_open&&!quit_submitted&&quit_posts==before);
    Click(0);CHECK(quit_open&&quit_submitted&&quit_posts==before+1);Settle();Key(ImGuiKey_Enter);CHECK(quit_posts==before+1);
    CloseQuit();Settle();
    Publish(main);Click(main.source.count);Key(ImGuiKey_LeftArrow);CHECK(quit_selection==0);
    Key(ImGuiKey_Space);CHECK(quit_submitted&&quit_posts==before+2);CloseQuit();Settle();
    keyboard_mode=0;Publish(main);CHECK(hits.empty()&&!TrialFrontendOwns(main.source.owner,UC_HF_LIST));
    TrialFrontendPad(0,0,0,0,0);TrialFrontendPad(0x200,0,0,0,0);DrawFrame();Settle();
    CHECK(quit_open);TrialFrontendPad(0,0,0,0,0);
    TrialFrontendPad(4,0,0,0,0);DrawFrame();CHECK(quit_selection==0);TrialFrontendPad(0,0,0,0,0);
    TrialFrontendPad(0,255,0,0,0);DrawFrame();CHECK(quit_submitted&&quit_posts==before+3);
    TrialFrontendPad(0,255,0,0,0);Settle();CHECK(quit_posts==before+3);
    TrialFrontendPad(0,0,0,0,0);CloseQuit();keyboard_mode=1;Settle();

    // A confirmation belongs to one exact main-menu instance. A new screen
    // invalidates it, and the pause menu retains only its original Quit action.
    Publish(main);Click(main.source.count);auto replacement=main;++replacement.source.epoch;
    Publish(replacement);CHECK(!quit_open&&quit_posts==before+3);EmptyAction();
    auto pause=MakeList(2);Publish(pause);CheckStrip(pause.source.count);Click(pause.source.count-1);
    auto action=Take();CHECK(action.kind==UC_HF_ACTIVATE&&action.index==pause.source.count-1&&!quit_open);
}
static void CheckPauseConfirmation(){
    const auto before=quit_posts;
    auto f=MakeList(2);f.list_kind=0;f.source.kind=UC_HF_DIALOG;f.source.count=2;
    f.source.owner=0x7800;f.source.selected=1;f.source.back_action=0x17;f.pause_dialog=true;
    f.source.rows[0].callback=0x267400;f.source.rows[1].callback=0x284310;
    std::strcpy(f.labels[0],"Yes");std::strcpy(f.labels[1],"No");
    std::strcpy(f.dialog_message,"You will lose your progress, are you sure?");
    CHECK(TrialFrontendSupported(f));
    KeyEvent(ImGuiKey_Enter,true);Publish(f);CheckStrip(2);EmptyAction();
    CHECK(hits[0].a.x==48.f*output.width/640.f&&hits[0].b.x==432.f*output.width/640.f);
    CHECK(hits[1].a.y>hits[0].a.y);
    for(unsigned i=0;i<4;++i){TrialFrontendPublish(f);DrawFrame();EmptyAction();}
    KeyEvent(ImGuiKey_Enter,false);Settle();Move(0,0);
    Key(ImGuiKey_Enter);auto a=Take();CHECK(a.kind==UC_HF_ACTIVATE&&a.index==1);
    Click(0);a=Take();CHECK(a.kind==UC_HF_ACTIVATE&&a.index==0&&a.owner==f.source.owner);
    Click(1);a=Take();CHECK(a.kind==UC_HF_ACTIVATE&&a.index==1);
    for(auto key:{ImGuiKey_Escape,ImGuiKey_Q}){Key(key);a=Take();CHECK(a.kind==UC_HF_BACK);}
    RightClick();a=Take();CHECK(a.kind==UC_HF_BACK);
    ++f.source.epoch;Publish(f);Move(0,0);Key(ImGuiKey_UpArrow);Key(ImGuiKey_Space);
    a=Take();CHECK(a.kind==UC_HF_ACTIVATE&&a.index==0);
    // Retain the paused scene: no opaque cover is allowed over its content.
    const float sx=output.width/640.f,sy=output.height/480.f;
    bool shade=false;
    for(const auto* list:ImGui::GetDrawData()->CmdLists)for(int i=0;i+3<list->VtxBuffer.Size;++i){
        const auto* v=&list->VtxBuffer[i];
        if(std::abs(v[0].pos.x-3*sx)<.01f&&std::abs(v[0].pos.y-73*sy)<.01f&&
           std::abs(v[2].pos.x-637*sx)<.01f&&std::abs(v[2].pos.y-379*sy)<.01f){
            CHECK((v[0].col>>IM_COL32_A_SHIFT)==112);shade=true;
        }
    }
    CHECK(shade&&quit_posts==before);
    // Switching devices relinquishes the native modal, including queued Yes.
    FrontendAction(f,UC_HF_ACTIVATE,0);keyboard_mode=0;Publish(f);
    CHECK(hits.empty()&&!frontend_presentation.visible&&!TrialFrontendOwns(f.source.owner,UC_HF_DIALOG));EmptyAction();
    keyboard_mode=1;++f.source.epoch;Publish(f);CheckStrip(2);
    auto invalid=f;invalid.source.rows[0].callback=0x267330;
    CHECK(!TrialFrontendSupported(invalid));
    invalid=f;invalid.source.source_kind=0;CHECK(!TrialFrontendSupported(invalid));
    invalid=f;invalid.dialog_message[0]=0;CHECK(!TrialFrontendSupported(invalid));
    CheckVisualHandoff(f);CHECK(quit_posts==before);
}
static void CheckSize(unsigned width,unsigned height){
    output={width,height};live={};Close();TrialFrontendPublish({});TrialGraphicsMenuContext(0,0,0,0);Settle();
    auto main=MakeList(1);Publish(main);CheckStrip(5);CHECK(ImGui::GetDrawData()->TotalVtxCount>100);
    Click(2);auto action=Take();CHECK(action.kind==UC_HF_ACTIVATE&&action.index==2&&action.row==main.source.rows[2].row);
    main.source.rows[1].enabled=0;++main.source.epoch;Publish(main);Click(1);EmptyAction();

    // Escape still held from gameplay must not immediately choose a pause item.
    ImGui::GetIO().AddKeyEvent(ImGuiKey_Escape,true);auto pause=MakeList(2);Publish(pause);
    CheckStrip(5);DrawFrame();EmptyAction();
    ImGui::GetIO().AddKeyEvent(ImGuiKey_Escape,false);Settle();EmptyAction();Key(ImGuiKey_Escape);
    action=Take();CHECK(action.kind==UC_HF_BACK);
    Publish(pause);Click(0);action=Take();CHECK(action.kind==UC_HF_ACTIVATE&&action.index==0);

    // Settings edit only the draft until Apply; Back keeps the same draft behind
    // the unsaved-changes confirmation, while Discard leaves live values alone.
    auto options=MakeList(3);OpenGraphics(options);unsigned before=applied;ChangeSetting(3);CHECK(Dirty());CHECK(live.settings.render_percent==100);
    sample_support=2|(1u<<4);ChangeSetting(9);CHECK(draft.msaa==4&&live.msaa==1);
    sample_support=2|(1u<<2)|(1u<<4);
    Click(18);CHECK(applied==before+1&&live.settings.render_percent==125&&live.msaa==4&&!Dirty());
    ChangeSetting(10);ChangeSetting(11);ChangeSetting(12);ChangeSetting(13);ChangeSetting(13);
    CHECK(!draft.scenery&&!draft.characters&&draft.character_quality==1&&draft.ragdolls==2&&Dirty());
    CHECK(live.scenery&&live.characters&&live.character_quality==0&&live.ragdolls==0);
    CHECK(hits.size()==21&&draft.ragdolls==2);Click(18);
    CHECK(applied==before+2&&!live.scenery&&!live.characters&&live.character_quality==1&&live.ragdolls==2&&!Dirty());
    ChangeSetting(13);CHECK(Dirty());Click(19);CHECK(!Dirty()&&draft.ragdolls==2);
    ChangeSetting(14);CHECK(draft.fov!=53&&live.fov==53&&Dirty());
    // Both real tracks drag/clamp without touching committed settings before Apply.
    auto drag=[&](unsigned row,float start,float end){
        const auto hit=hits[row];const float y=(hit.a.y+hit.b.y)*.5f;
        Move(start*width/640.f,y);ImGui::GetIO().AddMouseButtonEvent(0,true);DrawFrame();
        Move(end*width/640.f,y);DrawFrame();
        ImGui::GetIO().AddMouseButtonEvent(0,false);Settle();
    };
    drag(14,470,600);CHECK(draft.fov==90&&live.fov==53);
    drag(14,470,300);CHECK(draft.fov==40&&live.fov==53);
    drag(14,470,492.4f);CHECK(draft.fov==70&&live.fov==53);
    CHECK(draft.reticle_scale==100&&live.reticle_scale==100);
    drag(16,470,600);CHECK(draft.reticle_scale==200&&live.reticle_scale==100);
    drag(16,470,300);CHECK(draft.reticle_scale==50&&live.reticle_scale==100);
    drag(16,470,500.67f);CHECK(draft.reticle_scale==150&&live.reticle_scale==100);
    Key(ImGuiKey_LeftArrow);CHECK(draft.reticle_scale==145&&live.reticle_scale==100);
    Key(ImGuiKey_RightArrow);CHECK(draft.reticle_scale==150);
    // A key pressed while releasing a drag still applies exactly one step.
    const auto reticle_hit=hits[16];const float reticle_y=(reticle_hit.a.y+reticle_hit.b.y)*.5f;
    Move(500.67f*width/640.f,reticle_y);ImGui::GetIO().AddMouseButtonEvent(0,true);DrawFrame();
    CHECK(draft.reticle_scale==150);
    ImGui::GetIO().AddMouseButtonEvent(0,false);KeyEvent(ImGuiKey_LeftArrow,true);DrawFrame();
    KeyEvent(ImGuiKey_LeftArrow,false);Settle();CHECK(draft.reticle_scale==145);
    Key(ImGuiKey_RightArrow);CHECK(draft.reticle_scale==150);
    Click(18);CHECK(live.fov==70&&live.reticle_scale==150&&!Dirty());
    ChangeSetting(16);CHECK(draft.reticle_scale==155&&live.reticle_scale==150&&Dirty());
    Click(20);CHECK(confirm_back);Click(2);CHECK(PanelOpen()&&Dirty());
    Click(19);CHECK(draft.reticle_scale==150&&live.reticle_scale==150&&!Dirty());
    // Weapon FOV is independent of the world camera, with the same Apply policy.
    CHECK(draft.weapon_fov==100&&live.weapon_fov==100);
    drag(17,470,600);CHECK(draft.weapon_fov==150&&live.weapon_fov==100&&draft.fov==70);
    drag(17,470,300);CHECK(draft.weapon_fov==50&&live.weapon_fov==100&&draft.fov==70);
    drag(17,470,511);CHECK(draft.weapon_fov==125&&live.weapon_fov==100);
    fail_apply=true;Click(18);fail_apply=false;CHECK(Dirty()&&live.weapon_fov==100);
    Click(19);CHECK(!Dirty()&&draft.weapon_fov==100);
    ChangeSetting(17);CHECK(draft.weapon_fov==105&&Dirty());Click(20);CHECK(confirm_back);
    Click(2);CHECK(PanelOpen()&&Dirty());Click(18);
    CHECK(live.weapon_fov==105&&live.fov==70&&!Dirty());
    // HD changes use the same staged save/discard/back policy, including failures.
    CHECK(!live.hd_ui);ChangeSetting(15);CHECK(draft.hd_ui&&Dirty()&&!live.hd_ui);
    fail_apply=true;Click(18);fail_apply=false;CHECK(draft.hd_ui&&Dirty()&&!live.hd_ui);
    Click(19);CHECK(!draft.hd_ui&&!Dirty());
    ChangeSetting(15);Click(20);CHECK(confirm_back&&hits.size()==3);
    Click(2);CHECK(PanelOpen()&&draft.hd_ui&&Dirty());
    Click(20);Click(1);CHECK(!PanelOpen()&&!live.hd_ui);
    OpenGraphics(options);CHECK(!draft.hd_ui&&!Dirty());
    ChangeSetting(15);Click(18);CHECK(live.hd_ui&&!Dirty());
    ChangeSetting(15);Click(19);CHECK(live.hd_ui&&draft.hd_ui&&!Dirty());
    // One Fog control covers all four effective modes. Mouse edits stay in the
    // draft, and failed Apply rolls back the three compatible config keys.
    CHECK(TrialDistanceFog&&!TrialSmoothFog&&TrialFogDistanceMode==0);
    ChangeSetting(8);CHECK(draft_fog_mode==2&&!TrialSmoothFog&&Dirty());
    Click(19);CHECK(draft_fog_mode==1&&TrialDistanceFog&&!Dirty());
    ChangeSetting(8);Click(18);CHECK(TrialDistanceFog&&TrialFogDistanceMode==0&&TrialSmoothFog&&!Dirty());
    ChangeSetting(8);Click(18);CHECK(TrialDistanceFog&&TrialFogDistanceMode==1&&!TrialSmoothFog&&!Dirty());
    ChangeSetting(8);fail_apply=true;Click(18);fail_apply=false;
    CHECK(TrialDistanceFog&&TrialFogDistanceMode==1&&!TrialSmoothFog&&draft_fog_mode==0&&Dirty());
    Click(18);CHECK(!TrialDistanceFog&&TrialFogDistanceMode==0&&!TrialSmoothFog&&!Dirty());
    ChangeSetting(8);Click(18);CHECK(TrialDistanceFog&&TrialFogDistanceMode==0&&!TrialSmoothFog&&!Dirty());
    ChangeSetting(8);Click(20);CHECK(confirm_back&&hits.size()==3);
    Key(ImGuiKey_Escape);Settle();CHECK(!confirm_back&&PanelOpen()&&Dirty()&&hits.size()==21);
    CHECK(draft_fog_mode==2&&!TrialSmoothFog);Click(19);CHECK(!Dirty());
    ChangeSetting(6);CHECK(Dirty()&&message.empty());Click(20);CHECK(confirm_back&&hits.size()==3);
    Key(ImGuiKey_Escape);Settle();CHECK(!confirm_back&&PanelOpen()&&Dirty()&&hits.size()==21);
    Click(20);CHECK(confirm_back&&hits.size()==3);Click(1);CHECK(!PanelOpen());
    CHECK(live.settings.render_percent==125&&live.settings.fps_cap==60);CheckStrip(5);

    // A fresh controller Back reaches the open page; its held repeat cannot
    // immediately leave the parent menu after the page has closed.
    auto pause_options=MakeList(3);pause_options.source.source_kind=0x80000010u;
    OpenGraphics(pause_options);TrialFrontendPad(0,0,0,0,0);TrialFrontendPad(0,0,0,0,0);
    TrialFrontendPad(0,0,255,0,0);DrawFrame();CHECK(!PanelOpen());
    TrialFrontendPad(0,0,255,0,0);Settle();EmptyAction();
    TrialFrontendPad(0,0,0,0,0);TrialFrontendPad(0,0,255,0,0);DrawFrame();
    action=Take();CHECK(action.kind==UC_HF_BACK);TrialFrontendPad(0,0,0,0,0);

    // RMB has the same Back policy as Q/Escape, including dirty confirmation.
    OpenGraphics(options);ChangeSetting(3);RightClick();CHECK(confirm_back&&PanelOpen()&&Dirty());
    RightClick();CHECK(!confirm_back&&PanelOpen()&&Dirty());
    Click(18);CHECK(!Dirty());
    ImGui::GetIO().AddMouseButtonEvent(1,true);DrawFrame();CHECK(!PanelOpen());
    Settle();EmptyAction(); // held closing click cannot back out of Options too
    ImGui::GetIO().AddMouseButtonEvent(1,false);Settle();EmptyAction();
    RightClick();action=Take();CHECK(action.kind==UC_HF_BACK);
    for(const auto& page:{main,pause,options}){
        Publish(page);RightClick();action=Take();CHECK(action.kind==UC_HF_BACK);EmptyAction();
    }

    // Show all 17 unlocked slots in the recorded original order. The fixture
    // validates host hit regions/actions; original badge pixels are guest-owned.
    auto levels=MakeList(1);levels.list_kind=0;levels.source.kind=UC_HF_LEVELS;levels.source.count=17;levels.source.level_actions=3;
    for(unsigned i=0;i<17;++i){levels.source.rows[i].enabled=1;levels.source.rows[i].row=i;
        std::snprintf(levels.labels[i],sizeof(levels.labels[i]),"Mission %u",i+1);}
    Publish(levels);CHECK(hits.size()==21);
    for(unsigned i=0;i<17;++i){
        CHECK(hits[i].name=="level"&&hits[i].a.y>=93*height/480.f&&hits[i].b.y<=371*height/480.f);
        if(i)CHECK(hits[i-1].b.y<=hits[i].a.y+.1f);
    }
    Click(16);action=Take();CHECK(action.kind==UC_HF_ACTIVATE&&action.index==16&&action.row==16);
    Key(ImGuiKey_D);action=Take();CHECK(action.kind==UC_HF_DIFFICULTY);
    Key(ImGuiKey_R);action=Take();CHECK(action.kind==UC_HF_REWARDS);
    RightClick();action=Take();CHECK(action.kind==UC_HF_BACK);

    // Transition publications revoke input immediately, while the last copied
    // footer keeps the original fading navigation strip from flashing through.
    Click(0);auto sliding=levels;sliding.source.source_kind|=UC_HF_TRANSITION;
    TrialFrontendPublish(sliding);DrawFrame();CHECK(hits.size()==21&&ImGui::GetDrawData()->TotalVtxCount>100);
    CHECK(!TrialFrontendOwns(levels.source.owner,UC_HF_LEVELS));EmptyAction();
    KeyEvent(ImGuiKey_Enter,true);Publish(levels);EmptyAction();
    KeyEvent(ImGuiKey_Enter,false);Settle();EmptyAction();
    Key(ImGuiKey_Enter);action=Take();CHECK(action.kind==UC_HF_ACTIVATE);

    // The actual Graphics UI must consume controller navigation and Apply.
    OpenGraphics(options);keyboard_mode=0;
    auto pad=[&](unsigned buttons,unsigned a=0,unsigned b=0){
        TrialFrontendPad(0,0,0,0,0);DrawFrame();TrialFrontendPad(buttons,a,b,0,0);DrawFrame();
        TrialFrontendPad(0,0,0,0,0);Settle();};
    // Select a known row with mouse once, then operate entirely by controller.
    Move(0,0);ClickPoint(hits[7].a.x+2,hits[7].a.y+2);const bool old_fps=draft_fps;
    pad(8);CHECK(draft_fps!=old_fps);pad(4);CHECK(draft_fps==old_fps);
    for(unsigned i=0;i<8;++i)pad(2); // FPS row 7 -> HD UI row 15.
    const bool old_hd=live.hd_ui;pad(8);CHECK(draft.hd_ui!=old_hd&&live.hd_ui==old_hd&&Dirty());
    pad(2); // Reticle row 16, then Weapon FOV row 17.
    const auto old_reticle=live.reticle_scale;pad(8);
    CHECK(draft.reticle_scale==old_reticle+5&&live.reticle_scale==old_reticle&&Dirty());
    pad(4);CHECK(draft.reticle_scale==old_reticle);
    pad(2);
    const auto old_weapon=live.weapon_fov;pad(8);
    CHECK(draft.weapon_fov==old_weapon+5&&live.weapon_fov==old_weapon&&Dirty());
    pad(2); // Apply
    const auto saved=applied;pad(0,255);CHECK(applied==saved+1&&live.hd_ui!=old_hd&&!Dirty());
    pad(0,0,255);CHECK(!PanelOpen());EmptyAction();keyboard_mode=1;
    Publish(levels);pad(0x200);action=Take();CHECK(action.kind==UC_HF_DIFFICULTY);
    pad(0x100);action=Take();CHECK(action.kind==UC_HF_REWARDS);
    keyboard_mode=0;Publish(levels);CHECK(!TrialFrontendOwns(levels.source.owner,UC_HF_LEVELS));
    CHECK(!frontend_presentation.visible);DrawFrame();CHECK(hits.empty());
    auto controller_slide=levels;controller_slide.source.source_kind|=UC_HF_TRANSITION;
    Publish(controller_slide);CHECK(!frontend_presentation.visible);
    keyboard_mode=1;Publish(levels);CHECK(TrialFrontendOwns(levels.source.owner,UC_HF_LEVELS));
    auto rewards=MakeList(1);rewards.list_kind=0;rewards.source.kind=UC_HF_REWARD_BROWSER;rewards.source.count=55;
    for(unsigned i=0;i<55;++i){rewards.source.rows[i].row=0x4000+i*44;rewards.source.rows[i].enabled=1;
        std::snprintf(rewards.labels[i],sizeof(rewards.labels[i]),"Reward %u",i+1);}
    Publish(rewards);CHECK(hits.size()==12);Click(3);action=Take();CHECK(action.index==3&&action.kind==UC_HF_ACTIVATE);
    Click(11);action=Take();CHECK(action.kind==UC_HF_SELECT&&action.index==9);
    rewards.source.selected=54;++rewards.source.epoch;Publish(rewards);CHECK(hits.size()==4);Click(0);action=Take();CHECK(action.index==54);
    for(unsigned i=0;i<1;++i){const auto r=TrialLevelBounds(i,TrialRewardsPerPage);
        CHECK(hits[i].a.x==r.x*width/640.f&&hits[i].a.y==r.y*height/480.f);}
    RightClick();action=Take();CHECK(action.kind==UC_HF_BACK);
    keyboard_mode=0;Publish(rewards);CHECK(hits.empty()&&!frontend_presentation.visible);
    keyboard_mode=1;
    CheckControls(options);CheckLanguage(options);CheckProfilesAndName();CheckQuit(main);CheckPauseConfirmation();
    CheckVisualHandoff(main);CheckVisualHandoff(pause);CheckVisualHandoff(options);CheckVisualHandoff(levels);

    // A queued Graphics request must never attach to a newly switched screen.
    Publish(options);Click(options.source.count,false);CHECK(open_requested);
    TrialGraphicsMenuContext(0x800,0x9990,0x9994,3);DrawFrame();CHECK(!PanelOpen()&&hits.empty());
    // A settings overlay also relinquishes the outgoing scene immediately,
    // before a later context callback has published the new manager identity.
    OpenGraphics(options);auto moving_options=options;moving_options.source.source_kind|=UC_HF_TRANSITION;
    TrialFrontendPublish(moving_options);DrawFrame();CHECK(!PanelOpen()&&hits.size()==options.source.count+2);
    TrialFrontendPublish({});DrawFrame();CHECK(hits.empty()&&!TrialGraphicsOpen());
    // Render, effects, world/weapon FOV, HD, four fog modes, RMB flow, controller Apply.
    CHECK(applied==before+11);
    std::printf("PASS %ux%u: menu strips, profile targets, typed names, 17 level hits, draft/apply/discard, release gates and stale screen revocation\n",width,height);
}
int main(){
    ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.LogFilename=nullptr;
    TrialGraphicsFonts(/* GAME_PATH */);
    CHECK(body_font&&title_font&&std::strstr(body_font->GetDebugName(),"Urban Chaos"));
    float back_uv[4]{};CHECK(TrialMenuIconUV(VK_RBUTTON,back_uv));
    io.Fonts->SetTexID(ImTextureID(1));
    CheckLegacyFogSettings();CHECK(applied==8);
    const unsigned before_ui=applied;
    CheckSize(640,480);CheckSize(1920,1080);CheckSize(2560,1440);
    CheckNativeConfirmTransitions();
    for(unsigned kind:{4u,5u}){
        auto page=MakeList(kind);
        for(unsigned i=0;i<page.source.count;++i){page.source.rows[i].actions=UC_HF_ROW_LEFT|UC_HF_ROW_RIGHT;std::strcpy(page.values[i],"16");}
        keyboard_mode=1;Publish(page);CHECK(hits.size()==page.source.count+1);
        auto r=hits[0];ClickPoint(r.a.x+345*output.width/640.f,(r.a.y+r.b.y)/2);
        auto action=Take();CHECK(action.kind==UC_HF_LEFT&&action.index==0);
        Key(ImGuiKey_RightArrow);action=Take();CHECK(action.kind==UC_HF_RIGHT&&action.index==0);
        RightClick();action=Take();CHECK(action.kind==UC_HF_BACK);
        auto transition=page;transition.source.source_kind|=UC_HF_TRANSITION;
        TrialFrontendPublish(transition);DrawFrame();CHECK(!TrialFrontendOwns(page.source.owner,UC_HF_LIST));
        Key(ImGuiKey_RightArrow);EmptyAction();
        keyboard_mode=0;Publish(page);CHECK(hits.empty()&&!TrialGraphicsOpen());
        keyboard_mode=1;
    }
    std::printf("UI totals: input changes=%u, applies=%u, frames=%u\n",input_changes,applied-before_ui,frames);
    CHECK(input_changes>10&&applied==before_ui+33&&frames>100);
    ImGui::DestroyContext();std::puts("PASS actual menu UI + original fonts offline; no live scene or visual parity claim");
}

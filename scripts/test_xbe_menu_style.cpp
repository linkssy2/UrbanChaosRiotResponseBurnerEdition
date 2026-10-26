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
#include "xbe_health_packs.h"
#include "xbe_coop_interactions.h"
#include "xbe_coop_preview.h"
#include "xbe_coop_pause.h"
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)

struct HitRecord {std::string name;ImGuiID id;ImVec2 a,b;};
static std::vector<HitRecord> hits;
struct ReviveRecord {unsigned slot,source;std::string binding,label;};
static std::vector<ReviveRecord> revive_records;
struct CountdownRecord {unsigned slot,seconds;ImVec2 center;std::string label;ImU32 number_color;};
static std::vector<CountdownRecord> countdown_records;
static unsigned revive_seconds[2]{};
unsigned TrialCoopReviveSeconds(unsigned slot){return slot<2?revive_seconds[slot]:0;}
static void RecordReviveCountdown(unsigned slot,unsigned seconds,ImVec2 center,const char* label){
    // Observe the foreground glyphs actually emitted by Center, not its requested color.
    const auto* draw=ImGui::GetWindowDrawList();CHECK(!draw->VtxBuffer.empty());
    countdown_records.push_back({slot,seconds,center,label,draw->VtxBuffer.back().col});
}
struct PreviewRecord {unsigned character;ImVec2 minimum,maximum;double elapsed;};
static std::vector<PreviewRecord> preview_records;
static unsigned preview_initializations=0,preview_finalizations=0;
namespace trial_coop_preview {
bool Initialize(ImFontAtlas*,const std::filesystem::path&){++preview_initializations;return false;}
bool Finalize(){++preview_finalizations;return false;}
bool Draw(unsigned character,ImVec2 minimum,ImVec2 maximum,double elapsed){
    preview_records.push_back({character,minimum,maximum,elapsed});return true;
}
void Reset(){}
}
static void RecordRevivePrompt(unsigned slot,unsigned source,const char* binding,const char* label){
    revive_records.push_back({slot,source,binding,label});
}
static unsigned input_changes=0,applied=0,frames=0;
static app::Mode output{640,480};
static trial_graphics::Draft live;
static unsigned second_fov=53,second_weapon_fov=100;
static unsigned sample_support=2|(1u<<2)|(1u<<4);
static bool fail_apply=false;
static unsigned controls_applied=0;
static unsigned coop_presentation=0;
unsigned TrialCoopPresentation(){return coop_presentation;}
static TrialHealthPackStatus health_status[2]{};
TrialHealthPackStatus TrialHealthPackSnapshot(unsigned slot){return slot<2?health_status[slot]:TrialHealthPackStatus{};}
static TrialCoopInteractionStatus conversation_status[2]{};
TrialCoopInteractionStatus TrialCoopInteractionSnapshot(unsigned slot){return slot<2?conversation_status[slot]:TrialCoopInteractionStatus{};}
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
static float movie_progress=0;
float TrialIntroProgress(){return movie_progress;}
bool TrialIntroPlaying(){return movie_progress>0;}
static int keyboard_mode=1;
int xbox_KeyboardMode(){const int owner=trial_coop::PauseOwner();return owner<0?keyboard_mode:owner==0;}
void TrialGraphicsInputChanged(){++input_changes;}
namespace app {
void* Window(){return nullptr;}
Mode Output(){return output;}
}
namespace trial_graphics {
static bool hud_widescreen=true;
bool Widescreen(){return hud_widescreen;}
Draft Snapshot(){return live;}
Draft Snapshot(unsigned player){CHECK(player<2);auto result=live;if(player){result.fov=second_fov;result.weapon_fov=second_weapon_fov;}return result;}
bool Apply(const Draft& next,std::string& status){
    if(fail_apply){status="Could not save settings.";return false;}
    live=next;++applied;status="Changes applied and saved.";return true;
}
bool Apply(const Draft& next,std::string& status,unsigned player){
    CHECK(player<2);const auto first=live;
    if(!Apply(next,status))return false;
    if(player){second_fov=next.fov;second_weapon_fov=next.weapon_fov;live.fov=first.fov;live.weapon_fov=first.weapon_fov;}
    return true;
}
bool ScalePending(){return false;}
bool textures_available=true;
bool WorldTexturesAvailable(){return textures_available;}
bool HdUi(){return live.hd_ui;}
bool MsaaSupported(uint32_t samples){return samples<32&&(sample_support&(1u<<samples));}
}
namespace trial_language {
bool SpanishAvailable(){return false;}
bool SpanishVoicesAvailable(){return false;}
static uint32_t selected=0,active=0;static unsigned saves=0;
uint32_t Active(){return active;}
uint32_t Snapshot(){return selected;}
bool PendingRestart(){return selected!=active;}
bool Apply(uint32_t value,std::string& status){
    CHECK(value<Count);if(fail_apply){status="Could not save language.";return false;}
    selected=value;++saves;status.clear();return true;
}
}
namespace trial_controls {
static Draft live_controls;
static Draft second_controls;
static CaptureGate capture;
static CaptureGate second_capture;
Draft Defaults(){
    Draft result;const unsigned first[25]={'R','X','C','V','T','E',VK_LCONTROL,VK_LBUTTON,VK_RETURN,VK_BACK,VK_SHIFT,VK_RBUTTON,
        VK_UP,VK_DOWN,VK_LEFT,VK_RIGHT,'A','D','W','S','J','L','I','K',VK_MBUTTON};
    for(unsigned i=0;i<25;++i)result.keyboard[i][0]=first[i];result.keyboard[0][1]='F';
    result.previous_firearm[0]=257;
    result.health_pack[0][0]='H';
    for(unsigned i=0;i<24;++i)result.controller[i][0]=i+1;
    return result;
}
Draft Snapshot(unsigned player){CHECK(player<2);return player?second_controls:live_controls;}
bool Apply(const Draft& next,std::string& status,unsigned player){
    CHECK(player<2);
    if(fail_apply){status="Could not save controls.";return false;}
    (player?second_controls:live_controls)=next;++controls_applied;status="Controls applied and saved.";return true;
}
void BeginCapture(unsigned device,unsigned player){CHECK(player<2);(player?second_capture:capture).Begin(device);}
void CancelCapture(unsigned player){CHECK(player<2);(player?second_capture:capture).Cancel();}
unsigned TakeCapture(unsigned player){CHECK(player<2);return (player?second_capture:capture).Take();}
bool CaptureWaiting(unsigned player){CHECK(player<2);return (player?second_capture:capture).active;}
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
    unsigned profile_values[7];std::memcpy(profile_values,profile_live,sizeof(profile_values));
    TrialControlsProfile(profile_values,context.kind==3);
    hits.clear();revive_records.clear();countdown_records.clear();preview_records.clear();auto& io=ImGui::GetIO();io.DisplaySize={float(output.width),float(output.height)};io.DeltaTime=1.f/60;
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
    s.kind=UC_HF_LIST;s.count=kind==2?5:kind==3||kind==7?3:4;s.selected=0;
    if(kind==2||kind==7)s.source_kind=0x80000010u;
    const char* main[] ={"Single player","Multiplayer","Options","Credits"};
    const char* pause[]={"Return to game","Restart mission","Options","Objectives","Quit"};
    const char* options[]={"Controls","Audio","Back"};
    const char* failed[]={"Continue from last checkpoint","Restart mission","Quit"};
    for(unsigned i=0;i<s.count;++i){
        s.rows[i].enabled=1;s.rows[i].row=0x3000+i*0x100;s.rows[i].actions=UC_HF_ROW_ACTIVATE;
        std::snprintf(f.labels[i],sizeof(f.labels[i]),"%s",kind==2?pause[i]:kind==3?options[i]:kind==7?failed[i]:main[i]);
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
    Click(f.source.count);CHECK(PanelOpen());CHECK(hits.size()==23);
    for(unsigned i=0;i<20;++i){
        CHECK(hits[i].name=="setting");
        CHECK(hits[i].b.y<=364*output.height/480.f+.1f);
        if(i)CHECK(hits[i-1].b.y<=hits[i].a.y+.1f);
    }
}
static void ChangeSetting(unsigned index){
    CHECK(index<20&&hits.size()==23);auto r=hits[index];
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
static void CheckPanelShade(unsigned alpha){
    const float sx=output.width/640.f,sy=output.height/480.f;
    bool found=false;
    for(const auto* list:ImGui::GetDrawData()->CmdLists)for(int i=0;i+3<list->VtxBuffer.Size;++i){
        const auto* v=&list->VtxBuffer[i];
        if(std::abs(v[0].pos.x-3*sx)<.01f&&std::abs(v[0].pos.y-73*sy)<.01f&&
           std::abs(v[2].pos.x-637*sx)<.01f&&std::abs(v[2].pos.y-379*sy)<.01f){
            for(unsigned j=0;j<4;++j)CHECK(v[j].col==IM_COL32(0,0,0,alpha));
            found=true;
        }
    }
    CHECK(found==(alpha!=0));
}
static void CheckHealthPackIcons(const ImGuiWindow* hud,bool split){
    unsigned icons=0;
    const float sx=output.width/640.f,sy=output.height/480.f;
    for(int i=0;i+3<hud->DrawList->VtxBuffer.Size;++i){
        const auto* v=&hud->DrawList->VtxBuffer[i];
        if(v[0].col!=IM_COL32(38,102,49,255))continue;
        CHECK(icons<(split?2u:1u));
        const float raw_end=(split?74.f:106.f)+health_status[icons].bar_extent;
        const float bar_right=raw_end*(trial_graphics::hud_widescreen?.75f:1.f)-.5f;
        CHECK(std::abs(v[0].pos.x-bar_right*sx-6*sy)<.01f);
        CHECK(std::abs(v[0].pos.y-(split?14.f+icons*240.f:38.f)*sy)<.01f);
        CHECK(std::abs(v[2].pos.x-v[0].pos.x-16*sy)<.01f);
        CHECK(std::abs(v[2].pos.y-v[0].pos.y-16*sy)<.01f);
        CHECK(i+11<hud->DrawList->VtxBuffer.Size);
        // Both cross strokes stay centered, square-pixel scaled and opaque white.
        for(unsigned stroke=1;stroke<=2;++stroke){
            const auto* p=v+stroke*4;
            for(unsigned j=0;j<4;++j)CHECK(p[j].col==IM_COL32_WHITE);
            CHECK(std::abs((p[0].pos.x+p[2].pos.x)-(v[0].pos.x+v[2].pos.x))<.01f);
            CHECK(std::abs((p[0].pos.y+p[2].pos.y)-(v[0].pos.y+v[2].pos.y))<.01f);
            CHECK(std::abs(p[2].pos.x-p[0].pos.x-16*sy*(stroke==1?.2f:.6f))<.01f);
            CHECK(std::abs(p[2].pos.y-p[0].pos.y-16*sy*(stroke==1?.6f:.2f))<.01f);
        }
        i+=11;
        ++icons;
    }
    CHECK(icons==(split?2u:1u));
}
static void CheckHealthPackHud(){
    const bool previous_hd=live.hd_ui;
    const bool previous_wide=trial_graphics::hud_widescreen;
    TrialFrontendPublish({});TrialGraphicsMenuContext(0,0,0,0);
    for(const auto size:{app::Mode{640,480},app::Mode{1920,1080},app::Mode{2560,1440}})for(bool wide:{false,true}){
        trial_graphics::hud_widescreen=wide;
        output=size;trial_coop::SetActive(true);coop_presentation=1;
        health_status[0]={1,3,true,false};health_status[1]={2,3,true,true};
        DrawFrame();const auto* hud=ImGui::FindWindowByName("Stored health packs");
        CHECK(hud&&hud->Active&&(hud->Flags&ImGuiWindowFlags_NoInputs)==ImGuiWindowFlags_NoInputs&&hits.empty());
        const float sx=size.width/640.f,sy=size.height/480.f;
        CheckHealthPackIcons(hud,true);
        health_status[0].count=0;DrawFrame();CheckHealthPackIcons(hud,true);
        health_status[0].count=1;
        for(bool hd:{false,true}){live.hd_ui=hd;DrawFrame();CheckHealthPackIcons(hud,true);}
        for(unsigned extent:{120u,140u,160u,180u}){
            health_status[0].bar_extent=extent;health_status[1].bar_extent=300-extent;
            DrawFrame();CheckHealthPackIcons(hud,true);
        }
        for(unsigned speaker=0;speaker<2;++speaker)for(bool waiting:{false,true}){
            conversation_status[speaker]={!waiting,waiting};
            DrawFrame();unsigned prompts=0;
            for(int i=0;i+3<hud->DrawList->VtxBuffer.Size;++i){
                const auto* v=&hud->DrawList->VtxBuffer[i];
                if(v[0].col==IM_COL32(0,0,0,115)&&std::abs(v[0].pos.x-400*sx)<.01f&&std::abs(v[2].pos.x-616*sx)<.01f){
                    CHECK(std::abs(v[0].pos.y-(215+240*speaker)*sy)<.01f);
                    CHECK(std::abs(v[2].pos.y-(234+240*speaker)*sy)<.01f);++prompts;
                }
            }
            CHECK(prompts==1&&hits.empty());conversation_status[speaker]={};
        }
        conversation_status[1]={true,false};
        coop_presentation=2;DrawFrame();CHECK(!hud->Active); // shared cutscene has neither HUD nor divider
        coop_presentation=1;movie_progress=.5f;DrawFrame();CHECK(!hud->Active);movie_progress=0;
        Publish(MakeList(2));CHECK(!hud->Active); // global pause owns the screen
        TrialFrontendPublish({});TrialGraphicsMenuContext(0,0,0,0);
        conversation_status[0]=conversation_status[1]={};
        coop_presentation=0;trial_coop::SetActive(false);DrawFrame();CheckHealthPackIcons(hud,false);
        for(unsigned extent:{120u,140u,160u,180u}){
            health_status[0].bar_extent=extent;DrawFrame();CheckHealthPackIcons(hud,false);
        }
    }
    health_status[0]=health_status[1]={};coop_presentation=0;trial_coop::Reset();live.hd_ui=previous_hd;trial_graphics::hud_widescreen=previous_wide;DrawFrame();
}
static bool IsReviveAura(ImU32 color){
    return (color&IM_COL32(255,255,255,0))==IM_COL32(150,12,8,0);
}
static void CheckNoReviveCountdown(){
    CHECK(countdown_records.empty());
    // Inspect only the current frame, not a stale buffer retained by a hidden window.
    for(const auto* list:ImGui::GetDrawData()->CmdLists)for(const auto& v:list->VtxBuffer)CHECK(!IsReviveAura(v.col));
}
static unsigned CheckReviveDraw(unsigned slot,unsigned seconds){
    CHECK(countdown_records.size()==1&&hits.empty());
    const auto& r=countdown_records[0];
    CHECK(r.slot==slot&&r.seconds==seconds&&r.label=="Revive time remaining");
    CHECK(std::abs(r.center.x-output.width*.5f)<.01f);
    CHECK(std::abs(r.center.y-output.height*(slot?.75f:.25f))<.01f);
    const auto* hud=ImGui::FindWindowByName("Stored health packs");CHECK(hud&&hud->Active);
    const float sx=output.width/640.f,sy=output.height/480.f,top=slot*240.f;
    unsigned panels=0,edges=0,edge_alpha=0;
    const ImVec4 bounds[]={{0,top,48,top+240},{592,top,640,top+240},
        {0,top,640,top+32},{0,top+208,640,top+240}};
    const bool outer[4][4]={{true,false,false,true},{false,true,true,false},
        {true,true,false,false},{false,false,true,true}};
    for(int i=0;i+3<hud->DrawList->VtxBuffer.Size;++i){
        const auto* v=&hud->DrawList->VtxBuffer[i];
        if(IsReviveAura(v[0].col)){
            CHECK(edges<4);const auto& b=bounds[edges];
            CHECK(std::abs(v[0].pos.x-b.x*sx)<.01f&&std::abs(v[0].pos.y-b.y*sy)<.01f);
            CHECK(std::abs(v[2].pos.x-b.z*sx)<.01f&&std::abs(v[2].pos.y-b.w*sy)<.01f);
            for(unsigned j=0;j<4;++j){
                CHECK(IsReviveAura(v[j].col));const unsigned alpha=v[j].col>>IM_COL32_A_SHIFT;
                if(outer[edges][j]){
                    CHECK(alpha>0&&alpha<=104);if(!edge_alpha)edge_alpha=alpha;CHECK(alpha==edge_alpha);
                }else CHECK(!alpha);
            }
            // No aura geometry covers the central scene, including the countdown panel.
            CHECK(b.z<=48||b.x>=592||b.w<=top+32||b.y>=top+208);
            ++edges;i+=3;
        }else if(v[0].col==IM_COL32(0,0,0,145)){
            CHECK(v[0].pos.y>=output.height*slot*.5f&&v[2].pos.y<=output.height*(slot+1)*.5f);
            CHECK(std::abs((v[0].pos.x+v[2].pos.x)*.5f-output.width*.5f)<.01f);
            ++panels;i+=3;
        }
    }
    CHECK(edges==4&&panels==1);
    unsigned aura_indices=0;
    for(const auto& cmd:hud->DrawList->CmdBuffer)for(unsigned i=0;i<cmd.ElemCount;++i){
        const auto& v=hud->DrawList->VtxBuffer[cmd.VtxOffset+hud->DrawList->IdxBuffer[cmd.IdxOffset+i]];
        if(!IsReviveAura(v.col))continue;
        CHECK(std::abs(cmd.ClipRect.x)<.01f&&std::abs(cmd.ClipRect.z-output.width)<.01f);
        CHECK(std::abs(cmd.ClipRect.y-top*sy)<.01f&&std::abs(cmd.ClipRect.w-(top+240)*sy)<.01f);
        CHECK(v.pos.y>=cmd.ClipRect.y&&v.pos.y<=cmd.ClipRect.w);++aura_indices;
    }
    CHECK(aura_indices==24); // Four gradient quads, no full-screen tint or partner overlay.
    if(seconds>3)CHECK(r.number_color==white);
    else{
        const ImVec4 color=ImGui::ColorConvertU32ToFloat4(r.number_color);
        CHECK(color.x>=215.f/255&&color.x>color.y*2&&color.y>color.z&&color.w>=225.f/255);
    }
    return edge_alpha;
}
static void CheckReviveCountdown(){
    const auto previous_output=output;const bool previous_hd=live.hd_ui;
    TrialFrontendPublish({});TrialGraphicsMenuContext(0,0,0,0);
    trial_coop::SetActive(true);coop_presentation=1;
    // The dead player's native health HUD may be absent; the timer is independent.
    health_status[0]=health_status[1]={};
    for(const auto size:{app::Mode{640,480},app::Mode{1920,1080},app::Mode{2560,1440}})for(bool hd:{false,true}){
        output=size;live.hd_ui=hd;
        for(unsigned slot=0;slot<2;++slot)for(unsigned seconds:{10u,9u,3u,1u}){
            revive_seconds[slot]=seconds;
            unsigned min_edge=255,max_edge=0,min_number=255,max_number=0,min_red=255,max_red=0;
            // Observe a full real ImGui-time pulse without changing the remaining seconds.
            for(unsigned tick=0;tick<61;++tick){
                DrawFrame();const unsigned edge=CheckReviveDraw(slot,seconds);
                min_edge=std::min(min_edge,edge);max_edge=std::max(max_edge,edge);
                const unsigned number_alpha=countdown_records[0].number_color>>IM_COL32_A_SHIFT;
                min_number=std::min(min_number,number_alpha);max_number=std::max(max_number,number_alpha);
                const unsigned red=(countdown_records[0].number_color>>IM_COL32_R_SHIFT)&255;
                min_red=std::min(min_red,red);max_red=std::max(max_red,red);
            }
            CHECK(max_edge-min_edge>=38);
            if(seconds<=3)CHECK(max_number-min_number>=28&&max_red-min_red>=38);
            else CHECK(min_number==255&&max_number==255);
            revive_seconds[slot]=0;DrawFrame();CheckNoReviveCountdown(); // Revive/expiry clears in this frame.
        }
        revive_seconds[1]=10;coop_presentation=2;DrawFrame();CheckNoReviveCountdown();
        coop_presentation=1;movie_progress=.5f;DrawFrame();CheckNoReviveCountdown();movie_progress=0;
        Publish(MakeList(2));CheckNoReviveCountdown();
        TrialFrontendPublish({});TrialGraphicsMenuContext(0,0,0,0);
        coop_presentation=0;DrawFrame();CheckNoReviveCountdown();coop_presentation=1;
        revive_seconds[1]=0;
    }
    trial_language::active=4;revive_seconds[0]=10;DrawFrame();
    CHECK(countdown_records.size()==1&&countdown_records[0].label=="Tiempo restante para reanimar");
    revive_seconds[0]=11;DrawFrame();CheckNoReviveCountdown();
    revive_seconds[0]=0;trial_language::active=0;coop_presentation=0;trial_coop::Reset();
    output=previous_output;live.hd_ui=previous_hd;DrawFrame();
}
static void CheckRevivePrompts(){
    const auto previous_output=output;
    const auto p0=trial_controls::live_controls,p1=trial_controls::second_controls;
    TrialFrontendPublish({});TrialGraphicsMenuContext(0,0,0,0);
    trial_coop::SetActive(true);coop_presentation=1;
    trial_controls::live_controls.health_pack[0][0]='H';
    trial_controls::live_controls.health_pack[0][1]=0;
    trial_controls::second_controls.health_pack[1][0]=0;
    trial_controls::second_controls.health_pack[1][1]=4;
    for(const auto size:{app::Mode{640,480},app::Mode{1920,1080},app::Mode{2560,1440}}){
        output=size;health_status[0]=health_status[1]={1,3,true,false};DrawFrame();
        CHECK(revive_records.empty()&&!revive_slide[0]&&!revive_slide[1]);
        for(unsigned slot=0;slot<2;++slot){
            health_status[slot].can_revive=true;DrawFrame();
            CHECK(revive_records.size()==1&&revive_records[0].slot==slot);
            CHECK(revive_records[0].source==(slot?4u:unsigned('H')));
            CHECK(revive_records[0].binding==(slot?trial_controller::Label(4):xbox_KeyboardSourceName('H')));
            CHECK(revive_records[0].label=="Revive Partner");
            CHECK(revive_slide[slot]>0&&revive_slide[slot]<1&&!revive_slide[1-slot]);
            const auto* hud=ImGui::FindWindowByName("Stored health packs");
            float initial_x=-1,final_x=-1;
            for(unsigned step=0;step<14;++step){
                DrawFrame();unsigned strips=0;
                for(int i=0;i+3<hud->DrawList->VtxBuffer.Size;++i){
                    const auto* v=&hud->DrawList->VtxBuffer[i];
                    if(v[0].col!=IM_COL32(0,0,0,145))continue;
                    const float x=v[0].pos.x*640/size.width,y=v[0].pos.y*480/size.height;
                    CHECK(std::abs(y-(178+240*slot))<.01f);
                    CHECK(std::abs(v[2].pos.y*480/size.height-(206+240*slot))<.01f);
                    if(!step)initial_x=x;final_x=x;++strips;i+=3;
                }
                CHECK(strips==1&&hits.empty());
            }
            CHECK(initial_x>final_x&&std::abs(final_x-20)<.01f&&revive_slide[slot]==1);
            health_status[slot].can_revive=false;DrawFrame();CHECK(revive_records.empty()&&!revive_slide[slot]);
        }
        health_status[0].can_revive=health_status[1].can_revive=true;DrawFrame();CHECK(revive_records.size()==2);
        health_status[1].visible=false;DrawFrame();CHECK(revive_records.size()==1&&!revive_slide[1]);
        trial_controls::live_controls.health_pack[0][0]=0;DrawFrame();
        CHECK(revive_records.size()==1&&revive_records[0].source==0&&revive_records[0].label=="Revive Partner");
        trial_controls::live_controls.health_pack[0][0]='T';DrawFrame();
        CHECK(revive_records.size()==1&&revive_records[0].source=='T'&&revive_records[0].binding==xbox_KeyboardSourceName('T'));
        trial_controls::live_controls.health_pack[0][0]='H';
        health_status[0].visible=false;health_status[1].visible=true;
        trial_controls::second_controls.health_pack[1][1]=0;DrawFrame();CHECK(revive_records.size()==1&&revive_records[0].slot==1&&!revive_records[0].source);
        trial_controls::second_controls.health_pack[1][1]=5;DrawFrame();CHECK(revive_records.size()==1&&revive_records[0].source==5&&revive_records[0].binding==trial_controller::Label(5));
        trial_controls::second_controls.health_pack[1][1]=4;
        coop_presentation=2;DrawFrame();CHECK(revive_records.empty()&&!revive_slide[0]&&!revive_slide[1]);coop_presentation=1;
        movie_progress=.5f;DrawFrame();CHECK(revive_records.empty()&&!revive_slide[0]);movie_progress=0;
        Publish(MakeList(2));CHECK(revive_records.empty()&&!revive_slide[0]);
        TrialFrontendPublish({});TrialGraphicsMenuContext(0,0,0,0);
    }
    health_status[0]={1,3,true,true};health_status[1]={};trial_language::active=4;DrawFrame();
    CHECK(revive_records.size()==1&&revive_records[0].label=="Reanimar compa\xc3\xb1" "ero");
    trial_language::active=0;health_status[0]=health_status[1]={};coop_presentation=0;trial_coop::Reset();
    trial_controls::live_controls=p0;trial_controls::second_controls=p1;output=previous_output;DrawFrame();
}
static void CheckCoopDivider(){
    const auto old_output=output;
    const app::Mode sizes[]={{640,480},{1920,1080},{2560,1440},{1281,721}};
    const float expected_top[]={240,539,719,360};
    const float expected_bottom[]={243,546,728,365};
    for(unsigned index=0;index<4;++index){
        const auto size=sizes[index];
        output=size;auto& io=ImGui::GetIO();io.DisplaySize={float(size.width),float(size.height)};io.DeltaTime=1.f/60;
        ImGui::NewFrame();auto* view=ImGui::GetMainViewport();const auto old_pos=view->Pos;
        view->Pos={17,23};auto* foreground=ImGui::GetForegroundDrawList();
        // A prior narrow clip must not clip either end of the separator.
        foreground->PushClipRect({117,123},{217,223},false);
        DrawCoopDivider(view);foreground->PopClipRect();
        CHECK(foreground->VtxBuffer.Size==4);
        const auto* v=foreground->VtxBuffer.Data;
        CHECK(v[0].pos.x==17&&v[0].pos.y==23+expected_top[index]);
        CHECK(v[2].pos.x==17+size.width&&v[2].pos.y==23+expected_bottom[index]);
        for(unsigned i=0;i<4;++i)CHECK(v[i].col==IM_COL32(0,0,0,255));
        bool full_clip=false;
        for(const auto& cmd:foreground->CmdBuffer)if(cmd.ElemCount){
            CHECK(cmd.ClipRect.x==17&&cmd.ClipRect.y==23);
            CHECK(cmd.ClipRect.z==17+size.width&&cmd.ClipRect.w==23+size.height);full_clip=true;
        }
        CHECK(full_clip);view->Pos=old_pos;ImGui::Render();
    }
    output=old_output;
}
static void CheckCoopSelector(){
    const auto old_output=output;
    trial_coop::Reset();CHECK(trial_coop::Available());
    for(const auto size:{app::Mode{640,480},app::Mode{1920,1080},app::Mode{2560,1440}}){
        output=size;auto main=MakeList(1);
        main.source.rows[0].callback=0x271730;
        std::strcpy(main.source.rows[0].label,"NewFrontend-Singleplayer-1");
        Publish(main);CHECK(hits.size()==main.source.count+2);
        Click(0);auto single=Take();
        CHECK(single.kind==UC_HF_ACTIVATE&&single.index==0&&single.row==main.source.rows[0].row);
        CHECK(!trial_coop::Requested()&&!trial_coop::Active()&&trial_coop::Available());
        Publish(main);Click(main.source.count);Settle();
        CHECK(!trial_coop::Requested()&&!trial_coop::Active());
        CHECK(coop_open&&hits.size()==8&&preview_records.size()==2);
        CHECK(preview_records[0].character==0&&preview_records[1].character==0);
        CHECK(preview_records[0].maximum.x<preview_records[1].minimum.x);
        for(const auto& r:preview_records){
            CHECK(r.minimum.x>=0&&r.maximum.x<=size.width&&r.minimum.y>0&&r.maximum.y<size.height);
            CHECK(r.maximum.y<340*size.height/480.f);
        }
        // Mouse can choose either preview; keyboard and controller retain their own player.
        Click(1);CHECK(coop_menu.choice[0]==1&&coop_menu.choice[1]==0);
        const auto second_clock=coop_preview_started[1];
        Key(ImGuiKey_LeftArrow);CHECK(coop_menu.choice[0]==0&&coop_preview_started[1]==second_clock);
        Click(4);CHECK(coop_menu.choice[1]==1&&coop_menu.choice[0]==0);
        Click(5);CHECK(!coop_menu.ready[1]); // no controller, no accidental deployment
        TrialCoopMenuPad(true,0,0,0,0,0);Settle();
        TrialCoopMenuPad(true,8,0,0,0,0);DrawFrame();
        TrialCoopMenuPad(true,0,0,0,0,0);Settle();
        CHECK(coop_menu.choice[1]==2&&coop_menu.choice[0]==0);
        TrialCoopMenuPad(true,0,255,0,0,0);DrawFrame();
        TrialCoopMenuPad(true,0,0,0,0,0);Settle();CHECK(coop_menu.ready[1]);
        Click(2);CHECK(coop_menu.ready[0]&&coop_menu.CanContinue(true));
        TrialCoopMenuPad(false,0,0,0,0,0);Settle();CHECK(!coop_menu.CanContinue(false)&&!coop_menu.ready[1]);
        TrialCoopMenuPad(true,0,0,0,0,0);Settle();Click(5);CHECK(coop_menu.ready[1]);
        Click(7);const auto action=Take();CHECK(action.kind==UC_HF_ACTIVATE&&action.index==0&&!coop_open);
        CHECK(trial_coop::Selected(0)==trial_coop::Character::TZero&&trial_coop::Selected(1)==trial_coop::Character::Banger);
        DrawFrame();CHECK(preview_records.empty());
        // A pending co-op deployment does not carry into Single Player.
        Publish(main);Click(0);single=Take();
        CHECK(single.kind==UC_HF_ACTIVATE&&single.index==0&&!trial_coop::Requested()&&!trial_coop::Active());
        CHECK(trial_coop::Available());
        Publish(main);Click(main.source.count);Settle();
        CHECK(coop_menu.choice[0]==0&&coop_menu.choice[1]==0&&preview_records[0].elapsed<.1);
        RightClick();CHECK(!coop_open&&preview_records.empty());EmptyAction();
    }
    trial_coop::Reset();trial_coop::SetAvailable(false);output=old_output;
}
static void CheckCoopCampaignHeader(){
    const auto old_output=output;
    trial_coop::Reset();trial_coop::SetAvailable(true);
    auto header_covered=[](){
        for(const auto* list:ImGui::GetDrawData()->CmdLists)for(int i=0;i+3<list->VtxBuffer.Size;++i){
            const auto* v=&list->VtxBuffer[i];
            if(v[0].pos.x==0&&v[0].pos.y==0&&v[2].pos.x==float(output.width)&&
               std::abs(v[2].pos.y-62*output.height/480.f)<.01f&&
               v[0].col==IM_COL32(0,0,0,255)&&v[2].col==IM_COL32(0,0,0,255))return true;
        }
        return false;
    };
    for(const auto size:{app::Mode{640,480},app::Mode{1920,1080},app::Mode{2560,1440}}){
        output=size;auto levels=MakeList(1);levels.list_kind=0;levels.source.kind=UC_HF_LEVELS;
        Publish(levels);CHECK(!header_covered());const auto count=hits.size();
        CHECK(trial_coop::Request(trial_coop::Character::TZero,trial_coop::Character::Banger));
        DrawFrame();CHECK(header_covered()&&hits.size()==count);
        // Device-mode changes take effect at the guest's next frame publication.
        keyboard_mode=0;Publish(levels);CHECK(!header_covered());keyboard_mode=1;Publish(levels);
        trial_coop::Reset();DrawFrame();CHECK(!header_covered());
    }
    trial_coop::SetAvailable(false);output=old_output;
}
static void CheckVisualHandoff(const TrialFrontendFrame& f){
    Publish(f);const auto count=hits.size();CHECK(count&&ImGui::GetDrawData()->TotalVtxCount>100);
    FrontendAction(f,UC_HF_ACTIVATE,0); // Must be discarded by the real publisher.
    TrialFrontendFrame transition{};transition.source.epoch=f.source.epoch+1;
    transition.source.source_kind=UC_HF_TRANSITION;
    TrialGraphicsMenuContext(0,0,0,0);TrialFrontendPublish(transition);DrawFrame();
    CHECK(hits.size()==count&&ImGui::GetDrawData()->TotalVtxCount>100);
    if(f.list_kind==7)CheckPanelShade(0);
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
        auto original=f;original.source.kind=kind;original.list_kind=0;original.delete_dialog=false;original.pause_dialog=false;original.source.title[0]=0;
        TrialFrontendPublish(original);DrawFrame();
        CHECK(hits.empty()&&ImGui::GetDrawData()->TotalVtxCount==0&&!frontend_presentation.visible);
        EmptyAction();EmptyName();
    }
    TrialFrontendPublish({});DrawFrame();CHECK(hits.empty());
}
static void CheckProfilesAndName(){
    // Native rows retain the original profile identities and save callbacks.
    auto profiles=MakeList(1);profiles.list_kind=0;profiles.source.kind=UC_HF_PROFILES;
    for(unsigned i=0;i<4;++i)profiles.profile_bounds[i]={100,100+55.f*i,440,42};
    profiles.profile_delete_mask=3;
    Publish(profiles);CHECK(hits.size()==7);
    for(unsigned i=0;i<4;++i){
        CHECK(hits[i].name=="profile");
        CHECK(std::abs(hits[i].a.x-50.f*output.width/640.f)<.1f);
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
    auto footer=[&](unsigned i){Click(unsigned(hits.size())-4+i);};
    auto scroll=[&](float delta){Move(300*output.width/640.f,200*output.height/480.f);ImGui::GetIO().AddMouseWheelEvent(0,delta);DrawFrame();Settle();};
    auto open=[&](){
        Publish(options);Click(0);CHECK(PanelOpen()&&settings_page==1);
        // Hovering the existing original row legitimately publishes SELECT.
        // The host intercepts activation, not that guest selection update.
        UcHybridFrontendAction selected{};
        if(TrialFrontendTakeAction(selected))CHECK(selected.kind==UC_HF_SELECT&&selected.index==0&&selected.row==options.source.rows[0].row);
        EmptyAction();
    };
    trial_controls::live_controls=trial_controls::second_controls=trial_controls::Defaults();controls_device=controls_player=0;
    open();CHECK(hits.size()==20);
    CHECK(std::strcmp(ControlsName(0),"Reload")==0);
    CHECK(std::strcmp(ControlsName(28),"Interact / dialogue skip")==0);
    CHECK(&ControlsBinding(0,0)!=&ControlsBinding(28,0));
    CHECK(std::strcmp(ControlsName(2),"Next Firearm")==0&&std::strcmp(ControlsName(29),"Previous Firearm")==0);
    CHECK(ControlsAction(7)==2&&ControlsAction(8)==29&&ControlsSlots(2)==2&&ControlsSlots(29)==2);
    CHECK(&ControlsBinding(2,0)!=&ControlsBinding(29,0));
    CHECK(&ControlsBinding(29,1)==&controls_draft.previous_firearm[1]);
    CHECK(std::strcmp(ControlsGroup(13),"COMBAT / INTERACTION")==0&&std::strcmp(ControlsGroup(14),"MOVEMENT / AIMING")==0);
    CHECK(std::strcmp(ControlsGroup(24),"TEAMMATE ORDERS")==0&&std::strcmp(ControlsGroup(29),"MENU CONTROLS")==0);
    CHECK(ControlsAction(5)==30&&ControlsHealthPack(30));
    CHECK(&ControlsBinding(30,0)==&controls_draft.health_pack[0][0]);
    for(unsigned device=0;device<2;++device){
        controls_device=device;std::vector<bool> seen(ControlsCount());
        for(unsigned row=0;row<ControlsCount();++row){const auto action=ControlsAction(row);CHECK(action<ControlsCount()&&!seen[action]);seen[action]=true;}
        if(device)CHECK(std::strcmp(ControlsName(2),"Firearms")==0&&!ControlsPreviousFirearm(29));
    }
    controls_device=0;CHECK(ControlsAction(0)==0&&ControlsAction(1)==28);

    CHECK(!ControlsDirty());
    for(const auto* description:control_help)CHECK(std::strlen(description)>20);
    auto source=[](unsigned value){
        trial_controls::capture.Sample(0,0,true);
        trial_controls::capture.Sample(controls_device?0:value,controls_device?value:0,true);
        DrawFrame();Settle();
    };
    // Both firearm directions capture independent slots, take part in conflict
    // replacement, save together and return to their own defaults.
    ControlsBegin(2,0);DrawFrame();Settle();source('B');
    ControlsBegin(29,1);DrawFrame();Settle();source('N');
    CHECK(controls_draft.keyboard[2][0]=='B'&&controls_draft.previous_firearm[1]=='N');
    CHECK(!trial_controls::live_controls.previous_firearm[1]);
    ControlsBegin(29,0);DrawFrame();Settle();source('B');CHECK(controls_conflict_action==2);Click(0);
    CHECK(!controls_draft.keyboard[2][0]&&controls_draft.previous_firearm[0]=='B'&&controls_draft.previous_firearm[1]=='N');
    footer(0);CHECK(!ControlsDirty()&&trial_controls::live_controls.previous_firearm[0]=='B');
    ControlsBegin(29,0);DrawFrame();Settle();trial_controls::capture.Sample(0,0,true);
    trial_controls::capture.Wheel(257);DrawFrame();Settle();CHECK(controls_draft.previous_firearm[0]==257);
    footer(2);const auto defaults=trial_controls::Defaults();
    CHECK(!std::memcmp(controls_draft.previous_firearm,defaults.previous_firearm,sizeof(defaults.previous_firearm)));
    CHECK(controls_draft.keyboard[2][0]==defaults.keyboard[2][0]);footer(0);CHECK(!ControlsDirty());
    // The first held opening input is ignored. A later fresh key edits only
    // the selected slot, with explicit conflict decisions before publication.
    Click(1);CHECK(controls_capture_action==0);
    trial_controls::capture.Sample(VK_LBUTTON,0,true);DrawFrame();CHECK(controls_capture_action==0);
    source('S');CHECK(controls_conflict_action==19&&hits.size()==3);
    Click(2);CHECK(controls_capture_action<0&&!ControlsDirty());
    Click(1);source('Z');CHECK(controls_draft.keyboard[0][0]=='Z'&&ControlsDirty());
    CHECK(trial_controls::live_controls.keyboard[0][0]=='R');
    ControlsBegin(28,0);DrawFrame();Settle();source(VK_MBUTTON);CHECK(controls_conflict_action==24);Click(1);
    CHECK(controls_draft.keyboard[0][1]==VK_MBUTTON&&controls_draft.keyboard[24][0]==VK_MBUTTON);
    const unsigned before=controls_applied;footer(0);CHECK(controls_applied==before+1&&!ControlsDirty());
    // Wheel capture is an event, not a held virtual key; it can bind normal
    // actions but not the hold-and-drag orders action.
    Click(1);trial_controls::capture.Sample(0,0,true);trial_controls::capture.Wheel(256);DrawFrame();Settle();
    CHECK(controls_draft.keyboard[0][0]==256&&ControlsDirty());footer(1);CHECK(!ControlsDirty());
    controls_scroll=24;Settle();
    CHECK(ControlsAction(controls_scroll)==24);Click(1);CHECK(controls_capture_action==24);
    trial_controls::capture.Sample(0,0,true);trial_controls::capture.Wheel(257);DrawFrame();Settle();
    CHECK(controls_capture_action==24&&trial_controls::CaptureWaiting()&&!ControlsDirty());
    Key(ImGuiKey_Escape);Settle();CHECK(controls_capture_action<0&&PanelOpen());
    // Mouse-down on Cancel wins over that same raw left-button candidate.
    Click(1);auto cancel=hits[1];Move((cancel.a.x+cancel.b.x)/2,(cancel.a.y+cancel.b.y)/2);
    trial_controls::capture.Sample(0,0,true);trial_controls::capture.Sample(VK_LBUTTON,0,true);
    ImGui::GetIO().AddMouseButtonEvent(0,true);DrawFrame();
    CHECK(controls_capture_action<0&&!ControlsDirty());
    ImGui::GetIO().AddMouseButtonEvent(0,false);Settle();
    controls_scroll=12;Settle();Click(1);CHECK(controls_draft.spin_mode==1&&ControlsDirty());
    Click(3);CHECK(controls_capture_action==26);source('P');
    CHECK(controls_draft.spin_binding[0][0]=='P');
    footer(1);CHECK(!ControlsDirty()); // Discard restores mode and button together.
    controls_scroll=19;Settle();
    CHECK(controls_draft.sensitivity==400);
    const auto slider=hits[1];const float slider_y=(slider.a.y+slider.b.y)*.5f;
    Move(418*output.width/640.f,slider_y);ImGui::GetIO().AddMouseButtonEvent(0,true);DrawFrame();
    CHECK(controls_draft.sensitivity==1&&ControlsDirty());
    Move(542*output.width/640.f,slider_y);DrawFrame();CHECK(controls_draft.sensitivity==1600);
    Move(418*output.width/640.f,slider_y);DrawFrame();ImGui::GetIO().AddMouseButtonEvent(0,false);Settle();
    CHECK(controls_draft.sensitivity==1);Key(ImGuiKey_LeftArrow);CHECK(controls_draft.sensitivity==1);
    Key(ImGuiKey_RightArrow);CHECK(controls_draft.sensitivity==2);
    Key(ImGuiKey_Tab);Key(ImGuiKey_Enter);CHECK(trial_controls::live_controls.sensitivity==2&&!ControlsDirty());
    // Device selection returns to the top. Controller sources are raw, and
    // Replace clears an overlapping source instead of silently invoking both.
    auto device=hits[0];ClickPoint(device.b.x-15*float(output.width)/640,(device.a.y+device.b.y)/2);
    CHECK(controls_device==1&&controls_scroll==0&&hits.size()==22);
    Click(1);source(8);CHECK(controls_conflict_action==7);Click(0);
    CHECK(controls_draft.controller[0][0]==8&&!controls_draft.controller[7][0]);
    fail_apply=true;footer(0);fail_apply=false;CHECK(ControlsDirty()&&trial_controls::live_controls.controller[0][0]==1);
    footer(0);CHECK(!ControlsDirty()&&trial_controls::live_controls.controller[0][0]==8);
    footer(2);CHECK(ControlsDirty()&&controls_draft.controller[0][0]==1);
    footer(1);CHECK(!ControlsDirty()&&controls_draft.controller[0][0]==8);
    footer(2);RightClick();CHECK(confirm_back&&PanelOpen());
    RightClick();CHECK(!confirm_back&&ControlsDirty());
    footer(3);CHECK(confirm_back);Click(1);CHECK(!PanelOpen());EmptyAction();
    // The redundant Game Options entry is gone; profile values live in Controls.
    Publish(options);CHECK(hits.size()==options.source.count+2);
    // Controller left/right reaches every footer action. It
    // never silently reuses a queued Confirm from the capture dialog.
    open();auto apply=hits[hits.size()-4];Move((apply.a.x+apply.b.x)/2,(apply.a.y+apply.b.y)/2);
    frontend_events=32;DrawFrame();frontend_events=4;DrawFrame();Settle();CHECK(message=="Changes discarded.");
    // Wheel and keyboard scroll one list; all rows remain reachable without page actions.
    scroll(-1);CHECK(controls_scroll==3);scroll(1);CHECK(controls_scroll==0);
    const auto bar=hits[hits.size()-5];
    Move((bar.a.x+bar.b.x)*.5f,bar.b.y-1);ImGui::GetIO().AddMouseButtonEvent(0,true);DrawFrame();
    CHECK(controls_scroll==ControlsCount()-8);
    Move((bar.a.x+bar.b.x)*.5f,bar.a.y+1);DrawFrame();CHECK(controls_scroll==0);
    ImGui::GetIO().AddMouseButtonEvent(0,false);Settle();
    Move(0,0);ClickPoint(hits[1].a.x+2,hits[1].a.y+2);
    Key(ImGuiKey_Escape);Settle(); // Cancel capture, retaining row focus.
    for(unsigned i=0;i<ControlsCount()+8;++i)Key(ImGuiKey_DownArrow);
    CHECK(controls_scroll==ControlsCount()-8);
    footer(3);CHECK(!PanelOpen());EmptyAction();
    // Native edits commit only to the matching original Options profile snapshot.
    open();const unsigned settings[]={1,1,1,0,1,75,35};
    std::memcpy(profile_draft,settings,sizeof(settings));CHECK(ControlsDirty());
    footer(0);CHECK(!ControlsDirty()&&!std::memcmp(profile_live,settings,sizeof(settings)));
    profile_draft[0]=0;footer(1);CHECK(profile_draft[0]==1&&!ControlsDirty());
    unsigned snapshot[7];std::memcpy(snapshot,profile_live,sizeof(snapshot));
    profile_draft[5]=80;CHECK(ControlsApply());
    unsigned stale[7];std::memcpy(stale,snapshot,sizeof(stale));stale[6]=40;
    CHECK(!TrialControlsProfile(stale,true)&&stale[5]==75); // A changed profile rejects a queued write.
    TrialControlsProfile(snapshot,true);ControlsReadDraft();Close();Settle();
    open();controls_device=1;controls_player=1;ControlsReadDraft();Settle();
    CHECK(ControlsCount()==34&&ControlsProfileStart()==27&&ControlsHealthPack(26));
    const auto first_profile=trial_controls::live_controls;
    ControlsBegin(26,0);DrawFrame();Settle();
    trial_controls::second_capture.Sample(0,0,true);trial_controls::second_capture.Sample(0,8,true);
    DrawFrame();Settle();CHECK(controls_conflict_action==7);Click(0);
    CHECK(controls_draft.health_pack[1][0]==8&&!controls_draft.controller[7][0]);
    footer(0);CHECK(trial_controls::second_controls.health_pack[1][0]==8);
    CHECK(!std::memcmp(&first_profile,&trial_controls::live_controls,sizeof(first_profile)));
    Close();controls_player=controls_device=0;Settle();
    // Original Options Controls is intercepted only within the qualified hub.
    auto unrelated=options;unrelated.list_kind=1;++unrelated.source.epoch;
    Publish(unrelated);Click(0);auto action=Take();CHECK(action.kind==UC_HF_ACTIVATE&&!PanelOpen());
}
static void CheckPerPlayerSettings(){
    const auto saved_live=live;const auto first_controls=trial_controls::live_controls;
    const auto second_controls=trial_controls::second_controls;
    auto own=[](unsigned player){
        trial_coop::SetActive(true);trial_coop::ResetPauseOwner();
        trial_coop::pause_state.Sample(false,false);trial_coop::pause_state.Sample(player==0,player==1);
        trial_coop::pause_state.Consume(true);trial_coop::pause_state.Enter();
        CHECK(trial_coop::PauseOwner()==int(player));keyboard_mode=1;
        CHECK(xbox_KeyboardMode()==int(player==0));Move(0,0);
    };
    auto pad=[](unsigned buttons,unsigned confirm=0){
        TrialFrontendPad(0,0,0,0,0);DrawFrame();TrialFrontendPad(buttons,confirm,0,0,0);DrawFrame();
        TrialFrontendPad(0,0,0,0,0);Settle();Drain();
    };
    auto options=[](){auto f=MakeList(3);f.source.source_kind|=0x80000000u;return f;};
    for(const auto size:{app::Mode{640,480},app::Mode{1920,1080},app::Mode{2560,1440}}){
        output=size;trial_language::active=size.width==1920?4:0;
        for(unsigned player:{0u,1u}){
            own(player);auto f=options();Publish(f);CHECK(frontend_active&&hits.size()==f.source.count+2);
            controls_player=1-player;controls_device=controls_player?1:0;
            // The ordinary controller Controls row opens the owned editor, not the original shared profile page.
            if(player)pad(0,255);else Click(0);
            CHECK(PanelOpen()&&settings_page==1&&settings_pause_player==int(player));
            CHECK(controls_player==player&&controls_device==player);
            const auto device=hits[0];ClickPoint(device.b.x-15*output.width/640.f,(device.a.y+device.b.y)/2);
            CHECK(controls_player==player&&controls_device==player);
            const auto partner=trial_controls::Snapshot(1-player);
            unsigned native_before[7],pending_before[7];
            std::memcpy(native_before,profile_live,sizeof(native_before));std::memcpy(pending_before,profile_pending,sizeof(pending_before));
            const bool apply_before=profile_apply;
            const unsigned binding=controls_draft.controller[0][0]==22?23:22;
            controls_draft.controller[0][0]=binding;
            if(player){CHECK(ControlsCount()==34);profile_draft[1]^=1;profile_draft[5]=23;}
            CHECK(ControlsDirty());fail_apply=true;CHECK(!ControlsApply());fail_apply=false;CHECK(ControlsDirty());
            CHECK(ControlsApply());
            if(player)CHECK(profile_apply==apply_before&&!std::memcmp(native_before,profile_live,sizeof(native_before))&&!std::memcmp(pending_before,profile_pending,sizeof(pending_before)));
            Settle();CHECK(!ControlsDirty());
            const auto current=trial_controls::Snapshot(player),unchanged=trial_controls::Snapshot(1-player);
            CHECK(current.controller[0][0]==binding);
            CHECK(!std::memcmp(&partner,&unchanged,sizeof(partner)));
            if(player)CHECK(current.native_profile[5]==23&&current.native_profile[1]==profile_draft[1]);
            profile_draft[5]=99;CHECK(ControlsDirty());ControlsReadDraft();CHECK(!ControlsDirty());
            Close();Settle();
            f=options();Publish(f);
            if(player){for(unsigned i=0;i<f.source.count;++i)pad(2);pad(0,255);}else Click(f.source.count);
            CHECK(PanelOpen()&&settings_page==0&&settings_player==player&&settings_pause_player==int(player));
            const auto other=trial_graphics::Snapshot(1-player),before=trial_graphics::Snapshot(player);
            draft.fov=player?85:65;draft.weapon_fov=player?135:110;
            const auto target=draft;CHECK(Dirty()||before.fov==target.fov);
            fail_apply=true;CHECK(!Apply());fail_apply=false;
            CHECK(trial_graphics::Snapshot(player).fov==before.fov&&trial_graphics::Snapshot(player).weapon_fov==before.weapon_fov);
            CHECK(Apply()&&!Dirty());CHECK(trial_graphics::Snapshot(player).fov==target.fov);
            CHECK(trial_graphics::Snapshot(1-player).fov==other.fov&&trial_graphics::Snapshot(1-player).weapon_fov==other.weapon_fov);
            draft.fov=40;ReadDraft();CHECK(!Dirty()&&draft.fov==target.fov);
            // Resume revokes an unsaved panel before it can affect the next player's settings.
            draft.fov=90;trial_coop::ResetPauseOwner();DrawFrame();CHECK(!PanelOpen());
            CHECK(trial_graphics::Snapshot(player).fov==target.fov);Drain();
        }
    }
    trial_coop::Reset();trial_coop::ResetPauseOwner();keyboard_mode=1;trial_language::active=0;
    // A remembered P2 selection in the main menu must not leak into single-player pause.
    controls_player=controls_device=1;Publish(options());Click(0);CHECK(controls_player==0&&controls_device==0);Close();
    controls_player=controls_device=0;settings_player=0;settings_pause_player=-1;
    trial_controls::live_controls=first_controls;trial_controls::second_controls=second_controls;live=saved_live;
    TrialFrontendPublish({});TrialGraphicsMenuContext(0,0,0,0);Settle();Drain();
    std::puts("PASS owned pause Controls/Graphics: normal controller entry, independent apply/discard/failed save, locked player and stale-panel revocation");
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
    change();change();change();CHECK(language_draft==0);Click(1);CHECK(!trial_language::PendingRestart());
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
static void CheckPauseConfirmation(uint32_t callback=0x267400){
    const auto before=quit_posts;
    auto f=MakeList(2);f.list_kind=0;f.source.kind=UC_HF_DIALOG;f.source.count=2;
    f.source.owner=0x7800;f.source.selected=1;f.source.back_action=0x17;f.pause_dialog=true;
    f.source.rows[0].callback=callback;f.source.rows[1].callback=0x284310;
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
    CheckPanelShade(112);CHECK(quit_posts==before);
    // Switching devices relinquishes the native modal, including queued Yes.
    FrontendAction(f,UC_HF_ACTIVATE,0);keyboard_mode=0;Publish(f);
    CHECK(hits.empty()&&!frontend_presentation.visible&&!TrialFrontendOwns(f.source.owner,UC_HF_DIALOG));EmptyAction();
    keyboard_mode=1;++f.source.epoch;Publish(f);CheckStrip(2);
    auto invalid=f;invalid.source.rows[0].callback=0x267331;
    CHECK(!TrialFrontendSupported(invalid));
    invalid=f;invalid.source.source_kind=0;CHECK(!TrialFrontendSupported(invalid));
    invalid=f;invalid.dialog_message[0]=0;CHECK(!TrialFrontendSupported(invalid));
    CheckVisualHandoff(f);CHECK(quit_posts==before);
}
static void CheckSize(unsigned width,unsigned height){
    std::memset(profile_live,0,sizeof(profile_live));profile_apply=false;
    output={width,height};live={};Close();TrialFrontendPublish({});TrialGraphicsMenuContext(0,0,0,0);Settle();
    auto failed=MakeList(7);Publish(failed);CheckStrip(3);
    CheckPanelShade(0);
    for(unsigned i=0;i<3;++i){Click(i);auto selected=Take();CHECK(selected.kind==UC_HF_ACTIVATE&&selected.index==i&&selected.row==failed.source.rows[i].row);}
    keyboard_mode=0;Publish(failed);CHECK(hits.empty());keyboard_mode=1;
    CheckVisualHandoff(failed);
    auto main=MakeList(1);Publish(main);CheckStrip(5);CHECK(ImGui::GetDrawData()->TotalVtxCount>100);
    CheckPanelShade(168);
    Click(2);auto action=Take();CHECK(action.kind==UC_HF_ACTIVATE&&action.index==2&&action.row==main.source.rows[2].row);
    main.source.rows[1].enabled=0;++main.source.epoch;Publish(main);Click(1);EmptyAction();

    // Escape still held from gameplay must not immediately choose a pause item.
    ImGui::GetIO().AddKeyEvent(ImGuiKey_Escape,true);auto pause=MakeList(2);Publish(pause);
    CheckStrip(5);DrawFrame();EmptyAction();CheckPanelShade(112);
    ImGui::GetIO().AddKeyEvent(ImGuiKey_Escape,false);Settle();EmptyAction();Key(ImGuiKey_Escape);
    action=Take();CHECK(action.kind==UC_HF_BACK);
    Publish(pause);Click(0);action=Take();CHECK(action.kind==UC_HF_ACTIVATE&&action.index==0);

    // Settings edit only the draft until Apply; Back keeps the same draft behind
    // the unsaved-changes confirmation, while Discard leaves live values alone.
    auto options=MakeList(3);OpenGraphics(options);unsigned before=applied;ChangeSetting(3);CHECK(Dirty());CHECK(live.settings.render_percent==100);
    sample_support=2|(1u<<4);ChangeSetting(9);CHECK(draft.msaa==4&&live.msaa==1);
    sample_support=2|(1u<<2)|(1u<<4);
    Click(20);CHECK(applied==before+1&&live.settings.render_percent==125&&live.msaa==4&&!Dirty());
    ChangeSetting(10);ChangeSetting(11);ChangeSetting(12);ChangeSetting(13);ChangeSetting(13);
    CHECK(!draft.scenery&&!draft.characters&&draft.character_quality==1&&draft.ragdolls==2&&Dirty());
    CHECK(live.scenery&&live.characters&&live.character_quality==0&&live.ragdolls==0);
    CHECK(hits.size()==23&&draft.ragdolls==2);Click(20);
    CHECK(applied==before+2&&!live.scenery&&!live.characters&&live.character_quality==1&&live.ragdolls==2&&!Dirty());
    ChangeSetting(13);CHECK(Dirty());Click(21);CHECK(!Dirty()&&draft.ragdolls==2);
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
    Click(20);CHECK(live.fov==70&&live.reticle_scale==150&&!Dirty());
    ChangeSetting(16);CHECK(draft.reticle_scale==155&&live.reticle_scale==150&&Dirty());
    Click(22);CHECK(confirm_back);Click(2);CHECK(PanelOpen()&&Dirty());
    Click(21);CHECK(draft.reticle_scale==150&&live.reticle_scale==150&&!Dirty());
    // Weapon FOV is independent of the world camera, with the same Apply policy.
    CHECK(draft.weapon_fov==100&&live.weapon_fov==100);
    drag(17,470,600);CHECK(draft.weapon_fov==150&&live.weapon_fov==100&&draft.fov==70);
    drag(17,470,300);CHECK(draft.weapon_fov==50&&live.weapon_fov==100&&draft.fov==70);
    drag(17,470,511);CHECK(draft.weapon_fov==125&&live.weapon_fov==100);
    fail_apply=true;Click(20);fail_apply=false;CHECK(Dirty()&&live.weapon_fov==100);
    Click(21);CHECK(!Dirty()&&draft.weapon_fov==100);
    ChangeSetting(17);CHECK(draft.weapon_fov==105&&Dirty());Click(22);CHECK(confirm_back);
    Click(2);CHECK(PanelOpen()&&Dirty());Click(20);
    CHECK(live.weapon_fov==105&&live.fov==70&&!Dirty());
    // Movie scaling follows the same staged Apply, failure and Discard policy.
    CHECK(draft.fmv_scale==0&&live.fmv_scale==0);
    ChangeSetting(19);CHECK(draft.fmv_scale==1&&live.fmv_scale==0&&Dirty());
    Key(ImGuiKey_LeftArrow);CHECK(draft.fmv_scale==0&&!Dirty());
    Key(ImGuiKey_RightArrow);CHECK(draft.fmv_scale==1&&live.fmv_scale==0&&Dirty());
    fail_apply=true;Click(20);fail_apply=false;CHECK(Dirty()&&live.fmv_scale==0);
    Click(21);CHECK(!Dirty()&&draft.fmv_scale==0);
    ChangeSetting(19);Click(22);CHECK(confirm_back);Click(2);CHECK(PanelOpen()&&Dirty());
    Click(20);CHECK(live.fmv_scale==1&&!Dirty());
    ChangeSetting(19);Click(21);CHECK(draft.fmv_scale==1&&live.fmv_scale==1&&!Dirty());
    // HD changes use the same staged save/discard/back policy, including failures.
    CHECK(!live.hd_ui);ChangeSetting(15);CHECK(draft.hd_ui&&Dirty()&&!live.hd_ui);
    fail_apply=true;Click(20);fail_apply=false;CHECK(draft.hd_ui&&Dirty()&&!live.hd_ui);
    Click(21);CHECK(!draft.hd_ui&&!Dirty());
    ChangeSetting(15);Click(22);CHECK(confirm_back&&hits.size()==3);
    Click(2);CHECK(PanelOpen()&&draft.hd_ui&&Dirty());
    Click(22);Click(1);CHECK(!PanelOpen()&&!live.hd_ui);
    OpenGraphics(options);CHECK(!draft.hd_ui&&!Dirty());
    ChangeSetting(15);Click(20);CHECK(live.hd_ui&&!Dirty());
    ChangeSetting(15);Click(21);CHECK(live.hd_ui&&draft.hd_ui&&!Dirty());
    // One Fog control covers all four effective modes. Mouse edits stay in the
    // draft, and failed Apply rolls back the three compatible config keys.
    CHECK(TrialDistanceFog&&!TrialSmoothFog&&TrialFogDistanceMode==0);
    ChangeSetting(8);CHECK(draft_fog_mode==2&&!TrialSmoothFog&&Dirty());
    Click(21);CHECK(draft_fog_mode==1&&TrialDistanceFog&&!Dirty());
    ChangeSetting(8);Click(20);CHECK(TrialDistanceFog&&TrialFogDistanceMode==0&&TrialSmoothFog&&!Dirty());
    ChangeSetting(8);Click(20);CHECK(TrialDistanceFog&&TrialFogDistanceMode==1&&!TrialSmoothFog&&!Dirty());
    ChangeSetting(8);fail_apply=true;Click(20);fail_apply=false;
    CHECK(TrialDistanceFog&&TrialFogDistanceMode==1&&!TrialSmoothFog&&draft_fog_mode==0&&Dirty());
    Click(20);CHECK(!TrialDistanceFog&&TrialFogDistanceMode==0&&!TrialSmoothFog&&!Dirty());
    ChangeSetting(8);Click(20);CHECK(TrialDistanceFog&&TrialFogDistanceMode==0&&!TrialSmoothFog&&!Dirty());
    ChangeSetting(8);Click(22);CHECK(confirm_back&&hits.size()==3);
    Key(ImGuiKey_Escape);Settle();CHECK(!confirm_back&&PanelOpen()&&Dirty()&&hits.size()==23);
    CHECK(draft_fog_mode==2&&!TrialSmoothFog);Click(21);CHECK(!Dirty());
    ChangeSetting(6);CHECK(Dirty()&&message.empty());Click(22);CHECK(confirm_back&&hits.size()==3);
    Key(ImGuiKey_Escape);Settle();CHECK(!confirm_back&&PanelOpen()&&Dirty()&&hits.size()==23);
    Click(22);CHECK(confirm_back&&hits.size()==3);Click(1);CHECK(!PanelOpen());
    CHECK(live.settings.render_percent==125&&live.settings.fps_cap==60);CheckStrip(options.source.count+2);

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
    Click(20);CHECK(!Dirty());
    ImGui::GetIO().AddMouseButtonEvent(1,true);DrawFrame();CHECK(!PanelOpen());
    Settle();EmptyAction(); // held closing click cannot back out of Options too
    ImGui::GetIO().AddMouseButtonEvent(1,false);Settle();EmptyAction();
    RightClick();action=Take();CHECK(action.kind==UC_HF_BACK);
    for(const auto& page:{main,pause,options}){
        Publish(page);RightClick();action=Take();CHECK(action.kind==UC_HF_BACK);EmptyAction();
    }

    OpenGraphics(options);CHECK(!live.world_textures);trial_graphics::textures_available=false;ChangeSetting(18);CHECK(!draft.world_textures&&!Dirty());trial_graphics::textures_available=true;ChangeSetting(18);CHECK(draft.world_textures&&Dirty()&&!live.world_textures);
    Click(21);CHECK(!draft.world_textures&&!Dirty());ChangeSetting(18);Click(20);CHECK(live.world_textures&&!Dirty());Close();
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
    pad(2);pad(8);CHECK(draft.world_textures!=live.world_textures);
    pad(2);const auto old_fmv=live.fmv_scale;pad(8);
    CHECK(draft.fmv_scale!=old_fmv&&live.fmv_scale==old_fmv&&Dirty());
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
    CheckControls(options);CheckLanguage(options);CheckProfilesAndName();CheckQuit(main);CheckPauseConfirmation();CheckPauseConfirmation(0x267330);CheckPauseConfirmation(0x2672A0);
    CheckVisualHandoff(main);CheckVisualHandoff(pause);CheckVisualHandoff(options);CheckVisualHandoff(levels);

    // A queued Graphics request must never attach to a newly switched screen.
    Publish(options);Click(options.source.count,false);CHECK(open_requested);
    TrialGraphicsMenuContext(0x800,0x9990,0x9994,3);DrawFrame();CHECK(!PanelOpen()&&hits.empty());
    // A settings overlay also relinquishes the outgoing scene immediately,
    // before a later context callback has published the new manager identity.
    OpenGraphics(options);auto moving_options=options;moving_options.source.source_kind|=UC_HF_TRANSITION;
    TrialFrontendPublish(moving_options);DrawFrame();CHECK(!PanelOpen()&&hits.size()==options.source.count+2);
    TrialFrontendPublish({});DrawFrame();CHECK(hits.empty()&&!TrialGraphicsOpen());
    // Render, effects, world/weapon FOV, FMV scaling, HD, four fog modes, RMB flow, controller Apply.
    CHECK(applied==before+13);
    std::printf("PASS %ux%u: menu strips, profile targets, typed names, 17 level hits, draft/apply/discard, release gates and stale screen revocation\n",width,height);
}
int main(){
    ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;io.LogFilename=nullptr;
    trial_coop::Reset();trial_coop::SetAvailable(false);
    CHECK(SetEnvironmentVariableW(L"UCRR_COOP_TEST",nullptr));
    TrialGraphicsFonts(/* GAME_PATH */);
    CHECK(trial_coop::Available()&&!trial_coop::Requested()&&!trial_coop::Active());
    CHECK(preview_initializations==1&&preview_finalizations==1);
    CHECK(body_font&&title_font&&std::strstr(body_font->GetDebugName(),"Urban Chaos"));
    float back_uv[4]{};CHECK(TrialMenuIconUV(VK_RBUTTON,back_uv));
    io.Fonts->SetTexID(ImTextureID(1));
    CheckLegacyFogSettings();CHECK(applied==8);
    const unsigned before_ui=applied;
    CheckSize(640,480);CheckSize(1920,1080);CheckSize(2560,1440);
    CheckNativeConfirmTransitions();
    for(const auto size:{app::Mode{640,480},app::Mode{1920,1080},app::Mode{2560,1440}}){
        output=size;auto f=MakeList(2);f.list_kind=0;f.source.kind=UC_HF_OBJECTIVES;f.source.count=1;
        f.source.back_action=0x17;f.source.rows[0].row=f.source.owner;
        Publish(f);CheckStrip(3);
        for(unsigned i=0;i<3;++i){Click(i);const auto a=Take();CHECK(a.kind==(i==0?UC_HF_LEFT:i==1?UC_HF_BACK:UC_HF_RIGHT));}
        Key(ImGuiKey_UpArrow);CHECK(Take().kind==UC_HF_LEFT);
        Key(ImGuiKey_DownArrow);CHECK(Take().kind==UC_HF_RIGHT);
        ImGui::GetIO().AddMouseWheelEvent(0,-1);DrawFrame();CHECK(Take().kind==UC_HF_RIGHT);Settle();
        RightClick();CHECK(Take().kind==UC_HF_BACK);
        keyboard_mode=0;Publish(f);CHECK(hits.empty());keyboard_mode=1;
    }
    for(unsigned count:{3u,4u}){
        auto f=MakeList(1);f.list_kind=0;f.source.kind=UC_HF_DIALOG;f.source.count=count;
        std::strcpy(f.source.title,"NewFrontEnd-EMH-1");
        const char* keys[]={"NewFrontEnd-EMH-2","NewFrontEnd-EMH-3","NewFrontEnd-EMH-4","NewFrontEnd-EMH-5"};
        for(unsigned i=0;i<count;++i){std::strcpy(f.source.rows[i].label,keys[i]);f.source.rows[i].callback=0x280000+16*i;}
        CHECK(TrialFrontendSupported(f));Publish(f);CheckStrip(count);
        CHECK(hits[1].a.y>hits[0].a.y);
        for(unsigned i=0;i<count;++i){Click(i);auto a=Take();CHECK(a.kind==UC_HF_ACTIVATE&&a.index==i);}
        RightClick();auto a=Take();CHECK(a.kind==UC_HF_BACK);
        CheckVisualHandoff(f);
        keyboard_mode=0;Publish(f);CHECK(hits.empty()&&!frontend_presentation.visible);
        keyboard_mode=1;f.source.rows[0].callback=0;CHECK(!TrialFrontendSupported(f));
    }

    for(unsigned kind:{4u,5u,6u}){
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
    CHECK(input_changes>10&&applied==before_ui+39&&frames>100);
    CheckPerPlayerSettings();
    TrialFrontendPublish({});TrialGraphicsMenuContext(0,0,0,0);DrawFrame();
    movie_progress=.5f;DrawFrame();
    CHECK(ImGui::FindWindowByName("Hold to skip movie")->Active);
    movie_progress=0;DrawFrame();CHECK(!ImGui::FindWindowByName("Hold to skip movie")->Active);
    CheckHealthPackHud();
    CheckReviveCountdown();
    CheckRevivePrompts();
    CheckCoopDivider();
    CheckCoopSelector();
    CheckCoopCampaignHeader();
    trial_language::active=4;
    CHECK(std::strcmp(trial_language::Ui("Controls"),"Controles")==0);
    CHECK(std::strcmp(trial_language::Ui("Next Firearm"),"Siguiente arma de fuego")==0);
    CHECK(std::strcmp(trial_language::Ui("Previous Firearm"),"Arma de fuego anterior")==0);
    CHECK(std::strcmp(trial_language::Ui("Equip or select the next available firearm."),"Equipa o selecciona la siguiente arma de fuego disponible.")==0);
    CHECK(std::strcmp(trial_language::Ui("Equip or select the previous available firearm."),"Equipa o selecciona el arma de fuego anterior disponible.")==0);
    CHECK(std::strcmp(trial_language::Ui("FMV scaling"),"Escala de v\xc3\xad" "deos")==0);
    CHECK(std::strcmp(trial_language::Ui("Fill"),"Llenar")==0);
    CHECK(std::strcmp(trial_language::Ui("Nick Mason"),"Nick Mason")==0);
    for(const auto size:{app::Mode{640,480},app::Mode{1920,1080},app::Mode{2560,1440}}){
        output=size;auto page=MakeList(3);Publish(page);
        Click(page.source.count);CHECK(PanelOpen());
        CHECK(hits.size()==23);
        for(const auto& hit:hits)CHECK(hit.a.x>=0&&hit.b.x<=float(output.width)+1);
        RightClick();CHECK(!PanelOpen());EmptyAction();
    }
    trial_language::active=0;
    CHECK(std::strcmp(trial_language::Ui("Controls"),"Controls")==0);
    ImGui::DestroyContext();std::puts("PASS actual menu UI + original fonts offline; no live scene or visual parity claim");
}

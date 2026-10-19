#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>
#include "imgui.h"
#include "xbe_graphics_settings.h"
#include "xbe_trial_graphics_ui.h"
#include "xbe_frontend.h"
#include "xbe_frontend_fonts.h"
#include "xbe_pc_prompts.h"
#include "xbe_controls.h"
#include "xbe_language.h"
#include "xbe_controls_capture.h"
#include "third_party/xboxrecomp/include/host_controller.h"

bool TrialSmoothFog = false;
bool TrialDistanceFog = true;
uint32_t TrialFogDistanceMode = 0;

namespace {
struct Menu { uint32_t manager=0, screen=0, owner=0, kind=0; };
std::mutex context_lock;
Menu context, opened;
Menu quit_origin;
uint64_t quit_epoch=0;
std::atomic<bool> quit_open=false;
bool quit_submitted=false;
unsigned quit_selection=1;
TrialFrontendFrame frontend;
TrialFrontendPresentation frontend_presentation;
UcHybridFrontendAction frontend_action{};
TrialFrontendNameActionData frontend_name_action{};
std::atomic<bool> frontend_active=false;
std::atomic<unsigned> frontend_events=0;
std::atomic<uint64_t> navigation_generation=1;
// ImGui may spread rapid taps over several frames. Confirm belongs to the
// native page that received its window event, never a later page's key queue.
std::atomic<uint64_t> native_confirm_generation=0;
std::atomic<bool> frontend_text_entry=false,frontend_transition=false;
bool TakeNativeConfirm(bool ready){
    const auto received=native_confirm_generation.exchange(0);
    return ready&&received&&received==navigation_generation.load();
}
std::atomic<bool> panel_open=false, open_requested=false;
std::atomic<unsigned> settings_page=0; // 0 Graphics, 1 Controls, 2 Language, all under Options.
void ControlsReadDraft();
void LanguageReadDraft();
void ControlsCancelCapture();
TrialFrontendFrame controls_origin;
unsigned controls_origin_row=0;
trial_graphics::Draft draft;
bool draft_fps=true, confirm_back=false;
uint32_t draft_fog_mode=1;
std::string message;
ImFont *body_font=nullptr, *title_font=nullptr;
std::vector<app::Mode> modes;

bool SameMenu(const Menu& a,const Menu& b) {
    return a.kind && a.manager==b.manager && a.screen==b.screen && a.owner==b.owner && a.kind==b.kind;
}
bool PanelOpen(){return panel_open.load()||open_requested.load();}
uint32_t FogMode(){
    return !TrialDistanceFog?0:TrialFogDistanceMode==1?3:TrialSmoothFog?2:1;
}
void ReadDraft() {
    draft=trial_graphics::Snapshot();draft_fps=TrialShowFps;draft_fog_mode=FogMode();
}
bool Dirty() {
    auto live=trial_graphics::Snapshot();const auto&a=draft.settings;const auto&b=live.settings;
    return a.width!=b.width||a.height!=b.height||a.display_mode!=b.display_mode||
        a.render_percent!=b.render_percent||a.vsync!=b.vsync||a.fps_cap!=b.fps_cap||a.aniso!=b.aniso||
        draft.fov!=live.fov||draft.weapon_fov!=live.weapon_fov||draft.aspect!=live.aspect||draft.msaa!=live.msaa||draft.scenery!=live.scenery||
        draft.characters!=live.characters||draft.character_quality!=live.character_quality||
        draft.ragdolls!=live.ragdolls||draft.hd_ui!=live.hd_ui||draft.reticle_scale!=live.reticle_scale||draft_fps!=TrialShowFps||draft_fog_mode!=FogMode();
}
void Close() {
    ControlsCancelCapture();
    panel_open=false;open_requested=false;confirm_back=false;message.clear();
    frontend_events=0;++navigation_generation;TrialGraphicsInputChanged();
}
bool Apply() {
    bool old_fps=TrialShowFps,old_fog=TrialSmoothFog,old_distance_fog=TrialDistanceFog;
    const uint32_t old_distance_mode=TrialFogDistanceMode;
    TrialShowFps=draft_fps;
    // Keep the three existing config keys compatible. Inactive legacy flags
    // survive an unrelated Apply; selecting a different mode sets them together.
    if(draft_fog_mode!=FogMode()){
        TrialDistanceFog=draft_fog_mode!=0;
        TrialFogDistanceMode=draft_fog_mode==3?1:0;
        TrialSmoothFog=draft_fog_mode==2;
    }
    if(!trial_graphics::Apply(draft,message)){
        TrialShowFps=old_fps;TrialSmoothFog=old_fog;TrialDistanceFog=old_distance_fog;
        TrialFogDistanceMode=old_distance_mode;return false;
    }
    ReadDraft();return true;
}
void EnumerateModes() {
    modes={{0,0}};
    auto add=[](app::Mode m){
        if(m.width<640||m.height<480)return;
        for(const auto& existing:modes)if(existing.width==m.width&&existing.height==m.height)return;
        modes.push_back(m);
    };
    // Windowed/borderless output sizes are useful even without an exclusive monitor mode.
    for(auto mode:{app::Mode{1280,720},{1600,900},{1920,1080},{2560,1440},{3840,2160}})add(mode);
    MONITORINFOEXW monitor{};monitor.cbSize=sizeof(monitor);
    if(GetMonitorInfoW(MonitorFromWindow(static_cast<HWND>(app::Window()),MONITOR_DEFAULTTOPRIMARY),&monitor)) {
        DEVMODEW mode{};mode.dmSize=sizeof(mode);
        for(DWORD i=0;EnumDisplaySettingsW(monitor.szDevice,i,&mode);++i)add({mode.dmPelsWidth,mode.dmPelsHeight});
    }
    add(app::Output());
    std::sort(modes.begin()+1,modes.end(),[](auto a,auto b){return a.width!=b.width?a.width<b.width:a.height<b.height;});
}
constexpr ImU32 white=IM_COL32(239,241,239,255),muted=IM_COL32(139,144,141,255);
constexpr ImU32 disabled=IM_COL32(80,84,81,255),frame_color=IM_COL32(118,126,121,255);

// Match the original 640x480 menu bands. Text keeps its proportions as the
// framed panel expands to widescreen; input uses exactly the drawn rectangles.
struct MenuCanvas {
    const ImGuiViewport* view;
    ImDrawList* draw;
    float sx,sy;
    explicit MenuCanvas(const ImGuiViewport* v):view(v),draw(ImGui::GetWindowDrawList()),sx(v->Size.x/640.f),sy(v->Size.y/480.f){}
    ImVec2 Point(float x,float y)const{return {view->Pos.x+x*sx,view->Pos.y+y*sy};}
    ImFont* Font(bool title=false)const{return TrialFrontendFontForUi(title&&title_font?title_font:body_font?body_font:ImGui::GetFont());}
    float Width(const char* text,float height,bool title=false)const{
        return Font(title)->CalcTextSizeA(height*sy,100000,0,text).x/sx;
    }
    void Rect(float x,float y,float w,float h,ImU32 color){draw->AddRectFilled(Point(x,y),Point(x+w,y+h),color);}
    void Line(float x,float y,float w,ImU32 color=frame_color){draw->AddLine(Point(x,y),Point(x+w,y),color,std::max(1.f,sy));}
    void Text(const char* text,float x,float y,float height,ImU32 color=white,float max_width=0,bool title=false){
        if(max_width>0)height*=std::min(1.f,max_width/std::max(1.f,Width(text,height,title)));
        const auto p=Point(x,y);const float px=height*sy;
        draw->AddText(Font(title),px,{p.x+sy,p.y+sy},IM_COL32(0,0,0,210),text);
        draw->AddText(Font(title),px,p,color,text);
    }
    void Center(const char* text,float x,float y,float width,float height,ImU32 color=white,bool title=false){
        height*=std::min(1.f,width/std::max(1.f,Width(text,height,title)));
        Text(text,x+(width-Width(text,height,title))*.5f,y,height,color,0,title);
    }
    void Selection(float x,float y,float width,float height){
        // A faint, fading selection bar, never a boxed desktop button.
        const ImU32 lit=IM_COL32(159,173,162,57),clear=IM_COL32(159,173,162,0);
        draw->AddRectFilledMultiColor(Point(x,y),Point(x+width,y+height),lit,clear,clear,lit);
        draw->AddRectFilledMultiColor(Point(x,y),Point(x+width,y+1),IM_COL32(187,199,189,88),clear,clear,IM_COL32(187,199,189,88));
    }
    bool Hit(const char* id,float x,float y,float width,float height){
        ImGui::SetCursorScreenPos(Point(x,y));
        return ImGui::InvisibleButton(id,{width*sx,height*sy});
    }
    void Arrow(float x,float y,bool right,ImU32 color){
        const float direction=right?1.f:-1.f;
        draw->AddTriangleFilled(Point(x+direction*3.5f,y),Point(x-direction*3.5f,y-5),Point(x-direction*3.5f,y+5),color);
    }
    void Icon(unsigned code,float x,float y,float size){
        float uv[4];
        if(TrialMenuIconUV(code,uv))draw->AddImage(ImGui::GetIO().Fonts->TexID,Point(x,y),Point(x+size*sy/sx,y+size),{uv[0],uv[1]},{uv[2],uv[3]});
    }
    void Footer(float top=382){
        Rect(0,top,640,480-top,IM_COL32(0,0,0,255));Line(0,top,640);
    }
    void SettingsFrame(bool paused,const char* title="GRAPHICS"){
        Rect(0,0,640,70,IM_COL32(0,0,0,255));
        Rect(3,73,634,306,IM_COL32(0,0,0,paused?112:168));
        Rect(0,70,640,3,frame_color);Rect(0,379,640,3,frame_color);
        Rect(0,73,3,306,frame_color);Rect(637,73,3,306,frame_color);
        Footer();Text(title,50,35,25,white,540,true);
    }
};

struct MenuHint {unsigned key;const char* label;};
void FooterPrompts(MenuCanvas& c,std::initializer_list<MenuHint> hints){
    float width=0;for(const auto& h:hints)width+=24+c.Width(h.label,11)+15;
    float x=(640-width+15)*.5f;
    for(const auto& h:hints){c.Icon(h.key,x,448,21);c.Text(h.label,x+24,453,11,muted);x+=24+c.Width(h.label,11)+15;}
}

void BeginMenu(const char* name,const ImGuiViewport* view){
    ImGui::SetNextWindowPos(view->Pos);ImGui::SetNextWindowSize(view->Size);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(0,0));
    ImGui::Begin(name,nullptr,ImGuiWindowFlags_NoDecoration|ImGuiWindowFlags_NoMove|
        ImGuiWindowFlags_NoSavedSettings|ImGuiWindowFlags_NoBackground|ImGuiWindowFlags_NoNav);
}
void EndMenu(){ImGui::End();ImGui::PopStyleVar();}
bool PointerMoved(){return ImGui::GetIO().MouseDelta.x!=0||ImGui::GetIO().MouseDelta.y!=0;}
bool UiHeld(){return ImGui::IsMouseDown(0)||ImGui::IsMouseDown(1)||ImGui::IsKeyDown(ImGuiKey_Enter)||
    ImGui::IsKeyDown(ImGuiKey_Space)||ImGui::IsKeyDown(ImGuiKey_Q)||ImGui::IsKeyDown(ImGuiKey_Escape)||ImGui::IsKeyDown(ImGuiKey_Delete);}

// Labels determine each hit region's width, just like the original horizontal
// footer. Scale the whole strip together so long localized labels still fit.
template<class Hover,class Activate>
void RowStrip(MenuCanvas& c,const char*const* labels,const bool* enabled,unsigned count,
              float top,unsigned selected,Hover hover,Activate activate){
    float total=0;constexpr float gap=18;
    for(unsigned i=0;i<count;++i)total+=c.Width(labels[i],16)+gap;
    const float fit=std::min(1.f,604.f/std::max(1.f,total)),height=16*fit;
    float x=(640-total*fit)*.5f;
    for(unsigned i=0;i<count;++i){
        const float width=(c.Width(labels[i],16)+gap)*fit;
        ImGui::PushID(int(i));ImGui::BeginDisabled(enabled&&!enabled[i]);
        const bool clicked=c.Hit("row",x,top,width,27);
        const bool hot=ImGui::IsItemHovered();
        if(hot&&PointerMoved())hover(i);
        if(clicked)activate(i);
        const bool chosen=selected==i||hot;
        if(chosen&&(!enabled||enabled[i]))c.Selection(x+2,top+2,width-4,23);
        c.Center(labels[i],x+gap*fit*.5f,top+5,width-gap*fit,height,
                 enabled&&!enabled[i]?disabled:chosen?white:muted);
        ImGui::EndDisabled();ImGui::PopID();
        x+=width;
        if(i+1<count)c.Rect(x-.5f,top+6,1,15*fit,muted);
    }
}
void FrontendAction(const TrialFrontendFrame& f,unsigned action,unsigned index=0){
    std::lock_guard lock(context_lock);
    const auto& s=f.source;
    if((!frontend_active&&!PanelOpen())||frontend.source.epoch!=s.epoch||frontend.source.owner!=s.owner)return;
    if(frontend_action.kind&&frontend_action.kind!=UC_HF_SELECT)return;
    frontend_action={s.epoch,s.screen,s.owner,index<s.count?s.rows[index].row:0,index,action};
    if(action!=UC_HF_SELECT)native_confirm_generation=0;
}
bool IsMainMenu(const TrialFrontendFrame& f){
    return f.source.kind==UC_HF_LIST&&f.list_kind==1;
}
bool QuitMatches(const TrialFrontendFrame& f){
    return IsMainMenu(f)&&f.source.epoch==quit_epoch&&f.source.owner==quit_origin.owner&&
        f.source.screen==quit_origin.screen;
}
void OpenQuit(const TrialFrontendFrame& f){
    std::lock_guard lock(context_lock);
    if(!TrialFrontendSupported(frontend)||!IsMainMenu(f)||frontend.source.epoch!=f.source.epoch||
       frontend.source.owner!=f.source.owner||context.kind!=1||
       context.owner!=f.source.owner||context.screen!=f.source.screen)return;
    quit_origin=context;quit_epoch=f.source.epoch;quit_open=true;quit_submitted=false;quit_selection=1;
    frontend_action={};frontend_events=0;++navigation_generation;
}
void CloseQuit(){
    quit_open=false;quit_submitted=false;frontend_events=0;++navigation_generation;
    TrialGraphicsInputChanged();
}
void QuitUi(const ImGuiViewport* view){
    static TrialFrontendReleaseGate gate;
    const bool ready=!quit_submitted&&gate.Accept(navigation_generation.load(),UiHeld());
    const unsigned events=frontend_events.exchange(0);
    const bool confirm=TakeNativeConfirm(ready);
    BeginMenu("Quit to desktop",view);MenuCanvas canvas(view);canvas.SettingsFrame(false,"QUIT TO DESKTOP");
    canvas.Center("Are you sure you want to quit?",40,165,560,24,white,true);
    ImGui::BeginDisabled(!ready);
    auto activate=[&](unsigned choice){
        if(!ready)return;
        if(choice){CloseQuit();return;}
        // Use the same window-thread shutdown as closing the game window:
        // WM_CLOSE -> WM_DESTROY -> the app loop's kernel::Shutdown.
        {std::lock_guard lock(context_lock);
            if(!TrialFrontendSupported(frontend)||!QuitMatches(frontend)||!SameMenu(context,quit_origin))return;}
        if(PostMessageW(static_cast<HWND>(app::Window()),WM_CLOSE,0,0))quit_submitted=true;
    };
    if(ready){
        if((events&3)||ImGui::IsKeyPressed(ImGuiKey_LeftArrow)||ImGui::IsKeyPressed(ImGuiKey_RightArrow)||
           ImGui::IsKeyPressed(ImGuiKey_UpArrow)||ImGui::IsKeyPressed(ImGuiKey_DownArrow)||ImGui::IsKeyPressed(ImGuiKey_Tab))
            quit_selection=1-quit_selection;
        const bool back=(events&8)||ImGui::IsMouseClicked(1,false)||ImGui::IsKeyPressed(ImGuiKey_Q,false)||ImGui::IsKeyPressed(ImGuiKey_Escape,false);
        if(back)CloseQuit();
        else if((events&4)||confirm)activate(quit_selection);
    }
    const char* labels[]={"Yes","No"};
    RowStrip(canvas,labels,nullptr,2,257,quit_selection,[&](unsigned i){quit_selection=i;},
        [&](unsigned i){if(quit_open&&!quit_submitted)activate(i);});
    if(xbox_KeyboardMode())FooterPrompts(canvas,{{13,"Select"},{VK_RBUTTON,"Back"}});
    else FooterPrompts(canvas,{{0x103,"Navigate"},{0x101,"Select"},{0x102,"Back"}});
    ImGui::EndDisabled();EndMenu();
}
void ConfirmationUi(const TrialFrontendFrame& f,const ImGuiViewport* view,bool interactive=true){
    static uint64_t epoch=0;static unsigned selection=1;static TrialFrontendReleaseGate gate;
    if(epoch!=f.source.epoch){epoch=f.source.epoch;selection=1;}
    const bool ready=interactive&&gate.Accept(navigation_generation.load(),UiHeld());
    const unsigned events=interactive?frontend_events.exchange(0):0;
    const bool confirm=TakeNativeConfirm(ready);
    BeginMenu("Original confirmation",view);MenuCanvas c(view);
    c.SettingsFrame(f.pause_dialog,f.pause_dialog?"QUIT MISSION":"DELETE PROFILE");
    if(f.pause_dialog)c.Text(f.dialog_message,56,115,16,white,528);
    else{
        c.Rect(3,73,634,306,IM_COL32(0,0,0,255));
        c.Center("Delete this saved profile?",40,139,560,24,white,true);
        c.Center(f.delete_name,40,190,560,22,IM_COL32(237,212,166,255));
        c.Center("This cannot be undone.",40,224,560,13,muted);
    }
    ImGui::BeginDisabled(!ready);
    auto activate=[&](unsigned i){if(ready&&f.source.rows[i].enabled)FrontendAction(f,UC_HF_ACTIVATE,i);};
    if(ready){
        if((events&3)||ImGui::IsKeyPressed(ImGuiKey_LeftArrow)||ImGui::IsKeyPressed(ImGuiKey_RightArrow)||
           ImGui::IsKeyPressed(ImGuiKey_UpArrow)||ImGui::IsKeyPressed(ImGuiKey_DownArrow)||ImGui::IsKeyPressed(ImGuiKey_Tab))selection=1-selection;
        const bool back=(events&8)||ImGui::IsMouseClicked(1,false)||ImGui::IsKeyPressed(ImGuiKey_Q,false)||ImGui::IsKeyPressed(ImGuiKey_Escape,false);
        if(back)FrontendAction(f,UC_HF_BACK);
        else if((events&4)||confirm)activate(selection);
    }
    const char* labels[]={f.labels[0][0]?f.labels[0]:"Yes",f.labels[1][0]?f.labels[1]:"No"};
    const bool enabled[]={f.source.rows[0].enabled!=0,f.source.rows[1].enabled!=0};
    if(f.pause_dialog){
        // Match the pause list's full-width mouse targets; the guest still owns both choices.
        for(unsigned i=0;i<2;++i){
            const float y=190.f+30.f*i;
            ImGui::PushID(int(i));ImGui::BeginDisabled(!enabled[i]);
            if(c.Hit("row",48,y-2,384,30))activate(i);
            const bool hot=ImGui::IsItemHovered();
            if(hot&&PointerMoved())selection=i;
            if(enabled[i]&&(selection==i||hot))c.Selection(48,y,384,11);
            c.Text(labels[i],56,y+2,14,!enabled[i]?disabled:selection==i||hot?white:muted,360);
            ImGui::EndDisabled();ImGui::PopID();
        }
    }else RowStrip(c,labels,enabled,2,275,selection,[&](unsigned i){selection=i;},activate);
    if(xbox_KeyboardMode())FooterPrompts(c,{{13,"Select"},{VK_RBUTTON,"Cancel"}});
    else FooterPrompts(c,{{0x103,"Navigate"},{0x101,"Select"},{0x102,"Back"}});
    ImGui::EndDisabled();EndMenu();
}
void FrontendUi(const TrialFrontendFrame& f,const ImGuiViewport* view,bool interactive=true){
    const auto& src=f.source;
    const bool levels=src.kind==UC_HF_LEVELS;
    const bool profiles=src.kind==UC_HF_PROFILES;
    const bool rewards=src.kind==UC_HF_REWARD_BROWSER;
    // Graphics belongs to Options in both the front end and pause flow. Keep
    // the original main/pause rows instead of adding a second shortcut there.
    const bool graphics=src.kind==UC_HF_LIST&&f.list_kind==3;
    const bool main=IsMainMenu(f);
    const unsigned count=src.count+(profiles?3:graphics?2:main?1:0);
    static uint64_t last_epoch=0;static unsigned focus=0,profile_slot=0;static TrialFrontendReleaseGate gate;
    if(last_epoch!=src.epoch){last_epoch=src.epoch;focus=profile_slot=src.selected;}
    const bool ready=interactive&&gate.Accept(navigation_generation.load(),UiHeld());
    focus=std::min(focus,count-1);
    auto choose=[&](unsigned i){if(!ready||quit_open)return;focus=i;if(i<src.count&&src.rows[i].enabled){profile_slot=i;FrontendAction(f,UC_HF_SELECT,i);}};
    auto remove_profile=[&](){if(profiles&&(f.profile_delete_mask&(1u<<profile_slot)))FrontendAction(f,TRIAL_HF_DELETE,profile_slot);};
    auto activate=[&](unsigned i){
        if(!ready||quit_open||(i<src.count&&!src.rows[i].enabled))return;
        if(profiles&&i>=src.count){
            if(i==src.count+1)remove_profile();
            else if(i==src.count+2)FrontendAction(f,UC_HF_BACK);
            else if(profile_slot<src.count&&src.rows[profile_slot].enabled)FrontendAction(f,UC_HF_ACTIVATE,profile_slot);
            return;
        }
        if(main&&i==src.count){OpenQuit(f);return;}
        const bool controls=graphics&&i<src.count&&
            !std::strcmp(src.rows[i].label,"NewFrontEnd-ControllerOptions-1");
        if(graphics&&(i>=src.count||controls)){
            std::lock_guard lock(context_lock);
            if(frontend_active&&frontend.source.epoch==src.epoch&&context.owner==src.owner&&
               context.screen==src.screen&&context.kind==3){
                opened=context;settings_page=controls?1:i==src.count+1?2:0;open_requested=true;
                if(controls){controls_origin=f;controls_origin_row=i;}
            }
        }else FrontendAction(f,UC_HF_ACTIVATE,i);
    };
    const unsigned events=interactive?frontend_events.exchange(0):0;
    const bool confirm=TakeNativeConfirm(ready);
    if(ready){
        if((events&1)||ImGui::IsKeyPressed(ImGuiKey_LeftArrow)||ImGui::IsKeyPressed(ImGuiKey_UpArrow))choose((focus+count-1)%count);
        if((events&2)||ImGui::IsKeyPressed(ImGuiKey_RightArrow)||ImGui::IsKeyPressed(ImGuiKey_DownArrow)||ImGui::IsKeyPressed(ImGuiKey_Tab))choose((focus+1)%count);
        if((events&4)||confirm)activate(focus);
        if(!quit_open&&((events&8)||ImGui::IsMouseClicked(1,false)||ImGui::IsKeyPressed(ImGuiKey_Q,false)||ImGui::IsKeyPressed(ImGuiKey_Escape,false)))FrontendAction(f,UC_HF_BACK);
        if(levels&&((events&128)||ImGui::IsKeyPressed(ImGuiKey_D,false)))FrontendAction(f,UC_HF_DIFFICULTY);
        if(levels&&((events&64)||ImGui::IsKeyPressed(ImGuiKey_R,false)))FrontendAction(f,UC_HF_REWARDS);
        if(profiles&&ImGui::IsKeyPressed(ImGuiKey_Delete,false))remove_profile();
    }
    BeginMenu("Game menu rows",view);MenuCanvas canvas(view);
    // Keep the original title and moving panel, covering only its old footer.
    const float top=levels||profiles||rewards?382.f:std::clamp(f.row_top-5.f,379.f,400.f);
    canvas.Footer(top);
    ImGui::BeginDisabled(!ready||quit_open);
    if(profiles){
        for(unsigned i=0;i<src.count;++i){
            const auto& r=f.profile_bounds[i];ImGui::PushID(int(i));
            ImGui::BeginDisabled(!src.rows[i].enabled);
            if(canvas.Hit("profile",r.x,r.y,r.w,r.h))activate(i);
            if(ImGui::IsItemHovered()&&PointerMoved())choose(i);
            ImGui::EndDisabled();ImGui::PopID();
        }
        const char* labels[]={"Select","Delete profile","Back"};
        const bool enabled[]={src.rows[profile_slot].enabled!=0,(f.profile_delete_mask&(1u<<profile_slot))!=0,true};
        RowStrip(canvas,labels,enabled,3,398,focus>=src.count?focus-src.count:3,
            [&](unsigned i){choose(src.count+i);},[&](unsigned i){activate(src.count+i);});
    }else if(rewards){
        const unsigned page=src.selected/TrialRewardsPerPage,pages=(src.count+TrialRewardsPerPage-1)/TrialRewardsPerPage;
        for(unsigned slot=0;slot<TrialRewardsPerPage&&page*TrialRewardsPerPage+slot<src.count;++slot){
            const unsigned index=page*TrialRewardsPerPage+slot;const auto r=TrialLevelBounds(slot,TrialRewardsPerPage);
            ImGui::PushID(int(index));
            if(canvas.Hit("reward",r.x,r.y,r.w,r.h))activate(index);
            if(ImGui::IsItemHovered()&&PointerMoved())choose(index);
            ImGui::PopID();
        }
        const char* labels[]={"Previous","Back","Next"};
        const bool enabled[]={page>0,true,page+1<pages};
        RowStrip(canvas,labels,enabled,3,398,3,[](unsigned){},[&](unsigned i){
            if(i==1)FrontendAction(f,UC_HF_BACK);
            else choose((i==0?page-1:page+1)*TrialRewardsPerPage);
        });
        canvas.Center(f.labels[src.selected],50,432,540,12,muted);
        if(ready&&ImGui::GetIO().MouseWheel!=0){
            if(ImGui::GetIO().MouseWheel<0&&page+1<pages)choose((page+1)*TrialRewardsPerPage);
            else if(ImGui::GetIO().MouseWheel>0&&page)choose((page-1)*TrialRewardsPerPage);
        }
    }else if(levels){
        for(unsigned i=0;i<src.count;++i){
            auto r=TrialLevelBounds(i,src.count);ImGui::PushID(int(i));
            if(canvas.Hit("level",r.x,r.y,r.w,r.h))activate(i);
            if(ImGui::IsItemHovered()&&PointerMoved())choose(i);
            ImGui::PopID();
        }
        const char* labels[]={"Back","Difficulty","Rewards","Deploy"};
        const bool enabled[]={true,true,(src.level_actions&2)!=0,true};
        const unsigned actions[]={UC_HF_BACK,UC_HF_DIFFICULTY,UC_HF_REWARDS,UC_HF_ACTIVATE};
        RowStrip(canvas,labels,enabled,4,398,4,[](unsigned){},[&](unsigned i){FrontendAction(f,actions[i],src.selected);});
        canvas.Center(f.labels[src.selected],40,432,560,12,muted);
    }else{
        const char* labels[22]{};bool enabled[22]{};
        for(unsigned i=0;i<count;++i){
            labels[i]=i>=src.count?(main?"Quit to desktop":i==src.count?"Graphics":"Language"):
                graphics&&!std::strcmp(src.rows[i].label,"NewFrontEnd-ControllerOptions-1")?"Controls":f.labels[i];
            enabled[i]=i>=src.count||src.rows[i].enabled;
        }
        if(xbox_KeyboardMode()){
            const char* title=main?"MAIN MENU":f.list_kind==2?"PAUSED":"OPTIONS";
            canvas.SettingsFrame((src.source_kind&0x80000000u)!=0,title);
            // Match the old mouse frontend's roomy vertical rows and fading highlight.
            const float step=std::min(26.f,264.f/std::max(1u,count));
            for(unsigned i=0;i<count;++i){
                const float y=103.f+step*i;
                ImGui::PushID(int(i));ImGui::BeginDisabled(!enabled[i]);
                if(canvas.Hit("row",48,y-2,384,step))activate(i);
                const bool hot=ImGui::IsItemHovered();
                if(hot&&PointerMoved())choose(i);
                if(enabled[i]&&(focus==i||hot))canvas.Selection(48,y,384,11);
                canvas.Text(labels[i],56,y+2,14,!enabled[i]?disabled:focus==i||hot?white:muted,360);
                ImGui::EndDisabled();ImGui::PopID();
            }
        }else RowStrip(canvas,labels,enabled,count,top+9,focus,choose,activate);
    }
    if(xbox_KeyboardMode()){
        if(levels)FooterPrompts(canvas,{{1,"Deploy"},{'D',"Difficulty"},{'R',"Rewards"},{VK_RBUTTON,"Back"}});
        else FooterPrompts(canvas,{{1,"Select"},{13,"Select"},{VK_RBUTTON,"Back"}});
    }else if(levels)FooterPrompts(canvas,{{0x101,"Deploy"},{0x105,"Difficulty"},{0x104,"Rewards"},{0x102,"Back"}});
    else FooterPrompts(canvas,{{0x103,"Navigate"},{0x101,"Select"},{0x102,"Back"}});
    ImGui::EndDisabled();EndMenu();
}

int NameCharacter(ImGuiInputTextCallbackData* data){
    const unsigned c=data->EventChar;
    return !((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c==' ');
}
void NameUi(const TrialFrontendFrame& f,const ImGuiViewport* view,bool interactive=true){
    static uint64_t epoch=0,generation=0;static uint32_t owner=0;static char text[16]{};
    static bool focus_text=true;static TrialFrontendReleaseGate gate;
    if(epoch!=f.source.epoch||owner!=f.source.owner||generation!=navigation_generation.load()){
        epoch=f.source.epoch;owner=f.source.owner;generation=navigation_generation.load();
        std::memcpy(text,f.name,sizeof(text));text[15]=0;focus_text=true;
    }
    // Release the key/click that opened this page before accepting characters;
    // once acquired, Q and Space are ordinary name text, never Back shortcuts.
    const bool ready=interactive&&gate.Accept(navigation_generation.load(),UiHeld());
    if(!ready)TakeNativeConfirm(false);
    BeginMenu("Create profile",view);MenuCanvas c(view);c.SettingsFrame(false,"CREATE PROFILE");
    // The original on-screen keyboard remains authoritative for controllers.
    // On PC cover its keys; only the submitted text reaches its validator.
    c.Rect(3,73,634,306,IM_COL32(0,0,0,255));
    c.Text("PROFILE NAME",80,133,15,IM_COL32(237,212,166,255),480);
    c.Selection(76,176,488,40);c.Line(80,217,480);
    c.Text("Up to 15 letters, numbers or spaces.",80,236,12,muted,480);
    c.Text(f.name_error,80,277,13,IM_COL32(237,212,166,255),480);
    ImGui::BeginDisabled(!ready);
    auto submit=[&](unsigned kind){TrialFrontendNameAction(f,kind,text);if(kind!=UC_HF_SELECT)focus_text=true;};
    ImGui::PushFont(c.Font());ImGui::SetWindowFontScale(16*c.sy/c.Font()->FontSize);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,ImVec2(4*c.sy,6*c.sy));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize,0);ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,0);
    ImGui::PushStyleColor(ImGuiCol_FrameBg,ImVec4(0,0,0,0));ImGui::PushStyleColor(ImGuiCol_Text,ImVec4(.94f,.95f,.94f,1));
    ImGui::SetCursorScreenPos(c.Point(80,180));ImGui::SetNextItemWidth(480*c.sx);
    if(ready&&focus_text){ImGui::SetKeyboardFocusHere();focus_text=false;}
    char previous[16];std::memcpy(previous,text,sizeof(previous));
    const bool entered=ImGui::InputText("##profile name",text,sizeof(text),
        ImGuiInputTextFlags_EnterReturnsTrue|ImGuiInputTextFlags_CallbackCharFilter,NameCharacter);
    ImGui::PopStyleColor(2);ImGui::PopStyleVar(3);ImGui::SetWindowFontScale(1);ImGui::PopFont();
    // Old-page Enter events can still reach ImGui after its queued key-up;
    // only an Enter received by this name page may commit the text.
    if(entered){
        if(TakeNativeConfirm(ready))submit(UC_HF_ACTIVATE);
        else focus_text=true;
    }
    else if(std::memcmp(previous,text,sizeof(text)))submit(UC_HF_SELECT);
    const char* labels[]={"Create profile","Cancel"};
    RowStrip(c,labels,nullptr,2,395,2,[](unsigned){},[&](unsigned i){submit(i?UC_HF_BACK:UC_HF_ACTIVATE);});
    FooterPrompts(c,{{13,"Create profile"},{VK_RBUTTON,"Cancel"}});
    if(ready&&(ImGui::IsMouseClicked(1,false)||ImGui::IsKeyPressed(ImGuiKey_Escape,false)))submit(UC_HF_BACK);
    if(interactive)frontend_events=0;ImGui::EndDisabled();EndMenu();
}

void RequestBack(){
    if(confirm_back){confirm_back=false;++navigation_generation;frontend_events=0;}
    else if(Dirty()){confirm_back=true;++navigation_generation;frontend_events=0;}
    else Close();
}

int SettingRow(MenuCanvas& c,const char* label,const char* value,unsigned index,unsigned& focus,int key_delta,
               float step=18,float top=90){
    const float y=top+index*step;
    ImGui::PushID(int(index));
    const bool clicked=c.Hit("setting",50,y,540,std::min(18.f,step));
    const bool hot=ImGui::IsItemHovered();
    if(hot&&PointerMoved())focus=index;
    int delta=focus==index?key_delta:0;
    if(clicked){
        focus=index;
        const float x=(ImGui::GetIO().MousePos.x-c.view->Pos.x)/c.sx;
        // Clicking the label selects its row; the value or either arrow changes it.
        if(x>=376)delta=x<408?-1:1;
    }
    const bool selected=focus==index;
    if(selected)c.Selection(50,y,540,std::min(18.f,step));
    c.Text(label,61,y+2,13,selected?white:muted,304);
    c.Arrow(391,y+9,false,selected||hot?white:muted);
    c.Text(value,418,y+2,13,selected?white:muted,124);
    c.Arrow(555,y+9,true,selected||hot?white:muted);
    if(delta)message.clear();
    ImGui::PopID();return delta;
}

void OriginalSettingsUi(const TrialFrontendFrame& f,const ImGuiViewport* view,bool interactive=true){
    const auto& src=f.source;
    static uint32_t owner=0;static unsigned focus=0;static TrialFrontendReleaseGate gate;
    if(owner!=src.owner){owner=src.owner;focus=src.selected;}
    focus=std::min(focus,src.count);
    const bool ready=interactive&&gate.Accept(navigation_generation.load(),UiHeld());
    const unsigned events=interactive?frontend_events.exchange(0):0;
    const bool confirm=TakeNativeConfirm(ready);
    if(ready){
        if(ImGui::IsKeyPressed(ImGuiKey_UpArrow))focus=(focus+src.count)%(src.count+1);
        if(ImGui::IsKeyPressed(ImGuiKey_DownArrow)||ImGui::IsKeyPressed(ImGuiKey_Tab))focus=(focus+1)%(src.count+1);
        if((events&8)||ImGui::IsMouseClicked(1,false)||ImGui::IsKeyPressed(ImGuiKey_Q,false)||ImGui::IsKeyPressed(ImGuiKey_Escape,false))FrontendAction(f,UC_HF_BACK);
        if(focus==src.count&&confirm)FrontendAction(f,UC_HF_BACK);
    }
    const int direction=ready?(ImGui::IsKeyPressed(ImGuiKey_LeftArrow)?-1:
        ImGui::IsKeyPressed(ImGuiKey_RightArrow)?1:0):0;
    BeginMenu("Original settings",view);MenuCanvas canvas(view);
    canvas.SettingsFrame((src.source_kind&0x80000000u)!=0,f.list_kind==4?"AUDIO":"GENERAL");
    ImGui::BeginDisabled(!ready);
    const float step=std::min(26.f,260.f/std::max(1u,src.count));
    for(unsigned i=0;i<src.count;++i){
        ImGui::BeginDisabled(!src.rows[i].enabled);
        const int delta=SettingRow(canvas,f.labels[i],f.values[i],i,focus,direction,step,103);
        const unsigned action=delta<0?UC_HF_LEFT:UC_HF_RIGHT;
        const unsigned mask=delta<0?UC_HF_ROW_LEFT:UC_HF_ROW_RIGHT;
        if(ready&&delta&&src.rows[i].enabled&&(src.rows[i].actions&mask))FrontendAction(f,action,i);
        ImGui::EndDisabled();
    }
    const char* labels[]={"Back"};const bool enabled[]={true};
    RowStrip(canvas,labels,enabled,1,398,focus==src.count?0:1,
        [&](unsigned){focus=src.count;},[&](unsigned){if(ready)FrontendAction(f,UC_HF_BACK);});
    FooterPrompts(canvas,{{VK_LEFT,"Change"},{VK_RIGHT,"Change"},{VK_RBUTTON,"Back"}});
    ImGui::EndDisabled();EndMenu();
}

template<class T,size_t N>
void SettingChoice(MenuCanvas& c,const char* label,T& value,const T(&values)[N],const char*const(&names)[N],
                   unsigned index,unsigned& focus,int key_delta,float top=90,float step=17){
    size_t current=N;for(size_t i=0;i<N;++i)if(value==values[i])current=i;
    const int delta=SettingRow(c,label,current<N?names[current]:"Custom",index,focus,key_delta,step,top);
    if(delta)value=values[current<N?(current+N+(delta>0?1:-1))%N:delta>0?0:N-1];
}

// Sliders share the menu's row hit target and controller/key navigation.
void SettingSlider(MenuCanvas& c,const char* label,uint32_t& value,int low,int high,int increment,
                   const char* suffix,unsigned index,unsigned& focus,int key_delta,float top){
    char text[32];std::snprintf(text,sizeof(text),"%u%s",value,suffix);
    const int delta=SettingRow(c,label,text,index,focus,key_delta,16,top);
    const float grab_x=(ImGui::GetIO().MouseClickedPos[0].x-c.view->Pos.x)/c.sx;
    const bool track_grab=grab_x>=410&&grab_x<=548;
    // Releasing a track drag must not also count as the row's value click.
    const int change=track_grab&&ImGui::IsMouseReleased(0)?(focus==index?key_delta:0):delta;
    if(change)value=uint32_t(std::clamp(int(value)+(change>0?increment:-increment),low,high));
    if(ImGui::IsItemActive()&&ImGui::IsMouseDown(0)&&track_grab){
        const float x=(ImGui::GetIO().MousePos.x-c.view->Pos.x)/c.sx;
        const float raw=low+(x-418)*(high-low)/124;
        value=uint32_t(std::clamp(low+int(std::round((raw-low)/increment))*increment,low,high));
        focus=index;message.clear();
    }
    const float y=top+index*16+15;
    c.Line(418,y,124,IM_COL32(118,126,121,180));
    const float knob=418+124*(float(value)-low)/(high-low);
    c.Line(knob-2,y-1,4,white);
}

void SettingsUi(const ImGuiViewport* view,bool paused){
    BeginMenu("Burner graphics",view);MenuCanvas canvas(view);
    canvas.SettingsFrame(paused);
    static TrialFrontendReleaseGate gate;static uint64_t epoch=0;
    static unsigned focus=0,confirmation=2;
    const auto generation=navigation_generation.load();
    if(epoch!=generation){epoch=generation;confirmation=2;}
    const bool ready=gate.Accept(generation,UiHeld());
    const unsigned events=frontend_events.exchange(0);
    const bool back=ready&&((events&8)||ImGui::IsMouseClicked(1,false)||ImGui::IsKeyPressed(ImGuiKey_Escape,false)||ImGui::IsKeyPressed(ImGuiKey_Q,false));
    const bool enter=TakeNativeConfirm(ready)||(ready&&(events&4));
    ImGui::BeginDisabled(!ready);
    if(confirm_back){
        canvas.Center("Apply your changes?",40,163,560,24,white,true);
        canvas.Center("Save your graphics settings before going back.",50,204,540,14,white);
        const char* labels[]={"Apply","Discard","Keep editing"};
        auto activate=[&](unsigned i){
            if(i==0){if(Apply())Close();}
            else if(i==1)Close();
            else{confirm_back=false;++navigation_generation;frontend_events=0;}
        };
        if(ready){
            if(((events&16)||ImGui::IsKeyPressed(ImGuiKey_LeftArrow)))confirmation=(confirmation+2)%3;
            if(((events&32)||ImGui::IsKeyPressed(ImGuiKey_RightArrow))||ImGui::IsKeyPressed(ImGuiKey_Tab))confirmation=(confirmation+1)%3;
            if(enter)activate(confirmation);
        }
        RowStrip(canvas,labels,nullptr,3,257,confirmation,[&](unsigned i){confirmation=i;},activate);
    }else{
        constexpr unsigned rows=18,total=rows+3;
        constexpr float top=73; // Eighteen rows fit above the existing help divider.
        focus=std::min(focus,total-1);
        int delta=0;
        if(ready){
            if(((events&1)||ImGui::IsKeyPressed(ImGuiKey_UpArrow)))focus=(focus+total-1)%total;
            if(((events&2)||ImGui::IsKeyPressed(ImGuiKey_DownArrow))||ImGui::IsKeyPressed(ImGuiKey_Tab))focus=(focus+1)%total;
            if(focus<rows){
                if(((events&16)||ImGui::IsKeyPressed(ImGuiKey_LeftArrow)))delta=-1;
                if(((events&32)||ImGui::IsKeyPressed(ImGuiKey_RightArrow))||enter)delta=1;
            }else{
                if(((events&16)||ImGui::IsKeyPressed(ImGuiKey_LeftArrow)))focus=rows+(focus+2-rows)%3;
                if(((events&32)||ImGui::IsKeyPressed(ImGuiKey_RightArrow)))focus=rows+(focus+1-rows)%3;
            }
        }
        const unsigned keyboard_row=focus;
        const bool flags[]={false,true};const char* flag_names[]={"Off","On"};
        char resolution[48];
        if(!draft.settings.width)std::snprintf(resolution,sizeof(resolution),"Desktop");
        else std::snprintf(resolution,sizeof(resolution),"%u x %u",draft.settings.width,draft.settings.height);
        int resolution_delta=SettingRow(canvas,"Resolution",resolution,0,focus,keyboard_row==0?delta:0,16,top);
        if(resolution_delta&&!modes.empty()){
            size_t current=modes.size();
            for(size_t i=0;i<modes.size();++i)if(draft.settings.width==modes[i].width&&draft.settings.height==modes[i].height)current=i;
            const size_t next=current<modes.size()?(current+modes.size()+(resolution_delta>0?1:-1))%modes.size():0;
            draft.settings.width=modes[next].width;draft.settings.height=modes[next].height;
        }
        const app::DisplayMode display[]={app::DisplayMode::Fullscreen,app::DisplayMode::Borderless,app::DisplayMode::Windowed};
        const char* display_names[]={"Fullscreen","Borderless","Windowed"};
        SettingChoice(canvas,"Display mode",draft.settings.display_mode,display,display_names,1,focus,keyboard_row==1?delta:0,top,16);
        const uint32_t aspects[]={0,1,2};const char* aspect_names[]={"Automatic","4:3","16:9"};
        SettingChoice(canvas,"Aspect ratio",draft.aspect,aspects,aspect_names,2,focus,keyboard_row==2?delta:0,top,16);
        const uint32_t scales[]={25,50,67,75,100,125,150,200};const char* scale_names[]={"25%","50%","67%","75%","100%","125%","150%","200%"};
        SettingChoice(canvas,"Internal render scale",draft.settings.render_percent,scales,scale_names,3,focus,keyboard_row==3?delta:0,top,16);
        const uint32_t filters[]={0,1,2,4,8,16};const char* filter_names[]={"Game default","Off","2x","4x","8x","16x"};
        SettingChoice(canvas,"Anisotropic filtering",draft.settings.aniso,filters,filter_names,4,focus,keyboard_row==4?delta:0,top,16);
        SettingChoice(canvas,"VSync",draft.settings.vsync,flags,flag_names,5,focus,keyboard_row==5?delta:0,top,16);
        const uint32_t caps[]={30,60,90,120,144,165,240,0};const char* cap_names[]={"30","60","90","120","144","165","240","Unlimited"};
        SettingChoice(canvas,"FPS limit",draft.settings.fps_cap,caps,cap_names,6,focus,keyboard_row==6?delta:0,top,16);
        SettingChoice(canvas,"FPS counter",draft_fps,flags,flag_names,7,focus,keyboard_row==7?delta:0,top,16);
        const uint32_t fog_modes[]={0,1,2,3};const char* fog_names[]={"Off","Original","Smooth original","Distant only"};
        SettingChoice(canvas,"Fog",draft_fog_mode,fog_modes,fog_names,8,focus,keyboard_row==8?delta:0,top,16);
        const uint32_t samples[]={1,2,4};const char* sample_names[]={"Off","2x","4x"};
        unsigned sample_index=draft.msaa==4?2:draft.msaa==2?1:0;
        const int sample_delta=SettingRow(canvas,"MSAA",sample_names[sample_index],9,focus,keyboard_row==9?delta:0,16,top);
        if(sample_delta){
            for(unsigned i=0;i<3;++i){
                sample_index=(sample_index+(sample_delta>0?1:2))%3;
                if(trial_graphics::MsaaSupported(samples[sample_index])){draft.msaa=samples[sample_index];break;}
            }
        }
        SettingChoice(canvas,"Dynamic scenery lighting",draft.scenery,flags,flag_names,10,focus,keyboard_row==10?delta:0,top,16);
        SettingChoice(canvas,"World character lighting",draft.characters,flags,flag_names,11,focus,keyboard_row==11?delta:0,top,16);
        const uint32_t qualities[]={0,1};const char* quality_names[]={"Full","Simple"};
        SettingChoice(canvas,"Character lighting quality",draft.character_quality,qualities,quality_names,12,focus,keyboard_row==12?delta:0,top,16);
        const uint32_t amounts[]={0,1,2};const char* amount_names[]={"Default","More","Forever"};
        SettingChoice(canvas,"Ragdoll amount",draft.ragdolls,amounts,amount_names,13,focus,keyboard_row==13?delta:0,top,16);
        SettingSlider(canvas,"Field of view (vertical)",draft.fov,40,90,1," degrees",14,focus,keyboard_row==14?delta:0,top);
        SettingChoice(canvas,"HD HUD and menus",draft.hd_ui,flags,flag_names,15,focus,keyboard_row==15?delta:0,top,16);
        SettingSlider(canvas,"Reticle size",draft.reticle_scale,50,200,5,"%",16,focus,keyboard_row==16?delta:0,top);
        SettingSlider(canvas,"Weapon FOV",draft.weapon_fov,50,150,5,"%",17,focus,keyboard_row==17?delta:0,top);
        canvas.Line(60,364,520,IM_COL32(118,126,121,100));
        canvas.Text(focus==17?"Weapon FOV: 100% is original. Changes the gun and hands; keeps the world view.":
                    focus==16?"Reticle size: 100% is original. Applies to the crosshair, not the rest of the HUD.":
                    focus==15?"Sharper original HUD, menus, text and prompts. Keeps the original layout.":
                    focus==14?"Vertical FOV: 53 is original. Applies to gameplay; weapon zoom is preserved.":
                    focus==2?"Aspect ratio changes take effect after restarting the game.":
                    focus==3?"Render scale changes the resolution of the 3D scene.":
                    focus==8?(draft_fog_mode==0?"Removes distance fog.":
                              draft_fog_mode==1?"Uses the original game's distance fog.":
                              draft_fog_mode==2?"Keeps the original fog range with smoother colour transitions.":
                                                "A smooth far-distance fade that keeps the nearby scene clear."):
                    focus==9?"Smooths geometry edges. Higher settings increase GPU work.":
                    focus==10||focus==11?"Your gun, hands and flashlight stay lit in every mode.":
                    focus==12?"Simple reduces local light effects on world characters.":
                    focus==13?(draft.ragdolls==2?"Forever keeps bodies until mission exit; may reduce performance.":
                               "Default: 5 bodies / 30 seconds. More: 32 / 120 seconds."):
                    "Select a setting, then use the arrows to change it.",61,371,11,muted,518);
        const char* labels[]={"Apply","Discard changes","Back"};
        auto activate=[&](unsigned i){
            if(i==0)Apply();
            else if(i==1){ReadDraft();message="Changes discarded.";}
            else RequestBack();
        };
        if(enter&&focus>=rows)activate(focus-rows);
        RowStrip(canvas,labels,nullptr,3,395,focus>=rows?focus-rows:3,[&](unsigned i){focus=rows+i;},activate);
    }
    const char* status=!message.empty()?message.c_str():trial_graphics::ScalePending()?
        "Rendering quality will update on the next complete scene redraw.":Dirty()?"Changes not applied.":"";
    canvas.Center(status,30,429,580,11,IM_COL32(207,217,219,255));
    if(xbox_KeyboardMode()){
        if(confirm_back)FooterPrompts(canvas,{{13,"Select"},{VK_RBUTTON,"Keep editing"}});
        else FooterPrompts(canvas,{{37,""},{39,"Change"},{13,"Select"},{VK_RBUTTON,"Back"}});
    }else FooterPrompts(canvas,{{0x103,"Navigate"},{0x101,"Select"},{0x102,confirm_back?"Keep editing":"Back"}});
    if(back)RequestBack();
    ImGui::EndDisabled();EndMenu();
}
#include "xbe_controls_ui.inc"
#include "xbe_language_ui.inc"
}

void TrialFrontendPublish(const TrialFrontendFrame& frame){
    // Original profile/name/level screens suit a pad. Keep added settings and
    // desktop actions reachable in the shared main, pause and options lists.
    const bool native=xbox_KeyboardMode()||frame.delete_dialog;
    const bool active=TrialFrontendSupported(frame)&&native;bool changed;
    {std::lock_guard lock(context_lock);
        changed=frontend_active.load()!=active;
        if(changed||frontend.source.epoch!=frame.source.epoch||frontend.source.owner!=frame.source.owner||
           frontend.source.screen!=frame.source.screen||frontend.source.kind!=frame.source.kind){
            frontend_action={};frontend_name_action={};frontend_events=0;++navigation_generation;
            native_confirm_generation=0;
        }
        frontend=frame;frontend_active=active;
        frontend_text_entry=active&&frame.source.kind==TRIAL_HF_NAME;
        frontend_transition=native&&(frame.source.source_kind&UC_HF_TRANSITION);
        if(!native)frontend_presentation={};
        else frontend_presentation.Publish(frame,active);
    }
    if(changed)TrialGraphicsInputChanged();
}
bool TrialFrontendTakeAction(UcHybridFrontendAction& action){
    std::lock_guard lock(context_lock);if(!frontend_action.kind||!TrialFrontendSupported(frontend))return false;
    action=frontend_action;frontend_action={};return true;
}
bool TrialFrontendOwns(uint32_t owner,uint32_t kind){
    std::lock_guard lock(context_lock);return (frontend_active||frontend_action.kind)&&frontend.source.owner==owner&&frontend.source.kind==kind;
}
bool TrialFrontendPresents(uint32_t owner,uint32_t kind){
    std::lock_guard lock(context_lock);return frontend_presentation.Presents(owner,kind);
}
void TrialFrontendNameAction(const TrialFrontendFrame& frame,unsigned kind,const char* text){
    if(kind!=UC_HF_SELECT&&kind!=UC_HF_ACTIVATE&&kind!=UC_HF_BACK)return;
    TrialFrontendNameActionData pending{};
    if(text){size_t length=0;while(length<16&&text[length])++length;if(length>=16)return;std::memcpy(pending.text,text,length);}
    if(!TrialFrontendNameText(pending.text))return;
    std::lock_guard lock(context_lock);
    const auto& source=frame.source;
    if(!frontend_active||source.kind!=TRIAL_HF_NAME||frontend.source.kind!=TRIAL_HF_NAME||
       source.epoch!=frontend.source.epoch||source.owner!=frontend.source.owner||source.screen!=frontend.source.screen)return;
    if(frontend_name_action.identity.kind&&frontend_name_action.identity.kind!=UC_HF_SELECT)return;
    pending.identity={source.epoch,source.screen,source.owner,source.rows[0].row,0,kind};frontend_name_action=pending;
}
bool TrialFrontendTakeName(TrialFrontendNameActionData& action){
    std::lock_guard lock(context_lock);
    if(!frontend_active||frontend.source.kind!=TRIAL_HF_NAME||!frontend_name_action.identity.kind)return false;
    action=frontend_name_action;frontend_name_action={};return true;
}
static bool ControllerSupplement(){
    return !xbox_KeyboardMode()&&TrialFrontendSupported(frontend)&&
        frontend.source.kind==UC_HF_LIST&&context.owner==frontend.source.owner&&
        context.screen==frontend.source.screen&&(context.kind==1||context.kind==3);
}
void TrialFrontendPad(uint32_t buttons,uint8_t confirm,uint8_t back,int16_t x,int16_t y){
    const bool controls=PanelOpen();
    const unsigned mask=(controls?((buttons&1)||y>16000?1:0)|((buttons&2)||y<-16000?2:0)|
        ((buttons&4)||x<-16000?16:0)|((buttons&8)||x>16000?32:0):
        ((buttons&5)||x<-16000||y>16000?1:0)|((buttons&10)||x>16000||y<-16000?2:0))|
        (confirm>30?4:0)|(back>30?8:0)|((buttons&0x100)?64:0)|((buttons&0x200)?128:0);
    static uint64_t epoch=0;static unsigned previous=0;static bool released=false,panel_mode=false;
    std::lock_guard lock(context_lock);
    const bool supplement=ControllerSupplement();
    if(!frontend_active&&!supplement&&!PanelOpen()){epoch=0;released=false;previous=mask;frontend_events=0;return;}
    const bool panel=PanelOpen();
    if(epoch!=navigation_generation||panel_mode!=panel){
        epoch=navigation_generation;panel_mode=panel;released=false;previous=mask;frontend_events=0;
    }
    if(!mask)released=true;
    if(released)frontend_events.fetch_or((mask&~previous)&(panel?127u:255u));
    previous=mask;
}

void TrialGraphicsMenuContext(uint32_t manager,uint32_t screen,uint32_t owner,uint32_t kind) {
    std::lock_guard lock(context_lock);context={manager,screen,owner,kind};
}
bool TrialGraphicsOpen() { return PanelOpen()||quit_open.load()||frontend_active.load(); }
bool TrialGraphicsMessage(uint32_t msg,uintptr_t key,intptr_t position) {
    if(msg==WM_KILLFOCUS||(msg==WM_ACTIVATEAPP&&!key))native_confirm_generation=0;
    const bool owned=TrialGraphicsOpen();
    const bool confirm=key==VK_RETURN||key==VK_SPACE;
    if(owned&&confirm&&msg==WM_KEYDOWN&&!(uint64_t(position)&(1ull<<30))&&
       (key==VK_RETURN||!frontend_text_entry||PanelOpen()||quit_open)&&!trial_controls::CaptureWaiting())
        native_confirm_generation=navigation_generation.load();
    // Transition pixels are deliberately non-interactive. Do not retain their
    // confirm taps for the original screen which will appear underneath next.
    if(frontend_transition&&confirm&&(msg==WM_KEYDOWN||msg==WM_KEYUP))return true;
    return owned&&((msg>=WM_KEYFIRST&&msg<=WM_KEYLAST)||(msg>=WM_MOUSEFIRST&&msg<=WM_MOUSELAST)||msg==WM_INPUT);
}
void TrialGraphicsFonts(const std::filesystem::path& game) {
    auto& io=ImGui::GetIO();
    if(TrialLoadFrontendFonts(io.Fonts,game,body_font,title_font))return;
    wchar_t directory[MAX_PATH]{};if(!GetWindowsDirectoryW(directory,MAX_PATH))return;
    auto fonts=std::filesystem::path(directory)/"Fonts";
    auto body=(fonts/"arial.ttf").string(),title=(fonts/"arialbi.ttf").string();
    // Recovery fallback if the user's original font archives cannot be read.
    if(std::filesystem::exists(body))body_font=io.Fonts->AddFontFromFileTTF(body.c_str(),22);
    if(std::filesystem::exists(title))title_font=io.Fonts->AddFontFromFileTTF(title.c_str(),38);
}
void TrialGraphicsUi() {
    Menu menu;TrialFrontendFrame frame;TrialFrontendPresentation presentation;bool active;
    {std::lock_guard lock(context_lock);menu=context;frame=frontend;presentation=frontend_presentation;active=frontend_active.load();}
    const bool eligible=TrialFrontendSupported(frame)&&(frame.source.kind!=UC_HF_LIST||
        (menu.owner==frame.source.owner&&menu.screen==frame.source.screen&&menu.kind==frame.list_kind));
    const bool current=active&&eligible;
    const bool graphics=eligible&&frame.source.kind==UC_HF_LIST&&frame.list_kind==3;
    if(!xbox_KeyboardMode()&&eligible&&!PanelOpen()&&!quit_open){
        const unsigned events=frontend_events.exchange(0);
        if(menu.kind==1&&(events&128))OpenQuit(frame);
        if(graphics&&(events&(64|128))){
            std::lock_guard lock(context_lock);
            opened=menu;settings_page=(events&128)?1:0;open_requested=true;
            if(settings_page==1){controls_origin=frame;controls_origin_row=0;
                for(unsigned i=0;i<frame.source.count;++i)
                    if(!std::strcmp(frame.source.rows[i].label,"NewFrontEnd-ControllerOptions-1"))controls_origin_row=i;
            }
        }
    }
    if(open_requested.load()) {
        if(graphics&&SameMenu(menu,opened)){
            panel_open=true;open_requested=false;frontend_events=0;++navigation_generation;
            TrialGraphicsInputChanged();
            if(settings_page==1)ControlsReadDraft();else if(settings_page==2)LanguageReadDraft();else{ReadDraft();EnumerateModes();}
            message.clear();confirm_back=false;
        }else open_requested=false;
    }
    if(panel_open&&(!graphics||!SameMenu(menu,opened)))Close();
    if(quit_open&&(!eligible||!QuitMatches(frame)||!SameMenu(menu,quit_origin)))CloseQuit();
    const bool transition=!current&&presentation.visible&&presentation.transition;
    if(!menu.kind&&!current&&!transition)return;
    const auto* view=ImGui::GetMainViewport();
    if(view->Size.x<=0||view->Size.y<=0)return;
    if(quit_open)QuitUi(view);
    else if(panel_open){
        const bool paused=opened.kind==2||(frame.source.source_kind&0x80000000u);
        if(settings_page==1)ControlsUi(view,paused);else if(settings_page==2)LanguageUi(view,paused);else SettingsUi(view,paused);
    }
    else if(current){
        if(frame.delete_dialog||frame.pause_dialog)ConfirmationUi(frame,view);
        else if(frame.source.kind==UC_HF_LIST&&(frame.list_kind==4||frame.list_kind==5))OriginalSettingsUi(frame,view);
        else if(frame.source.kind==TRIAL_HF_NAME)NameUi(frame,view);else FrontendUi(frame,view);
    }
    else if(!xbox_KeyboardMode()&&eligible&&(menu.kind==1||menu.kind==3)){
        BeginMenu("PC controller shortcuts",view);MenuCanvas canvas(view);
        canvas.Rect(0,440,640,40,IM_COL32(0,0,0,255));
        if(menu.kind==3)FooterPrompts(canvas,{{0x104,"Graphics"},{0x105,"Controls"}});
        else FooterPrompts(canvas,{{0x105,"Quit to desktop"}});
        ImGui::End();ImGui::PopStyleVar();
    }
    else if(transition){
        // Keep only the last copied presentation while the original manager
        // fades its rows. Active identities/actions were already revoked.
        if(presentation.frame.delete_dialog||presentation.frame.pause_dialog)ConfirmationUi(presentation.frame,view,false);
        else if(presentation.frame.source.kind==UC_HF_LIST&&(presentation.frame.list_kind==4||presentation.frame.list_kind==5))OriginalSettingsUi(presentation.frame,view,false);
        else if(presentation.frame.source.kind==TRIAL_HF_NAME)NameUi(presentation.frame,view,false);
        else FrontendUi(presentation.frame,view,false);
    }
}

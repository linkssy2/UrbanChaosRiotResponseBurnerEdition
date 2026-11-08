// Actual ImGui state and exact host transition helper; window capture is a boundary.
#undef NDEBUG
#define NOMINMAX
#include <windows.h>
#include <cassert>
#include <cstdio>
#include <mutex>
#include "imgui.h"
#include "imgui_internal.h"
#include "xbe_coop_pause.h"
namespace app {std::recursive_mutex& UiLock(){static std::recursive_mutex lock;return lock;}}
static HWND game_window=reinterpret_cast<HWND>(1);
static constexpr UINT kPauseRelease=WM_APP+72;
static unsigned posts=0;
static bool menu_mode=false,graphics_open=false,coop_open=false,keyboard_mode=true;
static bool TrialGraphicsOpen(){return graphics_open;}
static bool TrialCoopMenuOpen(){return coop_open;}
static int xbox_KeyboardMode(){const int owner=trial_coop::PauseOwner();return owner<0?keyboard_mode:owner==0;}
static BOOL TestPostMessage(HWND window,UINT message,WPARAM,LPARAM){assert(window==game_window&&message==kPauseRelease);++posts;return TRUE;}
#undef PostMessage
#define PostMessage TestPostMessage
// HOST_IMGUI_HELPER
static void Dirty(){
    auto& io=ImGui::GetIO();io.AddKeyEvent(ImGuiKey_A,true);io.AddMouseButtonEvent(0,true);
    io.KeysData[ImGuiKey_A-ImGuiKey_NamedKey_BEGIN].Down=true;io.MouseDown[0]=true;
    assert(GImGui->InputEventsQueue.Size&&io.MouseDown[0]);
}
static void Enter(unsigned slot){
    trial_coop::ResetPauseOwner();trial_coop::pause_state.Sample(false,false);
    trial_coop::pause_state.Sample(slot==0,slot==1);trial_coop::pause_state.Consume(true);trial_coop::pause_state.Enter();
}
static void WindowSwitcher(bool corrected,int owner){
    ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;
    io.DisplaySize={640,480};io.DeltaTime=1.f/60;io.Fonts->Build();
    io.ConfigFlags|=ImGuiConfigFlags_NavEnableGamepad;io.BackendFlags|=ImGuiBackendFlags_HasGamepad;
    trial_coop::ResetPauseOwner();if(owner>=0)Enter(unsigned(owner));
    auto frame=[&]{
        if(corrected)TrialCoopPauseImGuiInput();
        ImGui::NewFrame();ImGui::Begin("Game HUD");ImGui::TextUnformatted("Health");ImGui::End();
        ImGui::Begin("Game menu");ImGui::TextUnformatted("Options");ImGui::End();ImGui::Render();
    };
    frame();frame();io.AddKeyEvent(ImGuiKey_GamepadFaceLeft,true);
    for(unsigned i=0;i<60;++i)frame();
    assert(bool(GImGui->NavWindowingTarget)==!corrected);
    ImGui::DestroyContext();
}
static void MenuPointerReturn(){
    ImGui::CreateContext();auto& io=ImGui::GetIO();io.IniFilename=nullptr;
    io.DisplaySize={640,480};io.DeltaTime=1.f/60;io.Fonts->Build();
    trial_coop::Reset();menu_mode=true;keyboard_mode=true;
    bool hovered=false,clicked=false;float wheel=0;
    auto frame=[&]{
        TrialCoopPauseImGuiInput();ImGui::NewFrame();
        ImGui::SetNextWindowPos({0,0});ImGui::SetNextWindowSize({200,150});
        ImGui::Begin("Menu handoff",nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize);
        ImGui::SetCursorPos({20,20});clicked=ImGui::InvisibleButton("row",{100,50});hovered=ImGui::IsItemHovered();
        wheel=io.MouseWheel;ImGui::End();ImGui::Render();
    };
    frame();io.AddMousePosEvent(50,40);frame();frame();assert(hovered);
    keyboard_mode=false;frame();frame();assert(!hovered&&io.MousePos.x==50&&io.MousePos.y==40);
    // Win32 button/wheel messages do not carry a separate mouse-position event.
    keyboard_mode=true;io.AddMouseButtonEvent(0,true);frame();assert(hovered);
    io.AddMouseButtonEvent(0,false);frame();assert(clicked);
    keyboard_mode=false;frame();assert(!hovered);
    keyboard_mode=true;io.AddMouseWheelEvent(0,1);frame();assert(hovered&&wheel==1);
    keyboard_mode=false;frame();keyboard_mode=true;io.AddMousePosEvent(60,40);frame();
    assert(hovered&&io.MouseDelta.x==10&&io.MouseDelta.y==0);
    menu_mode=false;ImGui::DestroyContext();
}
int main(){
    ImGui::CreateContext();trial_coop::SetActive(true);
    Dirty();Enter(1);TrialCoopPauseImGuiInput();
    auto& io=ImGui::GetIO();assert(!GImGui->InputEventsQueue.Size&&!io.MouseDown[0]);
    assert(!io.KeysData[ImGuiKey_A-ImGuiKey_NamedKey_BEGIN].Down&&posts==1);
    assert(io.ConfigFlags&ImGuiConfigFlags_NoMouse); // Win32's polled cursor fallback cannot hover P2 rows.
    Dirty();TrialCoopPauseImGuiInput();assert(GImGui->InputEventsQueue.Size&&posts==1); // no per-frame clearing
    Enter(0);TrialCoopPauseImGuiInput();assert(!GImGui->InputEventsQueue.Size&&!io.MouseDown[0]&&posts==2);
    assert(!(io.ConfigFlags&ImGuiConfigFlags_NoMouse));
    Dirty();trial_coop::ResetPauseOwner();TrialCoopPauseImGuiInput();assert(!GImGui->InputEventsQueue.Size&&posts==3);
    Dirty();TrialCoopPauseImGuiInput();assert(GImGui->InputEventsQueue.Size&&posts==3); // ordinary SP unchanged
    Enter(1);
    for(unsigned message:{WM_KEYDOWN,WM_KEYUP,WM_SYSKEYDOWN,WM_CHAR,WM_MOUSEMOVE,WM_MOUSEWHEEL,WM_LBUTTONDOWN,WM_INPUT})
        assert(trial_coop::PauseBlocksMessage(message));
    for(unsigned message:{WM_PAINT,WM_SIZE,WM_CLOSE,WM_SETFOCUS,WM_KILLFOCUS,WM_ACTIVATEAPP,WM_SETCURSOR})
        assert(!trial_coop::PauseBlocksMessage(message));
    trial_coop::Reset();assert(!trial_coop::PauseBlocksMessage(WM_KEYDOWN));
    TrialCoopPauseImGuiInput();io.ConfigFlags|=ImGuiConfigFlags_NoMouse;
    Enter(1);TrialCoopPauseImGuiInput();trial_coop::ResetPauseOwner();TrialCoopPauseImGuiInput();
    assert(io.ConfigFlags&ImGuiConfigFlags_NoMouse); // preserve an already configured NoMouse flag
    io.ConfigFlags&=~ImGuiConfigFlags_NoMouse;
    menu_mode=true;keyboard_mode=false;Dirty();TrialCoopPauseImGuiInput();
    assert((io.ConfigFlags&ImGuiConfigFlags_NoMouse)&&!GImGui->InputEventsQueue.Size&&!io.MouseDown[0]);
    Dirty();TrialCoopPauseImGuiInput();assert(!GImGui->InputEventsQueue.Size); // held key repeats stay suppressed
    keyboard_mode=true;TrialCoopPauseImGuiInput();assert(!(io.ConfigFlags&ImGuiConfigFlags_NoMouse));
    Dirty();TrialCoopPauseImGuiInput();assert(GImGui->InputEventsQueue.Size); // fresh desktop input survives
    keyboard_mode=false;coop_open=true;TrialCoopPauseImGuiInput();assert(!(io.ConfigFlags&ImGuiConfigFlags_NoMouse));
    menu_mode=false;coop_open=false;TrialCoopPauseImGuiInput();assert(!(io.ConfigFlags&ImGuiConfigFlags_NoMouse));
    graphics_open=true;TrialCoopPauseImGuiInput();assert(io.ConfigFlags&ImGuiConfigFlags_NoMouse);
    graphics_open=false;keyboard_mode=true;TrialCoopPauseImGuiInput();assert(!(io.ConfigFlags&ImGuiConfigFlags_NoMouse));
    ImGui::DestroyContext();
    WindowSwitcher(false,-1);for(int owner:{-1,0,1})WindowSwitcher(true,owner);
    MenuPointerReturn();
    std::puts("Pause ImGui: owner handoff and mouse isolation; original hold-X window-switcher reproduced and disabled in gameplay and both pause owners.");
}

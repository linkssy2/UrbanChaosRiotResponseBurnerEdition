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
    ImGui::DestroyContext();std::puts("Pause ImGui: actual held/queued key and mouse state cleared only on owner handoff; window lifecycle preserved.");
}

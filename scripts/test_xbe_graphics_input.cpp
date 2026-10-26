// Actual staged pad adapter plus the existing keyboard binding implementation.
// Only physical input/focus/window messaging are substituted; no ImGui mock.
#undef NDEBUG
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include "core/input.h"
#include "xbe_controls.h"
#include "xbe_controls_capture.h"
#include "xbe_coop.h"
#include "xbe_coop_pause.h"
#include "scripts/xbe_pc_prompts.h"
#include "scripts/xbe_intro_skip.h"
#include "scripts/xbe_menu_key_taps.h"

namespace {
// GENERATED_STATE
bool test_focused=true,test_open=false;
unsigned test_presentation=0;
unsigned TrialCoopPresentation(){return test_presentation;}
bool physical_keys[256]={};
unsigned key_queries[256]={};
unsigned capture_posts=0;
bool conversation_down=false,conversation_enabled=false;
bool reload_down=false,reload_enabled=false;
bool health_down[2]{},health_allowed[2]{};
void TrialHealthPackInput(unsigned slot,bool down,bool allowed){assert(slot<2);health_down[slot]=down;health_allowed[slot]=allowed;}
void TrialReloadPublish(bool down,bool enabled){reload_down=down;reload_enabled=enabled;}
void TrialConversationPublish(bool down,bool enabled){conversation_down=down;conversation_enabled=enabled;}
bool focused(){return test_focused;}
bool TrialGraphicsOpen(){return test_open;}
void TrialFrontendPad(uint32_t,uint8_t,uint8_t,int16_t,int16_t){}
bool menu_pad_connected=false;unsigned menu_pad_buttons=0,menu_pad_calls=0;
bool coop_menu_open=false;
bool TrialCoopMenuOpen(){return coop_menu_open;}
void TrialCoopMenuPad(bool connected,uint32_t buttons,uint8_t,uint8_t,int16_t,int16_t){
    menu_pad_connected=connected;menu_pad_buttons=buttons;++menu_pad_calls;
}
SHORT TestAsyncKeyState(int key){if(key>0&&key<256){++key_queries[key];return physical_keys[key]?SHORT(0x8000):0;}return 0;}
// GENERATED_KEY_DOWN
int controller_path(wchar_t*){return 0;}
BOOL TestPostMessage(HWND,UINT,WPARAM,LPARAM){++capture_posts;return TRUE;}
}
#define GetAsyncKeyState TestAsyncKeyState
#undef PostMessage
#define PostMessage TestPostMessage
#define xbox_KeyboardMode TrialGameplayKeyboardMode
#include "third_party/xboxrecomp/src/input/host_keyboard_config.h"
#undef xbox_KeyboardMode
// GENERATED_KEYBOARD_MODE
#include "xbe_controls_backend.inc"

// GENERATED_CONTROLS
// GENERATED_CHANGED
// GENERATED_RIGHT_BACK
// GENERATED_PUBLISH
// GENERATED_MENU_KEYS
// GENERATED_PAD
// GENERATED_CONTROL_STICK

static bool Neutral(const input::Pad& p){
    if(p.buttons||p.lx||p.ly||p.rx||p.ry)return false;
    for(auto value:p.analog)if(value)return false;
    return true;
}
static input::Pad Poll(int port,input::Pad p={}){TrialInputPad(port,p);return p;}
static void ReleaseAll(){
    std::memset(physical_keys,0,sizeof(physical_keys));test_open=false;test_focused=true;
    for(int port=0;port<4;++port)Poll(port);
}
static void Open(){
    ReleaseAll();test_open=true;captured=true;menu_mode=false;gameplay=true;
    dx=71;dy=-33;click_pending=true;order_phase=2;order_buttons=1;order_held=true;
    keyboard_wheel_record(WHEEL_DELTA);keyboard_wheel_publish();
    const unsigned posts=capture_posts;TrialGraphicsInputChanged();
    assert(!dx&&!dy&&!click_pending&&!order_phase&&!order_buttons&&!order_held);
    assert(!keyboard_wheel_active&&!keyboard_wheel_count&&capture_posts==posts+1);
}
static void Close(){test_open=false;TrialGraphicsInputChanged();}
static input::Pad Controller(unsigned control){
    input::Pad p;p.connected=true;
    switch(control){
        case 0:p.buttons=1;break;
        case 1:p.buttons=16;break;
        case 2:p.analog[0]=255;break;
        case 3:p.analog[7]=255;break;
        case 4:p.lx=30000;break;
        case 5:p.lx=-30000;break;
        case 6:p.ly=30000;break;
        case 7:p.ly=-30000;break;
        case 8:p.rx=30000;break;
        case 9:p.rx=-30000;break;
        case 10:p.ry=30000;break;
        case 11:p.ry=-30000;break;
    }
    return p;
}
int main(){
    keyboard_init();keyboard_defaults(&keyboard_config);
    controller_init();controller_defaults(&controller_config);controller_publish();
    // Character selection receives the first physical device before the keyboard
    // adapter marks logical port zero connected. Its input cannot hide the page.
    test_open=true;coop_menu_open=true;xbox_KeyboardNoteInput();
    Poll(0);assert(!menu_pad_connected&&xbox_KeyboardMode());
    Poll(0,Controller(0));assert(menu_pad_connected&&menu_pad_buttons==1&&xbox_KeyboardMode());
    const auto menu_calls=menu_pad_calls;Poll(1,Controller(2));assert(menu_pad_calls==menu_calls);
    assert(Neutral(Poll(0,Controller(8)))&&xbox_KeyboardMode());
    coop_menu_open=false;
    // Unbound, unflagged keys require no physical polling in the release guard.
    // Held transition keys remain blocked until a real release, even without focus.
    ReleaseAll();test_open=true;
    physical_keys[VK_F12]=true;TrialGraphicsInputChanged();
    std::memset(key_queries,0,sizeof(key_queries));
    Poll(0);assert(release_keys[VK_F12]&&key_queries[VK_F12]==1);
    assert(key_queries[VK_F11]==0);
    test_focused=false;physical_keys[VK_F12]=false;
    Poll(0);assert(!release_keys[VK_F12]&&key_queries[VK_F12]==2);
    Poll(0);assert(key_queries[VK_F12]==2&&key_queries[VK_F11]==0);
    // All 255 transition keys are still discovered when a handoff starts.
    physical_keys[VK_F11]=true;TrialGraphicsInputChanged();
    assert(release_keys[VK_F11]);
    std::memset(key_queries,0,sizeof(key_queries));
    Poll(1);assert(key_queries[VK_F11]==0&&release_keys[VK_F11]);
    Poll(0);assert(key_queries[VK_F11]==1&&release_keys[VK_F11]);
    ReleaseAll();menu_mode=false;captured=false;physical_keys['R']=true;
    TrialInputPublish();assert(!conversation_down&&conversation_enabled);
    physical_keys['F']=true;TrialInputPublish();assert(conversation_down&&conversation_enabled);
    for(unsigned blocked=0;blocked<3;++blocked){
        test_focused=blocked!=0;menu_mode=blocked==1;test_open=blocked==2;
        TrialInputPublish();assert(conversation_down&&!conversation_enabled);
    }
    ReleaseAll();
    // Conversation prompts and their exact source-qualified edge query read
    // the rebound interaction slot. Unbound Interact must not fall back to Reload.
    keyboard_config.binding[0][1]='G';physical_keys['F']=true;
    TrialInputPublish();assert(!conversation_down);physical_keys['G']=true;
    TrialInputPublish();assert(conversation_down);ReleaseAll();
    keyboard_config.binding[0][1]=0;physical_keys['R']=true;
    TrialInputPublish();assert(!conversation_down);ReleaseAll();
    keyboard_config.binding[0][1]=256;captured=true;keyboard_wheel_record(WHEEL_DELTA);
    TrialInputPublish();assert(conversation_down);TrialInputPublish();assert(!conversation_down);
    keyboard_defaults(&keyboard_config);
    // Independent firearm sources share the retail selection button but retain
    // direction for its qualified cycle consumer; wheel pulses are repeatable.
    gameplay=true;menu_mode=false;captured=true;ReleaseAll();
    trial_controls::firearm_loaded=true;trial_controls::previous_firearm[0]=257;
    trial_controls::previous_firearm[1]='B';
    physical_keys['C']=true;assert(Poll(0).analog[2]==255&&TrialFirearmDirection()==1);ReleaseAll();
    physical_keys['B']=true;assert(Poll(0).analog[2]==255&&TrialFirearmDirection()==-1);
    assert(Poll(0).analog[2]==255&&TrialFirearmDirection()==-1);
    physical_keys['C']=true;assert(Poll(0).analog[2]==0&&TrialFirearmDirection()==0);ReleaseAll();
    keyboard_wheel_record(-2*WHEEL_DELTA);
    for(unsigned pulse=0;pulse<2;++pulse){
        TrialInputPublish();assert(Poll(0).analog[2]==255&&TrialFirearmDirection()==-1);
        TrialInputPublish();assert(Poll(0).analog[2]==0&&TrialFirearmDirection()==0);
    }
    physical_keys['B']=true;test_focused=false;assert(Neutral(Poll(0))&&TrialFirearmDirection()==0);
    test_focused=true;test_open=true;assert(Neutral(Poll(0))&&TrialFirearmDirection()==0);ReleaseAll();
    menu_mode=true;physical_keys['B']=true;assert(Poll(0).analog[2]==0&&TrialFirearmDirection()==0);ReleaseAll();menu_mode=false;
    input::Pad firearm_pad;firearm_pad.connected=true;firearm_pad.analog[2]=255;
    physical_keys['B']=true;assert(Poll(0,firearm_pad).analog[2]==255&&TrialFirearmDirection()==1);ReleaseAll();
    trial_controls::previous_firearm[0]=VK_XBUTTON2;trial_controls::previous_firearm[1]=0;
    physical_keys[VK_XBUTTON2]=true;captured=false;assert(Poll(0).analog[2]==0);captured=true;
    assert(Poll(0).analog[2]==255&&TrialFirearmDirection()==-1);ReleaseAll();
    trial_controls::previous_firearm[0]=0;physical_keys[VK_XBUTTON2]=true;assert(Poll(0).analog[2]==0);ReleaseAll();
    trial_controls::previous_firearm[0]=257;

    // Only Interact reaches retail A during gameplay, including rebound keys.
    gameplay=true;menu_mode=false;captured=true;ReleaseAll();
    physical_keys['R']=true;TrialInputPublish();assert(reload_down&&reload_enabled&&!conversation_down);assert(Poll(0).analog[0]==0);ReleaseAll();
    physical_keys['F']=true;TrialInputPublish();assert(!reload_down&&conversation_down);assert(Poll(0).analog[0]==255);ReleaseAll();
    keyboard_config.binding[0][1]='G';physical_keys['G']=true;assert(Poll(0).analog[0]==255);ReleaseAll();
    keyboard_config.binding[0][1]=0;physical_keys['R']=true;assert(Poll(0).analog[0]==0);ReleaseAll();
    keyboard_defaults(&keyboard_config);
    // Capture sees raw input before the menu neutralizes/remaps it. It will
    // not bind the opening click or a held controller confirm, and wheel and
    // focus events use the real production message helper.
    test_open=true;trial_controls::BeginCapture(0);physical_keys[VK_LBUTTON]=true;
    Poll(0);assert(!trial_controls::TakeCapture()&&trial_controls::CaptureWaiting());
    physical_keys[VK_LBUTTON]=false;Poll(0);physical_keys[VK_MBUTTON]=true;Poll(0);
    assert(trial_controls::TakeCapture()==VK_MBUTTON&&!trial_controls::CaptureWaiting());
    ReleaseAll();test_open=true;trial_controls::BeginCapture(1);
    input::Pad idle;idle.connected=true;
    Poll(0,Controller(2));assert(!trial_controls::TakeCapture());Poll(0,idle);
    Poll(0,Controller(3));assert(trial_controls::TakeCapture()==8);
    trial_controls::BeginCapture(0);Poll(0);
    ControlsCaptureMessage(WM_MOUSEWHEEL,MAKEWPARAM(0,0));assert(!trial_controls::TakeCapture());
    ControlsCaptureMessage(WM_MOUSEWHEEL,MAKEWPARAM(0,-WHEEL_DELTA));assert(trial_controls::TakeCapture()==257);
    trial_controls::BeginCapture(0);Poll(0);ControlsCaptureMessage(WM_KILLFOCUS,0);
    assert(!trial_controls::CaptureWaiting()&&!trial_controls::TakeCapture());
    // Each capture listens only to its player's physical sources. One player
    // can bind a key while the other binds a controller without sharing events.
    ReleaseAll();test_open=true;trial_coop::SetActive(true);
    trial_controls::BeginCapture(0,0);trial_controls::BeginCapture(1,1);
    physical_keys['W']=true;Poll(1,idle);
    Poll(0,Controller(3));assert(!trial_controls::TakeCapture(1));
    Poll(1,Controller(2));assert(trial_controls::TakeCapture(1)==1);
    assert(trial_controls::CaptureWaiting(0));
    physical_keys['W']=false;Poll(0);physical_keys['G']=true;Poll(0);
    assert(trial_controls::TakeCapture(0)=='G');
    trial_controls::BeginCapture(1,1);Poll(1,idle);
    ControlsCaptureMessage(WM_MOUSEWHEEL,MAKEWPARAM(0,WHEEL_DELTA));
    Poll(0,Controller(2));assert(!trial_controls::TakeCapture(1));
    Poll(1);assert(!trial_controls::CaptureWaiting(1)); // Unplug cancels capture.
    trial_controls::BeginCapture(0,1);assert(!trial_controls::CaptureWaiting(1));
    trial_controls::BeginCapture(1,1);Poll(1,idle);
    trial_controls::BeginCapture(0,0);ControlsCaptureMessage(WM_KILLFOCUS,0);
    assert(!trial_controls::CaptureWaiting(0)&&!trial_controls::CaptureWaiting(1));
    trial_controls::BeginCapture(1,2);assert(!trial_controls::CaptureWaiting(2)&&!trial_controls::TakeCapture(2));
    trial_coop::Reset();ReleaseAll();test_open=true;
    // Pre-game P2 configuration uses the one controller on logical zero.
    // Logical one's normal disconnected poll cannot cancel that capture, and
    // the opening press must still be released before a new binding is read.
    trial_controls::BeginCapture(1,1);
    Poll(0,Controller(2));Poll(1);
    assert(trial_controls::CaptureWaiting(1)&&!trial_controls::TakeCapture(1));
    Poll(0,idle);Poll(1);Poll(0,Controller(3));
    assert(trial_controls::TakeCapture(1)==8&&!trial_controls::CaptureWaiting(1));
    trial_controls::BeginCapture(1,1);Poll(0,idle);Poll(1,Controller(2));
    assert(trial_controls::CaptureWaiting(1)&&!trial_controls::TakeCapture(1));
    Poll(0);assert(!trial_controls::CaptureWaiting(1)); // Physical controller disconnected.
    trial_controls::CancelCapture(2);
    ReleaseAll();
    // Shared in-game cinematics retain P1's original camera controls, block P2
    // gameplay and health use, and preserve the configured P2 Pause binding.
    trial_coop::SetActive(true);test_presentation=2;gameplay=true;menu_mode=false;
    trial_controls::LoadSecond();const auto second_before=trial_controls::second_controller;
    trial_controls::second_controller.binding[8][0]=1; // A -> Pause
    trial_controls::second_controller.binding[8][1]=0;
    input::Pad cinematic_input=Controller(3);cinematic_input.analog[0]=255;cinematic_input.lx=22000;
    auto shared=Poll(1,cinematic_input);assert(shared.buttons==16);
    shared.buttons=0;assert(Neutral(shared)&&!health_down[1]&&!health_allowed[1]);
    assert(Neutral(Poll(1,cinematic_input))); // Held Pause is also release-gated.
    physical_keys['W']=true;assert(Poll(0).ly==32767);physical_keys['W']=false;
    test_presentation=1;
    assert(Neutral(Poll(1,cinematic_input))); // Held fire/movement cannot leak into the split view.
    Poll(1,idle);auto resumed=Poll(1,cinematic_input);
    assert((resumed.buttons&16)&&resumed.analog[7]==255&&resumed.lx==22000);
    trial_controls::second_controller=second_before;test_presentation=0;trial_coop::Reset();ReleaseAll();
    Open();physical_keys['W']=physical_keys[VK_LBUTTON]=true;
    for(int port=0;port<4;++port){
        auto p=Controller(0);p.analog[0]=255;p.lx=20000;p.ry=-25000;
        assert(Neutral(Poll(port,p)));assert(Poll(port,p).connected);
        if(port)assert(!Poll(port).connected);
    }
    test_focused=false;
    for(int port=0;port<4;++port)assert(Neutral(Poll(port,Controller(0))));
    test_focused=true;

    // Closing key/click stays consumed until released, then original bindings
    // resume unchanged. This also exercises the real consumed-click latch.
    const int keys[]={'W','F',VK_ESCAPE,VK_LBUTTON,VK_RBUTTON};
    for(int key:keys){
        Open();physical_keys[key]=true;Close();
        assert(Neutral(Poll(0)));assert(Neutral(Poll(0)));
        physical_keys[key]=false;assert(Neutral(Poll(0)));
        physical_keys[key]=true;assert(!Neutral(Poll(0)));
        physical_keys[key]=false;assert(Neutral(Poll(0)));
    }
    // A consumed Back key cannot block a new movement/confirm input.
    Open();physical_keys['Q']=true;Close();physical_keys['W']=true;
    assert(Poll(0).ly==32767);physical_keys['Q']=physical_keys['W']=false;
    assert(Neutral(Poll(0)));physical_keys['W']=true;assert(Poll(0).ly==32767);

    for(int port=0;port<4;++port)for(unsigned control=0;control<12;++control){
        Open();auto held=Controller(control);assert(Neutral(Poll(port,held)));Close();
        assert(Neutral(Poll(port,held)));assert(Neutral(Poll(port,held)));
        auto released=held;released.buttons=0;for(auto& v:released.analog)v=0;
        released.lx=released.ly=released.rx=released.ry=0;
        assert(Neutral(Poll(port,released)));
        auto resumed=Poll(port,held);assert(!Neutral(resumed)&&resumed.connected);
    }
    // Releasing port 0 cannot unlock a still-held controller on another port.
    Open();for(int port=1;port<4;++port)Poll(port,Controller(0));Close();assert(Neutral(Poll(0)));
    for(int port=1;port<4;++port)assert(Neutral(Poll(port,Controller(0))));
    physical_keys['W']=true;assert(Poll(0).ly==32767);physical_keys['W']=false;
    for(int port=1;port<4;++port){assert(Neutral(Poll(port)));assert(Poll(port,Controller(0)).buttons==1);}

    // Real stick noise below the runtime deadzone must not strand the page.
    Open();Close();auto drift=Controller(4);drift.lx=500;
    Poll(0,drift);assert(Poll(0,Controller(0)).buttons==1);
    Open();Poll(0,Controller(0));Close();test_focused=false;assert(Neutral(Poll(0,Controller(0))));
    test_focused=true;assert(Neutral(Poll(0,Controller(0))));
    assert(Neutral(Poll(0)));assert(Poll(0,Controller(0)).buttons==1);

    // Difficulty handoff: an unrelated stuck key and drifting stick must not
    // silence Enter, A, or navigation. Only the closing press stays consumed.
    Open();physical_keys[VK_F12]=true;physical_keys[VK_LBUTTON]=true;
    auto noisy=Controller(4);noisy.analog[7]=1;Poll(0,noisy);Close();
    menu_mode=true;physical_keys[VK_RETURN]=true;
    auto confirm=Poll(0,noisy);assert(confirm.analog[0]==255&&confirm.lx==0);
    physical_keys[VK_RETURN]=false;
    noisy.analog[0]=255;assert(Poll(0,noisy).analog[0]==255);
    noisy.buttons=2;assert(Poll(0,noisy).buttons&2);
    physical_keys[VK_LBUTTON]=false;ReleaseAll();

    // Existing keyboard menu/gameplay Escape mappings and negative axes survive.
    ReleaseAll();menu_mode=true;physical_keys[VK_ESCAPE]=true;
    assert(Poll(0).analog[1]==255);physical_keys[VK_ESCAPE]=false;
    menu_mode=false;physical_keys['A']=true;assert(Poll(0).lx==-32767);
    physical_keys['A']=false;physical_keys[VK_ESCAPE]=true;assert(Poll(0).buttons&16);
    // The same Escape press cannot immediately back out of the pause menu.
    menu_mode=true;assert(!Poll(0).analog[1]);
    physical_keys[VK_ESCAPE]=false;Poll(0);
    physical_keys[VK_ESCAPE]=true;assert(Poll(0).analog[1]==255);
    physical_keys[VK_ESCAPE]=false;Poll(0);
    physical_keys[VK_RETURN]=true;assert(Poll(0).buttons&16);
    menu_mode=false;assert(!(Poll(0).buttons&16));
    // Suppress keyboard Start only: a controller still pauses normally.
    assert(Poll(0,Controller(1)).buttons&16);
    physical_keys[VK_RETURN]=false;
    keyboard_config.binding[8][0]='P';physical_keys['P']=true;assert(Poll(0).buttons&16);
    physical_keys['P']=false;keyboard_defaults(&keyboard_config);
    // Physical menu navigation remains standard after gameplay remapping.
    controller_config.binding[7][0]=1;controller_config.binding[0][0]=0;controller_publish();
    menu_mode=false;auto remapped=Poll(0,Controller(2));assert(remapped.analog[7]==255&&!remapped.analog[0]);
    menu_mode=true;auto original_menu=Poll(0,Controller(2));assert(original_menu.analog[0]==255&&!original_menu.analog[7]);
    menu_mode=false;controller_defaults(&controller_config);controller_publish();

    // Right-click Back is event-backed: a complete tap between title polls
    // still supplies exactly one B press, and no gameplay action is remapped.
    ReleaseAll();intro_gate={};menu_mode=true;captured=false;
    TrialMenuRightButton(true);TrialMenuRightButton(false);
    assert(Poll(0).analog[1]==255);assert(Neutral(Poll(0)));
    physical_keys[VK_RBUTTON]=true;TrialMenuRightButton(true);
    assert(Poll(0).analog[1]==255);assert(Neutral(Poll(0)));
    TrialMenuRightButton(true);assert(Neutral(Poll(0))); // repeated down is not a new click
    menu_mode=false;captured=true;
    assert(Neutral(Poll(0))&&menu_right_consumed); // closing hold cannot fire secondary
    physical_keys[VK_RBUTTON]=false;TrialMenuRightButton(false);assert(Neutral(Poll(0)));
    physical_keys[VK_RBUTTON]=true;TrialMenuRightButton(true);
    const auto gameplay_right=Poll(0);assert(!Neutral(gameplay_right)&&!gameplay_right.analog[1]);
    XBOX_GAMEPAD expected_right;keyboard_apply_bindings(&expected_right,captured);
    assert(gameplay_right.buttons==expected_right.wButtons);
    for(unsigned i=0;i<8;++i)assert(gameplay_right.analog[i]==expected_right.bAnalogButtons[i]);
    physical_keys[VK_RBUTTON]=false;TrialMenuRightButton(false);Poll(0);
    // Host-owned menus consume their own ImGui click; never queue a second
    // legacy Back, including a click held while native ownership disappears.
    menu_mode=true;test_open=true;physical_keys[VK_RBUTTON]=true;TrialMenuRightButton(true);
    assert(!menu_right_pending&&menu_right_consumed&&Neutral(Poll(0)));
    test_open=false;assert(Neutral(Poll(0)));
    menu_mode=false;assert(Neutral(Poll(0)));
    physical_keys[VK_RBUTTON]=false;TrialMenuRightButton(false);Poll(0);
    // Original intro playback and focus loss do not acquire menu Back.
    menu_mode=true;intro_gate.Context(0x600000,1,true,true);
    TrialMenuRightButton(true);TrialMenuRightButton(false);assert(!menu_right_pending);
    intro_gate={};test_focused=false;TrialMenuRightButton(true);TrialMenuRightButton(false);
    assert(!menu_right_pending);test_focused=true;assert(Neutral(Poll(0)));
    ReleaseAll();
    ReleaseAll();menu_mode=true;physical_keys[VK_RETURN]=true;
    assert(Poll(0).analog[0]==255); // Original difficulty modal consumes A.
    ReleaseAll();assert(!Poll(0).analog[0]);
    // Detection must run before a native menu consumes the physical pad.
    ReleaseAll();menu_mode=true;
    for(bool native_menu:{false,true})for(unsigned control=0;control<12;++control){
        test_open=native_menu;xbox_KeyboardNoteInput();assert(xbox_KeyboardMode());
        Poll(0,Controller(control));assert(!xbox_KeyboardMode());
        // Real keyboard/mouse event handler performs this same ownership call.
        xbox_KeyboardNoteInput();Poll(0);assert(xbox_KeyboardMode());
    }
    test_open=false;test_focused=false;xbox_KeyboardNoteInput();
    Poll(0,Controller(0));assert(xbox_KeyboardMode());test_focused=true;
    // Actual adapter observes raw physical inputs before neutralizing an intro
    // pad; neither a held skip nor its closing frame can activate the next menu.
    for(int key:{VK_RETURN,VK_LBUTTON,int('R')}){
        intro_gate={};ReleaseAll();menu_mode=true;
        intro_gate.Context(0x600000,1,true,true);physical_keys[key]=true;
        assert(Neutral(Poll(0)));intro_gate.Advance(.65f);assert(intro_gate.Take(0x600000,0));
        intro_gate.Close(0x600000);assert(Neutral(Poll(0)));
        intro_gate.Context(0x600000,2,true,true);assert(Neutral(Poll(0)));
        assert(!intro_gate.Take(0x600000,0));physical_keys[key]=false;Poll(0);
        physical_keys[key]=true;assert(Neutral(Poll(0)));intro_gate.Advance(.65f);assert(intro_gate.Take(0x600000,0));
        intro_gate.Close(0x600000);physical_keys[key]=false;Poll(0);
        physical_keys[key]=true;
        if(key!=VK_LBUTTON)assert(!Neutral(Poll(0))); // menu mouse goes through pointer path
    }
    for(int port=0;port<4;++port){
        intro_gate={};ReleaseAll();intro_gate.Context(0x600000,1,true,true);
        assert(Neutral(Poll(port,Controller(1)))&&intro_gate.Take(0x600000,unsigned(port)));
        intro_gate.Close(0x600000);assert(Neutral(Poll(port,Controller(1))));
        Poll(port);assert(Poll(port,Controller(1)).buttons&16);
    }
    intro_gate={};ReleaseAll();
    // The accepted CONTROL action adapter capped the full digital diagonal.
    // Compare the actual pad adapter against that exact helper for every WASD
    // combination, including opposite-key cancellation and the signed endpoint.
    gameplay=true;menu_mode=false;
    const int movement_keys[]={'A','D','W','S'};
    for(unsigned mask=0;mask<16;++mask){
        ReleaseAll();
        for(unsigned i=0;i<4;++i)physical_keys[movement_keys[i]]=(mask>>i)&1;
        XBOX_GAMEPAD old;keyboard_apply_bindings(&old,captured);
        auto p=Poll(0);
        const float x=ucrr_host_stick_value(p.lx),y=ucrr_host_stick_value(p.ly);
        if(old.sThumbLX&&old.sThumbLY){
            assert(std::abs(x-ucrr_host_stick_axis(old.sThumbLX,old.sThumbLY,0,7849))<0.00006f);
            assert(std::abs(y-ucrr_host_stick_axis(old.sThumbLX,old.sThumbLY,1,7849))<0.00006f);
            assert(x*x+y*y<=1.f&&x*x+y*y>0.9998f);
            // Releasing one direction immediately restores full straight input.
            physical_keys['A']=physical_keys['D']=false;
            assert(Poll(0).ly==old.sThumbLY);
        }else assert(p.lx==old.sThumbLX&&p.ly==old.sThumbLY);
    }
    // Rebound movement uses the same policy; keyboard look remains unchanged.
    ReleaseAll();auto saved_bindings=keyboard_config;
    keyboard_config.binding[17][0]='Z';keyboard_config.binding[18][0]='P';
    physical_keys['Z']=physical_keys['P']=physical_keys['L']=physical_keys['I']=true;
    auto rebound=Poll(0);assert(rebound.lx==23169&&rebound.ly==23169);
    assert(rebound.rx==32767&&rebound.ry==32767);keyboard_config=saved_bindings;
    // Never rescale a physical pad, another port, menu navigation or look.
    ReleaseAll();
    for(int port=0;port<4;++port)for(int sign:{-1,1}){
        input::Pad raw;raw.connected=true;raw.lx=sign*32767;raw.ly=sign*32767;
        raw.rx=-21345;raw.ry=17543;raw.buttons=64;raw.analog[3]=171;
        auto p=Poll(port,raw);
        assert(p.lx==raw.lx&&p.ly==raw.ly&&p.rx==raw.rx&&p.ry==raw.ry);
        assert(p.buttons==raw.buttons&&p.analog[3]==raw.analog[3]);
    }
    physical_keys['W']=physical_keys['D']=true;menu_mode=true;
    assert(Poll(0).lx==32767&&Poll(0).ly==32767);
    menu_mode=false;test_focused=false;assert(Neutral(Poll(0)));
    ReleaseAll();
    // A press and release between polls must reach the original title once.
    gameplay=false;menu_mode=false;captured=false;
    menu_key_taps.Event(VK_RETURN,true,false,true);
    menu_key_taps.Event(VK_RETURN,false,false,true);
    assert(Neutral(Poll(0))); // device discovery cannot consume the tap
    intro_gate.held[0]=intro_gate.consumed[0]=true; // released intro press
    TrialMenuKeysBegin();
    assert(Poll(0).buttons&16);
    assert(!intro_gate.consumed[0]);
    assert(Poll(0).buttons&16); // stable for this original input pass
    TrialMenuKeysEnd();TrialMenuKeysBegin();assert(Neutral(Poll(0)));TrialMenuKeysEnd();
    menu_mode=true;
    menu_key_taps.Event(VK_RETURN,true,false,true);menu_key_taps.Event(VK_RETURN,false,false,true);
    TrialMenuKeysBegin();assert(Poll(0).analog[0]==255);TrialMenuKeysEnd();
    TrialMenuKeysReset();TrialMenuKeysBegin();assert(Neutral(Poll(0)));TrialMenuKeysEnd();
    gameplay=true;menu_mode=false;
    menu_key_taps.Event(VK_RETURN,true,false,true);menu_key_taps.Event(VK_RETURN,false,false,true);
    TrialMenuKeysBegin();assert(Neutral(Poll(0)));TrialMenuKeysEnd();
    // Gameplay keeps a brief real key press for one original input update.
    // This neither lengthens physical holds nor invents movement between taps.
    gameplay=false;menu_mode=true;
    menu_key_taps.Event('W',true,false,true);menu_key_taps.Event('W',false,false,true);
    TrialGameplay();TrialMenuKeysBegin();assert(Neutral(Poll(0)));TrialMenuKeysEnd();
    menu_key_taps.Event('W',true,false,true);menu_key_taps.Event('W',false,false,true);
    assert(Neutral(Poll(0)));
    TrialMenuKeysBegin();assert(Poll(0).ly==32767);assert(Poll(0).ly==32767);TrialMenuKeysEnd();
    TrialMenuKeysBegin();assert(Neutral(Poll(0)));TrialMenuKeysEnd();
    menu_key_taps.Event('W',true,false,true);menu_key_taps.Event('W',false,false,true);
    menu_key_taps.Event('D',true,false,true);menu_key_taps.Event('D',false,false,true);
    TrialMenuKeysBegin();auto tapped_diagonal=Poll(0);
    assert(tapped_diagonal.lx==23169&&tapped_diagonal.ly==23169);TrialMenuKeysEnd();
    TrialMenuKeysBegin();assert(Neutral(Poll(0)));TrialMenuKeysEnd();
    // A pause/resume handoff cannot carry an old key into its new owner.
    menu_key_taps.Event(VK_ESCAPE,true,false,true);menu_key_taps.Event(VK_ESCAPE,false,false,true);
    TrialMenuKeysBegin();assert(Poll(0).buttons&16);TrialMenuKeysEnd();
    TrialMenuKeysBegin();assert(Neutral(Poll(0)));TrialMenuKeysEnd();
    menu_key_taps.Event('W',true,false,true);menu_key_taps.Event('W',false,false,true);
    ucrr_host_menu_mode(1);TrialMenuKeysBegin();assert(Neutral(Poll(0)));TrialMenuKeysEnd();
    menu_key_taps.Event('R',true,false,true);menu_key_taps.Event('R',false,false,true);
    ucrr_host_menu_mode(0);TrialMenuKeysBegin();assert(Neutral(Poll(0)));TrialMenuKeysEnd();
    for(bool native_owner:{false,true}){
        menu_key_taps.Event('W',true,false,true);menu_key_taps.Event('W',false,false,true);
        test_open=native_owner;test_focused=native_owner;
        TrialMenuKeysBegin();assert(Neutral(Poll(0)));TrialMenuKeysEnd();
        test_open=false;test_focused=true;
        TrialMenuKeysBegin();assert(Neutral(Poll(0)));TrialMenuKeysEnd();
    }
    ReleaseAll();
    // Simultaneous co-op devices: the second logical port keeps its original
    // analog controls and cannot steal keyboard presentation or acquire player
    // one's movement, interaction, reload, firearm or orders input.
    intro_gate={};gameplay=true;menu_mode=false;captured=true;
    keyboard_defaults(&keyboard_config);
    trial_coop::SetActive(true);
    trial_controls::LoadSecond();controller_defaults(&trial_controls::second_controller);
    for(int key:{int('W'),int('F'),int('R'),int('C'),VK_LBUTTON})physical_keys[key]=true;
    order_buttons=4;xbox_KeyboardNoteInput();TrialInputPublish();order_buttons=4;
    auto first=Poll(0);
    assert(first.ly==32767&&first.analog[0]==255&&first.analog[2]==255);
    assert(first.buttons&4);assert(conversation_down&&reload_down);
    const int first_direction=TrialFirearmDirection();
    input::Pad second;second.connected=true;second.buttons=2;
    second.analog[0]=173;second.analog[2]=191;second.analog[7]=123;
    second.lx=-21000;second.ly=-23000;second.rx=26000;second.ry=-29000;
    for(unsigned pass=0;pass<3;++pass){
        const auto actual=Poll(1,second);
        assert(actual.connected&&actual.buttons==second.buttons);
        assert(!std::memcmp(actual.analog,second.analog,sizeof second.analog));
        assert(actual.lx==second.lx&&actual.ly==second.ly&&actual.rx==second.rx&&actual.ry==second.ry);
        assert(xbox_KeyboardMode()&&TrialFirearmDirection()==first_direction);
    }
    assert(Neutral(Poll(1))); // A disconnected second controller gains no keys.
    ReleaseAll();captured=true;physical_keys['H']=true;Poll(0);
    assert(health_down[0]&&health_allowed[0]&&!health_down[1]);
    trial_controls::health_loaded[1]=true;trial_controls::health_binding[1][1][0]=12;
    second={};second.connected=true;second.buttons=128;Poll(1,second);
    assert(health_down[1]&&health_allowed[1]);
    test_open=true;Poll(0);Poll(1,second);
    assert(!health_down[0]&&!health_down[1]&&!health_allowed[0]&&!health_allowed[1]);
    test_open=false;test_focused=false;Poll(0);Poll(1,second);
    assert(!health_allowed[0]&&!health_allowed[1]);test_focused=true;
    trial_coop::Reset();order_buttons=0;ReleaseAll();
    for(unsigned player=0;player<2;++player){
        ReleaseAll();trial_coop::SetActive(true);trial_coop::ResetPauseOwner();
        trial_coop::pause_state.Sample(false,false);trial_coop::pause_state.Sample(player==0,player==1);
        trial_coop::pause_state.Consume(true);trial_coop::pause_state.Enter();
        xbox_KeyboardNoteInput();assert(TrialGameplayKeyboardMode()&&xbox_KeyboardMode()==int(player==0));
        menu_mode=true;gameplay=true;captured=false;Poll(0);Poll(1); // ownership handoff
        const unsigned calls=menu_pad_calls;
        input::Pad controller;controller.connected=true;controller.analog[0]=255;controller.ly=25000;
        physical_keys[VK_RETURN]=true;physical_keys['W']=true;
        const auto key=Poll(0),pad=Poll(1,controller);
        if(player==1){
            assert(Neutral(key)&&!Neutral(pad));assert(!keyboard_key_down(VK_RETURN));
            assert(menu_pad_calls==calls+1&&menu_pad_connected);
            menu_right_pending=true;assert(!TrialMenuRightBack());
        }else{
            assert(key.analog[0]==255&&Neutral(pad));assert(keyboard_key_down(VK_RETURN));
            assert(menu_pad_calls==calls+1&&!menu_pad_connected);
        }
        assert(trial_coop::PauseOwner()==int(player));
        test_open=true;assert(Neutral(Poll(0))&&Neutral(Poll(1,controller)));
        trial_coop::ResetPauseOwner();ReleaseAll();assert(!trial_coop::PauseBlocksKeyboard()&&xbox_KeyboardMode());
        trial_coop::Reset();
    }
    std::puts("graphics input: actual pad/keyboard ownership, focus, all ports, held keys/clicks/sticks and release handoff passed");
}

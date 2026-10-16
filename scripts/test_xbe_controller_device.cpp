#include "app/app.h"
#include "core/input.h"
#include <SDL3/SDL.h>
#include "xbe_controller_device.h"
#include <cstdio>
#include <cstdlib>
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %d: %s (%s)\n",__LINE__,#x,SDL_GetError());std::exit(1);}}while(0)
namespace app { extern const GameInfo kGame={}; }
int main(){
    CHECK(trial_controller::Identify(SDL_GAMEPAD_TYPE_PS4)==trial_controller::PS4);
    CHECK(trial_controller::Identify(SDL_GAMEPAD_TYPE_PS5)==trial_controller::PS5);
    CHECK(trial_controller::Identify(SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO)==trial_controller::Nintendo);
    CHECK(trial_controller::Identify(SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR)==trial_controller::Nintendo);
    CHECK(trial_controller::Identify(SDL_GAMEPAD_TYPE_UNKNOWN)==trial_controller::Xbox);
    input::Init();
    CHECK(!SDL_GetHintBoolean("SDL_GAMECONTROLLER_USE_BUTTON_LABELS",true));
    CHECK(SDL_GetGamepadButtonLabelForType(SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO,SDL_GAMEPAD_BUTTON_SOUTH)==SDL_GAMEPAD_BUTTON_LABEL_B);
    CHECK(SDL_GetGamepadButtonLabelForType(SDL_GAMEPAD_TYPE_PS5,SDL_GAMEPAD_BUTTON_WEST)==SDL_GAMEPAD_BUTTON_LABEL_SQUARE);
    CHECK(SDL_GetHintBoolean(SDL_HINT_XINPUT_ENABLED,true));
    SDL_VirtualJoystickDesc desc{};SDL_INIT_INTERFACE(&desc);
    desc.type=SDL_JOYSTICK_TYPE_GAMEPAD;desc.naxes=SDL_GAMEPAD_AXIS_COUNT;
    desc.nbuttons=SDL_GAMEPAD_BUTTON_COUNT;desc.name="Burner input fixture";
    const auto id=SDL_AttachVirtualJoystick(&desc);CHECK(id);
    auto* joystick=SDL_OpenJoystick(id);CHECK(joystick);
    SDL_Delay(3);int port=-1;
    // Use a free port without touching any attached physical controller.
    CHECK(SDL_SetJoystickVirtualButton(joystick,SDL_GAMEPAD_BUTTON_SOUTH,true));
    CHECK(SDL_SetJoystickVirtualButton(joystick,SDL_GAMEPAD_BUTTON_DPAD_DOWN,true));
    CHECK(SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_LEFTX,20000));
    CHECK(SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_LEFTY,-20000));
    SDL_UpdateJoysticks();SDL_Delay(3);
    for(int i=0;i<4;++i){auto p=input::Poll(i);if(p.analog[0]==255&&(p.buttons&2)&&p.lx==20000){port=i;CHECK(p.ly==19999);}}
    CHECK(port>=0);
    CHECK(SDL_SetJoystickVirtualButton(joystick,SDL_GAMEPAD_BUTTON_SOUTH,false));
    CHECK(SDL_SetJoystickVirtualButton(joystick,SDL_GAMEPAD_BUTTON_DPAD_DOWN,false));
    SDL_UpdateJoysticks();SDL_Delay(3);auto p=input::Poll(port);CHECK(!p.analog[0]&&!(p.buttons&2));
    SDL_CloseJoystick(joystick);CHECK(SDL_DetachVirtualJoystick(id));SDL_Delay(3);
    CHECK(!input::Poll(port).connected);
    std::puts("PASS production SDL controller discovery, buttons, signed axes, release and disconnect; physical pad acceptance remains separate");
}

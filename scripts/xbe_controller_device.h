#pragma once
#include <SDL3/SDL.h>
#include "xbe_controller_style.h"
namespace trial_controller {
inline unsigned Identify(SDL_GamepadType type){
    switch(type){
    case SDL_GAMEPAD_TYPE_PS3:case SDL_GAMEPAD_TYPE_PS4:return PS4;
    case SDL_GAMEPAD_TYPE_PS5:return PS5;
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO:
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT:
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT:
    case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:return Nintendo;
    default:return Xbox;
    }
}
}

#pragma once
#include <windows.h>
#include <algorithm>

// Window-delivered state for handoff guards; unrelated desktop state cannot block menus.
struct TrialWindowKeys {
    bool down[256]{};
    bool Held(unsigned key)const{return key<256&&down[key];}
    void Message(UINT msg,WPARAM wp,LPARAM lp,bool focused){
        if(msg==WM_KILLFOCUS||(msg==WM_ACTIVATEAPP&&!wp)){std::fill_n(down,256,false);return;}
        unsigned key=0;bool pressed=false;
        if(msg==WM_KEYDOWN||msg==WM_KEYUP||msg==WM_SYSKEYDOWN||msg==WM_SYSKEYUP){
            key=unsigned(wp);pressed=msg==WM_KEYDOWN||msg==WM_SYSKEYDOWN;
            if(key==VK_SHIFT)key=MapVirtualKeyW((unsigned(lp)>>16)&255,MAPVK_VSC_TO_VK_EX);
            if(key==VK_CONTROL)key=(lp&(1LL<<24))?VK_RCONTROL:VK_LCONTROL;
            if(key==VK_MENU)key=(lp&(1LL<<24))?VK_RMENU:VK_LMENU;
        }else switch(msg){
            case WM_LBUTTONDOWN:case WM_LBUTTONDBLCLK:pressed=true;[[fallthrough]];
            case WM_LBUTTONUP:key=VK_LBUTTON;break;
            case WM_RBUTTONDOWN:case WM_RBUTTONDBLCLK:pressed=true;[[fallthrough]];
            case WM_RBUTTONUP:key=VK_RBUTTON;break;
            case WM_MBUTTONDOWN:case WM_MBUTTONDBLCLK:pressed=true;[[fallthrough]];
            case WM_MBUTTONUP:key=VK_MBUTTON;break;
            case WM_XBUTTONDOWN:case WM_XBUTTONDBLCLK:pressed=true;[[fallthrough]];
            case WM_XBUTTONUP:key=HIWORD(wp)==XBUTTON1?VK_XBUTTON1:VK_XBUTTON2;break;
            default:return;
        }
        if(!key||key>=256||(!focused&&pressed))return;
        down[key]=pressed;
        down[VK_SHIFT]=down[VK_LSHIFT]||down[VK_RSHIFT];
        down[VK_CONTROL]=down[VK_LCONTROL]||down[VK_RCONTROL];
        down[VK_MENU]=down[VK_LMENU]||down[VK_RMENU];
    }
};

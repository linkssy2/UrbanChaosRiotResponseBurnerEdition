#pragma once
#include <windows.h>
#include <algorithm>
#include <stdexcept>

/* Fixed render size, ordinary windowed swapchain. Borderless fullscreen never
 * changes the desktop display mode; DXGI scales to the monitor's client area. */
struct NativeDisplay {
    enum : unsigned { resolution720=4100,resolution1080,resolution1440,fullscreenCommand,fpsCounterCommand };
    unsigned width=1280,height=720;
    bool fullscreen=false;
    bool fps_counter=true;
    HMENU menu=nullptr,options=nullptr;
    WINDOWPLACEMENT saved{sizeof(WINDOWPLACEMENT)};
    bool saved_valid=false;
    ~NativeDisplay(){if(menu)DestroyMenu(menu);}
    void create_menu(){
        menu=CreateMenu();options=CreatePopupMenu();
        if(!menu||!options||!AppendMenuW(menu,MF_POPUP,(UINT_PTR)options,L"Display")||
           !AppendMenuW(options,MF_STRING,resolution720,L"1280 x 720")||
           !AppendMenuW(options,MF_STRING,resolution1080,L"1920 x 1080")||
           !AppendMenuW(options,MF_STRING,resolution1440,L"2560 x 1440")||
           !AppendMenuW(options,MF_SEPARATOR,0,nullptr)||
           !AppendMenuW(options,MF_STRING,fullscreenCommand,L"Fullscreen\tF11")||
           !AppendMenuW(options,MF_STRING|MF_CHECKED,fpsCounterCommand,L"FPS counter\tF5"))
            throw std::runtime_error("Display menu creation failed");
    }
    bool apply(HWND window){
        MONITORINFO monitor{sizeof(monitor)};
        if(!GetMonitorInfoW(MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST),&monitor))return false;
        const bool was_full=(GetWindowLongPtrW(window,GWL_STYLE)&WS_OVERLAPPEDWINDOW)==0;
        if(fullscreen&&!was_full){saved_valid=GetWindowPlacement(window,&saved)!=FALSE;}
        SetWindowLongPtrW(window,GWL_STYLE,(GetWindowLongPtrW(window,GWL_STYLE)&WS_VISIBLE)|(fullscreen?WS_POPUP:WS_OVERLAPPEDWINDOW));
        if(!SetMenu(window,fullscreen?nullptr:menu))return false;
        RECT rect=monitor.rcMonitor;
        if(!fullscreen){
            rect={0,0,(LONG)width,(LONG)height};
            if(!AdjustWindowRectEx(&rect,WS_OVERLAPPEDWINDOW,menu!=nullptr,0))return false;
            const LONG w=std::min(rect.right-rect.left,monitor.rcWork.right-monitor.rcWork.left);
            const LONG h=std::min(rect.bottom-rect.top,monitor.rcWork.bottom-monitor.rcWork.top);
            LONG x=monitor.rcWork.left+(monitor.rcWork.right-monitor.rcWork.left-w)/2;
            LONG y=monitor.rcWork.top+(monitor.rcWork.bottom-monitor.rcWork.top-h)/2;
            if(was_full&&saved_valid){x=std::clamp(saved.rcNormalPosition.left,monitor.rcWork.left,monitor.rcWork.right-w);y=std::clamp(saved.rcNormalPosition.top,monitor.rcWork.top,monitor.rcWork.bottom-h);}
            rect={x,y,x+w,y+h};
        }
        if(options){
            CheckMenuRadioItem(options,resolution720,resolution1440,width==2560?resolution1440:width==1920?resolution1080:resolution720,MF_BYCOMMAND);
            CheckMenuItem(options,fullscreenCommand,MF_BYCOMMAND|(fullscreen?MF_CHECKED:MF_UNCHECKED));
        }
        return SetWindowPos(window,nullptr,rect.left,rect.top,rect.right-rect.left,rect.bottom-rect.top,
            SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED)!=FALSE;
    }
    bool command(unsigned id){
        if(id==fpsCounterCommand){fps_counter=!fps_counter;if(options)CheckMenuItem(options,fpsCounterCommand,MF_BYCOMMAND|(fps_counter?MF_CHECKED:MF_UNCHECKED));}
        else if(id==fullscreenCommand)fullscreen=!fullscreen;
        else if(id>=resolution720&&id<=resolution1440){
            constexpr unsigned widths[]={1280,1920,2560},heights[]={720,1080,1440};
            width=widths[id-resolution720];height=heights[id-resolution720];
        }else return false;
        return true;
    }
};

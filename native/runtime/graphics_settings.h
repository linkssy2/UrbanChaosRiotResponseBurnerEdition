#pragma once
#include "display.h"
#include "frontend.h"
#include <chrono>
#include <filesystem>

// Per-package PC preferences; never reads or writes Xbox profiles or saves.
struct NativeGraphicsSettings {
    NativeFrontendSettings value{};
    std::filesystem::path path;
    void load(const std::filesystem::path &directory){
        path=directory/L"native-settings.ini";
        auto get=[&](const wchar_t *key,unsigned fallback){return GetPrivateProfileIntW(L"Graphics",key,fallback,path.c_str());};
        auto choice=[](unsigned v,std::initializer_list<unsigned> choices,unsigned fallback){return std::find(choices.begin(),choices.end(),v)!=choices.end()?v:fallback;};
        value.height=choice(get(L"Resolution",720),{720,1080,1440},720);value.width=value.height*16/9;
        value.msaa=choice(get(L"MSAA",1),{1,2,4,8},1);
        value.anisotropy=choice(get(L"Anisotropy",1),{1,2,4,8,16},1);
        value.frame_limit=choice(get(L"FrameLimit",60),{0,30,60,120},60);
        value.fullscreen=get(L"Fullscreen",0)!=0;value.vsync=get(L"VSync",1)!=0;
        value.fps_counter=get(L"FPSCounter",1)!=0;value.lighting=get(L"Lighting",1)!=0;
        value.shadows=get(L"Shadows",1)!=0;value.color=get(L"Color",1)!=0;
    }
    bool save()const{
        bool ok=true;
        auto put=[&](const wchar_t *key,unsigned v){const auto text=std::to_wstring(v);if(!WritePrivateProfileStringW(L"Graphics",key,text.c_str(),path.c_str()))ok=false;};
        put(L"Resolution",value.height);put(L"MSAA",value.msaa);put(L"Anisotropy",value.anisotropy);put(L"FrameLimit",value.frame_limit);
        put(L"Fullscreen",value.fullscreen);put(L"VSync",value.vsync);put(L"FPSCounter",value.fps_counter);
        put(L"Lighting",value.lighting);put(L"Shadows",value.shadows);put(L"Color",value.color);return ok;
    }
};

// Absolute deadlines prevent accumulated drift. The wait is message-aware so
// menus and focus still respond at30Hz; a high-resolution Windows timer avoids
// the15.6ms Sleep rounding that affected the retired benchmark experiment.
class NativeFramePacer {
    HANDLE timer_=CreateWaitableTimerExW(nullptr,nullptr,0x2,TIMER_MODIFY_STATE|SYNCHRONIZE);
    using Clock=std::chrono::steady_clock;
    Clock::time_point deadline_{};
    unsigned previous_limit_=0;
public:
    NativeFramePacer(){if(!timer_)timer_=CreateWaitableTimerExW(nullptr,nullptr,0,TIMER_MODIFY_STATE|SYNCHRONIZE);}
    ~NativeFramePacer(){if(timer_)CloseHandle(timer_);}
    void wait(unsigned limit){
        auto now=Clock::now();
        if(!limit){deadline_=now;previous_limit_=0;return;}
        auto period=std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(1.0/limit));
        if(limit!=previous_limit_||deadline_==Clock::time_point{}||now>deadline_+period)deadline_=now;
        previous_limit_=limit;
        if(deadline_<=now){deadline_+=period;return;}
        while(Clock::now()<deadline_){
            auto ticks=std::chrono::duration_cast<std::chrono::nanoseconds>(deadline_-Clock::now()).count()/100;
            LARGE_INTEGER due{};due.QuadPart=-std::max<int64_t>(1,ticks);
            const bool armed=timer_&&SetWaitableTimer(timer_,&due,0,nullptr,nullptr,FALSE);
            if(armed)MsgWaitForMultipleObjectsEx(1,&timer_,INFINITE,QS_ALLINPUT,MWMO_INPUTAVAILABLE);
            else MsgWaitForMultipleObjectsEx(0,nullptr,1,QS_ALLINPUT,MWMO_INPUTAVAILABLE);
            MSG message{};
            while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){
                if(message.message==WM_QUIT){PostQuitMessage((int)message.wParam);return;}
                TranslateMessage(&message);DispatchMessageW(&message);
            }
        }
        deadline_+=period;
    }
};

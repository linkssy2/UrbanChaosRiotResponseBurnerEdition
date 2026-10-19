#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstdio>
#include <cstdarg>
#include <mutex>

// Opt-in, bounded diagnostics; never records key codes, text, or profile names.
namespace input_diagnostic {
inline bool Enabled(){static const bool on=GetEnvironmentVariableW(L"BURNER_INPUT_LOG",nullptr,0)>1;return on;}
inline void Log(const char* format,...){
    if(!Enabled())return;
    static std::mutex lock;std::lock_guard guard(lock);
    static FILE* file=[](){wchar_t path[32768];auto n=GetEnvironmentVariableW(L"BURNER_INPUT_LOG",path,32768);
        FILE* f=nullptr;if(n&&n<32768)_wfopen_s(&f,path,L"wb");
        if(f)std::fputs("Burner input diagnostic v1; no typed text recorded\n",f);return f;}();
    if(!file||std::ftell(file)>=2*1024*1024)return;
    std::fprintf(file,"%llu ",GetTickCount64());va_list args;va_start(args,format);std::vfprintf(file,format,args);va_end(args);
    std::fputc('\n',file);std::fflush(file);
}
struct Rate { ULONGLONG last=0; bool Due(){auto now=GetTickCount64();if(last&&now-last<1000)return false;last=now;return true;} };
}

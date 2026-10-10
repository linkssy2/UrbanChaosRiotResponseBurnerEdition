#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "coroutine.h"
#include <cstdlib>
namespace {
struct ThreadFibers {void *root=nullptr;unsigned count=0;bool converted=false;};
thread_local ThreadFibers thread;
}
NativeCoroutine::~NativeCoroutine(){if(close())std::abort();}
int NativeCoroutine::create(void (*callback)(void *,NativeCoroutine &),void *owner){
    if(fiber_||!callback)return -1;
    if(!thread.count){
        thread.converted=!IsThreadAFiber();
        thread.root=thread.converted?ConvertThreadToFiberEx(nullptr,FIBER_FLAG_FLOAT_SWITCH):GetCurrentFiber();
        if(!thread.root){thread={};return -1;}
    }
    fiber_=CreateFiberEx(0,0,FIBER_FLAG_FLOAT_SWITCH,enter,this);
    if(!fiber_){
        if(!thread.count){if(thread.converted&&!ConvertFiberToThread())std::abort();thread={};}
        return -1;
    }
    ++thread.count;thread_=GetCurrentThreadId();callback_=callback;owner_=owner;finished_=false;return 0;
}
void __stdcall NativeCoroutine::enter(void *p){
    auto &c=*static_cast<NativeCoroutine*>(p);
    try{c.callback_(c.owner_,c);}catch(...){c.failure_=std::current_exception();}
    c.finished_=true;SwitchToFiber(c.parent_);
    std::abort(); //Returning from a Windows fiber would terminate its thread.
}
int NativeCoroutine::resume(){
    if(!fiber_||thread_!=GetCurrentThreadId()||parent_||!IsThreadAFiber())return -1;
    if(!finished_){parent_=GetCurrentFiber();SwitchToFiber(fiber_);parent_=nullptr;}
    if(failure_)std::rethrow_exception(failure_);return finished_?1:0;
}
int NativeCoroutine::yield(){
    if(!fiber_||thread_!=GetCurrentThreadId()||!parent_||!IsThreadAFiber()||GetCurrentFiber()!=fiber_)return -1;
    SwitchToFiber(parent_);return 0;
}
int NativeCoroutine::close(){
    if(!fiber_)return 0;
    if(thread_!=GetCurrentThreadId()||parent_||!IsThreadAFiber()||
       (thread.count==1&&thread.converted&&GetCurrentFiber()!=thread.root))return -1;
    DeleteFiber(fiber_);fiber_=nullptr;callback_=nullptr;owner_=nullptr;thread_=0;finished_=false;failure_=nullptr;
    if(!--thread.count){if(thread.converted&&!ConvertFiberToThread())std::abort();thread={};}
    return 0;
}

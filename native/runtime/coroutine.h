#pragma once
#include <cstdint>
#include <exception>

/* Native stackful continuation for the game's1FB790/1FB930 contract. There are
 * no guest registers, guest addresses or copied guest stacks. Exclusive use on
 * the creating thread; nested resumes yield to their immediate caller.
 * Owners must release resources explicitly before closing a suspended callback:
 * closing its stack does not unwind C++ locals, just as source stack disposal.
 * Callback exceptions are retained and rethrown on the resuming caller's stack;
 * failure is sticky until close. Callback may yield but not close itself.
 * The last close restores a thread converted here; an existing fiber is borrowed.
 * These objects cannot move while live. No per-frame allocations or tracing. */
class NativeCoroutine {
public:
    NativeCoroutine()=default;
    NativeCoroutine(const NativeCoroutine&)=delete;
    NativeCoroutine&operator=(const NativeCoroutine&)=delete;
    ~NativeCoroutine();
    int create(void (*callback)(void *,NativeCoroutine &),void *owner);
    //0 yielded,1 completed,-1 invalid. Completion is sticky; no callback replay.
    int resume();
    //0 after resumption,-1 unless called by this currently running callback.
    int yield();
    //0 closed (also when empty),-1 if active, wrong thread or last close off root.
    int close();
    bool live()const{return fiber_!=nullptr;}
    bool finished()const{return finished_;}
private:
    void *fiber_=nullptr,*parent_=nullptr,*owner_=nullptr;
    void (*callback_)(void *,NativeCoroutine &)=nullptr;
    uint32_t thread_=0;
    bool finished_=false;
    std::exception_ptr failure_;
    static void __stdcall enter(void *);
};

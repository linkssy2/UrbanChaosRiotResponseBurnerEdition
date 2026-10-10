#ifndef UCRR_GPU_FENCE_PROBE_H
#define UCRR_GPU_FENCE_PROBE_H
/* Game-thread-only diagnostic. Exactly two clocks for a wait that reaches its
 * first RAM polling iteration, none on repeated polls. No GPU calls or writes. */
typedef struct UcrrFenceScope {uint64_t first,polls;} UcrrFenceScope;
static UcrrFenceScope *ucrr_fence_active;
static uint64_t ucrr_fence_calls,ucrr_fence_waits,ucrr_fence_polls;
static uint64_t ucrr_fence_ticks,ucrr_fence_max,ucrr_fence_nested;
static int ucrr_fence_enabled(void){
#if UCRR_PLAY_ONLY
    return 0;
#endif
static int e=-1;if(e<0){const char*v=getenv("UCRR_FENCE_WAIT_TRACE");e=v&&!strcmp(v,"1");}return e;}
#ifndef UCRR_FENCE_CLOCK
static uint64_t ucrr_fence_clock(void){LARGE_INTEGER q;QueryPerformanceCounter(&q);return (uint64_t)q.QuadPart;}
#define UCRR_FENCE_CLOCK() ucrr_fence_clock()
#endif
static void ucrr_fence_poll(void){
    UcrrFenceScope*s=ucrr_fence_active;if(!s)return;
    if(!s->polls)s->first=UCRR_FENCE_CLOCK();++s->polls;
}
static void ucrr_fence_finish(UcrrFenceScope*s){
    ++ucrr_fence_calls;if(!s->polls)return;
    uint64_t ticks=UCRR_FENCE_CLOCK()-s->first;
    ++ucrr_fence_waits;ucrr_fence_polls+=s->polls;ucrr_fence_ticks+=ticks;
    if(ticks>ucrr_fence_max)ucrr_fence_max=ticks;
}
static void ucrr_fence_report(void){
    static unsigned frames;if(!ucrr_fence_enabled()||++frames%120)return;
    LARGE_INTEGER f;QueryPerformanceFrequency(&f);
    fprintf(stderr,"[GUEST-FENCE-WAIT] frame=%u calls=%llu waits=%llu polls=%llu wait_ms=%.6f max_ms=%.6f nested=%llu\n",frames,
        (unsigned long long)ucrr_fence_calls,(unsigned long long)ucrr_fence_waits,(unsigned long long)ucrr_fence_polls,
        ucrr_fence_ticks*1000.0/f.QuadPart,ucrr_fence_max*1000.0/f.QuadPart,(unsigned long long)ucrr_fence_nested);
}
#endif

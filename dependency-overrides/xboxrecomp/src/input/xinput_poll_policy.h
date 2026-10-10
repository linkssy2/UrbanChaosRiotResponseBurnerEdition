#ifndef UCRR_XINPUT_POLL_POLICY_H
#define UCRR_XINPUT_POLL_POLICY_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* Physical GetState results are independent of port 0's virtual keyboard
 * success. No controller state is cached. Reconnection can take up to 1000 ms
 * after a real ERROR_DEVICE_NOT_CONNECTED, plus time until the next request.
 * Options are immutable after the first valid request, including InputInit. */
#define UCRR_XINPUT_RETRY_MS 1000u
#define UCRR_XINPUT_TRACE_REPORTS 128u
#define UCRR_XINPUT_TRACE_SAMPLES 4096u
#define UCRR_XINPUT_TRACE_SAMPLES_PER_REPORT 32u
typedef struct UcrrXInputPollSlot {
    uint64_t serial, completed_at;
    DWORD physical_result;
    unsigned physical_known;
    uint64_t requests, native_calls, skipped, results[3], status_changes;
    uint64_t in_flight, stale_completions, foreign_requests;
    uint64_t trace_begin, report_at;
    uint64_t sample_ticks[3], sample_max, slow_samples;
    unsigned samples[3], selected_samples, selected_since_report;
    unsigned sample_omitted, clock_errors, clocks, reports, trace_started;
    DWORD first_thread, last_raw_result;
} UcrrXInputPollSlot;
static UcrrXInputPollSlot ucrr_xinput_slots[XBOX_MAX_CONTROLLERS];
static INIT_ONCE ucrr_xinput_options_once=INIT_ONCE_STATIC_INIT;
static SRWLOCK ucrr_xinput_poll_lock=SRWLOCK_INIT;
static int ucrr_xinput_backoff, ucrr_xinput_trace;
static uint64_t ucrr_xinput_frequency;
static BOOL CALLBACK ucrr_xinput_options_init(PINIT_ONCE once,PVOID parameter,PVOID *context)
{
    const char *value;
    (void)once;(void)parameter;(void)context;
    value=getenv("UCRR_XINPUT_DISCONNECT_BACKOFF");ucrr_xinput_backoff=value&&!strcmp(value,"1");
#if UCRR_PLAY_ONLY
    ucrr_xinput_trace=0;
#else
    value=getenv("UCRR_XINPUT_POLL_TRACE");ucrr_xinput_trace=value&&!strcmp(value,"1");
#endif
    if(ucrr_xinput_trace){LARGE_INTEGER frequency;if(QueryPerformanceFrequency(&frequency)&&frequency.QuadPart>0)ucrr_xinput_frequency=(uint64_t)frequency.QuadPart;}
    return TRUE;
}
static void ucrr_xinput_trace_print(DWORD port,const UcrrXInputPollSlot *s)
{
    double ms=ucrr_xinput_frequency?1000.0/(double)ucrr_xinput_frequency:0;
    fprintf(stderr,"[XINPUT-POLL] port=%lu backoff=%d retry_ms=%u report=%u/%u requests=%llu native=%llu skipped=%llu success=%llu disconnected=%llu other=%llu last_raw=%lu physical_known=%u physical_result=%lu changes=%llu inflight=%llu stale=%llu first_thread=%lu foreign_requests=%llu elapsed_ms=%llu sampled_success=%u sampled_disconnected=%u sampled_other=%u success_ms=%.6f disconnected_ms=%.6f other_ms=%.6f max_ms=%.6f slow_1ms=%llu sample_omitted=%u clock_errors=%u clocks=%u (cumulative raw native-call samples; keyboard fallback excluded; not FPS)\n",
        (unsigned long)port,ucrr_xinput_backoff,UCRR_XINPUT_RETRY_MS,s->reports,UCRR_XINPUT_TRACE_REPORTS,
        (unsigned long long)s->requests,(unsigned long long)s->native_calls,(unsigned long long)s->skipped,
        (unsigned long long)s->results[0],(unsigned long long)s->results[1],(unsigned long long)s->results[2],
        (unsigned long)s->last_raw_result,s->physical_known,(unsigned long)s->physical_result,
        (unsigned long long)s->status_changes,(unsigned long long)s->in_flight,(unsigned long long)s->stale_completions,
        (unsigned long)s->first_thread,(unsigned long long)s->foreign_requests,
        (unsigned long long)(s->report_at>=s->trace_begin?s->report_at-s->trace_begin:0),
        s->samples[0],s->samples[1],s->samples[2],s->sample_ticks[0]*ms,s->sample_ticks[1]*ms,s->sample_ticks[2]*ms,
        s->sample_max*ms,(unsigned long long)s->slow_samples,s->sample_omitted,s->clock_errors,s->clocks);
}

/* Only bookkeeping is locked: never hold this lock across XInput, keyboard,
 * logging, or other callbacks. Concurrent due requests may both poll rather
 * than wait for another native call or invent a first-use disconnected result.
 * A serial prevents an older admitted poll from overwriting a newer result.
 * Existing keyboard/button snapshot ownership is deliberately unchanged. */
static DWORD ucrr_xinput_get_physical(DWORD port,XINPUT_STATE *state,int force_probe)
{
    UcrrXInputPollSlot *slot;
    UcrrXInputPollSlot report;
    uint64_t now=0,serial=0,begin=0,end=0;
    int trace_active=0,selected=0,skip=0,print_report=0,clock_ok=1;
    DWORD result;
    InitOnceExecuteOnce(&ucrr_xinput_options_once,ucrr_xinput_options_init,NULL,NULL);
    if(!ucrr_xinput_backoff&&!ucrr_xinput_trace)return XInputGetState(port,state);
    slot=&ucrr_xinput_slots[port];
    AcquireSRWLockExclusive(&ucrr_xinput_poll_lock);
    trace_active=ucrr_xinput_trace&&slot->reports<UCRR_XINPUT_TRACE_REPORTS;
    if(!ucrr_xinput_backoff&&!trace_active){ReleaseSRWLockExclusive(&ucrr_xinput_poll_lock);return XInputGetState(port,state);}
    now=GetTickCount64();
    if(trace_active){
        DWORD thread=GetCurrentThreadId();
        if(!slot->trace_started){slot->trace_started=1;slot->trace_begin=slot->report_at=now;slot->first_thread=thread;}
        ++slot->requests;slot->foreign_requests+=thread!=slot->first_thread;
    }
    skip=ucrr_xinput_backoff&&!force_probe&&slot->physical_known&&
        slot->physical_result==ERROR_DEVICE_NOT_CONNECTED&&now>=slot->completed_at&&
        now-slot->completed_at<UCRR_XINPUT_RETRY_MS;
    if(skip){if(trace_active)++slot->skipped;}
    else {
        serial=++slot->serial;
        if(trace_active){
            ++slot->native_calls;++slot->in_flight;
            /* Select BEFORE the native result: first eight, then every 64th,
             * plus the first native call after each five-second report. */
            if(slot->native_calls<=8||!(slot->native_calls&63u)||!slot->selected_since_report){
                if(slot->selected_samples<UCRR_XINPUT_TRACE_SAMPLES&&
                   slot->selected_since_report<UCRR_XINPUT_TRACE_SAMPLES_PER_REPORT&&ucrr_xinput_frequency){
                    selected=1;++slot->selected_samples;++slot->selected_since_report;slot->clocks+=2;
                }else ++slot->sample_omitted;
            }
        }
    }
    ReleaseSRWLockExclusive(&ucrr_xinput_poll_lock);
    if(skip)result=ERROR_DEVICE_NOT_CONNECTED;
    else {
        LARGE_INTEGER counter={0};
        if(selected){clock_ok=QueryPerformanceCounter(&counter)!=0&&counter.QuadPart>=0;begin=(uint64_t)counter.QuadPart;}
        result=XInputGetState(port,state);
        if(selected){clock_ok=(QueryPerformanceCounter(&counter)!=0)&&counter.QuadPart>=0&&clock_ok;end=(uint64_t)counter.QuadPart;}
        now=GetTickCount64();
    }
    AcquireSRWLockExclusive(&ucrr_xinput_poll_lock);
    if(!skip){
        unsigned category=result==ERROR_SUCCESS?0:result==ERROR_DEVICE_NOT_CONNECTED?1:2;
        if(serial==slot->serial){
            if(trace_active&&slot->physical_known&&slot->physical_result!=result)++slot->status_changes;
            slot->physical_result=result;slot->physical_known=1;slot->completed_at=now;
        }else if(trace_active)++slot->stale_completions;
        if(trace_active){
            ++slot->results[category];--slot->in_flight;slot->last_raw_result=result;
            if(selected){
                if(!clock_ok||end<begin)++slot->clock_errors;
                else {uint64_t elapsed=end-begin;++slot->samples[category];slot->sample_ticks[category]+=elapsed;
                    if(elapsed>slot->sample_max)slot->sample_max=elapsed;
                    slot->slow_samples+=elapsed>=ucrr_xinput_frequency/1000u;}
            }
        }
    }
    if(trace_active&&slot->reports<UCRR_XINPUT_TRACE_REPORTS){
        if(now<slot->report_at)slot->report_at=now;
        if(now-slot->report_at>=5000u){
            slot->report_at=now;++slot->reports;slot->selected_since_report=0;report=*slot;print_report=1;
        }
    }
    ReleaseSRWLockExclusive(&ucrr_xinput_poll_lock);
    if(print_report)ucrr_xinput_trace_print(port,&report);
    return result;
}
#endif

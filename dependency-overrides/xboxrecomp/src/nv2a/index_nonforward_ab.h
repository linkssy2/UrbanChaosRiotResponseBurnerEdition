#ifndef INDEX_NONFORWARD_AB_H
#define INDEX_NONFORWARD_AB_H
#include <stdint.h>
#include <string.h>

enum { INDEX_NONFORWARD_AB_WARMUP=3600, INDEX_NONFORWARD_AB_BOUNDARIES=600,
       INDEX_NONFORWARD_AB_PHASES=12 };
enum { INAB_NONE, INAB_WARMUP, INAB_PHASE, INAB_DONE, INAB_INVALID };
typedef struct IndexNonforwardAB {
    unsigned enabled, exhausted, started, measuring, configured, phase, remaining;
    uint64_t last_frame, frequency, last_qpc, draws, methods, element16, element32;
    unsigned have_record;
} IndexNonforwardAB;

static void index_nonforward_ab_init(IndexNonforwardAB *s,const char *option,
                                    int base_batch,int configured) {
    memset(s,0,sizeof(*s));
    s->enabled=base_batch&&option&&!strcmp(option,"1");
    s->configured=!!configured;
}
static unsigned index_nonforward_ab_mode(const IndexNonforwardAB *s) {
    if(!s->enabled||s->exhausted||!s->measuring)return s->configured;
    return s->phase%4u==1u||s->phase%4u==2u;
}
static unsigned index_nonforward_ab_invalid(IndexNonforwardAB *s) {
    s->exhausted=1;return INAB_INVALID;
}
/* Called exactly once after each completed outer Present and ++frames. Early
 * drains never advance this controller. No clocks, guest reads or rendering. */
static unsigned index_nonforward_ab_step(IndexNonforwardAB *s,uint64_t frame,int ready) {
    if(!s->enabled||s->exhausted)return INAB_NONE;
    if(!s->frequency||s->last_frame==UINT64_MAX||frame!=s->last_frame+1u)
        return index_nonforward_ab_invalid(s);
    s->last_frame=frame;
    if(!s->started){
        if(!ready)return INAB_NONE;
        s->started=1;s->remaining=INDEX_NONFORWARD_AB_WARMUP;
        return INAB_WARMUP;
    }
    if(!s->remaining)return index_nonforward_ab_invalid(s);
    if(--s->remaining)return INAB_NONE;
    if(!s->measuring){s->measuring=1;s->phase=0;}
    else if(++s->phase==INDEX_NONFORWARD_AB_PHASES){s->exhausted=1;return INAB_DONE;}
    s->remaining=INDEX_NONFORWARD_AB_BOUNDARIES;
    return INAB_PHASE;
}
/* Boundary-only raw counter observations. A decrease is invalid, never silently
 * clamped/unwrapped; draw_calls is uint32 in the renderer, widened by the caller.
 * QPC failure/nonmonotonicity likewise ends the experiment and restores config. */
static unsigned index_nonforward_ab_record(IndexNonforwardAB *s,unsigned event,
        uint64_t qpc,int clock_ok,uint64_t draws,uint64_t methods,uint64_t element16,uint64_t element32) {
    if(!event)return INAB_NONE;
    if(!clock_ok||!qpc||!s->frequency||(s->have_record&&
       (qpc<=s->last_qpc||draws<s->draws||methods<s->methods||
        element16<s->element16||element32<s->element32)))
        event=index_nonforward_ab_invalid(s);
    s->have_record=1;s->last_qpc=qpc;s->draws=draws;s->methods=methods;
    s->element16=element16;s->element32=element32;
    return event;
}
#endif

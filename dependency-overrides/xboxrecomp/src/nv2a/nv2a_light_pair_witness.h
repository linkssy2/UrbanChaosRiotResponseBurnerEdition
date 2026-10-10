#ifndef NV2A_LIGHT_PAIR_WITNESS_H
#define NV2A_LIGHT_PAIR_WITNESS_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>

/* Diagnostic only. No RAM reads, clocks, allocation, cache validity changes or
 * renderer calls. The embedding runtime serializes every entry with its lock.
 * A resource is "seen" only after a valid lighting lookup reaches the existing
 * interval test / exact comparison / cold miss. No seen resource does NOT mean
 * an empty decoder drain. Equality across a pair is NOT proof of write ownership.
 * The first lighting lookup arms capture; its partial startup window is dropped.
 * Capture then covers 64 complete windows of 300 renderer flush boundaries.
 */
enum { NV_LPW_ROWS=16, NV_LPW_REASONS=10, NV_LPW_SAMPLES=8,
       NV_LPW_WINDOW=300, NV_LPW_WINDOWS=64 };
enum { NV_LPW_INTERVAL, NV_LPW_EQUAL, NV_LPW_CHANGED, NV_LPW_COLD,
       NV_LPW_KINDS };
enum { NV_LPW_GUEST, NV_LPW_BEFORE, NV_LPW_AFTER };
typedef struct NvLightPairKey {
    uintptr_t ram;
    size_t ram_size, size;
    uint32_t offset, format, signs;
} NvLightPairKey;
typedef struct NvLightPairRow {
    NvLightPairKey key;
    uint64_t events[NV_LPW_REASONS][NV_LPW_KINDS];
    uint64_t halves[2][NV_LPW_KINDS];
    uint64_t before_pairs, after_pairs, both_seen_pairs, both_exact_pairs;
    uint64_t eligible_pairs, eligible_after_equal;
} NvLightPairRow;
typedef struct NvLightPairSample {
    uint64_t serial;
    uint32_t device, guest_stack, thread, seen[2], equal[2], changed, cold;
    uint32_t eligible, rejected;
} NvLightPairSample;
typedef struct NvLightPairWindow {
    NvLightPairRow rows[NV_LPW_ROWS];
    NvLightPairSample samples[NV_LPW_SAMPLES];
    uint32_t row_count, sample_count, index, first_frame, last_frame, boundaries;
    uint64_t attempts, completed, rejected, nested, foreign, bad_token, bad_phase;
    uint64_t no_before_resource, no_after_resource, no_resource;
    uint64_t mapping, incomplete, key_omissions, sample_omissions, outside;
    uint64_t bad_event, frame_gaps, duplicate_boundaries;
    uint64_t stranded_serial;
    uint32_t stranded_thread;
} NvLightPairWindow;
typedef struct NvLightPairWitness {
    NvLightPairWindow window;
    NvLightPairSample active;
    uint64_t next_serial, after_equal[NV_LPW_ROWS];
    uint64_t orphan_serial;
    uint32_t orphan_thread;
    uintptr_t pair_ram;
    size_t pair_ram_size;
    uint32_t phase, pair_has_mapping, armed, started, enabled, last_frame;
    uint32_t windows, serial_exhausted;
} NvLightPairWitness;

static void nv_lpw_init(NvLightPairWitness *s, int enabled) {
    memset(s,0,sizeof(*s));s->enabled=!!enabled;
}
static int nv_lpw_key_equal(const NvLightPairKey *a,const NvLightPairKey *b) {
    return a->ram==b->ram && a->ram_size==b->ram_size && a->size==b->size &&
           a->offset==b->offset && a->format==b->format && a->signs==b->signs;
}
static int nv_lpw_owner(NvLightPairWitness *s,uint32_t thread) {
    uint32_t owner=s->active.serial?s->active.thread:s->orphan_thread;
    if((s->active.serial || s->orphan_serial) && owner!=thread) {
        ++s->window.foreign;if(s->active.serial)s->active.rejected=1;return 0;
    }
    return 1;
}
static uint64_t nv_lpw_begin(NvLightPairWitness *s,uint32_t thread,
                             uint32_t device,uint32_t guest_stack) {
    if(!s->enabled)return 0;
    ++s->window.attempts;
    if(s->active.serial || s->orphan_serial) {
        ++s->window.nested;if(s->active.serial)s->active.rejected=1;
        nv_lpw_owner(s,thread);return 0;
    }
    if(s->next_serial==UINT64_MAX) {s->serial_exhausted=1;return 0;}
    memset(&s->active,0,sizeof(s->active));
    memset(s->after_equal,0,sizeof(s->after_equal));
    s->active.serial=++s->next_serial;s->active.thread=thread;
    s->active.device=device;s->active.guest_stack=guest_stack;
    s->phase=0;s->pair_has_mapping=0;
    return s->active.serial;
}
static int nv_lpw_token(NvLightPairWitness *s,uint64_t token,uint32_t thread) {
    if(!s->enabled || !token)return 0;
    if(token==s->orphan_serial) {nv_lpw_owner(s,thread);return 0;}
    if(token!=s->active.serial) {
        ++s->window.bad_token;if(s->active.serial)s->active.rejected=1;return 0;
    }
    return nv_lpw_owner(s,thread);
}
static void nv_lpw_half(NvLightPairWitness *s,uint64_t token,uint32_t thread,
                        unsigned half) {
    if(!nv_lpw_token(s,token,thread))return;
    unsigned next=half==NV_LPW_BEFORE?1:half==NV_LPW_GUEST?2:half==NV_LPW_AFTER?3:0;
    if(!next || next!=s->phase+1) {
        ++s->window.bad_phase;s->active.rejected=1;return;
    }
    s->phase=next;
}
static void nv_lpw_event(NvLightPairWitness *s,uint32_t thread,unsigned reason,
                         unsigned kind,const uint8_t *ram,size_t ram_size,
                         uint32_t offset,uint32_t format,uint32_t signs,size_t size) {
    if(!s->enabled)return;
    if(kind>=NV_LPW_KINDS || reason>=NV_LPW_REASONS || !size ||
       offset>ram_size || size>ram_size-offset) {
        ++s->window.bad_event;if(s->active.serial)s->active.rejected=1;return;
    }
    s->armed=1;
    int owner=nv_lpw_owner(s,thread);
    NvLightPairKey key={(uintptr_t)ram,ram_size,size,offset,format,signs};
    unsigned row=0;
    for(;row<s->window.row_count;++row)
        if(nv_lpw_key_equal(&s->window.rows[row].key,&key))break;
    if(row==s->window.row_count) {
        if(row==NV_LPW_ROWS) {
            ++s->window.key_omissions;if(s->active.serial)s->active.rejected=1;return;
        }
        s->window.rows[row].key=key;++s->window.row_count;
    }
    NvLightPairRow *r=&s->window.rows[row];++r->events[reason][kind];
    if(!s->active.serial) {++s->window.outside;return;}
    if(!owner)return;
    if(s->pair_has_mapping && (s->pair_ram!=(uintptr_t)ram || s->pair_ram_size!=ram_size)) {
        ++s->window.mapping;s->active.rejected=1;
    }
    s->pair_ram=(uintptr_t)ram;s->pair_ram_size=ram_size;s->pair_has_mapping=1;
    uint32_t bit=1u<<row;
    if(kind==NV_LPW_CHANGED)s->active.changed|=bit;
    if(kind==NV_LPW_COLD)s->active.cold|=bit;
    if(s->phase!=1 && s->phase!=3) {++s->window.outside;return;}
    unsigned half=s->phase==1?0:1;
    /* These are the actual public query-scope reason values, checked by the
     * production adapter's compile-time assertions. Never relabel a reason. */
    if(reason!=(half?5u:4u)) {++s->window.bad_phase;s->active.rejected=1;return;}
    ++r->halves[half][kind];s->active.seen[half]|=bit;
    if(kind==NV_LPW_EQUAL) {
        s->active.equal[half]|=bit;if(half)++s->after_equal[row];
    }
}
static void nv_lpw_finish(NvLightPairWitness *s,int incomplete) {
    NvLightPairSample *p=&s->active;
    if(!p->serial)return;
    if(incomplete) {
        ++s->window.incomplete;p->rejected=1;
        /* Keep the outstanding wrapper token after dropping its window data.
         * Otherwise a nested call before the old end could appear depth=1. An
         * unwound/nonreturning wrapper leaves a reported orphan, never a guess. */
        s->orphan_serial=p->serial;s->orphan_thread=p->thread;
    }
    if(p->rejected)++s->window.rejected;
    else {
        ++s->window.completed;
        if(!p->seen[0])++s->window.no_before_resource;
        if(!p->seen[1])++s->window.no_after_resource;
        if(!(p->seen[0]|p->seen[1]))++s->window.no_resource;
        for(unsigned i=0;i<s->window.row_count;++i) {
            uint32_t bit=1u<<i;NvLightPairRow *r=&s->window.rows[i];
            if(p->seen[0]&bit)++r->before_pairs;
            if(p->seen[1]&bit)++r->after_pairs;
            if((p->seen[0]&p->seen[1])&bit)++r->both_seen_pairs;
            if((p->equal[0]&p->equal[1])&bit) {
                ++r->both_exact_pairs;
                if(!((p->changed|p->cold)&bit)) {
                    p->eligible|=bit;++r->eligible_pairs;
                    r->eligible_after_equal+=s->after_equal[i];
                }
            }
        }
    }
    /* First four complete/rejected pairs, then reserve four for eligible pairs.
     * Aggregates include ALL pairs; omissions here concern serial samples only. */
    if(s->window.sample_count<NV_LPW_SAMPLES &&
       (s->window.sample_count<NV_LPW_SAMPLES/2 || p->eligible))
        s->window.samples[s->window.sample_count++]=*p;
    else ++s->window.sample_omissions;
    memset(p,0,sizeof(*p));s->phase=0;s->pair_has_mapping=0;
}
static void nv_lpw_end(NvLightPairWitness *s,uint64_t token,uint32_t thread) {
    if(s->enabled && token && token==s->orphan_serial) {
        if(nv_lpw_owner(s,thread)){s->orphan_serial=0;s->orphan_thread=0;}
        return;
    }
    if(!nv_lpw_token(s,token,thread))return;
    if(s->phase!=3) {++s->window.bad_phase;s->active.rejected=1;}
    nv_lpw_finish(s,0);
}
/* 0=no report, 1=arm (partial data intentionally discarded), 2=complete window.
 * A straddling pair is rejected at the boundary, never split into eligible pairs.
 * Frames count renderer flushes, NOT unique gameplay frames. */
static int nv_lpw_boundary(NvLightPairWitness *s,uint32_t frame,NvLightPairWindow *out) {
    if(!s->enabled || !s->armed)return 0;
    if(!s->started) {
        nv_lpw_finish(s,1);s->window.stranded_serial=s->orphan_serial;
        s->window.stranded_thread=s->orphan_thread;*out=s->window;
        memset(&s->window,0,sizeof(s->window));s->window.first_frame=frame;
        s->last_frame=frame;s->started=1;return 1;
    }
    if(frame==s->last_frame) {++s->window.duplicate_boundaries;return 0;}
    if(frame!=s->last_frame+1u)++s->window.frame_gaps;
    s->last_frame=frame;
    if(++s->window.boundaries<NV_LPW_WINDOW)return 0;
    nv_lpw_finish(s,1);s->window.last_frame=frame;
    s->window.stranded_serial=s->orphan_serial;s->window.stranded_thread=s->orphan_thread;
    s->window.index=++s->windows;*out=s->window;
    memset(&s->window,0,sizeof(s->window));s->window.first_frame=frame;
    if(s->windows==NV_LPW_WINDOWS)s->enabled=0;
    return 2;
}
#endif

#ifndef UCRR_BATCH_COVERAGE_H
#define UCRR_BATCH_COVERAGE_H
#include <windows.h>
/* Opt-in F8-window counts, not clocks. No increment per NOP word: retained NOP
 * presence is latched per raw group. Counts are sampled once at Present. */
enum {
    UC_BATCH_GROUPS, UC_BATCH_GUEST_DRAWS, UC_BATCH_SINGLES,
    UC_BATCH_HOST_DRAWS, UC_BATCH_NOP_GROUPS,
    UC_BATCH_NOP_BARRIERS, UC_BATCH_METHOD_BARRIERS,
    UC_BATCH_MEMORY_BARRIERS, UC_BATCH_GET_BARRIERS, UC_BATCH_POLL_BARRIERS,
    UC_BATCH_DRAW_ATTEMPTS, UC_BATCH_RAW_ATTEMPTS,
    UC_BATCH_MODE5_DRAWS, UC_BATCH_MODE6_DRAWS, UC_BATCH_MODE5_VERTICES, UC_BATCH_MODE6_VERTICES,
    UC_BATCH_BEGIN_ATTEMPTS, UC_BATCH_DISABLED, UC_BATCH_NO_COMBINER,
    UC_BATCH_INVALID_DRAW, UC_BATCH_TOPOLOGY_REJECT, UC_BATCH_COUNT_REJECT,
    UC_BATCH_SURFACE_REJECT, UC_BATCH_DEPTH_REJECT, UC_BATCH_CAPACITY_REJECT,
    UC_BATCH_JOIN_REJECT, UC_BATCH_ADMISSIONS, UC_BATCH_STRIP_GROUPS,
    UC_BATCH_STRIP_DRAWS, UC_BATCH_INDEX_HITS, UC_BATCH_INDEX_BUILDS, UC_BATCH_INDEX_FALLBACKS,
    UC_BATCH_RETAIN_GROUPS, UC_BATCH_RETAIN_GET, UC_BATCH_RETAIN_POLL, UC_BATCH_RETAIN_MEMORY,
    UC_BATCH_RETAIN_APPENDS, UC_BATCH_RETAIN_DIRTY, UC_BATCH_RETAIN_UNCERTIFIED,
    UC_BATCH_RETAIN_OBSERVERS, UC_BATCH_HOST_BARRIERS,
    UC_BATCH_COUNT
};
static const char *const uc_batch_names[UC_BATCH_COUNT]={
    "RawGroups","RawGuestDraws","RawSingles","RawHostDraws","RawNOPGroups",
    "RawNOPBarriers","RawMethodBarriers","RawMemoryBarriers","RawGETBarriers","RawPollBarriers",
    "DrawAttempts","RawDrawAttempts","Mode5Draws","Mode6Draws","Mode5Vertices","Mode6Vertices",
    "RawBeginAttempts","RawDisabled","RawNoCombiner","RawInvalidDraw","RawTopologyReject",
    "RawCountReject","RawSurfaceReject","RawDepthReject","RawCapacityReject","RawJoinReject",
    "RawAdmissions","RawStripGroups","RawStripDraws","RawIndexHits","RawIndexBuilds","RawIndexFallbacks",
    "RawRetainGroups","RawRetainGET","RawRetainPoll","RawRetainMemory","RawRetainAppends",
    "RawRetainDirty","RawRetainUncertified","RawRetainObservers","RawHostBarriers"
};
#ifdef UCRR_BATCH_COVERAGE_DEFINE
__declspec(align(64)) volatile LONG ucrr_batch_coverage_active;
__declspec(align(64)) volatile LONG64 ucrr_batch_coverage_counts[UC_BATCH_COUNT];
#else
extern volatile LONG ucrr_batch_coverage_active;
extern volatile LONG64 ucrr_batch_coverage_counts[UC_BATCH_COUNT];
#endif
static __forceinline void uc_batch_add(unsigned kind,unsigned count){
#if !UCRR_PLAY_ONLY
    if(ucrr_batch_coverage_active&&kind<UC_BATCH_COUNT&&count)
        InterlockedAdd64(&ucrr_batch_coverage_counts[kind],count);
#else
    (void)kind;(void)count;
#endif
}
#endif

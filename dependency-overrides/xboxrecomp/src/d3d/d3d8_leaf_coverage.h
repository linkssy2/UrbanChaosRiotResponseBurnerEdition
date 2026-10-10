#ifndef UCRR_LEAF_COVERAGE_H
#define UCRR_LEAF_COVERAGE_H
#include <windows.h>
/* Diagnostic only. One outcome per closed leaf call, across all guest threads.
 * Count is not duration; adjacent frame snapshots need not be simultaneous. */
#ifdef UCRR_LEAF_COVERAGE_DEFINE
__declspec(align(64)) volatile LONG ucrr_leaf_coverage_active;
__declspec(align(64)) volatile LONG64 ucrr_leaf_coverage_counts[5];
#else
extern volatile LONG ucrr_leaf_coverage_active;
extern volatile LONG64 ucrr_leaf_coverage_counts[5];
#endif
static __forceinline void uc_leaf_count(unsigned kind){
#if !UCRR_PLAY_ONLY
    if(ucrr_leaf_coverage_active)InterlockedIncrement64(&ucrr_leaf_coverage_counts[kind]);
#else
    (void)kind;
#endif
}
#endif

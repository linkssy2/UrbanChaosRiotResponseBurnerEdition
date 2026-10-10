/* Host timing observations, not a device-underrun oracle. No audio mutation. */
#ifndef AUDIO_QUEUE_TRACE_H
#define AUDIO_QUEUE_TRACE_H
#include <stdint.h>
typedef struct AudioQueueTrace {
    uint64_t submitted, last_us, covered_frames, last_played;
    uint64_t max_submit_gap_us, max_estimated_gap_us;
    unsigned estimated_gaps, clock_resets;
    int valid;
} AudioQueueTrace;

static void audio_queue_trace_submit(AudioQueueTrace *t, uint64_t now_us,
                                     uint64_t played, unsigned queued,
                                     unsigned frames)
{
    int discontinuity = played > t->submitted || (t->valid && played < t->last_played);
    if (t->valid && now_us >= t->last_us && !discontinuity) {
        uint64_t elapsed = now_us - t->last_us;
        uint64_t coverage_us = t->covered_frames * 1000000 / 48000;
        if (elapsed > t->max_submit_gap_us) t->max_submit_gap_us = elapsed;
        /* Previous full coverage is an upper bound (state queries can lag).
         * Require empty now and >1 ms beyond that bound to avoid treating
         * normal exact-boundary submissions as meaningful gaps. Device
         * scheduling/engine quantum still makes this an estimate. */
        if (!queued && elapsed > coverage_us + 1000) {
            uint64_t gap = elapsed - coverage_us;
            ++t->estimated_gaps;
            if (gap > t->max_estimated_gap_us) t->max_estimated_gap_us = gap;
        }
    } else if (t->valid) {
        ++t->clock_resets;
    }
    /* SamplesPlayed may reset after a device/voice discontinuity. Do not
     * infer silence across that boundary or allow unsigned underflow. */
    if (discontinuity) t->submitted = played;
    t->covered_frames = t->submitted - played + frames;
    t->submitted += frames;
    t->last_us = now_us;
    t->last_played = played;
    t->valid = 1;
}
#endif

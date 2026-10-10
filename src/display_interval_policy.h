#ifndef UCRR_DISPLAY_INTERVAL_POLICY_H
#define UCRR_DISPLAY_INTERVAL_POLICY_H
#include <stdint.h>
/* Explicit PC enhancement, not retail timing fidelity. The caller validates
 * complete guest ranges before supplying these values. Do not apply to the
 * tiny setter's unrelated callback-object callers, menu interval1, or unknown
 * intervals. Timers, vblank counters and simulation deltas remain original. */
static uint32_t ucrr_display_interval(int target60,uint32_t caller,
    uint32_t object,uint32_t display,uint32_t vtable,uint32_t requested)
{
    return target60 && caller==0x0005D0C8u && object==display && object!=0 &&
        vtable==0x004563E0u && requested==2 ? 1 : requested;
}
#endif

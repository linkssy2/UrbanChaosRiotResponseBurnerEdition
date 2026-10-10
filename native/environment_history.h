#ifndef UC_ENVIRONMENT_HISTORY_H
#define UC_ENVIRONMENT_HISTORY_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCEnvironmentHistoryEffect { float amount, remaining; } UCEnvironmentHistoryEffect;
typedef struct UCEnvironmentHistoryGroup {
    float minimum;
    const float *const *amounts;
    const uint8_t *const *enabled;
    uint32_t amount_count, enabled_count;
} UCEnvironmentHistoryGroup;
/* 445E0/44600/44630. The source clears amount on the tick AFTER remaining
 * reaches zero; it does not clamp remaining. Fresh reset is also445A0's fields,
 * after its external registration. Finite inputs, nearest/gradual arithmetic.
 * Returns1 success,0 failure without mutation. No registry is owned here. */
int uc_environment_history_reset(UCEnvironmentHistoryEffect *);
int uc_environment_history_set(UCEnvironmentHistoryEffect *,float amount,float duration);
int uc_environment_history_tick(UCEnvironmentHistoryEffect *,float dt);
/* 5F890 followed by installed9D480. Borrowed lists are the ORIGINAL tree's
 * in-order views, stable during this call, own group then optional immediate
 * parent (no recursive ancestor walk). Any zero enabled byte forces zero.
 * Source caps sum at0.8 BEFORE each group's minimum; final publication clamps
 * to[0,1]. Empty freshly constructed5F7B0 has minimum0 and no parent.
 * Each list<=1024, finite values, disjoint output, nearest/gradual arithmetic.
 * No allocations, native registry construction, callbacks, or surfaces here.
 * Returns1 success;0 rejects before output changes. */
int uc_environment_history_select(const UCEnvironmentHistoryGroup *group,
    const UCEnvironmentHistoryGroup *parent,float *out);
#ifdef __cplusplus
}
#endif
#endif

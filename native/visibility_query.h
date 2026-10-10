#ifndef UC_NATIVE_VISIBILITY_QUERY_H
#define UC_NATIVE_VISIBILITY_QUERY_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* A2560 manager's semantic fields. Provider is borrowed (graphics+68), next
 * is the single authoritative +8 ring cursor. GPU query slots belong to the
 * provider/backend, not this small manager. Original class-word is represented
 * by this type, not copied into native object memory. */
typedef struct UCVisibilityQuery {const void *provider;uint32_t next;} UCVisibilityQuery;
typedef struct UCVisibilityQueryResult {uint32_t state,samples;} UCVisibilityQueryResult;
typedef struct UCVisibilityQueryHooks {
    void (*activate)(void *,const void *provider);
    void (*begin)(void *);
    void (*end)(void *,uint16_t index);
    uint32_t (*data)(void *,uint16_t index,uint32_t *samples);
} UCVisibilityQueryHooks;
/* Constructor output is nonnull fresh storage, disjoint from provider. */
void uc_visibility_query_init(UCVisibilityQuery *,const void *provider);
/* A2420/A2470/A24C0 and A24A0's state projection. enabled/stall are actual
 * 4F88A8/4F88A9 low-byte control values, stable for the call. Full source ring
 * arithmetic, including low16 wrap and advance-before-activate, is retained.
 * Returned token is numeric, not a pointer or allocated object. Token0 means
 * disabled begin. End and data consume low16(token-1), even token0.
 * Poll state0=ready zero samples,1=ready nonzero,2=disabled/not ready. Original
 * caller treats full nonzero state as visible; pending is not a hidden object.
 *
 * Required backend services complete source activation/begin/end/data effects.
 * Data returns0 on success (and writes actual sample count), nonzero pending or
 * failure. Stall repeats until success, then performs ONE MORE data call exactly
 * as source. Caller must ensure eventual completion for stall; no artificial
 * retry cap or successful query result is fabricated. End result is discarded
 * by source. Backend effects cannot mutate manager, controls, hooks or provider;
 * no reentry/retirement. Source GPU allocation/completion belongs to backend.
 * Native rejection returns0 before effects; successful API return1 is distinct
 * from query's state. Outputs disjoint from borrowed inputs, remain unchanged
 * on rejection. No device/GPU implementation or B4 publication is claimed.
 */
int uc_visibility_query_begin(UCVisibilityQuery *,uint8_t enabled,const UCVisibilityQueryHooks *,void *,uint32_t *token);
int uc_visibility_query_end(const UCVisibilityQuery *,uint8_t enabled,uint32_t token,const UCVisibilityQueryHooks *,void *);
int uc_visibility_query_result(const UCVisibilityQuery *,uint8_t enabled,uint8_t stall,uint32_t token,const UCVisibilityQueryHooks *,void *,UCVisibilityQueryResult *);
#ifdef __cplusplus
}
#endif
#endif

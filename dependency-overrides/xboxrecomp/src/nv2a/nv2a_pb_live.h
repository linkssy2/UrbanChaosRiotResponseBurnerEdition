/*
 * Live NV2A push-buffer decoder.
 *
 * Recompiled titles that retain the original Xbox D3D library write native
 * NV2A commands into guest RAM rather than calling the host D3D8 shim.  This
 * module follows that command stream and forwards Kelvin methods to the
 * PGRAPH -> D3D11 translator.
 */

#ifndef NV2A_PB_LIVE_H
#define NV2A_PB_LIVE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Start a fresh live stream. Call after the host D3D device exists. */
void nv2a_pb_live_init(void);

/*
 * Consume commands written since the previous call, flush the translated
 * frame, and present it. device_pointer_va is a guest VA containing the
 * original Xbox D3DDevice pointer.
 *
 * Returns nonzero once a valid D3DDevice/push-buffer descriptor is present.
 */
int nv2a_pb_live_frame(uint8_t *guest_memory, size_t guest_memory_size,
                       uint32_t device_pointer_va);

void nv2a_pb_live_shutdown(void);
/* Consume submitted commands during a guest query wait, without presenting. */
int nv2a_pb_live_drain(uint8_t *guest_memory, size_t guest_memory_size,
                      uint32_t device_pointer_va);
int nv2a_pb_live_drain_reason(uint8_t *memory,size_t size,uint32_t device_pointer_va,unsigned reason);
void nv2a_pb_live_mark_slowdown(void);

/* Bounded first-failure evidence, also available in a no-console build.
 * reason: 1 address, 2 nested CALL, 3 invalid RETURN, 4 renderer rejection,
 * 5 owned traversal limit. Zero means no failure.
 * Read on the owning render thread (or after it has stopped), not concurrently. */
typedef struct NvPbFailureInfo {
    unsigned reason;
    uint32_t get,put;
    uint64_t internal_yields;
} NvPbFailureInfo;
void nv2a_pb_live_get_failure(NvPbFailureInfo *out);

#ifdef __cplusplus
}
#endif

#endif /* NV2A_PB_LIVE_H */

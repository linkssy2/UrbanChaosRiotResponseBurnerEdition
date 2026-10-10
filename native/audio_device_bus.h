#ifndef UC_NATIVE_AUDIO_DEVICE_BUS_H
#define UC_NATIVE_AUDIO_DEVICE_BUS_H
#include "audio_device_buffer.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCAudioMixBin {uint32_t index;int32_t volume;} UCAudioMixBin;
typedef struct UCAudioMixBins {uint32_t count;const UCAudioMixBin *pairs;} UCAudioMixBins;
typedef struct UCAudioDeviceBusDescriptor {
    uint32_t size,flags,buffer_bytes;
    const UCAudioDeviceFormat *format;
    const UCAudioMixBins *mix_bins;
    uint32_t input_mix_bin;
} UCAudioDeviceBusDescriptor;
typedef struct UCAudioDeviceBusEffects {
    void *context;
    /* Pure admission; device NULL means the prospective fresh allocation. */
    int (*preflight)(void *,const UCAudioDeviceProvider *,const UCAudioDeviceBuffer *,uint32_t);
    UCAudioDeviceBuffer *(*allocate)(void *);
    /* Completing actual backend creation, not a PCM/quiet-success substitute.
     * Descriptor/pairs are borrowed only during this call. The backend owns
     * any retained copy and its real bus lifetime. NULL is source failure. */
    void *(*create)(void *,void *backend,const UCAudioDeviceBusDescriptor *);
} UCAudioDeviceBusEffects;
/* Whole252100, nonstream flags12/14 with optional100/200. Returns0 completed,
 * 1 pure rejection. created receives the actual original AL result. A prior
 * buffer is overwritten without release exactly as source: its lifetime must
 * already be admitted by the caller. No format/PCM fallback is permitted.
 * Spatial14 preserves channel_mask and replaces copied bin10 volume with the
 * current gain; nonspatial12 publishes mask3F and retains static six-bin order.
 * Both set flags before the backend call, buffer afterward. Completing effects
 * may change represented scalar fields, preserving provider/rings/lifetimes.
 * Callbacks must not mutate transient descriptors or reenter this operation. */
int uc_audio_device_bus_setup(UCAudioDeviceBuffer *,uint32_t,const UCAudioDeviceBusEffects *,int *created);
/* Whole243970 type8 ->252A90/252100, sharing the SAME provider/device graph.
 * BackendNULL still appends/returns the wrapper, ignoring original setup AL.
 * Fresh typed allocation is required; NULL returns2 with outNULL after effects
 * (original subsequently dereferences NULL). Native guard is not rollback.
 * Allocator may update provider backend/ring while preserving validity. The
 * final append uses the captured provider, even if create changes d.provider.
 * out/device/provider/reference storage must be disjoint and remain live.
 * Source class words/native allocation layout are not represented. Final
 * backend and wrapper destruction remain the owning caller's responsibility. */
int uc_audio_device_bus_create(UCAudioDeviceProvider *,uint32_t,const UCAudioDeviceBusEffects *,UCAudioDeviceBuffer **out);
#ifdef __cplusplus
}
#endif
#endif

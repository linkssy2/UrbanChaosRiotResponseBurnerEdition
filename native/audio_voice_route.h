#ifndef UC_NATIVE_AUDIO_VOICE_ROUTE_H
#define UC_NATIVE_AUDIO_VOICE_ROUTE_H
#include "audio_device_buffer.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCAudioVoiceRouting {
    void *context;
    int (*preflight)(void *,const UCAudioInstance *,const UCAudioBuffer *,const UCAudioDeviceFormat *); /*pure0admit*/
    void (*parameter)(void *,void *backend,uint32_t); /*actual424F69(1200)*/
    void (*output)(void *,void *backend,void *output_backend); /*425F23*/
    void (*mixbins)(void *,void *backend,const uint32_t pairs[4][2]); /*425F3F*/
} UCAudioVoiceRouting;
/* Whole248B20→245640/251890 +245270/245290/252BC0. Same buffer/device graph;
 * ordinary devices only (no stream/helper). Exact2522C0 fixedmask400003 maps
 * to403 and pairs(0,0),(1,0),(10,0),(10,0) in stereo, not a guessed pan matrix.
 * Audio class interfaces/getters are actual pure2451C0/2516E0/252B40 projections.
 *
 * instance.sound is NULL or>=24bytes and immutable. Mono, after output callback,
 * requires a nonnull current parent; preflight admits all prospective live
 * parent/device/backend/ring identities and completing backend operations.
 * Current-field callbacks may change instance.buffer, buffer.flags/device/
 * spatial and device.buffer/flags; they preserve captured device/provider,
 * intrusive membership/children rings, wrapper types and lifetimes. No reentry,
 * frees or mutation of provider/ring links. Device routing moves its membership
 * between the provider ring and parent's children; there is no duplicate graph.
 * No source spatial-value owner, APU/DSP/mixer equivalence or panning invented.
 * 0 completed (including actual sound+22FFFF quiet return),1 pre-effect reject.
 */
int uc_audio_voice_route(UCAudioInstance *,UCAudioBuffer *,const UCAudioDeviceFormat *,const UCAudioVoiceRouting *);
#ifdef __cplusplus
}
#endif
#endif

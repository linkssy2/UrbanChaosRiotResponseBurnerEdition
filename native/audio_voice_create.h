#ifndef UC_NATIVE_AUDIO_VOICE_CREATE_H
#define UC_NATIVE_AUDIO_VOICE_CREATE_H
#include "audio_instance.h"
#ifdef __cplusplus
extern "C" {
#endif
/* The actual246FC0 buffer class is also the voice owner. The instance's buffer
 * and track's voice point to THIS type, not a copied graph. Original dispatch
 * words are represented by native calls; every other constructor field lives
 * here. links are original08(membership),10(children),18,20,28(instances). */
typedef struct UCAudioBuffer {
    UCAudioLink links[5];uint32_t references;
    void *owner34,*owner38,*device,*spatial;
    uint32_t flags,word48;uint16_t channels,priority;uint32_t word50;
} UCAudioBuffer;
int uc_audio_buffer_begin(UCAudioBuffer *); /* Fresh unpublished246FC0 storage. */
typedef struct UCAudioVoiceCreate {
    void *context;
    uint32_t *remaining_devices; /* SAME engine+78 count, not a private budget. */
    int (*preflight)(void *,const UCAudioInstance *,const UCAudioInstanceTrack *);
    UCAudioBuffer *(*allocate)(void *); /* actual fresh allocation, NULL allowed */
    void *(*create_device)(void *,uint32_t flags); /* completing243970 provider */
    void *(*steal_device)(void *,uint32_t flags,uint16_t priority); /*2406E0*/
    void (*initialize_device)(void *,UCAudioBuffer *); /* completing246300 */
} UCAudioVoiceCreate;
/* Actual248A60(instance,track,0)→24B0A0→240D80 ordinary voice factory, including
 * real typed allocation,246FC0 fields,2452C0 flags,245340 device request/budget,
 * and2456C0 retain-before-child-list publication. Existing track.voice must be
 * NULL (the original replacement/245200 retirement remains a separate owner).
 * instance.aux40 must be NULL. Parent buffer is NULL or a live UCAudioBuffer;
 * sound is NULL or its actual immutable >=9-byte record. Parent identity need
 * not equal track.parent: source uses the supplied instance.
 *
 * preflight is pure and admits completing allocator/device/configuration
 * effects and live reciprocal parent children list. Other services can use the
 * shared audio state/RNG but preserve input owners, metadata, lists and budget.
 * allocate returns distinct fresh stable heap storage, retained even if device
 * creation/steal returns NULL, exactly as source. Initialization is called only
 * with nonnull device and completes actual246300; no invented backend success.
 * device/steal NULL is a valid source result. No rollback or deletion is added.
 * Caller owns the exact source final lifetime (not free-immediately-on-NULL).
 * status0 source-completed;1 pre-effect rejection. Result is currenttrack.voice;
 * no playback/start/PCM ownership or generic voice exhaustion policy is claimed. */
int uc_audio_voice_create(UCAudioInstance *,UCAudioInstanceTrack *,const UCAudioVoiceCreate *);
#ifdef __cplusplus
}
#endif
#endif

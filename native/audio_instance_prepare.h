#ifndef UC_NATIVE_AUDIO_INSTANCE_PREPARE_H
#define UC_NATIVE_AUDIO_INSTANCE_PREPARE_H
#include "audio_instance.h"
#include "audio_impact_request.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCAudioInstanceBank {
    void *identity; /* same actual bank retained/published by request owner */
    const UCAudioImpactBank *metadata; /* borrowed actual mutable event/sound arrays */
    const uint8_t *track_chunks;size_t track_bytes; /* whole LIST/trkl header */
    const uint8_t *spatial;uint32_t spatial_count; /* actual amp3 records, stride40 */
} UCAudioInstanceBank;
typedef struct UCAudioInstancePrepareServices {
    void *context;
    UCAudioLink *engine_instances; /* actual singleton+28, not another voice list */
    int (*preflight)(void *,const UCAudioInstance *,const UCAudioInstanceBank *,uint32_t);
    void (*retain_bank)(void *,void *);
    void (*release_bank)(void *,void *);
    UCAudioInstanceTrack *(*allocate_track)(void *);
    void *(*buffer_auxiliary)(void *,void *buffer,uint32_t *flags); /* pure +3C,+44 */
    int16_t (*group_gain)(void *,void *group); /* actual247D30 */
    void (*set_attenuation)(void *,void *auxiliary,uint16_t); /* actual2523A0 */
    void (*set_spatial)(void *,void *auxiliary,const uint8_t *record40); /*251A80*/
} UCAudioInstancePrepareServices;
/* Full249280 ordinary AMPC104 path, including487F0 registration, actual track
 * constructors,24B050 and243F40. One existing instance graph; metadata points
 * to the SAME selected event/sound owner, not a private bank snapshot.
 * state!=0 or bankNULL completes initialized0 without reading other metadata.
 * Otherwise event index is valid and its selected sound is present. All bank,
 * event/sound/track/spatial storage stays live and immutable during this call;
 * metadata selection was already completed by24C650. No bank observer query.
 *
 * Services are required and completing after pure preflight. They may update
 * their own reference/backend state but cannot mutate this instance, graph,
 * metadata or request_flags, reenter, or invalidate any borrowed owner. The
 * track allocator returns distinct fresh typed storage. Pure preflight admits
 * successful allocation of all sound+6 tracks; <=255. This native owner owns
 * initialization/publication; caller owns the resulting allocated track graph.
 * Explicit failed allocation reports2 after the committed prefix; no rollback
 * or equivalence to the original NULL-allocation crash is claimed.
 *
 * A present spatial auxiliary receives original wrapped gain and actual amp3
 * record. These are required effects, never silent success. Full generic
 * non-AMPC resource path and later playback/stop are outside this owner.
 * Returns0 completed,1 before effects,2 committed allocation failure.
 * initialized is changed only on completed0; it does not mean audible output. */
int uc_audio_instance_prepare(UCAudioInstance *,const UCAudioInstanceBank *,
    uint32_t event_index,uint32_t *request_flags,
    const UCAudioInstancePrepareServices *,uint8_t *initialized);
#ifdef __cplusplus
}
#endif
#endif

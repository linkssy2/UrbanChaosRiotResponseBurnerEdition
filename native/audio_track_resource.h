#ifndef UC_NATIVE_AUDIO_TRACK_RESOURCE_H
#define UC_NATIVE_AUDIO_TRACK_RESOURCE_H
#include "audio_impact_request.h"
#ifdef __cplusplus
extern "C" {
#endif
/* 24D000 on the SAME live track, mutable bank selectors and session audio RNG.
 * Ordinary flags0/20/A0,4/24,64,84/A4,104; the typed command projects the original
 * upper reference word: FFFF for direct,0 for selectors. Direct refFFFF means
 * the original FFFF/FFFF no-resource pair. Mode3 retries until a different row,
 * preserving every draw. Other modes reuse the qualified weighted/shuffle/cycle
 * owners. Bank selector observers are absent; payloads are the original ordered
 * ampw list, 1..32767 rows each. All possible rows must be resident references
 * or FFFF/FFFF (no resource); subbanks/streams are not admitted.
 *
 * resolve completes actual24B760 GUID/cache lookup and returns a live READY
 * resident WAVE (virtual+34 data nonnull), or NULL. Its type/interface/readiness
 * queries are pure. It may populate that cache and consume the SAME RNG, but
 * keeps track/command/selector storage, metadata and lifetimes unchanged. It
 * observes resource=NULL/subsound=-1 before lookup; publication follows return.
 * This owner neither creates a voice nor invents successful playback.
 *
 * Inputs/storage are stable, valid and disjoint (selectors belong to bank, not
 * a copied playback graph). Rejection is before effects; completing resolver
 * cannot fail midway. Output is track.resource/subsound; all other fields stay.
 * 0 completed; nonzero rejected. Reference range/cache miss is resolved by the
 * real lookup service, not normalized into an arbitrary resource here. */
UCAudioStatus uc_audio_track_resource(UCAudioTrackState *,const UCAudioCommand *,
    const UCAudioImpactBank *,UCAudioRandom *,
    const void *(*resolve)(void *,uint16_t),void *);
#ifdef __cplusplus
}
#endif
#endif

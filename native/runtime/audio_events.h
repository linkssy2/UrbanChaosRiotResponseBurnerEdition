#pragma once
#include "audio.h"
extern "C" {
#include "../audio_event.h"
#include "../audio_selector.h"
}
/* Explicit current sound-instance/group values, not default mixer settings.
 * Fresh track-local offsets are0 (24CF10), sound gain/pitch come from amps.
 * This bridge has no live fade/group hierarchy, spatial mix or voice stealing. */
struct NativeAudioInstanceMix {
    int16_t gain,fade_gain,group_gain,pitch,group_pitch;
    bool mute;
    int16_t track_gain=0,track_pitch=0;
    // Optional real live-voice owner; the qualified event still resolves its
    // resource and consumes RNG before publishing this exact track state.
    void *owner=nullptr;const void *group=nullptr;
    void (*start)(void *,const NativeAudioSound &,const UCAudioTrackState &,
        std::shared_ptr<const NativeAudioClip>,const NativeAudioInstanceMix &)=nullptr;
};
struct NativeAudioParameters {
    int16_t sample_pitch,quantized_gain,quantized_pitch;
    float gain,frequency_ratio;
};
/* 248BF0/2440F0 rate compensation and244260/244120 conversions, stock48kHz
 * engine reference. Host PCM already has its authored sample rate, so divide
 * the48kHz-reference playback rate by that authored rate exactly once. RN only.
 * Throws on unsupported format/range; no device or mutable state touched. */
NativeAudioParameters native_audio_parameters(const NativeAudioSound &,
    const UCAudioTrackState &,const NativeAudioInstanceMix &,uint32_t sample_rate);
enum class NativeAudioPlayResult {MissingEvent,NoDirectSound,Started};
/* Actual bank name->sound->track->GUID->PCM->XAudio start, immediate resident
 * one-shots only: one ordinary track, one command at time0, no loops/choices,
 * and the observed pistol/police-shotgun event/sound policy metadata.
 * The caller has admitted its wider sound policy (no conflicting arbitration,
 * spatial routing or scheduled mix changes) and supplies current mix values.
 * Unsupported requests reject BEFORE audio RNG changes. Missing event and
 * authored soundFFFF are explicit results, not fabricated successful playback.
 * Device failure after RNG use is committed: an exception is not retryable as
 * a new event. Audio RNG is shared among audio owners, distinct from game RNG.
 * Same constructing thread as device; all borrowed owners stable for this call.
 */
NativeAudioPlayResult native_audio_play_immediate(NativeAudioDevice &,
    const NativeAudioBankAssets &,const char *event,const NativeAudioInstanceMix &,
    UCAudioRandom &);

/* Same qualified path, additionally admitting opcode3 flags0104. The bank owns
 * persistent selector payloads; mode5 selection commits inside resource
 * creation, after event callbacks, and is not rolled back on backend failure.
 * Other selector modes and subsounds remain rejected before mutable effects. */
NativeAudioPlayResult native_audio_play_cycle(NativeAudioDevice &,
    NativeAudioBankAssets &,const char *event,const NativeAudioInstanceMix &,
    UCAudioRandom &);

/* Exact LevelGlobals HeartBeep metadata: flags0, direct sound3/ref0, one
 * nonspatial command. Caller supplies the actual prepared Panel group mix.
 * Wider source configuration/instance allocation remains the same admitted
 * fresh, nonconflicting one-shot boundary as above. */
NativeAudioPlayResult native_audio_play_heartbeat(NativeAudioDevice &,
    const NativeAudioBankAssets &,const NativeAudioInstanceMix &,UCAudioRandom &);

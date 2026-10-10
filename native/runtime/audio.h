#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <filesystem>
#include <string>
#include <vector>
extern "C" {
#include "../audio_asset.h"
#include "../archive_reader/archive_reader.h"
}

using NativeAudioGuid=std::array<uint8_t,16>;
struct UCAudioDeviceBusDescriptor;
struct UCAudioRandom;
struct NativeAudioEvent {
    std::string name;
    std::array<uint8_t,16> authored{};
    uint16_t sound_index() const;
};
struct NativeAudioTrack {
    uint32_t offset=0; // Offset from the first ampt chunk, as stored in amps.
    std::vector<uint8_t> authored; // Complete ampt payload, not interpreted commands.
};
struct NativeAudioChunk {std::string tag;std::vector<uint8_t> authored;};
struct NativeAudioSound {
    std::array<uint8_t,28> authored{};
    uint32_t first_track=0,track_count=0;
};
/* Owns an immutable AMPC 0x120 resource. This bounded owner admits direct or
 * validated ampw event selection, resident GUID references and trkl tracks,
 * or the exact Default.amp metadata-only profile with no event hash.
 * It also retains validated ampw rows and opaque ampk/ampp/xfde metadata; this
 * does not execute their wider property programs.
 * It preserves command bytes; it does NOT schedule/interpret those commands or
 * implement the original mix graph, voice arbitration, music or random choices.
 * ASCII case-insensitive first matching event is equivalent to 24D2E0 after
 * validating each authored hash bucket. Loads are transactional and throw on
 * malformed/unsupported input. The source snapshot need not remain alive. */
class NativeAudioBank {
    std::vector<NativeAudioEvent> events_;
    std::vector<NativeAudioSound> sounds_;
    std::vector<NativeAudioTrack> tracks_;
    std::vector<NativeAudioGuid> references_;
    std::vector<std::vector<uint8_t>> selectors_;
    std::vector<NativeAudioChunk> auxiliary_;
public:
    void load(const void *,size_t);
    int32_t find(const char *) const;
    const std::vector<NativeAudioEvent>& events() const {return events_;}
    const std::vector<NativeAudioSound>& sounds() const {return sounds_;}
    const std::vector<NativeAudioTrack>& tracks() const {return tracks_;}
    const std::vector<NativeAudioGuid>& references() const {return references_;}
    const std::vector<std::vector<uint8_t>>& selectors() const {return selectors_;}
    const std::vector<NativeAudioChunk>& auxiliary() const {return auxiliary_;}
};
class NativeAudioClip {
    UCAudioAsset pcm_{};
    NativeAudioGuid guid_{};
    std::array<uint8_t,20> format_{};
    std::vector<uint8_t> encoded_;
public:
    NativeAudioClip(const void *,size_t);
    //Explicit preload of a validated WAVS descriptor and exact SAF span.
    //Shares the existing decoder; does not reproduce streaming timing.
    NativeAudioClip(const void *descriptor,size_t,const void *span,size_t);
    ~NativeAudioClip(){uc_audio_asset_free(&pcm_);}
    NativeAudioClip(const NativeAudioClip&)=delete;
    NativeAudioClip &operator=(const NativeAudioClip&)=delete;
    const UCAudioAsset& pcm() const {return pcm_;}
    const NativeAudioGuid& guid() const {return guid_;}
    /* Immutable original format/data, retained by the SAME resident owner.
     * Offsets in original device requests address encoded bytes, not PCM. */
    const std::array<uint8_t,20>& authored_format() const {return format_;}
    const std::vector<uint8_t>& encoded() const {return encoded_;}
};
struct NativeAudioBankAssets {
    NativeAudioBank bank;
    // Same order as amrf. Each shared owner keeps its PCM alive independently.
    std::vector<std::shared_ptr<const NativeAudioClip>> clips;
    // Persistent mutable ampw state, initialized from the immutable bank once.
    std::vector<std::vector<uint8_t>> selectors;
    std::vector<std::array<uint8_t,16>> event_state;
    //24C160 over this SAME bank's persistent event/selector state and audio RNG.
    //Returns -1 for missing event,65535 for no sound. Does not create a voice.
    int32_t select_event(const char *,uint32_t request_flags,UCAudioRandom &);
    /* Explicit local archive precedence. Scans actual WAVE/WAVS GUID chunks, never
     * infers a filename from Fire/Load. Duplicate GUIDs must have identical
     * complete RIFF bytes; missing REQUESTED references fail. WAVS needs an
     * explicitly supplied SAF file; exact GUID/format/name/prefix checks qualify
     * each read. Preloads whole clips, not original streaming timing. Empty
     * requested means all references. A nonempty subset leaves other slots null;
     * playback must reject those explicitly. Transactional. */
    void load(const UCArchive *bank_archive,size_t bank_index,
              const std::vector<const UCArchive*>& clip_archives,
              const std::vector<uint16_t>& requested={},
              const std::filesystem::path &stream_backing={});
};
/* Native Windows backend only: explicit already-resolved one-shot PCM requests.
 * Gain/frequency ratio are caller parameters, NOT reconstructed AMPC mixing.
 * No game event API until authored scheduling/gain/pitch/arbitration is owned.
 * All calls/destruction on the constructing thread; <=64 simultaneous voices.
 * Source PCM remains retained until DestroyVoice has synchronously completed.
 * Device errors throw; no silent substitute/voice stealing. No device is opened
 * until constructing this class. Zero gain permits a non-audible device test. */
class NativeAudioDevice {
    struct Impl;
    std::unique_ptr<Impl> impl_;
public:
    NativeAudioDevice();
    ~NativeAudioDevice();
    NativeAudioDevice(const NativeAudioDevice&)=delete;
    NativeAudioDevice &operator=(const NativeAudioDevice&)=delete;
    void play(std::shared_ptr<const NativeAudioClip>,float gain,float frequency_ratio);
    size_t collect();
    void stop_all();
    size_t active() const;
    /* Explicit PC output control; independent of original per-source mix. */
    void output_gain(float);
    void pause(bool); // PC engine pause retains queued samples for menu/focus resume.
    float output_gain() const;
    struct VoiceHandle {uint64_t value=0;const void *device=nullptr;};
    struct VoiceState {uint32_t buffers_queued;uint64_t samples_played;bool started;};
    /* Explicitly owned voices use this SAME engine/64-slot pool. create uses
     * the source default48kHz stereo format but neither submits nor starts.
     * prepare retains decoded PCM and queues the admitted one-shot region;
     * changing source format replaces only the PC source-voice object. It is
     * permitted only before start or after completed/stopped-and-flushed work.
     * Region units here are DECODED frames: source encoded byte offsets must be
     * converted by the resource owner, never passed through as frame numbers.
     * collect does not delete owned voices after completion. The actual game
     * graph must destroy them explicitly; stale/cross-device handles reject
     * within the device lifetime. A handle must not outlive its device owner.
     * stop_all invalidates every voice handle. PCM outlives synchronous DestroyVoice.
     * No source scheduling/arbitration/mixing or audible acceptance is implied.
     * prepare defaults to nonlooped mono/stereo PCM and gain0..1. Explicit loop
     * uses authored wsmp boundaries when present, otherwise the play region;
     * it repeats until stop/destroy. This is PC preload, not stream timing. Explicit
     * owned voices allow ratio1/1024..their chosen maximum (1..1024); default
     * maximum4 preserves old callers. Independent one-shot play stays.25..4. */
    VoiceHandle create_owned_voice(float maximum_frequency_ratio=4.f);
    void prepare_owned_voice(VoiceHandle,std::shared_ptr<const NativeAudioClip>,
                             float gain,float frequency_ratio,uint32_t first_frame,uint32_t frames,bool loop=false);
    void set_owned_parameters(VoiceHandle,float gain,float frequency_ratio);
    struct VoiceParameters {float gain,frequency_ratio;};
    /* Backend GetVolume reports latest submitted value; GetFrequencyRatio
     * reports active value and may lag until the next engine pass. */
    VoiceParameters owned_voice_parameters(VoiceHandle) const;
    struct VoiceFilter {float frequency,one_over_q;uint32_t type=0;};
    /* Real PC state-variable LowPassFilter, on the same owned source voice.
     * Only the source's symmetric mode1/Q1 profile, F[1/256,1], and an actual
     * 48k output destination are admitted. This is a PC filter adaptation,
     * not APU fixed-point/DLS2 waveform parity. Coefficients survive prepare;
     * readback includes the actual XAudio2 type (0=LowPassFilter) and may lag
     * one engine pass, including the fresh backend's default filter type. */
    void set_owned_filter(VoiceHandle,VoiceFilter);
    VoiceFilter owned_voice_filter(VoiceHandle) const;
    void start_owned_voice(VoiceHandle);
    void stop_owned_voice(VoiceHandle);
    VoiceState owned_voice_state(VoiceHandle) const;
    void destroy_owned_voice(VoiceHandle);
    struct BusHandle {uint64_t value=0;const void *device=nullptr;};
    struct BusState {uint32_t references,channels,rate;bool owner;std::array<float,6> speaker_matrix;};
    struct VoiceRouting {BusHandle destination;uint32_t headroom,channels;std::vector<float> matrix;};
    /* Exact nonspatial source0x12 descriptor only: mono48k destination, sends
     * FL/FR/FC/BL/BR at0 and LFE at-1200. A same-engine six-channel speaker bed
     * uses XAudio2's platform downmix to the actual master (explicit PC policy).
     * Spatial0x14/reverb, nested buses and arbitrary mixbins are rejected.
     * One owner ref plus one per routed voice; owner release may precede final
     * source detach. Identical routing preserves refs. Handles are device-local
     * and stale after final release. All operations use the constructing thread.
     * Source routing changes require a stopped voice. Matrix headroom is
     * independent of source gain; default0 preserves the prior PC adapter.
     * Explicit0..10000 headroom and source-rounded mono/stereo averaging survive
     * prepare's underlying SourceVoice replacement. No audible parity claim.
     * stop_all drops voice refs but preserves outstanding bus owner refs. */
    BusHandle create_owned_bus(const UCAudioDeviceBusDescriptor &);
    void release_owned_bus(BusHandle);
    BusState owned_bus_state(BusHandle) const;
    void route_owned_voice(VoiceHandle,BusHandle destination={});
    void set_owned_headroom(VoiceHandle,uint32_t millibels);
    VoiceRouting owned_voice_routing(VoiceHandle) const;
};

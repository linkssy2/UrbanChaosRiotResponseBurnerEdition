#pragma once
#include "audio.h"
extern "C" {
#include "../audio_resident_voice.h"
}
/* Fresh loaded resident WAVE projection: constructor24E6D0 defaults plus the
 * original fmt/data/wsmp reader and immutable getter fields. The existing clip
 * retains both encoded bytes and decoded PCM. This is not a second cache/graph
 * or a claim to implement the original broad resource factory/stream loader.
 * No strm(play-start), ngcc(position), mmrk chunks: the existing decoder rejects
 * them. Playback extent follows encoded data, not the shorter fact frame count.
 * Original pointers borrow this address; copy/move is deliberately forbidden. */
class NativeAudioResidentView {
    std::shared_ptr<const NativeAudioClip> clip_;
    UCAudioDeviceFormat format_{};
    UCAudioResidentWave wave_{};
public:
    explicit NativeAudioResidentView(std::shared_ptr<const NativeAudioClip>);
    NativeAudioResidentView(const NativeAudioResidentView&)=delete;
    NativeAudioResidentView& operator=(const NativeAudioResidentView&)=delete;
    NativeAudioResidentView(NativeAudioResidentView&&)=delete;
    NativeAudioResidentView& operator=(NativeAudioResidentView&&)=delete;
    const UCAudioResidentWave& wave() const{return wave_;}
    const std::shared_ptr<const NativeAudioClip>& clip() const{return clip_;}
    struct Frames {uint32_t first,count;};
    /* Explicit PC backend adaptation, Xbox ADPCM complete blocks only.
     * Rejects zero, unaligned/out-of-range regions before a device call. */
    Frames frames(uint32_t start_bytes,uint32_t length_bytes) const;
};

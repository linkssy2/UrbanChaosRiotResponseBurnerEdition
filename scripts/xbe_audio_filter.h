#pragma once
#include <cmath>
#include <cstdint>

struct UcAudioFilter {
    float frequency = 1;
    float low[2]{}, band[2]{};
};

// 251D80/244140 encode F=2*sin(pi*cutoff/48000) in signed log2/4096.
// All three title callers multiply resonance by literal zero (44C174), so
// 2441B0 produces Q=32768. Other Xbox filter modes remain unsupported.
inline bool AudioFilterSet(UcAudioFilter& state, const uint32_t packet[6]) {
    if (!packet || packet[0] != 1 || packet[1] || packet[2] > 65535 ||
        packet[2] != packet[4] || packet[3] != 32768 || packet[5] != 32768)
        return false;
    const int coefficient = packet[2] < 32768 ? int(packet[2]) : int(packet[2]) - 65536;
    if (coefficient > 0) return false;
    const float frequency = float(std::exp2(coefficient / 4096.0));
    if (frequency == 1) state = {}; // retail 8000 Hz reset: exact dry identity
    else state.frequency = frequency; // updates retain the running filter history
    return true;
}

// Standard Q=1 state-variable lowpass, on the 48 kHz mixer output of each voice
// before speaker/environment sends. This is a host adaptation, not an APU DSP
// waveform claim. Frequency follows XAUDIO2_FILTER_PARAMETERS' coefficient.
inline void AudioFilterSample(UcAudioFilter& state, float& left, float& right) {
    if (state.frequency == 1) return;
    float* samples[2] = {&left, &right};
    for (unsigned c = 0; c < 2; ++c) {
        state.low[c] += state.frequency * state.band[c];
        const float high = *samples[c] - state.low[c] - state.band[c];
        state.band[c] += state.frequency * high;
        *samples[c] = state.low[c];
    }
}

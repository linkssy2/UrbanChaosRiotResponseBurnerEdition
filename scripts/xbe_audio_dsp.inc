// Environmental sends and authored spatial bins use the title's effects image.
struct AudioEffectSegment {
    uint32_t address = 0, size = 0;
    std::vector<uint8_t> pending;
};
static std::vector<AudioEffectSegment> g_audio_effects;
static std::vector<uint32_t> g_audio_effect_allocations;
static std::vector<float> g_audio_effect_mix;
static float g_audio_listener_effect[12]{};
static bool g_audio_listener_effect_pending = false;
static std::array<uint8_t, dsp::kBins> g_audio_effect_headroom = [] {
    std::array<uint8_t, dsp::kBins> values; values.fill(1); values.back() = 0; return values;
}();

static void ResetAudioEffects() {
    dsp::Reset();
    for (uint32_t address : g_audio_effect_allocations) mem::FreeSystem(address);
    g_audio_effect_allocations.clear();
    g_audio_effects.clear();
    g_audio_effect_mix.clear();
    g_audio_listener_effect_pending = false;
}

static void CommitAudioListenerEffect() {
    if (!g_audio_listener_effect_pending) return;
    dsp::SetListener(g_audio_listener_effect);
    g_audio_listener_effect_pending = false;
}

static uint32_t SetAudioListenerEffect(uint32_t address, uint32_t apply) {
    if (apply > 1 || !TrialGuestReadable(address, 48)) return 0x80070057u;
    constexpr float minimum[] = {-10000,-10000,0,.1f,.1f,-10000,0,-10000,0,0,0,20};
    constexpr float maximum[] = {0,0,10,20,2,1000,.3f,2000,.1f,100,100,20000};
    float values[12];
    for (unsigned i = 0; i < 12; ++i) {
        const uint32_t raw = R32(address + i * 4);
        values[i] = i < 2 || i == 5 || i == 7 ? float(int32_t(raw)) : std::bit_cast<float>(raw);
        if (!std::isfinite(values[i]) || values[i] < minimum[i] || values[i] > maximum[i])
            return 0x80070057u;
    }
    std::copy_n(values, 12, g_audio_listener_effect);
    g_audio_listener_effect_pending = true;
    if (!apply) CommitAudioListenerEffect();
    return 0;
}

static AudioEffectSegment* AudioEffectState(uint32_t effect, uint32_t offset, uint32_t size) {
    if (effect >= g_audio_effects.size()) return nullptr;
    auto& state = g_audio_effects[effect];
    return offset <= state.size && size <= state.size - offset ? &state : nullptr;
}

static uint32_t SetAudioEffectData(uint32_t effect, uint32_t offset, uint32_t data,
                                 uint32_t size, uint32_t flags) {
    auto* state = AudioEffectState(effect, offset, size);
    if (!state || flags > 1 || (size && !TrialGuestReadable(data, size))) return 0x80070057u;
    if (!size) return 0;
    auto* active = static_cast<uint8_t*>(HostPtr(state->address));
    if (flags && state->pending.empty()) state->pending.assign(active, active + state->size);
    if (!flags) std::memcpy(active + offset, HostPtr(data), size);
    if (!state->pending.empty()) std::memcpy(state->pending.data() + offset, HostPtr(data), size);
    return 0;
}

static uint32_t GetAudioEffectData(uint32_t effect, uint32_t offset, uint32_t data, uint32_t size) {
    const auto* state = AudioEffectState(effect, offset, size);
    if (!state || (size && !TrialGuestReadable(data, size))) return 0x80070057u;
    if (size) std::memcpy(HostPtr(data), HostPtr(state->address + offset), size);
    return 0;
}

static void CommitAudioEffectData() {
    for (auto& state : g_audio_effects) {
        if (state.pending.empty()) continue;
        std::memcpy(HostPtr(state.address), state.pending.data(), state.size);
        state.pending.clear();
    }
}

static float AudioEffectAttenuation(const Voice& voice) {
    if (!voice.is3d || voice.mode == 2) return 1;
    float d[3];
    for (unsigned i = 0; i < 3; ++i) d[i] = voice.p[i] - (voice.mode == 1 ? 0.f : g_listener[i]);
    return AudioSpatialAttenuation(voice, d, std::sqrt(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]));
}

// Retail 2523A0 updates bin 10. Stereo slots alternate channels, and MIXINs
// inherit spatial gain without panning the environmental send a second time.
static void AudioEffectLevelsDirect(const Voice& voice, float gain[2]) {
    gain[0] = gain[1] = 0;
    if (!voice.nbins || voice.bins_empty || !voice.channels || voice.channels > 2) return;
    const float level = std::ldexp(Db(voice.bin_mb[10]) * voice.volume * voice.headroom *
                                   AudioEffectAttenuation(voice), -g_audio_effect_headroom[10]);
    for (unsigned slot = 0; slot < voice.nbins; ++slot)
        if (voice.bin[slot] == 10) gain[voice.channels == 1 ? 0 : slot % 2] += level;
}

static void AudioEffectLevels(const Voice& source, float gain[2]) {
    AudioEffectLevelsDirect(source, gain);
    float route = AudioMonoSendGain(source);
    uint32_t output = source.output;
    size_t remaining = g_voices.size();
    while (output && remaining--) {
        auto it = g_voices.find(output);
        if (it == g_voices.end()) break;
        const Voice& bus = it->second;
        float parent[2]; AudioEffectLevelsDirect(bus, parent);
        for (unsigned c = 0; c < std::min<unsigned>(source.channels, 2); ++c)
            gain[c] += route * parent[0];
        if (!bus.output) return;
        route *= AudioMonoSendGain(bus);
        output = bus.output;
    }
    if (output) gain[0] = gain[1] = 0;
}

static bool AudioHasCrosstalk(const Voice& source) {
    const Voice* voice = &source;
    size_t remaining = g_voices.size() + 1;
    while (remaining--) {
        if (!voice->bins_empty)
            for (unsigned slot = 0; slot < voice->nbins; ++slot)
                if (voice->bin[slot] >= 6 && voice->bin[slot] <= 9) return true;
        if (!voice->output) return false;
        const auto found = g_voices.find(voice->output);
        if (found == g_voices.end()) return false;
        voice = &found->second;
    }
    return false;
}

// Preserve the current spatial gain/placement and separate only the contribution
// of bins 6..9. Direct speakers/music never enter the crosstalk processors.
static void AudioSplitCrosstalk(Voice& voice, uint32_t mode, float stereo[2][2],
                                float surround[2][8], float cross[2][8]) {
    if (!AudioHasCrosstalk(voice)) return;
    g_audio_split_crosstalk = true;
    if (mode == trial_audio::Stereo) {
        float direct[2][2]; Levels(voice, direct);
        for (unsigned c = 0; c < 2; ++c) for (unsigned s = 0; s < 2; ++s) {
            cross[c][s] = stereo[c][s] - direct[c][s];
            stereo[c][s] = direct[c][s];
        }
    } else {
        float direct[2][8]; SurroundLevels(voice, mode == trial_audio::Surround71, direct);
        for (unsigned c = 0; c < 2; ++c) for (unsigned s = 0; s < 8; ++s) {
            cross[c][s] = surround[c][s] - direct[c][s];
            surround[c][s] = direct[c][s];
            if (s < 2) stereo[c][s] = direct[c][s];
        }
    }
    g_audio_split_crosstalk = false;
    constexpr unsigned bins[] = {6,7,0,0,8,9,8,9};
    for (unsigned c = 0; c < 2; ++c) for (unsigned s : {0u,1u,4u,5u,6u,7u})
        cross[c][s] = std::ldexp(cross[c][s], -g_audio_effect_headroom[bins[s]]);
}

static void AudioMixEffectSends(uint32_t frame, float a, float b, const float wet[2],
                                const float cross[2][8]) {
    float* bins = g_audio_effect_mix.data() + size_t(frame) * dsp::kBins;
    bins[10] += a * wet[0] + b * wet[1];
    constexpr unsigned outputs[] = {0,1,4,5,6,7};
    constexpr unsigned inputs[] = {6,7,8,9,dsp::kSideLeft,dsp::kSideRight};
    for (unsigned i = 0; i < 6; ++i)
        bins[inputs[i]] += a * cross[0][outputs[i]] + b * cross[1][outputs[i]];
}

static void MixAudioEffects(float* output, uint32_t frames, uint32_t channels, uint32_t mode) {
    dsp::Run(g_audio_effect_mix.data(), frames);
    for (uint32_t i = 0; i < frames; ++i) {
        const float* wet = g_audio_effect_mix.data() + size_t(i) * dsp::kBins;
        float* out = output + size_t(i) * channels;
        if (mode == trial_audio::Stereo) {
            out[0] += wet[0] + .707f * (wet[2] + wet[4]);
            out[1] += wet[1] + .707f * (wet[2] + wet[5]);
        } else {
            for (unsigned s = 0; s < 6; ++s) out[s] += wet[s];
            if (mode == trial_audio::Surround71) {
                out[6] += wet[dsp::kSideLeft]; out[7] += wet[dsp::kSideRight];
            }
        }
    }
}

static uint32_t EffectsDesc(const std::vector<uint8_t>& image, uint32_t location) {
    XDK_REQUIRE("dsound", XDK_KEY(EffectsImage_Skip0), XDK_KEY(EffectsImage_Skip1), XDK_KEY(EffectsImage_Desc));
    if (image.size() > 8u * 1024 * 1024 || !TrialGuestReadable(location, 8)) return 0;
    auto inside = [&](size_t at, size_t size) { return at <= image.size() && size <= image.size() - at; };
    auto word = [&](size_t at) { uint32_t value; std::memcpy(&value, image.data() + at, 4); return value; };
    if (!inside(XDK_EffectsImage_Skip0, 4) || !inside(XDK_EffectsImage_Skip1, 4)) return 0;
    const size_t table = size_t(XDK_EffectsImage_Desc) +
        (size_t(word(XDK_EffectsImage_Skip0)) + word(XDK_EffectsImage_Skip1)) * 4;
    if (!inside(table, 8)) return 0;
    const uint32_t count = word(table);
    if (!count || count > 64 || !inside(table + 8, size_t(count) * 40)) return 0;
    size_t allocation_bytes = 0;
    for (uint32_t i = 0; i < count; ++i) {
        for (unsigned segment = 0; segment < 4; ++segment) {
            const size_t entry = table + 8 + i * 32 + segment * 8;
            const uint32_t at = word(entry), size = word(entry + 4);
            if (size > 4u * 1024 * 1024 || (segment < 2 && !inside(at, size))) return 0;
            allocation_bytes += std::max(size, 64u);
        }
    }
    if (allocation_bytes > 8u * 1024 * 1024) return 0;
    std::vector<uint32_t> allocations, addresses, sizes;
    std::vector<AudioEffectSegment> states;
    auto allocate = [&](uint32_t bytes) {
        const uint32_t address = mem::AllocSystem(bytes);
        if (address) allocations.push_back(address);
        return address;
    };
    auto fail = [&]() { for (uint32_t address : allocations) mem::FreeSystem(address); return 0u; };
    const uint32_t descriptor = allocate(8 + count * 32);
    if (!descriptor) return fail();
    W32(descriptor, count); W32(descriptor + 4, word(table + 4));
    for (uint32_t i = 0; i < count; ++i) {
        for (unsigned segment = 0; segment < 4; ++segment) {
            const size_t entry = table + 8 + i * 32 + segment * 8;
            const uint32_t size = word(entry + 4), address = allocate(std::max(size, 64u));
            if (!address) return fail();
            std::memset(HostPtr(address), 0, std::max(size, 64u));
            if (segment == 1) {
                std::memcpy(HostPtr(address), image.data() + word(entry), size);
                states.push_back({address, size, {}});
                addresses.push_back(address); sizes.push_back(size);
            }
            W32(descriptor + 8 + i * 32 + segment * 8, address);
            W32(descriptor + 12 + i * 32 + segment * 8, size);
        }
    }
    if (!dsp::Load(image, table, addresses, sizes, int32_t(R32(location)), int32_t(R32(location + 4))))
        return fail();
    for (uint32_t address : g_audio_effect_allocations) mem::FreeSystem(address);
    g_audio_effect_mix.clear();
    g_audio_listener_effect_pending = false;
    g_audio_effect_allocations = std::move(allocations);
    g_audio_effects = std::move(states);
    return descriptor;
}

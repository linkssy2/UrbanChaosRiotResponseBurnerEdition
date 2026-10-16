// The retail title routes mono sample voices into zero-byte MIXIN buffers.
// Those buffers own its changing spatial/mix-bin gain (251890/252D20); they
// are not PCM voices and must not end merely because their data length is zero.
// All access is under g_lock. Routing owns a destination reference, so neither
// an owner Release nor a reused guest address can leave a dangling output.
static void ReleaseAudioOutput(Voice& source) {
    uint32_t output = source.output;
    source.output = 0;
    while (output) {
        auto it = g_voices.find(output);
        if (it == g_voices.end()) return;
        Voice& bus = it->second;
        if (--bus.refs > 0) return;
        output = bus.output;
        const uint32_t object = bus.obj;
        // Only zero-byte, non-stream MIXIN destinations can enter this graph.
        g_voices.erase(it);
        mem::FreeSystem(object);
    }
}

static void UpdateAudioRouteFormat(Voice& source) {
    if (source.output)
        source.route_gain = Db(int32_t(float(2000.0 * std::log10(1.0 / source.channels))));
}

static uint32_t SetAudioOutput(Voice& source, uint32_t output) {
    if (source.output == output) return 0;
    if (output) {
        auto it = g_voices.find(output);
        if (it == g_voices.end() || it->second.stream) return 0x80070057u;
        const Voice& bus = it->second;
        if (!bus.mixin || bus.bytes || bus.data || bus.channels != 1) return 0x80004001u;
        // The current stereo renderer cannot represent wider source layouts.
        if (!source.channels || source.channels > 2) return 0x80004001u;
        uint32_t node = output;
        size_t remaining = g_voices.size();
        while (node) {
            if (!remaining-- || node == source.obj) return 0x80070057u;
            auto next = g_voices.find(node);
            if (next == g_voices.end()) return 0x80070057u;
            node = next->second.output;
        }
        // Acquire first: replacing an ancestor must not delete that ancestor
        // while releasing the previous chain. Same-destination calls are inert.
        ++it->second.refs;
    }
    const bool detached = source.output != 0;
    ReleaseAudioOutput(source);
    source.output = output;
    if (output) {
        // 425183 -> 42396B(1/channels): original truncated millibel conversion,
        // not the source's previous speaker gains multiplied a second time.
        UpdateAudioRouteFormat(source);
        source.nbins = 0;
        source.bins_empty = false;
    } else if (detached) {
        // 425183 removes the old bus-input entries; 42A9CC silences empty lists.
        source.bins_empty = source.nbins == 0;
    }
    return 0;
}

static bool AudioIsMixin(uint32_t flags, uint32_t bytes) {
    return (flags & 0x82000u) == 0x2000u && bytes == 0;
}

// Direct levels remain unchanged. A routed voice has one automatic mono send;
// parent speaker, spatial, headroom and volume gains are applied once per block,
// before the existing sample loop. Never render the bus separately as PCM.
static float AudioMonoSendGain(const Voice& source) {
    float gain = source.volume * source.headroom * source.route_gain;
    if (!source.is3d || source.mode == 2) return gain;
    float d[3];
    for (unsigned i = 0; i < 3; ++i)
        d[i] = source.p[i] - (source.mode == 1 ? 0.f : g_listener[i]);
    const float distance = std::sqrt(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]);
    // SDK42A9CC uses the base distance/cone attenuation for the mono input bin;
    // spatial left/right panning belongs to the eventual destination's bins.
    return gain * AudioSpatialAttenuation(source, d, distance);
}

static void Levels(Voice& source, float g[2][2]) {
    if (!source.output) {
        if (source.bins_empty) { std::memset(g, 0, sizeof(float) * 4); return; }
        LevelsDirect(source, g);
        return;
    }
    std::memset(g, 0, sizeof(float) * 4);
    // 4250B8 keeps explicitly configured speaker sends and appends the automatic
    // destination send. Attaching itself clears old sends (425183).
    if (source.nbins) LevelsDirect(source, g);
    float gain = AudioMonoSendGain(source);
    uint32_t output = source.output;
    size_t remaining = g_voices.size();
    while (output && remaining--) {
        auto it = g_voices.find(output);
        if (it == g_voices.end()) break;
        Voice& bus = it->second;
        if (!bus.output || bus.nbins) {
            float parent[2][2]{};
            if (!bus.bins_empty) LevelsDirect(bus, parent);
            for (unsigned c = 0; c < 2; ++c)
                for (unsigned side = 0; side < 2; ++side)
                    if (c < source.channels) g[c][side] += gain * parent[0][side];
            if (!bus.output) return;
        }
        gain *= AudioMonoSendGain(bus);
        output = bus.output;
    }
    // Impossible for an admitted, referenced acyclic graph; never fall through
    // to full-volume speakers if a corrupt destination is encountered.
    std::memset(g, 0, sizeof(float) * 4);
}

static double RoutedPlaybackStep(const Voice& source) {
    double step = AudioPlaybackStep(source, !source.output);
    if (!source.output) return step;
    auto it = g_voices.find(source.output);
    if (it != g_voices.end()) {
        // 42AC20 inherits the immediate output's pitch and Doppler, not its
        // sample cursor and not each ancestor's rate recursively.
        step *= AudioPlaybackStep(it->second, false);
    }
    return std::clamp(step, std::exp2(-32767 / 4096.0), std::exp2(8191 / 4096.0));
}

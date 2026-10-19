// UC_PACKED_STRIDE10: packed Xbox positions/normals need aligned host storage.
// Other declarations keep the established raw or float4 conversion paths.
bool CanPackStride10(const State& s, const Inputs& inputs) {
    if (!kRawVertices || !(s.vs_handle & 1)) return false;
    const auto& position = inputs.layout.inputs[0];
    if (position.stream != 0 || position.type != 0x31 || position.offset != 0 ||
        !s.streams[0].vb || StrideOf(s, inputs.layout, position) != 10) return false;
    for (int k = 0; k < inputs.nregs; ++k) {
        const auto& in = inputs.layout.inputs[size_t(inputs.regs[k])];
        switch (in.type) {
            case 0x31: case 0x16: case 0x22: case 0x35: case 0x24: case 0x40: break;
            default: return false;
        }
        if (in.stream == 0) {
            if (!((in.type == 0x31 && in.offset == 0) || (in.type == 0x16 && in.offset == 6))) return false;
        } else {
            const auto host = HostFormat(in.type);
            const uint32_t stride = StrideOf(s, inputs.layout, in);
            if (!stride || stride % 4 || in.offset >= kElementOffsetEnd ||
                in.offset % std::min<uint32_t>(host.bytes, 4)) return false;
        }
    }
    return true;
}

// UC_PACKED_PARTIAL_STREAMS
bool PackStride10All(const State& s, const Inputs& inputs, const DrawRange& d, bool ranged,
                  const std::vector<uint32_t>& seq, Assembled& out) {
    using namespace kernel;
    uint32_t packed_stride = 0;
    for (int k = 0; k < inputs.nregs; ++k)
        packed_stride += (HostFormat(inputs.layout.inputs[size_t(inputs.regs[k])].type).bytes + 3u) & ~3u;
    const uint64_t byte_count = uint64_t(out.vertex_count) * packed_stride;
    if (byte_count > UINT32_MAX) return false;
    auto& packed = out.gathered[0];
    packed.resize(size_t(byte_count));
    uint32_t offset = 0;
    for (int k = 0; k < inputs.nregs; ++k) {
        const int reg = inputs.regs[k];
        const auto& in = inputs.layout.inputs[size_t(reg)];
        const auto& stream = s.streams[size_t(in.stream)];
        const auto host = HostFormat(in.type);
        const uint32_t source_bytes = TypeBytes(in.type), slot_bytes = (host.bytes + 3u) & ~3u;
        const uint32_t address = mem::VirtualFromPhysical(R32(stream.vb + kResData));
        const auto* source = static_cast<const uint8_t*>(HostPtr(address + in.offset));
        const uint32_t stride = StrideOf(s, inputs.layout, in);
        const auto copy = [&]<uint32_t Bytes>() {
            constexpr uint32_t slot = (Bytes + 3u) & ~3u;
            for (uint32_t v = 0; v < out.vertex_count; ++v) {
                const uint32_t id = ranged ? d.first_vertex + v : seq[v];
                auto* target = packed.data() + size_t(v) * packed_stride + offset;
                // SHORT3 is six source bytes but the host format reads eight.
                if constexpr (Bytes < slot) std::memset(target + Bytes, 0, slot - Bytes);
                std::memcpy(target, source + size_t(id) * stride, Bytes);
            }
        };
        switch (source_bytes) {
            case 2: copy.operator()<2>(); break;
            case 4: copy.operator()<4>(); break;
            case 6: copy.operator()<6>(); break;
            case 8: copy.operator()<8>(); break;
        }
        out.elements[size_t(reg)] = {host.format, 0, uint16_t(offset)};
        out.decode |= uint64_t(host.decode) << (reg * 4);
        offset += slot_bytes;
    }
    out.buffers[0] = {packed.data(), uint32_t(byte_count), packed_stride};
    out.buffer_count = 1;
    out.repacked = true;
    return true;
}

// UC_FUSED_STRIDE10: preserve the captured layout while copying each source once.
// Unusual/duplicate register declarations retain the generic attribute copier.
bool PackStride10Position(const State& s, const Inputs& inputs, const DrawRange& d, Assembled& out) {
    if ((inputs.nregs != 1 && inputs.nregs != 2) || inputs.regs[0] != 0) return false;
    const auto& position = inputs.layout.inputs[0];
    if (position.stream != 0 || position.type != 0x31 || position.offset != 0 ||
        StrideOf(s, inputs.layout, position) != 10) return false;
    const bool normal = inputs.nregs == 2;
    if (normal) {
        const auto& attribute = inputs.layout.inputs[1];
        if (inputs.regs[1] != 1 || attribute.stream != 0 || attribute.type != 0x16 || attribute.offset != 6)
            return false;
    }
    const uint32_t stride = normal ? 12u : 8u;
    const uint64_t bytes = uint64_t(out.vertex_count) * stride;
    if (bytes > UINT32_MAX) return false;
    auto& packed = out.gathered[0];
    packed.resize(size_t(bytes));
    using namespace kernel;
    const uint32_t address = mem::VirtualFromPhysical(R32(s.streams[0].vb + kResData));
    const auto* first = static_cast<const uint8_t*>(HostPtr(address)) + size_t(d.first_vertex) * 10;
    const auto copy = [&]<bool Normal>() {
        constexpr unsigned target_stride = Normal ? 12 : 8;
        const uint8_t* source = first;
        uint8_t* target = packed.data();
        for (uint32_t v = 0; v < out.vertex_count; ++v, source += 10, target += target_stride) {
            // A position-only stream is authored through byte five of its last vertex.
            std::memcpy(target, source, 6);
            std::memset(target + 6, 0, 2);
            if constexpr (Normal) std::memcpy(target + 8, source + 6, 4);
        }
    };
    if (normal) copy.operator()<true>();
    else copy.operator()<false>();
    const auto position_host = HostFormat(0x31);
    out.elements[0] = {position_host.format, 0, 0};
    out.decode |= uint64_t(position_host.decode);
    if (normal) {
        const auto normal_host = HostFormat(0x16);
        out.elements[1] = {normal_host.format, 0, 8};
        out.decode |= uint64_t(normal_host.decode) << 4;
    }
    out.buffers[0] = {packed.data(), uint32_t(bytes), stride};
    out.buffer_count = 1;
    out.repacked = true;
    return true;
}

// Retain aligned guest streams only when their host reads fit the authored range.
// Sparse draws and declarations needing an extra unread tail retain full packing.
bool PackStride10Partial(const State& s, const Inputs& inputs, const DrawRange& d, bool ranged,
                         const std::vector<uint32_t>& seq, Assembled& out) {
    if (!ranged) return false;
    uint32_t host_end[16]{}, authored_end[16]{}, strides[16]{}, lengths[16]{};
    int position_regs[16], position_count = 0;
    for (int k = 0; k < inputs.nregs; ++k) {
        const int reg = inputs.regs[k];
        const auto& in = inputs.layout.inputs[size_t(reg)];
        if (in.stream == 0) {
            position_regs[position_count++] = reg;
            continue;
        }
        const unsigned stream = unsigned(in.stream);
        host_end[stream] = std::max(host_end[stream], in.offset + HostFormat(in.type).bytes);
        authored_end[stream] = std::max(authored_end[stream], in.offset + TypeBytes(in.type));
        strides[stream] = StrideOf(s, inputs.layout, in);
    }
    for (unsigned stream = 1; stream < 16; ++stream) {
        if (!host_end[stream]) continue;
        if (host_end[stream] > authored_end[stream]) return false;
        const uint64_t bytes = uint64_t(out.vertex_count - 1) * strides[stream] + host_end[stream];
        if (bytes > UINT32_MAX) return false;
        lengths[stream] = uint32_t(bytes);
    }
    const Inputs position_inputs{inputs.layout, position_regs, position_count};
    if (!PackStride10Position(s, position_inputs, d, out) &&
        !PackStride10All(s, position_inputs, d, ranged, seq, out)) return false;
    int slots[16];std::fill(std::begin(slots), std::end(slots), -1);
    for (int k = 0; k < inputs.nregs; ++k) {
        const int reg = inputs.regs[k];
        const auto& in = inputs.layout.inputs[size_t(reg)];
        const unsigned stream = unsigned(in.stream);
        if (!stream) continue;
        if (slots[stream] < 0) {
            using namespace kernel;
            const uint32_t address = mem::VirtualFromPhysical(R32(s.streams[stream].vb + kResData));
            const auto* source = static_cast<const uint8_t*>(HostPtr(address));
            slots[stream] = int(out.buffer_count);
            out.buffers[out.buffer_count++] = {
                source + size_t(d.first_vertex) * strides[stream], lengths[stream], strides[stream]};
        }
        const auto host = HostFormat(in.type);
        out.elements[size_t(reg)] = {host.format, uint8_t(slots[stream]), uint16_t(in.offset)};
        out.decode |= uint64_t(host.decode) << (reg * 4);
    }
    return true;
}

bool PackStride10(const State& s, const Inputs& inputs, const DrawRange& d, bool ranged,
                  const std::vector<uint32_t>& seq, Assembled& out) {
    return PackStride10Partial(s, inputs, d, ranged, seq, out) ||
           PackStride10All(s, inputs, d, ranged, seq, out);
}

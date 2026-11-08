"""Specialize full state blocks without changing the renderer queue's deltas."""
from pathlib import Path
from prepare_xbe_perf import once
from prepare_xbe_prompt_handoff import between


SPECIAL64 = '''    // UC_STATE_COMPARE64: SendState's full blocks need exactly four unaligned loads per side.
    if (n == 64) {
        const auto a0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(a));
        const auto a1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(a + 16));
        const auto a2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(a + 32));
        const auto a3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(a + 48));
        const auto b0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(b));
        const auto b1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(b + 16));
        const auto b2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(b + 32));
        const auto b3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(b + 48));
        const auto d0 = _mm_or_si128(_mm_xor_si128(a0,b0),_mm_xor_si128(a1,b1));
        const auto d1 = _mm_or_si128(_mm_xor_si128(a2,b2),_mm_xor_si128(a3,b3));
        const auto diff = _mm_or_si128(d0,d1);
        return _mm_testz_si128(diff,diff) != 0;
    }
'''

ORIGINAL_BLOCKS = '''        for (const Range& r : ranges_)
            for (size_t off = r.off, end = r.off + r.len; off < end; off += 64)
                Delta(reinterpret_cast<const uint8_t*>(&sent_) + off, now + off, std::min<size_t>(64, end - off));'''

SPLIT64 = '''        // UC_STATE_BLOCKS64: keep full-block copies constant-sized; preserve each range's tail.
        for (const Range& r : ranges_) {
            const size_t full = r.len & ~size_t(63);
            size_t off = r.off;
            for (const size_t end = r.off + full; off < end; off += 64)
                Delta(reinterpret_cast<const uint8_t*>(&sent_) + off, now + off, 64);
            if (const size_t tail = r.len - full)
                Delta(reinterpret_cast<const uint8_t*>(&sent_) + off, now + off, tail);
        }'''

ORIGINAL_HEADER_COPY = '        std::memcpy(now, kernel::HostPtr(header), type < 0x20000 || type == 0x30000 ? 12 : kHeaderBytes);'

HEADER_COPY = '''        // UC_HEADER_COPY_SIZE: keep short headers bounded and expose constant copy lengths.
        if (type < 0x20000 || type == 0x30000)
            std::memcpy(now, kernel::HostPtr(header), 12);
        else
            std::memcpy(now, kernel::HostPtr(header), kHeaderBytes);'''


def transform(source):
    if '// UC_STATE_COMPARE64:' in source:
        assert source.count(SPECIAL64) == 1
    else:
        signature = 'bool Same(const uint8_t* a, const uint8_t* b, size_t n) {\n'
        source = once(source, signature, signature + SPECIAL64)
    if '// UC_STATE_BLOCKS64:' in source:
        assert source.count(SPLIT64) == 1 and ORIGINAL_BLOCKS not in source
    else:
        source = once(source, ORIGINAL_BLOCKS, SPLIT64)
    if '// UC_HEADER_COPY_SIZE:' in source:
        assert source.count(HEADER_COPY) == 1 and ORIGINAL_HEADER_COPY not in source
        return source
    return once(source, ORIGINAL_HEADER_COPY, HEADER_COPY)


def replay(source):
    # Reuse the production state-queue fixture; only guest header dereferencing
    # is stubbed. The candidate changes no resource/header handling.
    template = Path(__file__).with_name('test_xbe_prompt_handoff.cpp').read_text()
    transport = between(template, '// TRANSPORT_BEGIN\n', '// TRANSPORT_END\n').split('\n', 1)[1]
    for marker, value in (
        ('/* ORIGINAL_COMMAND_TYPES */', between(source, 'enum class Op :', '// the plain data in State')),
        ('/* ORIGINAL_STATE_RANGES */', between(source, 'struct Range {', 'using Clock =')),
        ('/* ORIGINAL_SEND_METHODS */', between(source, "    // a field of the render thread's State,", '    // resources by guest header:')),
        ('/* ORIGINAL_STATE_REPLAY */', between(source, '            case Op::State: {', '            case Op::Draw:')),
    ):
        transport = once(transport, marker, value)
    transport = once(transport, 'private:\n', 'public:\n')
    transport = once(transport, 'uint32_t Header(uint32_t header){assert(header==0);return header;}',
                     'uint32_t Header(uint32_t header){return header;}')
    transport = once(transport, '    std::vector<State> draws;\n', '''    std::vector<State> draws;
    void Discard() {
        cur_->cmds.clear();cur_->bytes.clear();cur_->vs.clear();cur_->ps.clear();
    }
''')
    if 'void SendVertexConstants(const State& s)' in transport:
        transport = once(transport, '    State sent_{},state_{};\n',
                         '    const State* vsc_source_ = nullptr;\n    State sent_{},state_{};\n')
    return transport


def header_replay(source):
    # Exercise the actual header/ring/cache code separately from state replay;
    # only guest-address-to-host mapping is replaced by bounded fixture memory.
    members = between(source, '    struct HeaderCopy {', '    Batch* cur_ =')
    methods = between(source, '    uint32_t Scratch(', "    // a field of the render thread's State,")
    return members + methods


def prepare(area):
    area = Path(area)
    path = area / 'src/compat/render_thread.cpp'
    source = path.read_text()
    baseline = source.replace(SPECIAL64, '')
    if '// UC_STATE_BLOCKS64:' in baseline:
        baseline = once(baseline, SPLIT64, ORIGINAL_BLOCKS)
    if '// UC_HEADER_COPY_SIZE:' in baseline:
        baseline = once(baseline, HEADER_COPY, ORIGINAL_HEADER_COPY)
    optimized = transform(source)
    assert transform(optimized) == optimized
    assert optimized.replace(SPECIAL64, '').replace(SPLIT64, ORIGINAL_BLOCKS).replace(HEADER_COPY, ORIGINAL_HEADER_COPY) == baseline
    path.write_text(optimized)
    fixture = Path(__file__).with_name('test_xbe_state_costs.cpp').read_text()
    fixture = once(fixture, '/* BASELINE_TRANSPORT */', replay(baseline))
    fixture = once(fixture, '/* OPTIMIZED_TRANSPORT */', replay(optimized))
    fixture = once(fixture, '/* HEADER_CONSTANTS */', between(source, 'constexpr uint32_t kHeaderBytes', 'enum class WaitFor'))
    fixture = once(fixture, '/* BASELINE_HEADER */', header_replay(baseline))
    fixture = once(fixture, '/* OPTIMIZED_HEADER */', header_replay(optimized))
    (area / 'src/compat/test_state_costs.cpp').write_text(fixture)

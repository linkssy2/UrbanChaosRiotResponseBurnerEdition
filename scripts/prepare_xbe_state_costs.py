"""Specialize the renderer queue's common 64-byte comparison without changing its deltas."""
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


def transform(source):
    if '// UC_STATE_COMPARE64:' in source:
        assert source.count(SPECIAL64) == 1
        return source
    signature = 'bool Same(const uint8_t* a, const uint8_t* b, size_t n) {\n'
    return once(source, signature, signature + SPECIAL64)


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
    return transport


def prepare(area):
    area = Path(area)
    path = area / 'src/compat/render_thread.cpp'
    source = path.read_text()
    baseline = source.replace(SPECIAL64, '')
    optimized = transform(source)
    assert transform(optimized) == optimized
    assert optimized.replace(SPECIAL64, '') == baseline
    path.write_text(optimized)
    fixture = Path(__file__).with_name('test_xbe_state_costs.cpp').read_text()
    fixture = once(fixture, '/* BASELINE_TRANSPORT */', replay(baseline))
    fixture = once(fixture, '/* OPTIMIZED_TRANSPORT */', replay(optimized))
    (area / 'src/compat/test_state_costs.cpp').write_text(fixture)

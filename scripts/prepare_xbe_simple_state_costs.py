"""Track the audited host simple-state writers instead of rescanning 8 KiB per draw."""
from pathlib import Path
import re

from prepare_xbe_perf import once
from prepare_xbe_prompt_handoff import between
from prepare_xbe_state_costs import replay


TRACKING = '''    // UC_SIMPLE_DIRTY_BLOCKS: producer bookkeeping, outside every serialized range.
    // Only the audited device state opts in; arbitrary/direct State users keep full comparison.
    mutable std::array<uint64_t, 2> simple_dirty{};
    bool simple_tracked = false;
    void TrackSimple() { simple_dirty.fill(~uint64_t(0)); simple_tracked = true; }
    void SetSimple(uint32_t index, uint32_t value) {
        simple[index] = value;
        if (simple_tracked) simple_dirty[index >> 10] |= uint64_t(1) << ((index >> 4) & 63);
    }
    void MarkSimpleRange(uint32_t first, uint32_t count) {
        if (!simple_tracked || first >= simple.size() || !count) return;
        if (count > simple.size() - first) count = uint32_t(simple.size()) - first;
        const uint32_t begin = first >> 4, end = (first + count - 1) >> 4;
        for (uint32_t word = begin >> 6; word <= end >> 6; ++word) {
            const unsigned low = word == begin >> 6 ? begin & 63 : 0;
            const unsigned high = word == end >> 6 ? end & 63 : 63;
            simple_dirty[word] |= (~uint64_t(0) << low) & (~uint64_t(0) >> (63 - high));
        }
    }
'''

PACKET_STORE = 'if (m >> 2 < g_state.simple.size()) g_state.simple[m >> 2] = v;'
PACKET_MARK = '''        // UC_SIMPLE_PACKET_DIRTY: bounded packet range, direct per-value stores below.
        g_state.MarkSimpleRange(method >> 2, head & 0x40000000 ? uint32_t(n != 0) : n);
'''
PACKET_REMARK = '''                // DrawUp can consume the mask in the middle of an incrementing packet.
                if (!(head & 0x40000000)) g_state.MarkSimpleRange((m >> 2) + 1, n - i - 1);
'''

SEND_SIMPLE = '''    // Tracked state has one producer/queue consumer. Headers and all other state still compare normally.
    void SendSimple(const State& s) {
        const auto* now = reinterpret_cast<const uint8_t*>(s.simple.data());
        auto* sent = reinterpret_cast<const uint8_t*>(sent_.simple.data());
        if (!s.simple_tracked || simple_source_ != &s) {
            for (size_t off = 0; off < sizeof(s.simple); off += 64)
                Delta(sent + off, now + off, 64);
        } else {
            for (size_t word = 0; word < s.simple_dirty.size(); ++word) {
                uint64_t bits = s.simple_dirty[word];
                while (bits) {
                    const size_t off = (word * 64 + std::countr_zero(bits)) * 64;
                    Delta(sent + off, now + off, 64);
                    bits &= bits - 1;
                }
            }
        }
        if (s.simple_tracked) s.simple_dirty.fill(0);
        simple_source_ = s.simple_tracked ? &s : nullptr;
    }

'''

OLD_RANGE = '{At(s, &s.simple), At(s, &s.textures) - At(s, &s.simple)}'
NEW_RANGE = '{At(s, &s.rs), At(s, &s.textures) - At(s, &s.rs)}'
MEMBER = '    const State* simple_source_ = nullptr; // UC_SIMPLE_DIRTY_BLOCKS: source identity, never queued\n'

D3D_WRITES = (
    ('g_state.simple[method >> 2] = value;', 'g_state.SetSimple(method >> 2, value);'),
    # This exact write occurs in both NotInline and the SetPixelShader put lambda.
    ('g_state.simple[method >> 2] = v;', 'g_state.SetSimple(method >> 2, v);'),
    ('g_state.simple[gpu::kMCountEnable >> 2] = v;', 'g_state.SetSimple(gpu::kMCountEnable >> 2, v);'),
)
TEXTURE_WRITES = (
    ('s.simple[(kMPsConstant0 >> 2) + i] = PackColor(s.psc[m0]);',
     's.SetSimple((kMPsConstant0 >> 2) + i, PackColor(s.psc[m0]));'),
    ('s.simple[(kMPsConstant1 >> 2) + i] = PackColor(s.psc[m1]);',
     's.SetSimple((kMPsConstant1 >> 2) + i, PackColor(s.psc[m1]));'),
    ('s.simple[kMPsFinalConstant0 >> 2] = PackColor(s.psc[f0]);',
     's.SetSimple(kMPsFinalConstant0 >> 2, PackColor(s.psc[f0]));'),
    ('s.simple[kMPsFinalConstant1 >> 2] = PackColor(s.psc[f1]);',
     's.SetSimple(kMPsFinalConstant1 >> 2, PackColor(s.psc[f1]));'),
)


def rewrite_writes(source, edits, counts, allowed=()):
    for (old, new), count in zip(edits, counts):
        if new in source:
            assert source.count(new) == count and old not in source, new
        else:
            assert source.count(old) == count, old
            source = source.replace(old, new)
    checked = source
    for permitted in allowed:
        assert checked.count(permitted) == 1
        checked = checked.replace(permitted, '')
    assert not re.search(r'\bs(?:tate)?\.simple\[[^\n]+\]\s*=(?!=)|\bg_state\.simple\[[^\n]+\]\s*=(?!=)', checked)
    return source


def transform_state(source):
    if '// UC_SIMPLE_DIRTY_BLOCKS:' in source:
        assert source.count(TRACKING) == 1
        return source
    return once(source, 'struct State {\n', 'struct State {\n' + TRACKING)


def transform_thread(source):
    if 'void SendSimple(const State& s)' in source:
        assert source.count(SEND_SIMPLE) == 1 and source.count(MEMBER) == 1
        assert OLD_RANGE not in source and source.count(NEW_RANGE) == 1
        assert source.count('        SendSimple(s);\n') == 1
        return source
    source = once(source, '#include <algorithm>\n', '#include <algorithm>\n#include <bit>\n')
    source = once(source, OLD_RANGE, NEW_RANGE)
    source = once(source, '    // what changed since the last command, in 64 byte pieces\n',
                  SEND_SIMPLE + '    // what changed since the last command, in 64 byte pieces\n')
    source = once(source, '        size_t first = cur_->bytes.size();\n',
                  '        size_t first = cur_->bytes.size();\n        SendSimple(s);\n')
    source = once(source, '    State sent_;  // the state as the render thread will have it\n',
                  MEMBER + '    State sent_;  // the state as the render thread will have it\n')
    return source


def undo_thread(source):
    if 'void SendSimple(const State& s)' not in source:
        return source
    return (source.replace(SEND_SIMPLE, '').replace(MEMBER, '').replace(NEW_RANGE, OLD_RANGE)
            .replace('        SendSimple(s);\n', ''))


def transform_d3d(source):
    source = rewrite_writes(source, D3D_WRITES, (1, 2, 1), (PACKET_STORE,))
    if '// UC_SIMPLE_PACKET_DIRTY:' not in source:
        old_packet = between(source, 'HLE_STDCALL(D3DDevice_EndPush, 1) {', '// frame buffer compression')
        packet = once(old_packet, '        for (uint32_t i = 0; i < n; i++, p += 4) {\n',
                      PACKET_MARK + '        for (uint32_t i = 0; i < n; i++, p += 4) {\n')
        packet = once(packet, '                prim = 0;\n', PACKET_REMARK + '                prim = 0;\n')
        source = once(source, old_packet, packet)
    assert source.count(PACKET_MARK) == 1 and source.count(PACKET_REMARK) == 1
    reset = '    g_state = gpu::State{};\n'
    if '    g_state.TrackSimple();\n' not in source:
        source = once(source, reset, reset + '    g_state.TrackSimple();\n')
    assert source.count(reset) == 1 and source.count('    g_state.TrackSimple();\n') == 1
    return source


def fixture(before, after, texture, d3d):
    old_replay, new_replay = replay(before), replay(after)
    # This pass runs after the consumer-owned VSC revision was added to replay.
    for body in (old_replay, new_replay):
        assert 'vsc_revision_' in body
    old_replay = once(old_replay, '    State sent_{},state_{};\n',
                      '    uint64_t vsc_revision_ = 1;\n    State sent_{},state_{};\n')
    new_replay = once(new_replay, '    State sent_{},state_{};\n',
                      '    uint64_t vsc_revision_ = 1;\n    const State* simple_source_ = nullptr;\n    State sent_{},state_{};\n')
    text = Path(__file__).with_name('test_xbe_simple_state_costs.cpp').read_text()
    text = once(text, '/* BASELINE_TRANSPORT */', old_replay)
    text = once(text, '/* CANDIDATE_TRANSPORT */', new_replay)
    mapping = between(texture, 'uint32_t PackColor(', '\n}  // namespace gpu')
    text = once(text, '/* PRODUCTION_PIXEL_MAPPING */', mapping)
    original_mapping = mapping
    for old, new in TEXTURE_WRITES:
        original_mapping = original_mapping.replace(new, old)
    text = once(text, '/* BASELINE_PIXEL_MAPPING */', original_mapping)
    shader = between(d3d, '    for (int i = 0; i < 8; i++) {\n        put(gpu::kMPsAlphaInputs',
                     '    SetComplex(gpu::kRsPsTextureModes')
    assert shader.count('        put(') == 6 and shader.count('\n    put(') == 8
    text = once(text, '/* PRODUCTION_SHADER_WRITES */', shader)
    packet = between(d3d, '    constexpr uint32_t kBeginEnd = 0x17FC, kInlineArray = 0x1818;',
                     '    g_push_begin = 0;')
    text = once(text, '/* CANDIDATE_PACKET */', packet)
    original_packet = packet.replace(PACKET_MARK, '').replace(PACKET_REMARK, '')
    text = once(text, '/* PER_VALUE_PACKET */', original_packet.replace(PACKET_STORE,
                'if (m >> 2 < g_state.simple.size()) g_state.SetSimple(m >> 2, v);'))
    return once(text, '/* BASELINE_PACKET */', original_packet)


def prepare(area):
    area = Path(area)
    paths = {name: area / path for name, path in (
        ('state', 'src/gpu/state.h'), ('thread', 'src/compat/render_thread.cpp'),
        ('d3d', 'src/compat/d3d8.cpp'), ('texture', 'src/compat/texture.cpp'))}
    old = {name: path.read_text() for name, path in paths.items()}
    result = {'state': transform_state(old['state']), 'thread': transform_thread(old['thread']),
              'd3d': transform_d3d(old['d3d']),
              'texture': rewrite_writes(old['texture'], TEXTURE_WRITES, (1, 1, 1, 1))}
    assert transform_state(result['state']) == result['state']
    assert transform_thread(result['thread']) == result['thread']
    assert transform_d3d(result['d3d']) == result['d3d']
    assert rewrite_writes(result['texture'], TEXTURE_WRITES, (1, 1, 1, 1)) == result['texture']
    # The prefix must never enter byte replay; simple[] retains its 64-byte block boundaries.
    assert 'At(s, &s.rs)' in result['thread'] and 'At(s, &s.material)' in result['thread']
    output = fixture(undo_thread(result['thread']), result['thread'], result['texture'], result['d3d'])
    for name, path in paths.items():
        path.write_text(result[name])
    (area / 'src/compat/test_simple_state_costs.cpp').write_text(output)


def finalize(area):
    """Regenerate the focused fixture after downstream preparers and reject erased writers."""
    prepare(area)


"""Track audited vertex-constant writes without changing queued bytes or draw state."""
from pathlib import Path
import re

from prepare_xbe_perf import once, function
from prepare_xbe_state_costs import replay


TRACKING = '''    // UC_VSC_DIRTY_BLOCKS: producer-only metadata, never part of serialized state.
    mutable uint64_t vsc_dirty = 0;
    bool vsc_tracked = false;
    void TrackVertexConstants() { vsc_dirty = (uint64_t(1) << 48) - 1; vsc_tracked = true; }
    void MarkVertexConstants(uint32_t first, uint32_t count) {
        if (!vsc_tracked || first >= vsc.size() || !count) return;
        if (count > vsc.size() - first) count = uint32_t(vsc.size()) - first;
        const unsigned low = first >> 2, high = (first + count - 1) >> 2;
        vsc_dirty |= (~uint64_t(0) << low) & (~uint64_t(0) >> (63 - high));
    }
'''

SEND = '''    // Match the original 64-byte VSC deltas at their original position in the state command.
    void SendVertexConstants(const State& s) {
        static_assert(sizeof(s.vsc) == 48 * 64);
        const auto* now = reinterpret_cast<const uint8_t*>(s.vsc.data());
        auto* sent = reinterpret_cast<uint8_t*>(sent_.vsc.data());
        if (!s.vsc_tracked || vsc_source_ != &s || s.vsc_dirty == (uint64_t(1) << 48) - 1) {
            for (size_t off = 0; off < sizeof(s.vsc); off += 64) Delta(sent + off, now + off, 64);
        } else {
            uint64_t bits = s.vsc_dirty;
            while (bits) {
                const size_t off = std::countr_zero(bits) * 64;
                Delta(sent + off, now + off, 64);
                bits &= bits - 1;
            }
        }
        if (s.vsc_tracked) s.vsc_dirty = 0;
        vsc_source_ = s.vsc_tracked ? &s : nullptr;
    }

'''
BRANCH = '''            // VSC is exactly 48 full blocks; leave PSC, viewport and range tails unchanged.
            if (off == At(s, &s.vsc)) { SendVertexConstants(s); off += sizeof(s.vsc); }
'''
MEMBER = '    const State* vsc_source_ = nullptr; // UC_VSC_DIRTY_BLOCKS: synchronous producer identity\n'
SETTERS = ('D3DDevice_SetVertexShaderConstant1Fast', 'D3DDevice_SetVertexShaderConstant1',
           'D3DDevice_SetVertexShaderConstant4', 'D3DDevice_SetVertexShaderConstantNotInlineFast')
SHADER_OLD = '''            for (auto& [reg, v] : it->second->constants)
                if (reg < 192) g_state.vsc[reg] = v;'''
SHADER_NEW = SHADER_OLD + '''
            if (!it->second->constants.empty()) g_state.MarkVertexConstants(0, 192);'''


def transform_state(source):
    if '// UC_VSC_DIRTY_BLOCKS:' in source:
        assert source.count(TRACKING) == 1
        return source
    return once(source, 'struct State {\n', 'struct State {\n' + TRACKING)


def transform_thread(source):
    if 'void SendVertexConstants(const State& s)' in source:
        assert source.count(SEND) == source.count(BRANCH) == source.count(MEMBER) == 1
        return source
    assert '#include <bit>' in source and source.count('{At(s, &s.vsc), At(s, &s.rt) - At(s, &s.vsc)}') == 1
    source = once(source, '    // what changed since the last command, in 64 byte pieces\n',
                  SEND + '    // what changed since the last command, in 64 byte pieces\n')
    source = once(source, '            size_t off = r.off;\n', '            size_t off = r.off;\n' + BRANCH)
    return once(source, '    State sent_;  // the state as the render thread will have it\n',
                MEMBER + '    State sent_;  // the state as the render thread will have it\n')


def undo_thread(source):
    return source.replace(SEND, '').replace(BRANCH, '').replace(MEMBER, '')


def transform_d3d(source):
    if '    g_state.TrackVertexConstants();\n' not in source:
        source = once(source, '    g_state.TrackSimple();\n',
                      '    g_state.TrackSimple();\n    g_state.TrackVertexConstants();\n')
        line = '        g_state.vsc[59] = {v.x + g_state.ss_offset_x + hw, v.y + g_state.ss_offset_y + hh, v.min_z * g_state.zscale, 0};\n'
        source = once(source, line, line + '        g_state.MarkVertexConstants(58, 2);\n')
        source = once(source, SHADER_OLD, SHADER_NEW)
        for name, count in zip(SETTERS, ('1', '1', '4', 'dwords / 4')):
            old = function(source, 'HLE_FASTCALL(' + name + ',')
            new = once(old, '    return 0;\n', '    g_state.MarkVertexConstants(reg, ' + count + ');\n    return 0;\n')
            source = once(source, old, new)
    assert source.count('    g_state.TrackVertexConstants();\n') == 1
    assert source.count('g_state.MarkVertexConstants(') == 6
    # Fail closed if the exact audited production writer set changes.
    writes = re.findall(r'^.*\bg_state\.vsc\[[^\n]*$', source, re.M)
    assert len(writes) == 7, writes
    assert len(re.findall(r'\bg_state\s*\.\s*vsc\b', source)) == 7, 'unrecognized VSC reference or bulk writer'
    assert source.count(SHADER_NEW) == 1
    assert source.count('    g_state = gpu::State{};\n') == 1
    for name in SETTERS:
        assert function(source, 'HLE_FASTCALL(' + name + ',').count('MarkVertexConstants(') == 1
    return source


def writers(d3d):
    result = function(d3d, 'float ZScale(') + '\n' + function(d3d, 'void UpdateViewportConstants()')
    for name in SETTERS:
        body = function(d3d, 'HLE_FASTCALL(' + name + ',')
        body = re.sub(r'HLE_FASTCALL\([^\n]+\) \{', 'uint32_t ' + name + '(const uint32_t* a) {', body, count=1)
        result += '\n' + body
    shader = (SHADER_NEW if SHADER_NEW in d3d else SHADER_OLD).replace('it->second->constants', 'constants')
    result += '\nvoid Shader(const decltype(VertexShader::constants)& constants) {\n'
    result += shader + '\n}\n'
    return result


def fixture(before, after, d3d_before, d3d_after):
    bodies = []
    for source in (before, after):
        body = replay(source)
        members = '    uint64_t vsc_revision_ = 1;\n    const State* simple_source_ = nullptr;\n'
        body = once(body, '    State sent_{},state_{};\n', members + '    State sent_{},state_{};\n')
        bodies.append(body)
    text = Path(__file__).with_name('test_xbe_vertex_constant_costs.cpp').read_text()
    for marker, value in zip(('BASELINE_TRANSPORT', 'CANDIDATE_TRANSPORT', 'BASELINE_WRITERS', 'CANDIDATE_WRITERS'),
                             (*bodies, writers(d3d_before), writers(d3d_after))):
        text = once(text, '/* ' + marker + ' */', value)
    return text


def prepare(area):
    area = Path(area)
    paths = {name: area / ('src/' + relative) for name, relative in (
        ('state', 'gpu/state.h'), ('thread', 'compat/render_thread.cpp'), ('d3d', 'compat/d3d8.cpp'))}
    old = {name: path.read_text() for name, path in paths.items()}
    result = {name: transform(old[name]) for name, transform in (
        ('state', transform_state), ('thread', transform_thread), ('d3d', transform_d3d))}
    for name, transform in (('state', transform_state), ('thread', transform_thread), ('d3d', transform_d3d)):
        assert transform(result[name]) == result[name]
    before_d3d = result['d3d'].replace('    g_state.TrackVertexConstants();\n', '')
    before_d3d = re.sub(r'^\s*g_state\.MarkVertexConstants\([^\n]+\);\n', '', before_d3d, flags=re.M)
    before_d3d = before_d3d.replace(SHADER_NEW, SHADER_OLD)
    output = fixture(undo_thread(result['thread']), result['thread'], before_d3d, result['d3d'])
    for name, path in paths.items():
        path.write_text(result[name])
    (area / 'src/compat/test_vertex_constant_costs.cpp').write_text(output)


def finalize(area):
    prepare(area)

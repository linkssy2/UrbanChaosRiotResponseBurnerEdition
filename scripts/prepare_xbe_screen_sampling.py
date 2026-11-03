"""Keep one-to-one scene copies aligned with each player's later HUD."""
from pathlib import Path
from prepare_xbe_perf import once, function

HELPER = r'''// UC_ALIGNED_SCREEN_COPY: these UVs already interpolate at host pixel centres.
void AlignScreenCopies(const State& s, const Assembled& a, GuestExtent size, DrawTextures& stages) {
    if (!(s.vs_handle & 1) || !s.vs || s.vs->program.size() != 12 ||
        a.regs != 3 || a.decode || a.pretransformed || a.vertex_count != 4 ||
        a.base_vertex || !a.vertices.empty() || a.topology != Topology::Triangles || !size.w || !size.h ||
        !s.viewport.w || !s.viewport.h || uint64_t(s.viewport.x) + s.viewport.w > size.w ||
        uint64_t(s.viewport.y) + s.viewport.h > size.h) return;
    // MOV oPos,v0; MOV oT0,v1; MOV oT1,v1. Reject transformed/filtering programs.
    for (unsigned n = 0; n != 3; ++n) {
        const auto* t = &s.vs->program[n * 4];
        const unsigned out = n ? n + 8 : 0;
        if (((t[1] >> 21) & 127) != 1 || (t[1] & 511) != 27 ||
            ((t[1] >> 9) & 15) != unsigned(n != 0) || ((t[2] >> 26) & 3) != 2 ||
            ((t[3] >> 24) & 15) || ((t[3] >> 12) & 15) != 15 ||
            !(t[3] & 0x800) || ((t[3] >> 3) & 15) != out ||
            (t[3] & 4) || (t[3] & 1) != unsigned(n == 2)) return;
    }
    if (a.elements[0].format != DXGI_FORMAT_R32G32B32_FLOAT ||
        a.elements[1].format != DXGI_FORMAT_R32G32_FLOAT) return;
    const float left = float(s.viewport.x), top = float(s.viewport.y);
    const float right = float(s.viewport.x + s.viewport.w), bottom = float(s.viewport.y + s.viewport.h);
    const float half = float(size.h) * .5f;
    unsigned full_corners = 0, view_corners = 0, upper_corners = 0, lower_corners = 0;
    for (unsigned v = 0; v != 4; ++v) {
        float p[3], uv[2];
        for (unsigned r = 0; r != 2; ++r) {
            const auto& e = a.elements[r];
            if (e.buffer >= a.buffer_count || e.buffer >= a.buffers.size()) return;
            const auto& b = a.buffers[e.buffer];
            const size_t bytes = r ? sizeof(uv) : sizeof(p);
            const uint64_t offset = uint64_t(v) * b.stride + e.offset;
            if (!b.src || !b.stride || offset + bytes > b.bytes) return;
            std::memcpy(r ? uv : p, b.src + offset, bytes);
        }
        if (p[2] != 0 || p[0] != uv[0] || p[1] != uv[1]) return;
        if ((p[0] == 0 || p[0] == float(size.w)) && (p[1] == 0 || p[1] == float(size.h)))
            full_corners |= 1u << (unsigned(p[0] != 0) + 2 * unsigned(p[1] != 0));
        if ((p[0] == left || p[0] == right) && (p[1] == top || p[1] == bottom))
            view_corners |= 1u << (unsigned(p[0] == right) + 2 * unsigned(p[1] == bottom));
        if ((p[0] == 0 || p[0] == float(size.w)) && (p[1] == 0 || p[1] == half))
            upper_corners |= 1u << (unsigned(p[0] != 0) + 2 * unsigned(p[1] == half));
        if ((p[0] == 0 || p[0] == float(size.w)) && (p[1] == half || p[1] == float(size.h)))
            lower_corners |= 1u << (unsigned(p[0] != 0) + 2 * unsigned(p[1] != half));
    }
    // Late co-op effects reset the guest viewport but retain a half-target quad.
    if (full_corners != 15 && view_corners != 15 && upper_corners != 15 && lower_corners != 15) return;
    for (unsigned i = 0; i != 2; ++i)
        stages.aligned_screen_copy[i] = stages.res[i] && stages.rt[i] &&
            stages.info[i].kind == kTex2D && stages.info[i].rect && !stages.info[i].shadow &&
            !stages.aligned_depth_alias[i] && stages.width[i] == size.w && stages.height[i] == size.h &&
            stages.k[i] == 1 && stages.wide[i] == 0;
}
'''

CALL = '    if (!ext_) AlignScreenCopies(s, a, size, stages);\n'
GUARD = 'if (!stages.rt[i] || stages.aligned_depth_alias[i] || stages.aligned_screen_copy[i]) return;'


def patch(source):
    if '// UC_ALIGNED_SCREEN_COPY:' in source:
        previous = function(source, 'void AlignScreenCopies(')
        source = once(source, previous, HELPER[HELPER.index('void AlignScreenCopies('):].rstrip())
        assert source.count(HELPER) == source.count(CALL) == source.count(GUARD) == 1
        return source
    source = once(source, '    bool aligned_depth_alias[4] = {};',
                  '    bool aligned_depth_alias[4] = {};\n    bool aligned_screen_copy[4] = {};')
    source = once(source, 'if (!stages.rt[i] || stages.aligned_depth_alias[i]) return;', GUARD)
    source = once(source, '    DrawConstants c{};\n    UploadConstants(s, sh, k, stages, size, c, analytic_fog, vsc_revision);',
                  CALL + '    DrawConstants c{};\n    UploadConstants(s, sh, k, stages, size, c, analytic_fog, vsc_revision);')
    return once(source, 'DrawShaders ClassifyShaders(', HELPER + '\nDrawShaders ClassifyShaders(')


def prepare(area):
    compat = Path(area) / 'src/compat'
    path = compat / 'd3d12.cpp'
    source = patch(path.read_text())
    assert patch(source) == source
    path.write_text(source)
    stages = source[source.index('struct DrawTextures {'):source.index('\n};', source.index('struct DrawTextures {')) + 3]
    texel = function(source, 'void D3D12Backend::FillTexelConstants(').replace('D3D12Backend::', '')
    fixture = Path(__file__).with_name('test_xbe_screen_sampling.cpp').read_text()
    fixture = once(fixture, '// PRODUCTION_SCREEN_SAMPLING', stages + '\nstruct GuestExtent { uint32_t w, h; };\n' +
                   HELPER + '\nconstexpr float kGlowBoxTexels = 3.0f;\n' + texel)
    (compat / 'test_xbe_screen_sampling.cpp').write_text(fixture)

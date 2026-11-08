"""Preserve exact vertex data while reducing conversion and index-expansion work."""
from pathlib import Path
from prepare_xbe_perf import once, function


def list_indices(source):
    if '// UC_CONVERTED_LIST_INDICES' in source:
        return source
    old = function(source, 'bool AssembleConverted(')
    new = once(old, '''    std::vector<uint32_t> seq(d.count);
    for (uint32_t i = 0; i < d.count; i++) seq[i] = DrawVertex(s, d, i);
    bool ranged = uint64_t(d.span) <= uint64_t(d.count) * kRangedSpanPerVertex + kRangedSpanSlack;''', '''    bool ranged = uint64_t(d.span) <= uint64_t(d.count) * kRangedSpanPerVertex + kRangedSpanSlack;
    // UC_CONVERTED_LIST_INDICES: conversion changes vertex storage, not list order.
    const bool native_list = ranged && (d.prim == 1 || d.prim == 2 || d.prim == 5);
    std::vector<uint32_t> seq;
    if (!native_list) {
        seq.resize(d.count);
        for (uint32_t i = 0; i < d.count; i++) seq[i] = DrawVertex(s, d, i);
    }''')
    new = once(new, '    auto local = [&](uint32_t i) { return ranged ? seq[i] - d.first_vertex : i; };',
        '''    if (native_list) {
        DrawRange list = d;
        const uint32_t vertices_per_primitive = d.prim == 5 ? 3 : d.prim == 2 ? 2 : 1;
        list.count -= list.count % vertices_per_primitive;
        return DrawNative(list, true, out);
    }
    auto local = [&](uint32_t i) { return ranged ? seq[i] - d.first_vertex : i; };''')
    return once(source, old, new)


def format_dispatch(source):
    source = list_indices(source)
    if '// UC_CONVERTED_FORMAT_DISPATCH' in source:
        return source
    old = '''        for (uint32_t v = 0; v < n; v++) {
            uint32_t id = ranged ? d.first_vertex + v : seq[v];
            Convert(in.type, src + size_t(id) * stride, &out.vertices[(size_t(v) * nregs + k) * 4]);
        }'''
    new = '''        // UC_CONVERTED_FORMAT_DISPATCH: the format is constant for this attribute.
        const auto convert = [&]<uint32_t Type>() {
            for (uint32_t v = 0; v < n; v++) {
                uint32_t id = ranged ? d.first_vertex + v : seq[v];
                Convert(Type ? Type : in.type, src + size_t(id) * stride,
                        &out.vertices[(size_t(v) * nregs + k) * 4]);
            }
        };
        switch (in.type) {
'''
    for value in (0x12, 0x22, 0x32, 0x42, 0x72, 0x40, 0x11, 0x21, 0x31, 0x41,
                  0x14, 0x24, 0x34, 0x44, 0x15, 0x25, 0x35, 0x45, 0x16):
        new += f'            case 0x{value:02X}: convert.operator()<0x{value:02X}>(); break;\n'
    new += '''            default: convert.operator()<0>(); break;
        }'''
    return once(source, old, new)


def converted_storage(source):
    if '// UC_CONVERTED_STORAGE' in source:
        return source
    source = once(source, '    out.vertices.clear();', '''    // UC_CONVERTED_STORAGE: keep initialized floats across raw and rejected draws.
    if (!out.vertices.empty()) out.vertices.swap(out.converted_storage);''')
    return once(source, '    out.vertices.resize(size_t(n) * nregs * 4);', '''    out.vertices.swap(out.converted_storage);
    out.vertices.resize(size_t(n) * nregs * 4);''')


def storage_header(source):
    if 'std::vector<float> converted_storage;' in source:
        return source
    return once(source, 'struct Assembled {', '''struct Assembled {
    std::vector<float> converted_storage; // reused float storage; empty while vertices is populated''')


def packed_header(source):
    if 'bool repacked = false;' in source:
        return source
    return once(source, '    bool strip = false;', '''    bool repacked = false; // compact copy; retain converted-path sampler behavior
    bool strip = false;''')


def packed_renderer(source):
    if 'if (a.repacked || !a.vertices.empty()' in source:
        return source
    source = once(source, 'if (!a.vertices.empty() || !a.vertex_count',
                  'if (a.repacked || !a.vertices.empty() || !a.vertex_count')
    return source.replace('g_perf.raw += a.vertices.empty();',
                          'g_perf.raw += !a.repacked && a.vertices.empty();')


def packed_vertices(source):
    if '// UC_PACKED_STRIDE10' in source:
        return source
    source = once(source, '    out.decode = 0;', '    out.decode = 0;\n    out.repacked = false;')
    helper = Path(__file__).with_name('xbe_packed_vertices.inc').read_text()
    source = once(source, '// converted on the CPU: each element', helper + '\n// converted on the CPU: each element')
    begin = source.index('    out.vertices.swap(out.converted_storage);')
    end_text = '    out.buffers[0] = {bytes, uint32_t(out.vertices.size() * 4), uint32_t(nregs * 16)};'
    end = source.index(end_text, begin) + len(end_text)
    previous = source[begin:end]
    packed = '    if (!CanPackStride10(s, inputs) || !PackStride10(s, inputs, d, ranged, seq, out)) {\n'
    packed += '\n'.join('    ' + line for line in previous.splitlines()) + '\n    }'
    return source[:begin] + packed + source[end:]


def transform(source):
    return strip_pairs(index_dispatch(quad_index_fill(strip_index_fill(ranged_sequence(packed_vertices(converted_storage(format_dispatch(source))))))))


def strip_pairs(source):
    if '// UC_STRIP_PAIRS' in source:return source
    before='''            for (uint32_t i = 0; i + 2 < count; i++) {
                output[size_t(i) * 3] = local(i + (i & 1));
                output[size_t(i) * 3 + 1] = local(i + !(i & 1));
                output[size_t(i) * 3 + 2] = local(i + 2);
            }'''
    after='''            // UC_STRIP_PAIRS: reuse the shared edge, preserving winding and degenerates.
            uint32_t a = local(0), b = local(1), i = 2;
            for (; i < count - 1; i += 2, output += 6) {
                const uint32_t c = local(i), d = local(i + 1);
                output[0] = a; output[1] = b; output[2] = c;
                output[3] = c; output[4] = b; output[5] = d;
                a = c; b = d;
            }
            if (i < count) {
                output[0] = a; output[1] = b; output[2] = local(i);
            }'''
    return once(source,before,after)


def index_dispatch(source):
    if '// UC_CONVERTED_INDEX_DISPATCH' in source:return source
    assert function(source, 'uint32_t DrawVertex(') == '''uint32_t DrawVertex(const State& s, const DrawRange& d, uint32_t i) {
    if (!d.ip) return d.start + i;
    uint16_t v;
    std::memcpy(&v, d.ip + i * 2, 2);
    return v + s.base_vertex;
}'''
    assert '    d.first = bounds.first;\n    d.first_vertex = ip ? bounds.first + s.base_vertex : start;' in source
    before='''    auto local = [&](uint32_t i) { return ranged ? DrawVertex(s, d, i) - d.first_vertex : i; };
    ListIndices(d.prim, d.count, local, out.indices, out.topology);'''
    after='''    // UC_CONVERTED_INDEX_DISPATCH: base vertex cancels; choose the source once per draw.
    if (ranged && d.ip) {
        const auto local = [ip = d.ip, first = d.first](uint32_t i) {
            uint16_t v;
            std::memcpy(&v, ip + i * 2, 2);
            return uint32_t(v) - first;
        };
        ListIndices(d.prim, d.count, local, out.indices, out.topology);
    } else {
        ListIndices(d.prim, d.count, [](uint32_t i) { return i; }, out.indices, out.topology);
    }'''
    return once(source,before,after)


def ranged_sequence(source):
    if '// UC_RANGED_SEQUENCE_DIRECT' in source:return source
    source=once(source,'    if (!native_list) {\n        seq.resize(d.count);',
                '    // UC_RANGED_SEQUENCE_DIRECT: ranged attributes never consume the sequence.\n    if (!ranged) {\n        seq.resize(d.count);')
    return once(source,'return ranged ? seq[i] - d.first_vertex : i;',
                'return ranged ? DrawVertex(s, d, i) - d.first_vertex : i;')


def strip_index_fill(source):
    if '// UC_STRIP_INDEX_FILL' in source:return source
    before='''        case 6:  // triangle strip
            for (uint32_t i = 0; i + 2 < count; i++) {
                if (i & 1) ix.insert(ix.end(), {local(i + 1), local(i), local(i + 2)});
                else ix.insert(ix.end(), {local(i), local(i + 1), local(i + 2)});
            }
            break;'''
    after='''        case 6: { // UC_STRIP_INDEX_FILL: retain the exact triangle-list order.
            if (count < 3) break;
            const size_t first = ix.size();
            ix.resize(first + size_t(count - 2) * 3);
            uint32_t* output = ix.data() + first;
            for (uint32_t i = 0; i + 2 < count; i++) {
                output[size_t(i) * 3] = local(i + (i & 1));
                output[size_t(i) * 3 + 1] = local(i + !(i & 1));
                output[size_t(i) * 3 + 2] = local(i + 2);
            }
            break;
        }'''
    return once(source,before,after)


def quad_index_fill(source):
    if '// UC_QUAD_INDEX_FILL' in source:return source
    before='''        case 8:  // quad list
            for (uint32_t i = 0; i + 3 < count; i += 4) {
                ix.insert(ix.end(), {local(i), local(i + 1), local(i + 2), local(i), local(i + 2), local(i + 3)});
            }
            break;'''
    after='''        case 8: { // UC_QUAD_INDEX_FILL: retain original winding and discard incomplete quads.
            const size_t quads = count / 4, first = ix.size();
            ix.resize(first + quads * 6);
            for (size_t q = 0; q < quads; ++q) {
                const uint32_t i = uint32_t(q * 4);
                uint32_t* output = ix.data() + first + q * 6;
                output[0] = local(i); output[1] = local(i + 1); output[2] = local(i + 2);
                output[3] = local(i); output[4] = local(i + 2); output[5] = local(i + 3);
            }
            break;
        }'''
    return once(source,before,after)


def prepare(source, area, header):
    baseline = converted_storage(format_dispatch(source))
    before_dispatch = quad_index_fill(strip_index_fill(ranged_sequence(packed_vertices(baseline))))
    before_pairs = index_dispatch(before_dispatch)
    result = strip_pairs(before_pairs)
    assert transform(result) == result
    target = Path(area) / 'src/gpu/vertex.h'
    target.parent.mkdir(parents=True, exist_ok=True)
    prepared_header = packed_header(storage_header(header))
    assert packed_header(storage_header(prepared_header)) == prepared_header
    target.write_text(prepared_header)
    original = once(baseline.replace('FvfInputs(', 'OriginalFvfInputs(').replace('Convert(', 'OriginalConvert('), 'namespace gpu {', '''namespace original_vertex {
using namespace gpu;
void OriginalFvfInputs(uint32_t, VertexShader&);''')
    fixture = Path(__file__).with_name('test_xbe_vertex_costs.cpp').read_text()
    fixture = once(fixture, '/* ORIGINAL_VERTEX */', original)
    fixture = once(fixture, '/* PRODUCTION_VERTEX */', result)
    previous_pairs = before_pairs.replace('FvfInputs(', 'PreviousFvfInputs(').replace('Convert(', 'PreviousConvert(')
    previous_pairs = previous_pairs.replace('ListIndices(', 'PreviousListIndices(')
    previous_pairs = once(previous_pairs, 'namespace gpu {', '''namespace previous_strip_pairs {
using namespace gpu;
void PreviousFvfInputs(uint32_t, VertexShader&);''')
    fixture = once(fixture, '/* PREVIOUS_STRIP_PAIRS */', previous_pairs)
    previous = function(before_dispatch, 'bool AssembleConverted(')
    previous += '\n' + once(function(before_dispatch, 'bool Assemble('),
        'return AssembleConverted(s, inputs, d, out);',
        'return previous_index_dispatch::AssembleConverted(s, inputs, d, out);')
    fixture = once(fixture, '/* PREVIOUS_INDEX_DISPATCH */',
        'namespace previous_index_dispatch {\nusing namespace gpu;\n' + previous + '\n}\n')
    (Path(area) / 'src/compat/test_vertex_costs.cpp').write_text(fixture)
    return result

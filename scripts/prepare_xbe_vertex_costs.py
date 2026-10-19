"""Keep converted draw storage and choose conversion formats outside hot loops."""
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
    return strip_index_fill(ranged_sequence(packed_vertices(converted_storage(format_dispatch(source)))))


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


def prepare(source, area, header):
    baseline = converted_storage(format_dispatch(source))
    result = transform(source)
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
    (Path(area) / 'src/compat/test_vertex_costs.cpp').write_text(fixture)
    return result

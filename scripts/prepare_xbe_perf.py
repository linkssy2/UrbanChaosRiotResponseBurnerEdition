"""Keep renderer diagnostics opt-in, including their clocks, in Release builds."""
from pathlib import Path


def once(source, old, new):
    assert source.count(old) == 1, old
    return source.replace(old, new, 1)


def function(source, signature):
    begin = source.index(signature)
    return source[begin:source.index('\n}', begin) + 2]


def transform(source):
    gate = '    if (diag::Perf()) LogPerf();'
    assert source.count(gate) == 1
    marker = '    double pipeline = 0;  // opt-in draw PSO creation time\n'
    if marker in source:
        report = function(source, 'void LogPerf() {')
        assert report.count('LOGR(') == 3 and 'LOGI(' not in report
        assert 'p.compile / n, p.pipeline / n,' in report
        assert '    ~PerfScope() { PerfAdd(acc, t); }' in source
        assert source.count('= PerfStart();') == 7  # scope, five manual sites, PSO miss
        assert source.count('PerfClock::now()') == 4  # Ms, gated Start, gated LogPerf twice
        return source
    source = once(source, 'struct Perf {\n', '''// diag::Perf is latched for the process; disabled diagnostics never read the clock.
PerfClock::time_point PerfStart() {
    return diag::Perf() ? PerfClock::now() : PerfClock::time_point{};
}
void PerfAdd(double& acc, PerfClock::time_point start) {
    if (diag::Perf()) acc += PerfMs(start);
}
struct Perf {
''')
    source = once(source, '    PerfClock::time_point t = PerfClock::now();\n'
                         '    ~PerfScope() { acc += PerfMs(t); }',
                         '    PerfClock::time_point t = PerfStart();\n'
                         '    ~PerfScope() { PerfAdd(acc, t); }')
    # These are only diagnostic spans; FPS, pacing, capture and fence clocks stay intact.
    for variable, count in (('perf_t', 3), ('perf_wait', 1), ('perf_swap', 1)):
        old = f'auto {variable} = PerfClock::now();'
        assert source.count(old) == count, old
        source = source.replace(old, f'auto {variable} = PerfStart();')
    for field, variable in (('sig', 'perf_t'), ('bind', 'perf_t'),
                            ('assemble', 'perf_t'), ('wait', 'perf_wait'),
                            ('swap', 'perf_swap')):
        source = once(source, f'g_perf.{field} += PerfMs({variable});',
                      f'PerfAdd(g_perf.{field}, {variable});')
    source = once(source, '    double compile = 0;\n', '    double compile = 0;\n' + marker)
    old = function(source, 'ID3D12PipelineState* D3D12Backend::EnsurePipeline(')
    new = once(old, '    g_perf.psos++;\n',
               '    g_perf.psos++;\n'
               '    const auto perf_pso = PerfStart();\n')
    new = once(new, '    psos_[h] = pso;\n',
               '    PerfAdd(g_perf.pipeline, perf_pso);\n'
               '    psos_[h] = pso;\n')
    source = once(source, old, new)
    old = function(source, 'void LogPerf() {')
    assert old.count('LOGI(') == 3 and 'LOGR(' not in old
    new = old.replace('LOGI(', 'LOGR(')
    new = once(new, 'compile %.2f; clear', 'compile %.2f pipeline %.2f; clear')
    new = once(new, 'p.compile / n, p.clear / n,', 'p.compile / n, p.pipeline / n, p.clear / n,')
    source = once(source, old, new)
    assert source.count(gate) == 1
    return source


def prepare(source, area):
    result = transform(source)
    assert transform(result) == result
    fixture = Path(__file__).with_name('test_xbe_perf.cpp').read_text()
    start = result.index('PerfClock::time_point PerfStart() {')
    end = result.index('\n// F3 captures', start)
    fixture = fixture.replace('/* PERF_IMPLEMENTATION */', result[start:end])
    fixture = fixture.replace('/* PIPELINE_IMPLEMENTATION */', function(result, 'ID3D12PipelineState* D3D12Backend::EnsurePipeline('))
    fixture = fixture.replace('/* REPORT_IMPLEMENTATION */', function(result, 'void LogPerf() {'))
    fixture = fixture.replace('/* PRESENT_GATE */', 'if (diag::Perf()) LogPerf();')
    Path(area, 'src/compat/test_perf.cpp').write_text(fixture)
    return result

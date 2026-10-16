"""Compile out diagnostic-only D3D12 work, preserving the diagnostic build verbatim."""
from pathlib import Path
import re


BEGIN = '#ifndef UC_BURNER_RELEASE // UC_RELEASE_GPU\n'
END = '#endif // UC_RELEASE_GPU\n'


def function(source, signature):
    start = source.index(signature)
    return source[start:source.index('\n}', start) + 2]


def views(source):
    """Project only our guards, for exact non-release and hot-path checks."""
    debug, release, hidden = [], [], False
    for line in source.splitlines(keepends=True):
        if line == BEGIN:
            assert not hidden
            hidden = True
        elif line == END:
            assert hidden
            hidden = False
        else:
            debug.append(line)
            if not hidden:
                release.append(line)
    assert not hidden
    return ''.join(debug), ''.join(release)


def transform(source):
    original, _ = views(source)
    source = original
    spans = []

    def section(start, until):
        assert source.count(start) == 1, start
        first = source.index(start)
        last = source.index(until, first)
        assert first < last and source[last - 1] == '\n'
        spans.append((first, last))

    # These blocks contain only capture bookkeeping/readbacks, never draw setup.
    section('        if(cap_ && s.uc_fog_pass==1)fprintf(cap_,\n',
            '        return false;')
    section('    if (cap_) {\n        if(s.uc_fog_pass==1)fprintf(cap_,',
            '\n    // texture views and samplers')
    section('    if (cap_) {\n        CapStep(s.rt);\n',
            '    Target* rt = (flags & kClearTarget)')
    section('    if (cap_) {\n        const char* from = "memory";',
            '    // pixels the CPU made:')
    section('    if (cap_) {\n        char name[32];\n',
            '}\n\nvoid D3D12Backend::ResizeSwap()')
    section('    if (cap_) {\n        FinishCapture(front);',
            '    ResizeSwap();')

    # The backend's virtual capture request becomes an empty operation too.
    section('    if (!cap_) return;\n    char name[32];',
            '}\n\nvoid D3D12Backend::SetGamma(')
    report = function(source, 'void LogPerf() {')
    first = source.index(report)
    spans.append((first, first + len(report) + 1))

    # All remaining capture probes are one complete statement on one line.
    captures = list(re.finditer(
        r'^ +if\s*\((?:cap_|!analytic_fog && cap_)\)[^\n]*(?:fprintf|CapStep)[^\n]*\n',
        source, re.M))
    assert len(captures) == 7
    spans.extend((match.start(), match.end()) for match in captures)

    # Explicitly remove scopes/counters instead of relying on interprocedural
    # optimization of unused globals. Frame pacing/FPS/query counters stay live.
    pattern = (r'^ +(?:'
               r'(?:\+\+g_perf\.\w+|g_perf\.\w+(?:\+\+| \+= [^\n]+));'
               r'|PerfScope perf_\w+\{g_perf\.\w+\};'
               r'|(?:const )?auto perf_\w+ = PerfStart\(\);'
               r'|PerfAdd\(g_perf\.\w+, perf_\w+\);'
               r'|if \(diag::Perf\(\)\) LogPerf\(\);'
               r'|if \(presents_ < 3 \|\| presents_ == 120\) LOGR\([^\n]+;'
               r')\n')
    measurements = list(re.finditer(pattern, source, re.M))
    assert len(measurements) == 40, len(measurements)
    spans.extend((match.start(), match.end()) for match in measurements)

    spans.sort()
    assert all(left[1] <= right[0] for left, right in zip(spans, spans[1:]))
    for first, last in reversed(spans):
        source = source[:first] + BEGIN + source[first:last] + END + source[last:]

    debug, release = views(source)
    assert debug == original  # exact diagnostic build, including its capture data
    # Negative witness: the original really performs both per-frame/per-draw work.
    assert 'g_capture.exchange(false)' in original and 'g_perf.draws++;' in original
    for name in ('DrawDepthCopy', 'PrepareFogLookup', 'BeginSmoothFog',
                 'BindDepthColorAlias', 'Draw', 'Clear', 'Copy', 'Present', 'Capture'):
        body = function(release, f'D3D12Backend::{name}(')
        assert not re.search(r'\b(?:cap_|CapStep|CaptureDraw|CaptureShaders|CapSave|g_capture|g_perf|PerfScope|PerfStart|PerfAdd)\b', body), name
    assert 'void LogPerf()' not in release
    # Real render, pacing, occlusion, FPS, upload and format-lifetime paths remain.
    for name in ('DrawGeometry', 'DeliverReports', 'BindTargets', 'ResetFrame',
                 'CountFps', 'SetGamma', 'EndSmoothFog', 'CopySmoothFogImage'):
        signature = f'D3D12Backend::{name}('
        before, after = function(original, signature), function(release, signature)
        # ResetFrame has a diagnostic wait span around the unchanged real wait.
        if name == 'ResetFrame':
            before = re.sub(pattern, '', before, flags=re.M)
        assert before == after, name
    return source


def prepare(area):
    path = Path(area) / 'src/compat/d3d12.cpp'
    result = transform(path.read_text())
    assert transform(result) == result
    path.write_text(result)

"""Complete real GPU visibility reports for the title's synchronous result poll."""
from pathlib import Path
from prepare_xbe_perf import function as method


ADAPTER = r'''
// 003E2C30: GetVisibilityTestResult(index, pixels, optional 64-bit timestamp).
// A24C0 retries before Present; wait here rather than burning CPU in its retry loop.
// Deliver the real report, never an invented visibility count.
#define SYM_UrbanChaos_VisibilityResult 003E2C30
HLE_STDCALL(UrbanChaos_VisibilityResult, 3) {
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    uint32_t slot = kDevice + 0x7d4 + (a[0] >> 8) * 4;
    if (a[0] > 65535 || !TrialGuestReadable(slot, 4) ||
        !TrialGuestReadable(a[1], 4) || (a[2] && !TrialGuestReadable(a[2], 8)))
        return 0x80070057;
    uint32_t page = R32(slot);
    uint32_t report = page + (a[0] & 255) * 16;
    if (!page || report < page || !TrialGuestReadable(report, 16)) return 0x80070057;
    Walk(Dev(kDevPush));
    if (R32(report + 12) == 0xffffffff && g_backend) g_backend->Flush();
    if (R32(report + 12) == 0xffffffff) return 0x88760828;
    std::atomic_thread_fence(std::memory_order_acquire);
    W32(a[1], R32(report + 8));
    if (a[2]) {
        W32(a[2], R32(report));
        W32(a[2] + 4, R32(report + 4));
    }
    return 0;
}

'''

FLUSH = '''void D3D12Backend::Flush() {
    Submit(true);
    // The title may wait for a visibility result without presenting a frame.
    DeliverReports();
}'''

QUERY_RESOLVE = '''// UC_QUERY_RESOLVE_BATCH: all queries in this range have ended; the
// existing submission fence still covers the same query-result writes.
void D3D12Backend::ResolveCountedDraws() {
    if (!pending_resolve_count_) return;
    const uint64_t at = uint64_t(pending_resolve_first_) * sizeof(uint64_t);
    cl_->ResolveQueryData(query_heap_.Get(), D3D12_QUERY_TYPE_OCCLUSION,
        pending_resolve_first_, pending_resolve_count_, query_rb_.Get(), at);
    pending_resolve_count_ = 0;
}

'''


def once(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new, 1)


def function(text, begin, end):
    start = text.index(begin)
    return text[start:text.index(end, start)]


def batch_query_resolves(source):
    """Resolve ended ranges at existing submits, before any ring slot is reused."""
    assert 'UC_QUERY_RESOLVE_BATCH' not in source
    assert 'constexpr uint32_t kQueries = 8192;' in source
    assert source.count('query_next_++') == 1
    for old, new in [
        ('    void EndCountedDraw(uint32_t query);',
         '    void EndCountedDraw(uint32_t query);\n    void ResolveCountedDraws();'),
        ('    uint32_t query_next_ = 0;',
         '    uint32_t query_next_ = 0;\n    uint32_t pending_resolve_first_ = 0, pending_resolve_count_ = 0;'),
        ('void D3D12Backend::Submit(bool wait) {',
         'void D3D12Backend::Submit(bool wait) {\n    ResolveCountedDraws();'),
        ('    uint32_t query = query_next_++ % kQueries;\n    cl_->BeginQuery',
         '    uint32_t query = query_next_++ % kQueries;\n'
         '    // Resolve the old generation before reusing a query-heap slot.\n'
         '    if (!query) ResolveCountedDraws();\n    cl_->BeginQuery'),
        ('    uint64_t at = uint64_t(query) * sizeof(uint64_t);\n'
         '    cl_->ResolveQueryData(query_heap_.Get(), D3D12_QUERY_TYPE_OCCLUSION, query, 1, query_rb_.Get(), at);',
         '    if (!pending_resolve_count_) pending_resolve_first_ = query;\n    ++pending_resolve_count_;'),
        ("// the console counts its own pixels: the bound target's scale and multisampling come out",
         QUERY_RESOLVE + "// the console counts its own pixels: the bound target's scale and multisampling come out"),
    ]:
        source = once(source, old, new)
    submit = method(source, 'void D3D12Backend::Submit(bool wait) {')
    assert submit.index('ResolveCountedDraws();') < submit.index('cl_->Close()')
    return source


def query_fixtures(before, after, compat):
    """Keep the old per-query resolve as the reference; extract both real paths."""
    local = Path(__file__).parent
    fixture = (local / 'test_xbe_query_resolves.cpp').read_text()
    methods = []
    for label, source in [('BEFORE', before), ('AFTER', after)]:
        bodies = '\n\n'.join(method(source, signature) for signature in (
            'uint32_t D3D12Backend::BeginCountedDraw(',
            'void D3D12Backend::EndCountedDraw(',
            'void D3D12Backend::DeliverReports('))
        if label == 'AFTER':
            bodies += '\n\n' + method(source, 'void D3D12Backend::ResolveCountedDraws(')
        fixture = once(fixture, f'/* {label}_METHODS */', bodies)
        methods.append(f'namespace {label.lower()} {{\n' + bodies + '\n}\n')
    (compat / 'test_query_resolves.cpp').write_text(fixture)
    (compat / 'query_resolve_methods.h').write_text('\n'.join(methods))
    (compat / 'test_query_resolves_gpu.cpp').write_text((local / 'test_xbe_query_resolves_gpu.cpp').read_text())


def prepare(sdk, area):
    """Run after prepare_xbe_graphics_compat; only changes its isolated outputs."""
    sdk, area = Path(sdk), Path(area)
    compat = area / 'src/compat'
    d3d = (compat / 'd3d8.cpp').read_text()
    marker = '// synchronisation, the renderer never runs behind'
    assert 'SYM_UrbanChaos_VisibilityResult' not in d3d
    (compat / 'd3d8.cpp').write_text(once(d3d, marker, ADAPTER + marker))
    gpu = (compat / 'd3d12.cpp').read_text()
    gpu = once(gpu, 'void D3D12Backend::Flush() { Submit(true); }', FLUSH)
    batched = batch_query_resolves(gpu)
    (compat / 'd3d12.cpp').write_text(batched)
    query_fixtures(gpu, batched, compat)

    # Compile the actual guest result reader, production HLE adapter and backend
    # completion body into an offline fixture; GPU submission is the only fake.
    original = (area / 'generated/game/x86_recomp.120.cpp').read_text()
    oracle = function(original, 'X86_FUNC(__imp__sub_003E2C30) {', '\nX86_WEAK_FUNC(sub_003E2C30')
    source = (sdk / 'runtime/gpu/d3d12.cpp').read_text()
    deliver = function(source, 'void D3D12Backend::DeliverReports() {', '\nvoid D3D12Backend::Draw(')
    test = Path(__file__).with_name('test_xbe_visibility.cpp').read_text()
    for marker, body in [('// GENERATED_ADAPTER', ADAPTER), ('// GENERATED_ORIGINAL', oracle),
                         ('// GENERATED_FLUSH', FLUSH), ('// GENERATED_DELIVER', deliver)]:
        test = once(test, marker, body)
    (compat / 'test_visibility.cpp').write_text(test)


if __name__ == '__main__':
    root = Path(__file__).resolve().parents[1]
    prepare(root / 'build/upstream-review-xboxxbe-20261014',
            root / 'build/xbe-full-integration-20261014')

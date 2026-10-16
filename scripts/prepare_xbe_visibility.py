"""Complete real GPU visibility reports for the title's synchronous result poll."""
from pathlib import Path


ADAPTER = r'''
// 003E2C30: GetVisibilityTestResult(index, pixels, optional 64-bit timestamp).
// A24C0 can poll before Present, so drain the report command and complete that
// GPU work here. Never substitute a made-up visibility count for a pending query.
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
    // shortcut: pending polls synchronize; add an asynchronous report pump after gameplay parity.
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


def once(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new, 1)


def function(text, begin, end):
    start = text.index(begin)
    return text[start:text.index(end, start)]


def prepare(sdk, area):
    """Run after prepare_xbe_graphics_compat; only changes its isolated outputs."""
    sdk, area = Path(sdk), Path(area)
    compat = area / 'src/compat'
    d3d = (compat / 'd3d8.cpp').read_text()
    marker = '// synchronisation, the renderer never runs behind'
    assert 'SYM_UrbanChaos_VisibilityResult' not in d3d
    (compat / 'd3d8.cpp').write_text(once(d3d, marker, ADAPTER + marker))
    gpu = (compat / 'd3d12.cpp').read_text()
    (compat / 'd3d12.cpp').write_text(once(gpu, 'void D3D12Backend::Flush() { Submit(true); }', FLUSH))

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

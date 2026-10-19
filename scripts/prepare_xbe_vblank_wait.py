"""Wait for the next completed vertical blank instead of sleeping a full period."""
from pathlib import Path
import argparse
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from prepare_xbe_perf import once, function


MARKER = '// UC_VBLANK_COMPLETION_WAIT'
GATE = '''// UC_VBLANK_COMPLETION_WAIT
// The callback must finish before the title resumes and reads its blank count.
// A separate lock avoids holding renderer/kernel locks while guest code runs.
class VerticalBlankCompletion {
public:
    uint64_t BeginTick() {
        std::lock_guard lock(mutex_);
        return ++started_;
    }

    void CompleteTick(uint64_t tick) {
        {
            std::lock_guard lock(mutex_);
            completed_ = tick;
        }
        changed_.notify_all();
    }

    // Acquire callback publication before examining its guest counter. A blank
    // still executing cannot satisfy the PC display-interval enhancement.
    template<class Predicate>
    bool AlreadySatisfied(Predicate&& predicate) {
        std::lock_guard lock(mutex_);
        return started_ == completed_ && predicate();
    }

    void WaitForNextTick() {
        std::unique_lock lock(mutex_);
        const uint64_t observed = started_;
#ifdef UC_VBLANK_WAIT_TEST
        ++waiters_;
        test_changed_.notify_all();
#endif
        changed_.wait(lock, [&] { return completed_ > observed; });
#ifdef UC_VBLANK_WAIT_TEST
        --waiters_;
        test_changed_.notify_all();
#endif
    }

private:
    std::mutex mutex_;
    std::condition_variable changed_;
    uint64_t started_ = 0, completed_ = 0;
#ifdef UC_VBLANK_WAIT_TEST
    friend struct VerticalBlankTestAccess;
    unsigned waiters_ = 0;
    std::condition_variable test_changed_;
#endif
};
'''


def transform(source):
    if MARKER in source:
        raise RuntimeError('Vertical blank wait already applied; regenerate first')
    source = once(source, '#include <chrono>',
                  '#include <chrono>\n#include <condition_variable>\n#include <mutex>')
    source = once(source, 'constexpr bool kVBlank =',
                  GATE + '\nVerticalBlankCompletion g_vblank_completion;\n'
                  '// Thread creation may fail; only the running ticker publishes readiness.\n'
                  'std::atomic<bool> g_vblank_ready{false};\n\nconstexpr bool kVBlank =')
    tick = function(source, 'void VerticalBlank(X86Context& ctx) {')
    changed_tick = once(tick, '    LOGR("UC vertical blank thread started");',
                        '    LOGR("UC vertical blank thread started");\n'
                        '    g_vblank_ready.store(true, std::memory_order_release);')
    changed_tick = once(changed_tick, '        uint32_t count = Dev(XDK_CDevice_VBlankCount) + 1, swaps = Dev(kDevSwapCount);',
                        '        const uint64_t tick = g_vblank_completion.BeginTick();\n'
                        '        uint32_t count = Dev(XDK_CDevice_VBlankCount) + 1, swaps = Dev(kDevSwapCount);')
    changed_tick = once(changed_tick, '        shown = swaps;',
                        '        g_vblank_completion.CompleteTick(tick);\n        shown = swaps;')
    source = once(source, tick, changed_tick)
    source = once(source, '''HLE_STDCALL(D3DDevice_BlockUntilVerticalBlank, 0) {
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
    return 0;
}''', '''HLE_STDCALL(D3DDevice_BlockUntilVerticalBlank, 0) {
    // The original clears the blank event and waits for its next signal. A full
    // 16 ms sleep ignores the current scanout phase and can miss that next blank.
    if (kVBlank && g_backend && g_vblank_ready.load(std::memory_order_acquire)) {
        if (!g_vblank_completion.AlreadySatisfied([&] { return TrialCompletedDisplayInterval(ctx); }))
            g_vblank_completion.WaitForNextTick();
    }
    else std::this_thread::sleep_for(std::chrono::milliseconds(16));
    return 0;
}''')
    return source


def check(source):
    changed = transform(source)
    old_tick = function(source, 'void VerticalBlank(X86Context& ctx) {')
    tick = function(changed, 'void VerticalBlank(X86Context& ctx) {')
    # Readiness and the two host notifications are the only ticker changes:
    # no cadence, event order, callback, guest counter, or clock modifications.
    stripped = tick.replace('        const uint64_t tick = g_vblank_completion.BeginTick();\n', '')
    stripped = stripped.replace('        g_vblank_completion.CompleteTick(tick);\n', '')
    stripped = stripped.replace('    g_vblank_ready.store(true, std::memory_order_release);\n', '')
    assert stripped == old_tick
    assert changed.count('g_vblank_ready.store(true, std::memory_order_release);') == 1
    assert tick.index('mem::AllocSystem(12)') < tick.index('g_vblank_ready.store(true')
    assert tick.index('BeginTick();') < tick.index('SetDev(XDK_CDevice_VBlankCount')
    assert tick.index('CallGuest(ctx, callback, {info});') < tick.index('CompleteTick(tick);')
    assert tick.index('CompleteTick(tick);') < tick.index('ResetEvent(*event);')
    wait = function(changed, 'HLE_STDCALL(D3DDevice_BlockUntilVerticalBlank, 0) {')
    assert 'if (kVBlank && g_backend && g_vblank_ready.load(std::memory_order_acquire))' in wait
    assert 'else std::this_thread::sleep_for(std::chrono::milliseconds(16));' in wait
    assert all(term not in GATE for term in ('W32(', 'SetDev(', 'CallGuest(', 'sleep', 'g_lock'))
    try:
        transform(changed)
    except RuntimeError:
        pass
    else:
        raise AssertionError('Repeated transform accepted')
    return changed


def prepare(area):
    path = Path(area) / 'src/compat/d3d8.cpp'
    changed = check(path.read_text())
    fixture = Path(__file__).with_name('test_xbe_vblank_wait.cpp').read_text()
    fixture = once(fixture, '/* PRODUCTION_VBLANK_COMPLETION */', GATE)
    fixture = once(fixture, '/* PRODUCTION_VBLANK_WAIT_HLE */',
                   function(changed, 'HLE_STDCALL(D3DDevice_BlockUntilVerticalBlank, 0) {'))
    path.write_text(changed)
    (path.parent / 'test_vblank_wait.cpp').write_text(fixture)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--area', type=Path, required=True)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    path = args.area / 'src/compat/d3d8.cpp'
    source = path.read_text()
    if args.check:
        check(source)
        print('Vertical blank wait transform checks passed; no files changed')
    else:
        prepare(args.area)

"""Stage release-only diagnostics removal without changing gameplay or GPU policy."""
from pathlib import Path


def once(source, old, new):
    assert source.count(old) == 1, old
    return source.replace(old, new, 1)


def prepare(sdk, area):
    sdk, area = Path(sdk), Path(area)
    core = area / 'src/core'
    core.mkdir(exist_ok=True)
    original = (sdk / 'runtime/core/log.h').read_text()
    start = original.index('#define LOGD(')
    quiet = '''// Release removes arguments and rate-limit atomics as well as file writes.
#ifdef UC_BURNER_RELEASE
#define LOGD(...) ((void)0)
#define LOGI(...) ((void)0)
#define LOGW(...) ((void)0)
#define LOGR(...) ((void)0)
#define LOG_FIRST(...) ((void)0)
#define LOGE(...) ::logx::Write(::logx::Level::Error, __VA_ARGS__)
#else
'''
    (core / 'log.h').write_text(original[:start] + quiet + original[start:] + '\n#endif\n')
    diag = (sdk / 'runtime/core/diag.h').read_text()
    quiet_diag = '''#pragma once
#include <filesystem>
#ifdef UC_BURNER_RELEASE
namespace diag {
inline constexpr bool Perf() { return false; }
inline const std::filesystem::path& CaptureDir() {
    static const std::filesystem::path empty;
    return empty;
}
}
#else
'''
    (core / 'diag.h').write_text(quiet_diag + diag + '\n#endif\n')

    log = (sdk / 'runtime/core/log.cpp').read_text()
    # Errors still reach the launcher's bounded failure report, but normal play
    # never creates logs, captures, or memory dumps.
    release_log = r'''#include "core/log.h"
#include <cstdio>
#ifdef UC_BURNER_RELEASE
namespace logx {
void Init(const std::string&, const std::string&, bool) {}
const std::string& Path() { static const std::string empty; return empty; }
void Flush() { std::fflush(stderr); }
void Write(Level level, const char* format, ...) {
    if (level != Level::Error) return;
    va_list args; va_start(args, format);
    std::vfprintf(stderr, format, args); va_end(args);
    std::fputc('\n', stderr);
}
}
#else
'''
    (area / 'src/compat/log.cpp').write_text(release_log + log + '\n#endif\n')

    render = (sdk / 'runtime/gpu/render_thread.cpp').read_text()
    # These clocks measure diagnostic spans only; waits and frame pacing remain.
    assert render.count('auto t = Clock::now();') == 5
    render = render.replace('auto t = Clock::now();',
                            'auto t = diag::Perf() ? Clock::now() : Clock::time_point{};')
    render = once(render, 'uint64_t Ns(Clock::time_point t) { return ',
                  'uint64_t Ns(Clock::time_point t) { if (!diag::Perf()) return 0; return ')
    for line in ('g_perf_capture_ns += Ns(t);',
                 '(late ? g_perf_sync_late : g_perf_sync_block)++;',
                 'g_perf_wait_ns += ns;', 'g_perf_wait_kind_ns[size_t(kind)] += ns;',
                 'g_perf_waits++;'):
        assert line in render
        render = render.replace(line, 'if (diag::Perf()) ' + line)
    (area / 'src/compat/render_thread.cpp').write_text(render)

    main_path = area / 'src/compat/main.cpp'
    main = main_path.read_text()
    main = once(main, '    auto dir = ExeDir() / "CrashDumps";',
                '#ifdef UC_BURNER_RELEASE\n    logx::Flush();\n    return EXCEPTION_CONTINUE_SEARCH;\n#else\n'
                '    auto dir = ExeDir() / "CrashDumps";')
    main = once(main, '    return EXCEPTION_CONTINUE_SEARCH;\n}\n\nvoid DumpThreads()',
                '    return EXCEPTION_CONTINUE_SEARCH;\n#endif\n}\n\nvoid DumpThreads()')
    main = once(main, '            if (wp == VK_F3) DumpThreads();',
                '#ifndef UC_BURNER_RELEASE\n            if (wp == VK_F3) DumpThreads();\n#endif')
    main = once(main, '    if (relaunched && !logx::kVerbose) LOGR("Relaunched: %s", IMAGE_XBE);\n    else app::LogSystem();',
                '#ifndef UC_BURNER_RELEASE\n    if (relaunched && !logx::kVerbose) LOGR("Relaunched: %s", IMAGE_XBE);\n    else app::LogSystem();\n#endif')
    main = main.replace('The game failed to start, see the log in the logs folder.',
                        'The game failed to start. Run Burner Edition Setup to verify your imported game files.')
    main_path.write_text(main)

    # No allocator observation wrappers in release: retain original weak bodies.
    probe = area / 'src/common/xbe_heap_probe.cpp'
    probe.write_text('#ifndef UC_BURNER_RELEASE\n' + probe.read_text() + '\n#endif\n')
    header = area / 'src/compat/xbe_audio_loop.h'
    audio = header.read_text()
    audio = once(audio, 'bool Active();', '#ifdef UC_BURNER_RELEASE\ninline constexpr bool Active(){return false;}\ninline constexpr bool SnapshotDue(){return false;}\ninline uint64_t ClockMicros(){return 0;}\ninline void Append(Record){}\ninline void OutputState(uint32_t,uint64_t,uint32_t){}\n#else\nbool Active();')
    audio = once(audio, 'void OutputState(uint32_t queued, uint64_t render_us, uint32_t result);',
                 'void OutputState(uint32_t queued, uint64_t render_us, uint32_t result);\n#endif')
    audio = once(audio, 'bool TrialAudioTraceStart(const char* output_csv);',
                 '#ifdef UC_BURNER_RELEASE\ninline bool TrialAudioTraceStart(const char*){return false;}\n#else\nbool TrialAudioTraceStart(const char* output_csv);\n#endif')
    audio = once(audio, '    return message==0x100',
                 '#ifdef UC_BURNER_RELEASE\n    return false;\n#else\n    return message==0x100')
    audio = once(audio, '        !(lparam&((uint64_t(1)<<29)|(uint64_t(1)<<30)));',
                 '        !(lparam&((uint64_t(1)<<29)|(uint64_t(1)<<30)));\n#endif')
    header.write_text(audio)
    implementation = area / 'src/compat/xbe_audio_loop.cpp'
    implementation.write_text('#ifndef UC_BURNER_RELEASE\n' + implementation.read_text() + '\n#endif\n')

    fixture = Path(__file__).with_name('test_xbe_release.cpp').read_text()
    (area / 'src/compat/test_release.cpp').write_text(fixture)
    from prepare_xbe_release_gpu import prepare as prepare_gpu
    prepare_gpu(area)

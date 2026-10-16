// Release must ignore diagnostic environment flags and never evaluate log data.
#undef NDEBUG
#include "core/log.h"
#include "core/diag.h"
#include "xbe_audio_loop.h"
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <cstdio>

int main() {
    _putenv_s("XBE_PERF", "1");
    _putenv_s("XBE_CAPTURE_DIR", "must-not-create");
    unsigned evaluated=0;
    LOGD("%u",++evaluated); LOGI("%u",++evaluated);
    LOGW("%u",++evaluated); LOGR("%u",++evaluated);
    LOG_FIRST(5,"%u",++evaluated);
    assert(!evaluated&&!diag::Perf()&&diag::CaptureDir().empty());
    logx::Init("must-not-create", "release");
    assert(logx::Path().empty()&&!std::filesystem::exists("must-not-create"));
    assert(!TrialAudioTraceStart("must-not-create/audio.csv"));
    assert(!TrialAudioTraceHotkey(0x100,0x73,0,true,true));
    assert(!uc_audio_trace::Active()&&!uc_audio_trace::SnapshotDue());
    std::puts("release: logs have no argument/atomic work; capture/perf/audio diagnostics cannot activate");
}

// Release must ignore diagnostic environment flags and never evaluate log data.
#undef NDEBUG
#include "core/log.h"
#include "core/diag.h"
#include "xbe_audio_loop.h"
#include "test_release_environment.h"
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <cstdio>

int main(int argc, char** argv) {
    const bool support = argc > 1;
    _putenv_s("UCRR_SESSION_LOG", support ? "1" : "");
    _putenv_s("XBE_PERF", "1");
    _putenv_s("XBE_CAPTURE_DIR", "must-not-create");
    _putenv_s("UCRR_COROUTINE_TRACE", "1");
    _putenv_s("XBE_D3D12_DEBUG", "1");
    TestReleaseCoroutineTrace();
    TestReleaseDeviceDebugLayer();
    unsigned evaluated=0;
    LOGD("%u",++evaluated); LOGI("%u",++evaluated);
    LOGW("%u",++evaluated); LOGR("%u",++evaluated);
    LOG_FIRST(5,"%u",++evaluated);
    assert(!evaluated&&!diag::Perf()&&diag::CaptureDir().empty());
    logx::Init("must-not-create", "release");
    assert(logx::Path().empty()&&!std::filesystem::exists("must-not-create"));
    assert(logx::SessionEnabled() == support);
    unsigned calls=0;
    for(unsigned i=0;i<10;i++) { LOG_FIRST(2,"limited %u",++calls); }
    assert(calls==(support?2u:0u));
    LOGR("enabled argument %u",++evaluated);
    LOGD("never enabled %u",++evaluated);
    assert(evaluated==(support?1u:0u));
    for(unsigned i=0;i<10;i++)logx::SessionFrame(60.0);
    assert(!TrialAudioTraceStart("must-not-create/audio.csv"));
    assert(!TrialAudioTraceHotkey(0x100,0x73,0,true,true));
    assert(!uc_audio_trace::Active()&&!uc_audio_trace::SnapshotDue());
    if(support && std::string(argv[1])=="cap") {
        const std::string message(5000,'x');
        for(unsigned i=0;i<20000;i++)LOGR("%s",message.c_str());
        assert(!logx::SessionEnabled());
        LOGE("after cap");
    }
    std::puts("release: default-off arguments; opt-in bounded support log; heavy captures remain disabled");
}

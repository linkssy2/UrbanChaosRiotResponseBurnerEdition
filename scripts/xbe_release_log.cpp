// Public support logging uses the launcher's bounded stderr pipe; no captures.
#include "core/log.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <fcntl.h>
#include <io.h>

namespace logx {
namespace {
std::atomic<bool> enabled{false};
std::mutex mutex;
size_t remaining = 64 * 1024 * 1024;
const auto start = std::chrono::steady_clock::now();
}
bool SessionEnabled() { return enabled.load(std::memory_order_relaxed); }
void Init(const std::string&, const std::string&, bool) {
    const char* option = std::getenv("UCRR_SESSION_LOG");
    enabled.store(option && std::strcmp(option, "1") == 0, std::memory_order_relaxed);
    if (SessionEnabled()) _setmode(_fileno(stderr), _O_BINARY);
    LOGR("Burner Edition session diagnostics; build %s %s; local logs only", __DATE__, __TIME__);
}
const std::string& Path() { static const std::string empty; return empty; }
void Flush() { std::fflush(stderr); }
void Write(Level level, const char* format, ...) {
    const bool active = SessionEnabled();
    if (!active && level != Level::Error) return;
    std::lock_guard<std::mutex> lock(mutex);
    if (!remaining) return;
    char line[4096];
    double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    int prefix = std::snprintf(line, sizeof(line), "[%.3f %d] ", seconds, int(level));
    va_list args; va_start(args, format);
    std::vsnprintf(line + prefix, sizeof(line) - prefix - 1, format, args);
    va_end(args);
    size_t bytes = std::strlen(line); line[bytes++] = '\n';
    if (active) bytes = std::min(bytes, remaining);
    std::fwrite(line, 1, bytes, stderr);
    std::fflush(stderr);
    if (active) {
        remaining -= bytes;
        if (!remaining) enabled.store(false, std::memory_order_relaxed);
    }
}
void SessionFrame(double fps) {
    if (!SessionEnabled()) return;
    // CountFps already samples every half-second; no additional per-draw clocks.
    static unsigned samples = 0;
    if (++samples % 10 == 0) LOGR("Presentation sample: %.1f FPS", fps);
}
}

// Deterministic admission barriers exercise the actual production gate. This
// checks synchronization semantics, not game timing or live frame-rate gains.
#undef NDEBUG
#define UC_VBLANK_WAIT_TEST 1
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <future>
#include <mutex>
#include <thread>
#include <utility>

/* PRODUCTION_VBLANK_COMPLETION */

// Extract the exact production selector, including its real timed fallback.
static VerticalBlankCompletion g_vblank_completion;
static std::atomic<bool> g_vblank_ready{false};
static bool kVBlank = false, g_backend = false;
struct X86Context {};
static X86Context ctx;
static std::atomic<bool> interval_satisfied{false};
static std::atomic<unsigned> interval_checks{0};
static bool TrialCompletedDisplayInterval(const X86Context&) {
    ++interval_checks;
    return interval_satisfied.load();
}
#define HLE_STDCALL(name, count) static uint32_t name()
/* PRODUCTION_VBLANK_WAIT_HLE */
#undef HLE_STDCALL

using namespace std::chrono_literals;

struct VerticalBlankTestAccess {
    static void Waiters(VerticalBlankCompletion& gate, unsigned count) {
        std::unique_lock lock(gate.mutex_);
        assert(gate.test_changed_.wait_for(lock, 2s, [&] { return gate.waiters_ == count; }));
    }
    static void SpuriousNotification(VerticalBlankCompletion& gate) {
        gate.changed_.notify_all();
    }
    static std::pair<uint64_t, uint64_t> Counters(VerticalBlankCompletion& gate) {
        std::lock_guard lock(gate.mutex_);
        return {gate.started_, gate.completed_};
    }
};

static void StillWaiting(std::future<unsigned>& waiter) {
    assert(waiter.wait_for(10ms) == std::future_status::timeout);
}

static void Finished(std::future<unsigned>& waiter, unsigned expected) {
    assert(waiter.wait_for(2s) == std::future_status::ready);
    assert(waiter.get() == expected);
}

static void CheckAvailability() {
    // A backend/layout alone does not prove that Spawn/CreateThread succeeded.
    // Every unavailable combination must finish even when no ticker ever runs.
    for (bool layout : {false, true})
    for (bool backend : {false, true})
    for (bool ready : {false, true}) {
        if (layout && backend && ready) continue;
        kVBlank = layout;
        g_backend = backend;
        g_vblank_ready.store(ready, std::memory_order_release);
        interval_satisfied = true;
        interval_checks = 0;
        auto unavailable = std::async(std::launch::async, D3DDevice_BlockUntilVerticalBlank);
        Finished(unavailable, 0);
        assert(interval_checks == 0); // Unavailable ticker never grants a skip.
        VerticalBlankTestAccess::Waiters(g_vblank_completion, 0);
        assert((VerticalBlankTestAccess::Counters(g_vblank_completion) ==
                std::pair<uint64_t, uint64_t>{0, 0}));
    }

    kVBlank = g_backend = true;
    g_vblank_ready.store(true, std::memory_order_release);
    interval_satisfied = false;
    interval_checks = 0;
    auto available = std::async(std::launch::async, D3DDevice_BlockUntilVerticalBlank);
    VerticalBlankTestAccess::Waiters(g_vblank_completion, 1);
    StillWaiting(available);
    const auto tick = g_vblank_completion.BeginTick();
    g_vblank_completion.CompleteTick(tick);
    Finished(available, 0);
    assert(interval_checks == 1);

    // A completed, qualified interval needs no additional blank. The helper's
    // guest owner/caller/count matrix is exercised by test_target60 instead.
    interval_satisfied = true;
    const auto before = VerticalBlankTestAccess::Counters(g_vblank_completion);
    auto satisfied = std::async(std::launch::async, D3DDevice_BlockUntilVerticalBlank);
    Finished(satisfied, 0);
    assert(interval_checks == 2);
    assert(VerticalBlankTestAccess::Counters(g_vblank_completion) == before);

    // Even a positive helper cannot skip while the callback is running; it
    // must not read that callback's partially published guest count at all.
    const auto in_flight_tick = g_vblank_completion.BeginTick();
    auto in_flight = std::async(std::launch::async, D3DDevice_BlockUntilVerticalBlank);
    VerticalBlankTestAccess::Waiters(g_vblank_completion, 1);
    assert(interval_checks == 2);
    g_vblank_completion.CompleteTick(in_flight_tick);
    StillWaiting(in_flight);
    const auto next_tick = g_vblank_completion.BeginTick();
    g_vblank_completion.CompleteTick(next_tick);
    Finished(in_flight, 0);
    assert(interval_checks == 2);
}

int main() {
    CheckAvailability();
    VerticalBlankCompletion gate;
    unsigned callback_value = 0;
    // This stands for unrelated guest timing state. Only the callback witness
    // changes in this fixture; the production gate has no guest-memory access.
    std::array<uint32_t, 16> guest_clocks{};
    guest_clocks.fill(0x519f70);
    const auto original_clocks = guest_clocks;
    const auto wait = [&] {
        return std::async(std::launch::async, [&] {
            gate.WaitForNextTick();
            return callback_value;
        });
    };

    // A completed earlier blank is not permission for a new wait to return.
    uint64_t tick = gate.BeginTick();
    callback_value = 1;
    unsigned predicate_calls = 0;
    assert(!gate.AlreadySatisfied([&] { ++predicate_calls; return true; }));
    assert(predicate_calls == 0);
    gate.CompleteTick(tick);
    assert(gate.AlreadySatisfied([&] { ++predicate_calls; return callback_value == 1; }));
    assert(predicate_calls == 1);
    auto first = wait();
    VerticalBlankTestAccess::Waiters(gate, 1);
    StillWaiting(first);
    VerticalBlankTestAccess::SpuriousNotification(gate);
    StillWaiting(first);
    tick = gate.BeginTick();
    StillWaiting(first); // Publishing a start cannot expose an unfinished callback.
    callback_value = 2;
    gate.CompleteTick(tick);
    Finished(first, 2);

    // A wait admitted during a callback must wait for a later start, not the
    // completion of the already-started interrupt.
    tick = gate.BeginTick();
    auto in_flight = wait();
    VerticalBlankTestAccess::Waiters(gate, 1);
    callback_value = 3;
    gate.CompleteTick(tick);
    StillWaiting(in_flight);
    tick = gate.BeginTick();
    callback_value = 4;
    gate.CompleteTick(tick);
    Finished(in_flight, 4);

    // Blank completion is broadcast; one waiter cannot consume another's tick.
    std::array<std::future<unsigned>, 4> many;
    for (auto& waiter : many) waiter = wait();
    VerticalBlankTestAccess::Waiters(gate, 4);
    tick = gate.BeginTick();
    callback_value = 5;
    gate.CompleteTick(tick);
    for (auto& waiter : many) Finished(waiter, 5);

    // Repeated immediate completions cover the snapshot-to-sleep boundary. The
    // predicate retains completion even when a notification precedes sleeping.
    for (unsigned i = 0; i < 200; ++i) {
        auto waiter = wait();
        VerticalBlankTestAccess::Waiters(gate, 1);
        tick = gate.BeginTick();
        callback_value = 6 + i;
        gate.CompleteTick(tick);
        Finished(waiter, 6 + i);
    }

    const auto [started, completed] = VerticalBlankTestAccess::Counters(gate);
    assert(started == 205 && completed == started);
    assert(guest_clocks == original_clocks);
    std::puts("vblank wait: unavailable ticker fallback, qualified completed-interval skip, in-flight rejection, callback publication, multiple waiters, lost/spurious notifications passed; guest clocks unchanged");
}

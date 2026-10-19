// Portable state-machine checks; production guest seams and live input are
// verified separately. No window events or synthetic input are sent here.
#undef NDEBUG
#include <cassert>
#include <cstdio>
#include <initializer_list>
#include "xbe_menu_key_taps.h"

using trial_input::MenuKeyTaps;
static constexpr unsigned enter = 13, escape = 27;

static void Tap(MenuKeyTaps& taps, unsigned key = enter) {
    taps.Event(key, true, false, true);
    taps.Event(key, false, false, true);
}

static bool Sample(MenuKeyTaps& taps, unsigned key = enter,
                   bool physical_down = false) {
    taps.Begin();
    const bool result = physical_down || taps.Retained(key);
    taps.GuestPortPolled(0);
    taps.End();
    return result;
}

int main() {
    MenuKeyTaps taps;

    // A complete down/up between guest polls survives arbitrary peek reads.
    Tap(taps);
    assert(!taps.Retained(enter));
    taps.GuestPortPolled(0); // Outside a guest update cannot consume anything.
    taps.Begin();
    for (unsigned i = 0; i < 32; ++i) assert(taps.Retained(enter));
    taps.GuestPortPolled(1);
    taps.GuestPortPolled(-1);
    taps.End(); // No guest port 0: restore the reserved snapshot.
    assert(Sample(taps));
    assert(!Sample(taps));
    assert(!Sample(taps));

    // Multiple port-0 reads see one stable snapshot, not different edges.
    Tap(taps);
    taps.Begin();
    taps.GuestPortPolled(0);
    assert(taps.Retained(enter));
    taps.GuestPortPolled(0);
    assert(taps.Retained(enter));
    taps.End();

    // A new tap during a sampled update remains pending. It cannot erase the
    // required neutral snapshot between this key's retained pulses.
    assert(!Sample(taps));
    Tap(taps);
    taps.Begin();
    assert(taps.Retained(enter));
    Tap(taps);
    taps.GuestPortPolled(0);
    taps.End();
    assert(!Sample(taps));
    assert(Sample(taps));
    assert(!Sample(taps));

    // An unacknowledged neutral update does not advance the release gap.
    Tap(taps);
    assert(Sample(taps));
    Tap(taps);
    taps.Begin();
    assert(!taps.Retained(enter));
    taps.End();
    assert(!Sample(taps));
    assert(Sample(taps));
    assert(!Sample(taps));

    // A different key is independent; duplicate/repeat messages add no taps.
    taps.Event(enter, true, false, true);
    taps.Event(enter, true, false, true);
    taps.Event(enter, true, true, true);
    assert(Sample(taps));
    Tap(taps, escape);
    taps.Begin();
    assert(!taps.Retained(enter));
    assert(taps.Retained(escape));
    taps.GuestPortPolled(0);
    taps.End();
    taps.Event(enter, false, false, true);
    assert(!Sample(taps));

    // Keep the current hold source: retained-pulse gaps must not release a
    // physically held key, and releasing it must not leave a queued duplicate.
    taps.Clear();
    taps.Event(enter, true, false, true);
    assert(Sample(taps, enter, true));
    for (unsigned i = 0; i < 8; ++i) {
        taps.Event(enter, true, true, true);
        assert(Sample(taps, enter, true));
    }
    taps.Event(enter, false, false, true);
    assert(!Sample(taps));
    assert(!Sample(taps));

    // Focus/owner handoff drops both queued and already published presses.
    Tap(taps);
    taps.Begin();
    assert(taps.Retained(enter));
    taps.Clear();
    taps.GuestPortPolled(0);
    taps.End();
    assert(!Sample(taps));
    // Repeats of a key held across a Clear cannot create another press.
    taps.Event(enter, true, true, true);
    assert(!Sample(taps));
    taps.Event(enter, false, false, true);
    Tap(taps);
    assert(Sample(taps));
    taps.Clear();
    // A fresh non-repeat keydown after focus returns remains usable even if
    // the old focus loss prevented delivery of the previous keyup.
    taps.Event(enter, true, false, true);
    taps.Clear();
    taps.Event(enter, true, false, true);
    assert(Sample(taps));
    taps.Clear();

    // Ineligible events, unknown key codes, and dense taps remain bounded.
    taps.Event(enter, true, false, false);
    taps.Event(enter, true, true, true);
    assert(!Sample(taps));
    taps.Event(enter, false, false, true);
    for (unsigned i = 0; i < 1000; ++i) Tap(taps);
    assert(Sample(taps));
    assert(!Sample(taps));
    assert(!Sample(taps));
    for (unsigned key : {0u, 256u, 0xffffffffu}) {
        taps.Event(key, true, false, true);
        taps.Begin();
        assert(!taps.Retained(key));
        taps.GuestPortPolled(0);
        taps.End();
    }
    std::puts("menu key tap snapshot checks passed");
}

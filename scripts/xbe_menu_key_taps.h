#pragma once
#include <bitset>

namespace trial_input {
// The caller holds input_lock. Physical holds still come from GetAsyncKeyState;
// this snapshot only keeps a window-delivered press alive until a guest read.
class MenuKeyTaps {
    std::bitset<256> down_, pending_, published_, previous_;
    bool active_ = false, sampled_ = false;

public:
    void Event(unsigned key, bool down, bool repeat, bool eligible) {
        if (!key || key >= down_.size()) return;
        const bool was_down = down_[key];
        down_[key] = down;
        if (down && !was_down && !repeat && eligible) pending_.set(key);
    }

    void Begin() {
        if (active_) return;
        active_ = true;
        sampled_ = false;
        // Coalesce presses between updates; an acknowledged empty snapshot
        // separates two retained presses of the same key.
        published_ = pending_ & ~previous_;
        pending_ &= ~published_;
    }

    bool Retained(unsigned key) const {
        return active_ && key < published_.size() && published_[key];
    }

    // Call only from the original guest poll scope, after ownership gates.
    // Connection discovery may peek at Retained but must never acknowledge it.
    void GuestPortPolled(int port) {
        if (active_ && port == 0) sampled_ = true;
    }

    void End() {
        if (!active_) return;
        if (sampled_) previous_ = published_;
        else pending_ |= published_;
        published_.reset();
        active_ = sampled_ = false;
    }

    void Clear() {
        down_.reset();
        pending_.reset();
        published_.reset();
        previous_.reset();
        active_ = sampled_ = false;
    }
};
} // namespace trial_input

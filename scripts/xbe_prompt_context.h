#pragma once
#include <cstdint>
#include <mutex>
struct X86Context;

// The Orders update runs outside HUD rendering; qualify its behavior owner.
bool TrialPromptOrdersKeyboard(const X86Context&,const uint8_t*);

// Original HUD-layer ownership, not screen coordinates. 0 = automatic,
// 1 = co-op keyboard/mouse player, 2 = co-op controller player.
struct TrialPromptPlayerScope {
    unsigned previous;
    TrialPromptPlayerScope();
    ~TrialPromptPlayerScope();
    void Select(const X86Context&,const uint8_t*);
    void Clear();
};

// Text identity follows each original queued quad until its individual draw.
// 0 = ordinary binding, 1 = interaction/speech secondary binding (default F).
struct TrialPromptTextScope {
    unsigned previous;
    TrialPromptTextScope(uint8_t* base,uint32_t text,uint32_t flags,uint32_t caller=0,uint32_t label=0);
    ~TrialPromptTextScope();
};
void TrialPromptQueueRecord(uint8_t* base,uint32_t owner,uint32_t record);
void TrialPromptQueueReset(uint32_t owner);
struct TrialPromptDrawScope {
    std::unique_lock<std::recursive_mutex> lock;
    unsigned previous,previous_player;
    TrialPromptDrawScope(uint8_t* base,uint32_t owner);
    ~TrialPromptDrawScope();
};

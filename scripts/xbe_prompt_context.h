#pragma once
#include <cstdint>
#include <mutex>

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
    unsigned previous;
    TrialPromptDrawScope(uint8_t* base,uint32_t owner);
    ~TrialPromptDrawScope();
};

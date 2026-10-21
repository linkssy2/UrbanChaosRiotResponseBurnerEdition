#pragma once
#include <cstdint>
struct X86Context;

// Published once alongside the title's original port-zero input update. Reading
// the edge does not consume it: all speech readers in that update see the same value.
void TrialConversationPublish(bool down,bool enabled);
bool TrialConversationQuery(X86Context&,uint8_t*);
void TrialConversationPrompt(X86Context&,uint8_t*);
void TrialConversationClose(uint32_t owner);
bool TrialConversationPromptActive();

void TrialReloadPublish(bool down,bool enabled);
void TrialReloadQuery(X86Context&,uint8_t*);

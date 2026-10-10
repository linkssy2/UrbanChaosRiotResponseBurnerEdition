#pragma once
#include "assets.h"
class NativeDialogueText;
class NativePlayerAudio;
//73060/77E60 ordinary Objective messages (icon=-1). Shared request queue,
//original panel/text motion and2.5s lifetime; no objective status mutation.
class NativeObjectiveMessages {
    struct Impl;std::unique_ptr<Impl> p_;
public:
    NativeObjectiveMessages();~NativeObjectiveMessages();
    void load(const std::filesystem::path &,NativeRenderer &,NativeDialogueText &,NativePlayerAudio &);
    int add(int32_t type,const char *title,const char *text,const char *value=nullptr,int32_t icon=-1);
    void step(float delta);
    void draw(NativeRenderer &);
    size_t size()const;
};

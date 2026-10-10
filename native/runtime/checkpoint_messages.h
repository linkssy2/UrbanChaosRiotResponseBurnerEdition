#pragma once
#include "assets.h"
// Actual type15 provider queue and large-font blurred horizontal motion.
// Shared by checkpoint feedback and71200's resolved team-join message.
class NativeCheckpointMessages {
    struct Impl;std::unique_ptr<Impl> p_;
public:
    NativeCheckpointMessages();~NativeCheckpointMessages();
    void load(const std::filesystem::path &,NativeRenderer &,const std::string &resolved_text);
    int add();int add(const char *resolved_text);int step(float);void draw(NativeRenderer &);
    size_t size()const;
};

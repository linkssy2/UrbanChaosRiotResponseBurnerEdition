#pragma once
#include <array>
#include <filesystem>
#include <optional>
#include <vector>
extern "C" {
#include "../checkpoint.h"
#include "../game_difficulty.h"
}

// Native checkpoint progress; never a translated Xbox save or a running-world
// memory dump. Resume rebuilds the authored checkpoint and restores these owners.
struct NativeSaveRecord {
    std::array<uint8_t,32> level_hash{};
    UCCheckpointSnapshot checkpoint{};
    std::vector<int32_t> completed_objectives;
    UCGameDifficulty difficulty{1,.5f,1};
};
class NativeSaveStore {
    std::filesystem::path directory_;
public:
    explicit NativeSaveStore(std::filesystem::path directory):directory_(std::move(directory)){}
    static std::filesystem::path default_directory();
    std::filesystem::path path(unsigned slot)const;
    // Missing returns nullopt; corrupt/unreadable never means a fresh profile.
    std::optional<NativeSaveRecord> load(unsigned slot)const;
    // Durable temporary write, atomic replace, previous version retained as.bak.
    void save(unsigned slot,const NativeSaveRecord &)const;
};

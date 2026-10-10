#pragma once
#include "player_mission.h"
#include "restart_point.h"
#include "simple_cutscene.h"
#include "fade_draw.h"
#include "mission_objective.h"
#include "checkpoint_messages.h"
class NativeDialogueText;
extern "C" {
#include "../game_difficulty.h"
}

// Fresh PC session owner for the actual registered player, checkpoint snapshot,
// intro cameras, checkpoint feedback and fade. Snapshots are retained in memory;
// world_flags bit2 is a real pending world restart, not a teleport or disk save.
// The mission world must close before this owner and its borrowed resources.
class NativeMissionHandoff {
    NativeMissionWorld &world_;
    NativeMissionPlayer &player_;
    NativePlayerCamera &camera_;
    NativeRenderer &renderer_;
    NativePlayerAudio &audio_;
    const UCArchive *archive_;
    std::array<uint8_t,32> hash_{};
    std::filesystem::path catalog_;
    UCAnimationBlendMath math_{};
    std::vector<std::unique_ptr<NativeRestartPoint>> restarts_;
    std::vector<std::unique_ptr<NativeSimpleCutscene>> scenes_;
    uint32_t events_[2]={103,105};
    UCFadeServices fade_services_{};
    NativeGpuMesh fade_mesh_;
    unsigned white_depth_=0;
    NativeObjectiveSavedStatus *objective_saved_=nullptr;
    UCGameDifficulty *difficulty_=nullptr;
    NativeCheckpointMessages messages_;
    std::filesystem::path stream_;
    std::map<NativeRestartPoint*,std::unique_ptr<NativePlayerAudio::MissionSource>> checkpoint_audio_;
    bool feedback_loaded_=false;
    NativeDialogueText *feedback_text_=nullptr;
    int capture_player();
    int checkpoint_sound();
    int control_event(int32_t,uint32_t,uint32_t sender);
    NativeRestartPointServices restart_services();
    NativeSimpleCutsceneServices scene_services();
public:
    NativeCheckpointManager checkpoints;
    uint32_t session_mode=0;
    uint32_t world_flags=0; //world+10; distinct from session+74
    uint8_t adaptive_disabled=0; //fresh5096DC, distinct from difficulty mode
    int camera_mode=-1;
    bool skip=false;
    uint8_t save_pending=0,flashlight=0;
    uint32_t item_extra[27]{},mission[2]{},statistics[18]{};
    UCMainplayerItemSpec specs[27]{};
    UCCheckpointSnapshot snapshot{};
    uint64_t snapshots=0,takeovers=0,returns=0;
    UCEnvironmentHistoryEffect history{};
    UCFade fade{2,0,0,0,nullptr,-1,0};
    NativeMissionHandoff(NativeMissionWorld &,NativeMissionPlayer &,NativePlayerCamera &,
        NativeRenderer &,NativePlayerAudio &,const UCArchive *,const uint8_t hash[32],
        const std::filesystem::path &assets);
    int construct(uint32_t);
    void bind_checkpoint_state(NativeObjectiveSavedStatus &s,UCGameDifficulty &d){objective_saved_=&s;difficulty_=&d;}
    void load_feedback(const std::filesystem::path &game,NativeDialogueText &);
    int hud_message(const char *resource);
    int step_feedback(float delta){return messages_.step(delta);}
    void draw_feedback(){messages_.draw(renderer_);}
    int request_restart(int32_t number);
    bool supports(const char *)const;
    int helper_tick(void *,float);
    int step_fade(float delta){return uc_fade_step(&fade,&fade_services_,delta);}
    int close_fade(){return uc_fade_close(&fade,&fade_services_);}
    void draw_fade(){native_fade_draw(renderer_,fade_mesh_,fade);}
    NativeSimpleCutscene *scene()const;
    NativeRestartPoint *restart(int32_t number)const;
};

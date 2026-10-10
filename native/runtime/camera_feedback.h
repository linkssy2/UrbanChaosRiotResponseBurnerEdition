#pragma once
#include "mission_world.h"
#include <vector>
extern "C" {
#include "../archive_reader/archive_reader.h"
}
extern "C" {
#include "../feedback_manager.h"
#include "../scene_camera_world.h"
}
struct NativeFirstperson;
/* Shared55E00 camera feedback for the current native single-view keyboard
 * input provider. physical_port is that provider's live index (-1 keyboard).
 * Four output handles have real registration/removal lifetime here; no
 * controller motor API is claimed. A future physical port rejects the bounded
 * tick instead of fabricating suppressed output. All spatial/timed pulses and
 * observer membership remain owned by the one original-derived manager.
 * The world, RNG, port and bound player must outlive this owner. */
class NativeCameraFeedback {
 NativeMissionWorld *world_;UCVectorRandom *random_;const int32_t *physical_port_;
 UCFeedbackManager *manager_=nullptr;UCFeedbackOutput *outputs_[4]{};
 bool registered_=false;NativeFirstperson *player_=nullptr;
public:
 UCSceneCameraAuxiliary auxiliary{};
 NativeCameraFeedback(NativeMissionWorld&,UCVectorRandom&,const int32_t &physical_port);
 ~NativeCameraFeedback();
 NativeCameraFeedback(const NativeCameraFeedback&)=delete;
 NativeCameraFeedback&operator=(const NativeCameraFeedback&)=delete;
 int bind(NativeFirstperson&);int step();int close();
 int spatial(const float position[3],float radius,float amplitude,float duration,uint32_t kind);
 UCFeedbackManager *manager()const{return manager_;}
};

// F45A0/F4B00 retained position receiver. Admits the authored1-01 fade3,
// non-player-centric, bank-only (no #event) domain; other domains reject.
class NativeTriggeredCameraShake {
 NativeMissionWorld *world_=nullptr;UCFeedbackManager *manager_=nullptr;
 UCSpatialFeedbackEffect effect_{};UCFeedbackPositionObserver observer_{};
 std::vector<uint32_t> flags_;float duration_=0,remaining_=0,amplitude_=0;
 bool active_=false,latched_=false;
 int stop();
public:
 std::vector<std::unique_ptr<NativeTriggeredCameraShake>> *owners=nullptr;
 NativeMissionEntity entity;
 int construct(NativeMissionWorld &,uint32_t,UCFeedbackManager *,const UCArchive *);
 int update();int late();int remove();int close_detached();
 bool active()const{return active_;}
 float amplitude()const{return effect_.amplitude;}
 const UCFeedbackPositionObserver *observer()const{return &observer_;}
};

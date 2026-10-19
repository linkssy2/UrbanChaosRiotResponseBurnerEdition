#pragma once
#include "app/app.h"
#include <string>

// Persist through GameInfo.options; zero follows the selected display at boot.
extern uint32_t TrialAspectRatio;
extern uint32_t TrialMsaaSamples;
extern uint32_t TrialFieldOfView;
extern uint32_t TrialReticleScale;
extern uint32_t TrialWeaponFovScale;
extern bool TrialSceneryLighting, TrialCharacterLighting;
extern bool TrialHdUi;
extern uint32_t TrialCharacterQuality, TrialRagdollMode;
namespace trial_graphics {
struct Draft {
    app::Settings settings;
    uint32_t aspect = 0, msaa = 1;
    bool scenery = true, characters = true;
    uint32_t character_quality = 0, ragdolls = 0;
    uint32_t fov = 53; // vertical degrees; retail player camera default
    bool hd_ui = false;
    uint32_t reticle_scale = 100; // percentage of the authored crosshair size
    uint32_t weapon_fov = 100; // percentage of each weapon's authored view window
};
Draft Snapshot();
void Initialize();
bool Widescreen();
uint32_t FieldOfView();
uint32_t ReticleScale();
uint32_t WeaponFovScale();
bool HdUi(); // committed selection; both font atlases are already resident
bool FullscreenRequested();
bool ScalePending();
void SetScalePending(bool pending); // renderer frame/clear boundaries only
uint32_t MsaaSamples();
bool MsaaSupported(uint32_t samples);
void PublishMsaaSupport(uint32_t supported_mask); // renderer's checked formats, bit N = N samples
bool SceneryLighting();
bool CharacterLighting();
uint32_t CharacterQuality(); // 0 full, 1 simple; player lighting stays intact
uint32_t RagdollMode(); // 0 original, 1 more, 2 retain until mission teardown
bool Apply(const Draft& draft, std::string& message);
// Validation is separate from the platform's fullscreen-mode availability test.
bool Validate(const Draft& draft, app::Mode desktop, std::string& message);
}

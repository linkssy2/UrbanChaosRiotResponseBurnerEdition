#pragma once
#include "app/app.h"
#include <string>
#include <array>
#include <algorithm>

// Persist through GameInfo.options; zero follows the selected display at boot.
// Stable IDs: 1=4:3, 2=16:9, 3=21:9, 4=32:9, 5=16:10.
extern uint32_t TrialAspectRatio;
extern uint32_t TrialMsaaSamples;
extern uint32_t TrialFieldOfView;
extern uint32_t TrialReticleScale;
extern std::array<uint32_t,4> TrialReticleColors[2];
extern uint32_t TrialWeaponFovScale;
// Missing values migrate once from the existing camera settings at startup.
extern uint32_t TrialPlayer2FieldOfView, TrialPlayer2WeaponFovScale;
extern uint32_t TrialFmvScale;
extern bool TrialSceneryLighting, TrialCharacterLighting;
extern bool TrialHdUi;
extern bool TrialWorldTextures;
extern bool TrialDeathConvulsions;
extern bool TrialGameplayBlur;
extern bool TrialFrameGhosting;
extern uint32_t TrialCharacterQuality, TrialRagdollMode;
namespace trial_graphics {
// Zero retains the authored state color. Values are stable settings-file IDs.
inline constexpr uint32_t ReticlePalette[]={0,0xffffff,0xff4040,0x40ff40,0x00ffff,0xffff00,0xffa000,0xff60c0,0xb080ff,0x4080ff};
inline constexpr const char* ReticleColorNames[]={"Original","White","Red","Green","Cyan","Yellow","Orange","Pink","Purple","Blue"};
uint32_t ReticleColor(unsigned player,unsigned state);
struct Draft {
    app::Settings settings;
    uint32_t aspect = 0, msaa = 1;
    bool scenery = true, characters = true;
    uint32_t character_quality = 0, ragdolls = 0;
    uint32_t fov = 53; // vertical degrees; retail player camera default
    bool hd_ui = false;
    uint32_t reticle_scale = 100; // percentage of the authored crosshair size
    bool world_textures = false;
    uint32_t weapon_fov = 100; // percentage of each weapon's authored view window
    uint32_t fmv_scale = 0; // 0 proportional fill, 1 original frame, 2 borderless stretch
    std::array<uint32_t,4> reticle_colors{}; // normal, enemy, friendly, spin
    bool death_convulsions = true;
    bool gameplay_blur = true;
    bool frame_ghosting = true;
};
Draft Snapshot();
Draft Snapshot(unsigned player);
void Initialize();
bool Widescreen();
float AspectRatio(); // fixed at startup, like the guest's root viewport
inline float HudScale(){return (4.f/3.f)/AspectRatio();}
struct ViewRect {float x,y,width,height;};
inline ViewRect ViewBounds(float width,float height,bool menu=false){
    if(width<=0||height<=0)return {};
    const float aspect=AspectRatio();
    const float w=(std::min)(width,height*aspect),h=w/aspect;
    const float content=menu?(std::min)(w,h*(16.f/9.f)):w;
    return {(width-content)*.5f,(height-h)*.5f,content,h};
}
float GameViewWidth(); // displayed game pixels after aspect bars; zero if no valid client area
uint32_t FieldOfView();
uint32_t FieldOfView(unsigned player);
uint32_t ReticleScale();
uint32_t WeaponFovScale();
uint32_t WeaponFovScale(unsigned player);
uint32_t FmvScale();
bool WorldTextures();
bool WorldTexturesAvailable();
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
bool DeathConvulsions();
bool GameplayBlur();
bool FrameGhosting();
bool Apply(const Draft& draft, std::string& message);
bool Apply(const Draft& draft, std::string& message, unsigned player);
// Validation is separate from the platform's fullscreen-mode availability test.
bool Validate(const Draft& draft, app::Mode desktop, std::string& message);
}

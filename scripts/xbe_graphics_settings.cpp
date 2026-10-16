#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <algorithm>
#include <atomic>
#include <exception>
#include "xbe_graphics_settings.h"

uint32_t TrialAspectRatio = 0;
uint32_t TrialMsaaSamples = 1;
bool TrialSceneryLighting = true, TrialCharacterLighting = true;
uint32_t TrialCharacterQuality = 0, TrialRagdollMode = 0;
namespace trial_graphics {
static std::atomic<bool> boot_wide{true};
static std::atomic<bool> scale_pending{false};
static std::atomic<uint32_t> msaa_samples{1},msaa_supported{2};
static std::atomic<uint32_t> effects{3};
bool SceneryLighting(){return effects.load()&1;}
bool CharacterLighting(){return effects.load()&2;}
uint32_t CharacterQuality(){return (effects.load()>>2)&1;}
uint32_t RagdollMode(){return effects.load()>>3;}
static void PublishEffects(const Draft& d){
    effects=uint32_t(d.scenery)|(uint32_t(d.characters)<<1)|(d.character_quality<<2)|(d.ragdolls<<3);
}
static Draft Current(){return {app::Get(),TrialAspectRatio,TrialMsaaSamples,
    TrialSceneryLighting,TrialCharacterLighting,TrialCharacterQuality,TrialRagdollMode};}
static void Store(const Draft& d){
    app::Get()=d.settings;TrialAspectRatio=d.aspect;TrialMsaaSamples=d.msaa;
    TrialSceneryLighting=d.scenery;TrialCharacterLighting=d.characters;
    TrialCharacterQuality=d.character_quality;TrialRagdollMode=d.ragdolls;
}
uint32_t MsaaSamples(){return msaa_samples.load();}
bool MsaaSupported(uint32_t samples){return samples<32&&(msaa_supported.load()&(1u<<samples));}
void PublishMsaaSupport(uint32_t mask){
    std::lock_guard lock(app::UiLock());
    msaa_supported=mask|2u;
    if(!MsaaSupported(TrialMsaaSamples))TrialMsaaSamples=1;
    msaa_samples=TrialMsaaSamples;
}

static bool RequestedWide(const Draft& d, app::Mode desktop) {
    if (d.aspect) return d.aspect == 2;
    const auto& s = d.settings;
    auto m = s.width && s.height ? app::Mode{s.width,s.height} : desktop;
    return uint64_t(m.width)*2 >= uint64_t(m.height)*3;
}

bool Validate(const Draft& d, app::Mode desktop, std::string& message) {
    const auto& s=d.settings;
    if(uint32_t(s.display_mode)>2 || d.aspect>2) message="Invalid display or aspect setting.";
    else if((s.width==0)!=(s.height==0) || (s.width &&
            (s.width<320 || s.height<240 || s.width>7680 || s.height>4320)))
        message="Choose Desktop or a resolution between 320 x 240 and 7680 x 4320.";
    else if(s.render_percent<25 || s.render_percent>200)
        message="Internal resolution must be between 25% and 200%.";
    else if(d.msaa!=1&&d.msaa!=2&&d.msaa!=4)
        message="Choose Off, 2x or 4x MSAA.";
    else if(d.character_quality>1||d.ragdolls>2)
        message="Invalid character lighting or ragdoll setting.";
    else if(s.aniso!=0 && s.aniso!=1 && s.aniso!=2 && s.aniso!=4 && s.aniso!=8 && s.aniso!=16)
        message="Choose automatic, 1x, 2x, 4x, 8x or 16x texture filtering.";
    else if(s.fps_cap && (s.fps_cap<20 || s.fps_cap>240))
        message="Choose unlimited or an FPS limit between 20 and 240.";
    else {
        auto output=s.width ? app::Mode{s.width,s.height} : desktop;
        // Guest targets can be 4:3 even while presentation is widescreen.
        uint64_t h=(uint64_t(output.height)*s.render_percent+99)/100;
        if(!output.width || !output.height || h>12288)
            message="That internal resolution exceeds the renderer's texture limit.";
        else { message.clear();return true; }
    }
    return false;
}

Draft Snapshot() {
    std::lock_guard lock(app::UiLock());
    return Current();
}

void Initialize() {
    std::lock_guard lock(app::UiLock());
    Draft d=Current();
    std::string ignored;
    if(!Validate(d,app::Desktop(),ignored)) {
        // Invalid edited files must not request huge resources during boot.
        auto join=app::Get().join_ip;
        d={};d.settings.join_ip=std::move(join);Store(d);
    }
    boot_wide.store(RequestedWide(d,app::Desktop()));
    msaa_samples=d.msaa;
    PublishEffects(d);
}

bool Widescreen() { return boot_wide.load(); }
bool ScalePending() { return scale_pending.load(); }
void SetScalePending(bool pending) { scale_pending.store(pending); }
bool FullscreenRequested() {
    auto s=Snapshot().settings;
    return s.display_mode==app::DisplayMode::Fullscreen && s.width && s.height;
}

static bool DisplaySupported(const app::Settings& s,std::string& message) {
    if(s.display_mode!=app::DisplayMode::Fullscreen || !s.width) return true;
    HMONITOR monitor=MonitorFromWindow(static_cast<HWND>(app::Window()),MONITOR_DEFAULTTOPRIMARY);
    MONITORINFOEXW info{};info.cbSize=sizeof(info);
    if(!GetMonitorInfoW(monitor,&info)) { message="Could not inspect the current monitor.";return false; }
    DEVMODEW mode{};mode.dmSize=sizeof(mode);
    mode.dmPelsWidth=s.width;mode.dmPelsHeight=s.height;
    mode.dmFields=DM_PELSWIDTH|DM_PELSHEIGHT;
    if(ChangeDisplaySettingsExW(info.szDevice,&mode,nullptr,CDS_TEST,nullptr)!=DISP_CHANGE_SUCCESSFUL) {
        message="This monitor does not support that fullscreen resolution. Try windowed or borderless.";
        return false;
    }
    return true;
}

bool Apply(const Draft& draft,std::string& message) {
    // Called by the renderer's UI callback, under its existing recursive lock.
    // The window receives only an asynchronous display request after saving.
    std::lock_guard lock(app::UiLock());
    auto desktop=app::Desktop();
    if(!Validate(draft,desktop,message) || !DisplaySupported(draft.settings,message)) return false;
    if(!MsaaSupported(draft.msaa)){message="That MSAA mode is not supported by this graphics device.";return false;}
    const Draft old=Current();
    bool display=old.settings.display_mode!=draft.settings.display_mode ||
                 old.settings.width!=draft.settings.width || old.settings.height!=draft.settings.height;
    Store(draft);
    app::Get().join_ip=old.settings.join_ip;
    try { app::Save(); }
    catch(const std::exception& e) {
        Store(old);
        message=std::string("Could not save graphics settings: ")+e.what();return false;
    }
    msaa_samples=draft.msaa;
    PublishEffects(draft);
    if(display) app::ApplyDisplay();
    if(RequestedWide(draft,desktop)!=boot_wide.load())
        message="Changes applied and saved. Restart to update the game camera aspect.";
    else if(old.settings.render_percent!=draft.settings.render_percent || old.settings.height!=draft.settings.height||old.msaa!=draft.msaa)
        message="Changes saved. Rendering quality updates when the game redraws the scene.";
    else message="Changes applied and saved.";
    return true;
}
}

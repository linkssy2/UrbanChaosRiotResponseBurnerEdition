#undef NDEBUG
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdio>
#include <exception>
#include <fstream>
#include <mutex>
#include <sstream>
#include <system_error>
#include "xbe_graphics_settings.h"

static bool platform_supported=true;
namespace app {
static Settings settings;
static std::recursive_mutex ui_lock;
static std::filesystem::path file;
static bool fail_save=false;
static unsigned display_posts=0;
Settings& Get() { return settings; }
std::recursive_mutex& UiLock() { return ui_lock; }
Mode Desktop() { return {2560,1440}; }
void ApplyDisplay() { ++display_posts; }
static std::filesystem::path File() { return file; }
static std::string Table() {
    if(fail_save)throw std::runtime_error("injected write error");
    auto& s=Get();
    return "width="+std::to_string(s.width)+"\nheight="+std::to_string(s.height)+
        "\nrender_percent="+std::to_string(s.render_percent)+
        "\naspect_ratio="+std::to_string(TrialAspectRatio)+"\nmsaa_samples="+std::to_string(TrialMsaaSamples)+
        "\nscenery_lighting="+std::to_string(TrialSceneryLighting)+"\ncharacter_lighting="+std::to_string(TrialCharacterLighting)+
        "\ncharacter_quality="+std::to_string(TrialCharacterQuality)+"\nragdoll_amount="+std::to_string(TrialRagdollMode);
}
// GENERATED_SAVE
}
// GENERATED_SETTINGS

static std::string Read() {
    std::ifstream f(app::file);return {std::istreambuf_iterator<char>(f),{}};
}
int main() {
    app::file=std::filesystem::temp_directory_path()/
        (L"uc-graphics-fixture-"+std::to_wstring(GetCurrentProcessId())+L".toml");
    app::settings.join_ip="retained";
    trial_graphics::Initialize();assert(trial_graphics::Widescreen());
    assert(trial_graphics::MsaaSamples()==1&&!trial_graphics::MsaaSupported(4));
    assert(trial_graphics::SceneryLighting()&&trial_graphics::CharacterLighting());
    assert(trial_graphics::CharacterQuality()==0&&trial_graphics::RagdollMode()==0);
    trial_graphics::PublishMsaaSupport((1u<<2)|(1u<<4));
    auto draft=trial_graphics::Snapshot();std::string message;
    draft.settings.width=2560;draft.settings.height=1440;
    draft.settings.render_percent=150;draft.settings.aniso=16;draft.settings.vsync=false;
    draft.settings.fps_cap=120;draft.settings.display_mode=app::DisplayMode::Windowed;
    draft.settings.join_ip="must not overwrite unrelated setting";
    draft.msaa=4;
    draft.scenery=false;draft.characters=false;draft.character_quality=1;draft.ragdolls=2;
    assert(trial_graphics::Apply(draft,message));
    assert(app::display_posts==1&&app::settings.render_percent==150&&app::settings.aniso==16);
    assert(app::settings.fps_cap==120&&!app::settings.vsync&&app::settings.join_ip=="retained");
    auto saved=Read();assert(saved.find("width=2560")!=std::string::npos);
    assert(saved.find("msaa_samples=4")!=std::string::npos&&trial_graphics::MsaaSamples()==4);
    assert(saved.find("ragdoll_amount=2")!=std::string::npos&&!trial_graphics::SceneryLighting()&&!trial_graphics::CharacterLighting());
    assert(trial_graphics::CharacterQuality()==1&&trial_graphics::RagdollMode()==2);
    draft=trial_graphics::Snapshot();draft.settings.render_percent=50;
    assert(trial_graphics::Apply(draft,message)&&app::display_posts==1);
    assert(message.find("redraws")!=std::string::npos);
    draft.aspect=1;assert(trial_graphics::Apply(draft,message));
    assert(message.find("Restart")!=std::string::npos&&trial_graphics::Widescreen());
    assert(TrialAspectRatio==1);trial_graphics::Initialize();assert(!trial_graphics::Widescreen());
    saved=Read();auto before=trial_graphics::Snapshot();
    app::fail_save=true;draft.settings.width=1920;draft.settings.height=1080;draft.aspect=2;draft.msaa=2;
    draft.scenery=true;draft.characters=true;draft.character_quality=0;draft.ragdolls=0;
    assert(!trial_graphics::Apply(draft,message));app::fail_save=false;
    assert(Read()==saved&&app::settings.width==before.settings.width&&TrialAspectRatio==before.aspect);
    assert(TrialMsaaSamples==4&&trial_graphics::MsaaSamples()==4);
    assert(!TrialSceneryLighting&&!TrialCharacterLighting&&TrialCharacterQuality==1&&TrialRagdollMode==2);
    assert(!trial_graphics::SceneryLighting()&&!trial_graphics::CharacterLighting()&&trial_graphics::CharacterQuality()==1&&trial_graphics::RagdollMode()==2);
    assert(app::display_posts==1);
    platform_supported=false;assert(!trial_graphics::Apply(draft,message));platform_supported=true;
    assert(Read()==saved&&app::settings.width==before.settings.width);
    for(unsigned which=0;which<10;++which) {
        auto bad=before;
        if(which==0)bad.settings.width=0;
        if(which==1)bad.settings.render_percent=0;
        if(which==2)bad.settings.aniso=3;
        if(which==3)bad.settings.fps_cap=1;
        if(which==4)bad.aspect=3;
        if(which==5)bad.settings.display_mode=app::DisplayMode(3);
        if(which==6)bad.settings.height=99999;
        if(which==7)bad.msaa=3;
        if(which==8)bad.character_quality=2;
        if(which==9)bad.ragdolls=3;
        assert(!trial_graphics::Apply(bad,message));
    }
    assert(Read()==saved&&app::display_posts==1);
    trial_graphics::PublishMsaaSupport(1u<<2);
    assert(TrialMsaaSamples==1&&trial_graphics::MsaaSamples()==1);
    draft=trial_graphics::Snapshot();draft.msaa=4;
    assert(!trial_graphics::Apply(draft,message)&&trial_graphics::MsaaSamples()==1);
    for(bool enabled:{true,false,true}){
        draft=trial_graphics::Snapshot();draft.scenery=enabled;draft.characters=enabled;
        draft.character_quality=enabled?0:1;draft.ragdolls=enabled?0:1;
        assert(trial_graphics::Apply(draft,message));
        assert(trial_graphics::SceneryLighting()==enabled&&trial_graphics::CharacterLighting()==enabled);
        assert(trial_graphics::CharacterQuality()==draft.character_quality&&trial_graphics::RagdollMode()==draft.ragdolls);
    }
    trial_graphics::SetScalePending(true);assert(trial_graphics::ScalePending());
    trial_graphics::SetScalePending(false);assert(!trial_graphics::ScalePending());
    std::filesystem::remove(app::file);
    std::puts("graphics settings: staged apply, persistence rollback, validation, aspect restart and display routing passed");
}

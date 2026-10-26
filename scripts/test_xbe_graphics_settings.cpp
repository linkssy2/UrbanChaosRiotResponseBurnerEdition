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
#include "xbe_world_textures.h"

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
        "\nfield_of_view="+std::to_string(TrialFieldOfView)+"\nrender_percent="+std::to_string(s.render_percent)+
        "\naspect_ratio="+std::to_string(TrialAspectRatio)+"\nmsaa_samples="+std::to_string(TrialMsaaSamples)+
        "\nscenery_lighting="+std::to_string(TrialSceneryLighting)+"\ncharacter_lighting="+std::to_string(TrialCharacterLighting)+
        "\ncharacter_quality="+std::to_string(TrialCharacterQuality)+"\nragdoll_amount="+std::to_string(TrialRagdollMode)+
        "\nhd_ui="+std::to_string(TrialHdUi)+"\nreticle_scale="+std::to_string(TrialReticleScale)+
        "\nweapon_fov_scale="+std::to_string(TrialWeaponFovScale)+"\nfmv_scale="+std::to_string(TrialFmvScale)+
        "\nplayer2_field_of_view="+std::to_string(TrialPlayer2FieldOfView)+
        "\nplayer2_weapon_fov_scale="+std::to_string(TrialPlayer2WeaponFovScale);
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
    assert(!TrialWorldTextures&&!trial_graphics::WorldTextures());
    assert(!TrialHdUi&&!trial_graphics::HdUi()&&!trial_graphics::Snapshot().hd_ui);
    assert(TrialReticleScale==100&&trial_graphics::ReticleScale()==100);
    assert(TrialWeaponFovScale==100&&trial_graphics::WeaponFovScale()==100);
    assert(TrialFmvScale==0&&trial_graphics::FmvScale()==0&&trial_graphics::Snapshot().fmv_scale==0);
    assert(trial_graphics::MsaaSamples()==1&&!trial_graphics::MsaaSupported(4));
    assert(trial_graphics::SceneryLighting()&&trial_graphics::CharacterLighting());
    assert(trial_graphics::CharacterQuality()==0&&trial_graphics::RagdollMode()==0);
    trial_graphics::PublishMsaaSupport((1u<<2)|(1u<<4));
    auto draft=trial_graphics::Snapshot();std::string message;
    draft.settings.width=2560;draft.settings.height=1440;
    draft.settings.render_percent=150;draft.settings.aniso=16;draft.settings.vsync=false;
    draft.settings.fps_cap=120;draft.settings.display_mode=app::DisplayMode::Windowed;
    draft.settings.join_ip="must not overwrite unrelated setting";
    draft.world_textures=true;draft.msaa=4;draft.fov=70;draft.hd_ui=true;draft.reticle_scale=150;draft.weapon_fov=125;
    draft.fmv_scale=1;assert(TrialFmvScale==0&&trial_graphics::FmvScale()==0);
    assert(trial_graphics::WeaponFovScale()==100);
    assert(!TrialHdUi&&!trial_graphics::HdUi()); // Draft edits cannot change live rendering.
    draft.scenery=false;draft.characters=false;draft.character_quality=1;draft.ragdolls=2;
    assert(trial_graphics::Apply(draft,message));
    assert(app::display_posts==1&&app::settings.render_percent==150&&app::settings.aniso==16);
    assert(app::settings.fps_cap==120&&!app::settings.vsync&&app::settings.join_ip=="retained");
    assert(trial_graphics::FieldOfView()==70&&trial_graphics::ReticleScale()==150);
    assert(trial_graphics::WeaponFovScale()==125);
    assert(TrialFmvScale==1&&trial_graphics::FmvScale()==1&&trial_graphics::Snapshot().fmv_scale==1);
    assert(TrialWorldTextures&&trial_graphics::WorldTextures());
    assert(TrialHdUi&&trial_graphics::HdUi()&&trial_graphics::Snapshot().hd_ui);
    auto invalid_fov=draft;invalid_fov.fov=0;assert(!trial_graphics::Apply(invalid_fov,message));
    invalid_fov.fov=91;assert(!trial_graphics::Apply(invalid_fov,message));
    assert(trial_graphics::FieldOfView()==70&&trial_graphics::ReticleScale()==150);
    auto saved=Read();assert(saved.find("reticle_scale=150")!=std::string::npos);assert(saved.find("hd_ui=1")!=std::string::npos);assert(saved.find("field_of_view=70")!=std::string::npos);assert(saved.find("width=2560")!=std::string::npos);
    assert(saved.find("weapon_fov_scale=125")!=std::string::npos);
    assert(saved.find("fmv_scale=1")!=std::string::npos);
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
    draft.scenery=true;draft.characters=true;draft.character_quality=0;draft.ragdolls=0;draft.hd_ui=false;draft.reticle_scale=50;draft.weapon_fov=50;
    draft.fmv_scale=0;
    assert(!trial_graphics::Apply(draft,message));app::fail_save=false;
    assert(Read()==saved&&app::settings.width==before.settings.width&&TrialAspectRatio==before.aspect);
    assert(TrialReticleScale==150&&trial_graphics::ReticleScale()==150);
    assert(TrialWeaponFovScale==125&&trial_graphics::WeaponFovScale()==125);
    assert(TrialFmvScale==1&&trial_graphics::FmvScale()==1&&trial_graphics::Snapshot().fmv_scale==1);
    assert(TrialMsaaSamples==4&&trial_graphics::MsaaSamples()==4);
    assert(TrialWorldTextures&&trial_graphics::WorldTextures());
    assert(TrialHdUi&&trial_graphics::HdUi()&&trial_graphics::Snapshot().hd_ui);
    assert(!TrialSceneryLighting&&!TrialCharacterLighting&&TrialCharacterQuality==1&&TrialRagdollMode==2);
    assert(!trial_graphics::SceneryLighting()&&!trial_graphics::CharacterLighting()&&trial_graphics::CharacterQuality()==1&&trial_graphics::RagdollMode()==2);
    assert(app::display_posts==1);
    platform_supported=false;assert(!trial_graphics::Apply(draft,message));platform_supported=true;
    assert(Read()==saved&&app::settings.width==before.settings.width);
    for(unsigned which=0;which<11;++which) {
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
        if(which==10)bad.fmv_scale=2;
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
    for(bool enabled:{false,true,false}){
        draft=trial_graphics::Snapshot();draft.hd_ui=enabled;
        const unsigned posts=app::display_posts;
        assert(trial_graphics::Apply(draft,message));
        assert(TrialHdUi==enabled&&trial_graphics::HdUi()==enabled&&app::display_posts==posts);
    }
    for(unsigned size:{50u,100u,200u}){
        draft=trial_graphics::Snapshot();draft.reticle_scale=size;
        const unsigned posts=app::display_posts;
        assert(trial_graphics::Apply(draft,message));
        assert(TrialReticleScale==size&&trial_graphics::ReticleScale()==size&&app::display_posts==posts);
    }
    for(unsigned size:{0u,49u,201u,0xffffffffu}){
        draft=trial_graphics::Snapshot();draft.reticle_scale=size;
        assert(!trial_graphics::Apply(draft,message));assert(trial_graphics::ReticleScale()==200);
    }
    TrialReticleScale=125;trial_graphics::Initialize();assert(trial_graphics::ReticleScale()==125);
    for(unsigned size:{50u,100u,150u}){
        draft=trial_graphics::Snapshot();draft.weapon_fov=size;
        const unsigned posts=app::display_posts;
        assert(trial_graphics::Apply(draft,message));
        assert(TrialWeaponFovScale==size&&trial_graphics::WeaponFovScale()==size&&app::display_posts==posts);
        assert(trial_graphics::FieldOfView()==70&&trial_graphics::ReticleScale()==125);
    }
    for(unsigned size:{0u,49u,151u,0xffffffffu}){
        draft=trial_graphics::Snapshot();draft.weapon_fov=size;
        assert(!trial_graphics::Apply(draft,message));assert(trial_graphics::WeaponFovScale()==150);
    }
    TrialWeaponFovScale=120;trial_graphics::Initialize();assert(trial_graphics::WeaponFovScale()==120);
    TrialHdUi=true;trial_graphics::Initialize();assert(trial_graphics::HdUi());
    TrialHdUi=false;trial_graphics::Initialize();assert(!trial_graphics::HdUi());
    for(unsigned cap:{30u,60u,120u,144u,240u,0u}){
        draft=trial_graphics::Snapshot();draft.settings.fps_cap=cap;
        assert(trial_graphics::Apply(draft,message));
    }
    for(unsigned scale:{0u,1u,0u}){
        draft=trial_graphics::Snapshot();draft.fmv_scale=scale;
        const unsigned posts=app::display_posts;
        assert(trial_graphics::Apply(draft,message));
        assert(TrialFmvScale==scale&&trial_graphics::FmvScale()==scale&&app::display_posts==posts);
        assert(Read().find("fmv_scale="+std::to_string(scale))!=std::string::npos);
    }
    for(unsigned scale:{2u,0xffffffffu}){
        draft=trial_graphics::Snapshot();draft.fmv_scale=scale;
        assert(!trial_graphics::Apply(draft,message));assert(trial_graphics::FmvScale()==0);
    }
    TrialFmvScale=1;trial_graphics::Initialize();assert(trial_graphics::FmvScale()==1);
    TrialFmvScale=2;trial_graphics::Initialize();
    assert(TrialFmvScale==0&&trial_graphics::FmvScale()==0&&app::settings.join_ip=="retained");
    // Existing installs migrate P2 once, before either player's first edit.
    TrialFieldOfView=70;TrialWeaponFovScale=120;
    TrialPlayer2FieldOfView=0;TrialPlayer2WeaponFovScale=0;
    trial_graphics::Initialize();
    assert(trial_graphics::FieldOfView(1)==70&&trial_graphics::WeaponFovScale(1)==120);
    draft=trial_graphics::Snapshot(0);draft.fov=90;draft.weapon_fov=150;
    assert(trial_graphics::Apply(draft,message,0));
    assert(trial_graphics::FieldOfView(0)==90&&trial_graphics::WeaponFovScale(0)==150);
    assert(trial_graphics::FieldOfView(1)==70&&trial_graphics::WeaponFovScale(1)==120);
    assert(Read().find("player2_field_of_view=70")!=std::string::npos);
    assert(Read().find("player2_weapon_fov_scale=120")!=std::string::npos);
    for(unsigned player:{1u,0u,1u,0u}){
        const auto other=trial_graphics::Snapshot(1-player);
        draft=trial_graphics::Snapshot(player);draft.fov=player?40:85;draft.weapon_fov=player?50:140;
        draft.settings.render_percent=player?100:125;draft.scenery=player==0;
        assert(trial_graphics::Apply(draft,message,player));
        assert(trial_graphics::FieldOfView(player)==draft.fov&&trial_graphics::WeaponFovScale(player)==draft.weapon_fov);
        assert(trial_graphics::FieldOfView(1-player)==other.fov&&trial_graphics::WeaponFovScale(1-player)==other.weapon_fov);
        assert(trial_graphics::Snapshot(1-player).settings.render_percent==draft.settings.render_percent);
        assert(trial_graphics::SceneryLighting()==draft.scenery); // Fidelity remains shared.
        saved=Read();const unsigned posts=app::display_posts;
        app::fail_save=true;draft.fov=53;draft.weapon_fov=100;draft.settings.width=1920;draft.settings.height=1080;
        assert(!trial_graphics::Apply(draft,message,player));app::fail_save=false;
        assert(Read()==saved&&app::display_posts==posts);
        assert(trial_graphics::Snapshot(player).fov==(player?40:85));
        assert(trial_graphics::Snapshot(player).weapon_fov==(player?50:140));
        assert(trial_graphics::FieldOfView(1-player)==other.fov&&trial_graphics::WeaponFovScale(1-player)==other.weapon_fov);
        trial_graphics::Initialize(); // Restart publication preserves the concrete saved pair.
        assert(trial_graphics::FieldOfView(player)==(player?40:85));
        assert(trial_graphics::FieldOfView(1-player)==other.fov&&trial_graphics::WeaponFovScale(1-player)==other.weapon_fov);
    }
    saved=Read();draft=trial_graphics::Snapshot(1);
    assert(!trial_graphics::Apply(draft,message,2)&&Read()==saved);
    assert(trial_graphics::Snapshot().fov==trial_graphics::Snapshot(0).fov);
    assert(trial_graphics::FieldOfView()==trial_graphics::FieldOfView(0));
    // Invalid optional P2 values cannot reset valid P1/display preferences.
    TrialPlayer2FieldOfView=91;TrialPlayer2WeaponFovScale=49;
    const auto shared=trial_graphics::Snapshot(0);trial_graphics::Initialize();
    assert(trial_graphics::Snapshot(0).fov==shared.fov&&app::Get().render_percent==shared.settings.render_percent);
    assert(trial_graphics::FieldOfView(1)==shared.fov&&trial_graphics::WeaponFovScale(1)==shared.weapon_fov);
    // Saving before Initialize also seeds from the old P1 values, never its draft.
    TrialPlayer2FieldOfView=0;TrialPlayer2WeaponFovScale=0;
    draft=trial_graphics::Snapshot(0);draft.fov=60;draft.weapon_fov=80;
    app::fail_save=true;assert(!trial_graphics::Apply(draft,message));app::fail_save=false;
    assert(TrialPlayer2FieldOfView==0&&TrialPlayer2WeaponFovScale==0);
    assert(trial_graphics::Apply(draft,message));
    assert(trial_graphics::FieldOfView(0)==60&&trial_graphics::WeaponFovScale(0)==80);
    assert(trial_graphics::FieldOfView(1)==shared.fov&&trial_graphics::WeaponFovScale(1)==shared.weapon_fov);
    std::filesystem::remove(app::file);
    std::puts("graphics settings: staged apply, per-player FOV migration/isolation, persistence rollback, validation, aspect restart and shared display routing passed");
}

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
static HWND client_window=reinterpret_cast<HWND>(uintptr_t(1));
static RECT client_rect{0,0,2560,1440};
static bool client_available=true;
static unsigned client_queries=0;
static BOOL WINAPI TestGetClientRect(HWND window,LPRECT rect) {
    assert(window==client_window&&window&&rect);++client_queries;
    if(!client_available)return FALSE;
    *rect=client_rect;return TRUE;
}
namespace app {
static Settings settings;
static std::recursive_mutex ui_lock;
static std::filesystem::path file;
static bool fail_save=false;
static std::string ColorTable(){
    std::string result;
    const char* states[]={"normal","enemy","friendly","spin"};
    for(unsigned p=0;p<2;++p)for(unsigned s=0;s<4;++s)
        result+="\n"+std::string(p?"player2_":"")+"reticle_"+states[s]+"_color="+std::to_string(TrialReticleColors[p][s]);
    return result;
}
static unsigned display_posts=0;
static Mode desktop{2560,1440};
Settings& Get() { return settings; }
std::recursive_mutex& UiLock() { return ui_lock; }
Mode Desktop() { return desktop; }
void* Window() { return client_window; }
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
        "\ndeath_convulsions="+std::to_string(TrialDeathConvulsions)+
        "\nhd_ui="+std::to_string(TrialHdUi)+"\nreticle_scale="+std::to_string(TrialReticleScale)+
        "\nweapon_fov_scale="+std::to_string(TrialWeaponFovScale)+"\nfmv_scale="+std::to_string(TrialFmvScale)+
        "\nplayer2_field_of_view="+std::to_string(TrialPlayer2FieldOfView)+
        "\nplayer2_weapon_fov_scale="+std::to_string(TrialPlayer2WeaponFovScale)+ColorTable();
}
// GENERATED_SAVE
}
#define GetClientRect TestGetClientRect
// GENERATED_SETTINGS
#undef GetClientRect

static void CheckGameViewWidth() {
    const auto saved_aspect=TrialAspectRatio;
    const auto saved_scale=app::settings.render_percent;
    auto check=[](LONG width,LONG height,float expected) {
        client_rect={0,0,width,height};
        const float actual=trial_graphics::GameViewWidth();
        assert(actual>=expected-0.001f&&actual<=expected+0.001f);
    };
    TrialAspectRatio=2;trial_graphics::Initialize();
    check(2560,1440,2560);check(3440,1440,2560);check(1920,1200,1920);
    check(1280,720,1280);check(800,600,800);check(1001,500,500*(16.0f/9.0f));
    app::settings.render_percent=200;check(2560,1440,2560);
    TrialAspectRatio=1; // A requested aspect does not alter the running presentation.
    check(2560,1440,2560);
    trial_graphics::Initialize();
    check(2560,1440,1920);check(1920,1080,1440);check(800,600,800);check(600,800,600);
    client_rect={17,29,817,629};assert(trial_graphics::GameViewWidth()==800);
    TrialAspectRatio=5;trial_graphics::Initialize();
    check(1280,800,1280);check(1920,1200,1920);check(2560,1600,2560);
    check(2560,1440,2304);check(3440,1440,2304);
    check(0,720,0);check(1280,0,0);check(-1,720,0);check(1280,-1,0);
    client_available=false;check(1280,720,0);client_available=true;
    const unsigned queries=client_queries;client_window=nullptr;
    assert(trial_graphics::GameViewWidth()==0&&client_queries==queries);
    client_window=reinterpret_cast<HWND>(uintptr_t(1));client_rect={0,0,2560,1440};
    TrialAspectRatio=saved_aspect;app::settings.render_percent=saved_scale;
    trial_graphics::Initialize();
}

static std::string Read() {
    std::ifstream f(app::file);return {std::istreambuf_iterator<char>(f),{}};
}
static void CheckAspectSettings(){
    const auto saved=app::settings;const auto desktop=app::desktop;const auto aspect=TrialAspectRatio;
    auto check_close=[](float a,float b){assert(std::abs(a-b)<.001f);};
    TrialAspectRatio=0;
    for(const auto size:{app::Mode{640,480},app::Mode{1280,800},app::Mode{1920,1200},app::Mode{2560,1600},
                        app::Mode{1920,1080},app::Mode{2560,1080},app::Mode{3440,1440},app::Mode{5120,1440}}){
        for(bool use_desktop:{false,true}){
            app::desktop=use_desktop?size:desktop;
            app::settings.width=use_desktop?0:size.width;app::settings.height=use_desktop?0:size.height;
            trial_graphics::Initialize();
            assert(TrialAspectRatio==0);
            check_close(trial_graphics::AspectRatio(),float(size.width)/size.height);
            const auto game=trial_graphics::ViewBounds(float(size.width),float(size.height));
            check_close(game.x,0);check_close(game.y,0);check_close(game.width,float(size.width));check_close(game.height,float(size.height));
            const auto menu=trial_graphics::ViewBounds(float(size.width),float(size.height),true);
            check_close(menu.width,std::min(float(size.width),size.height*(16.f/9.f)));
            check_close(menu.x,(size.width-menu.width)/2);check_close(menu.y,0);check_close(menu.height,float(size.height));
            check_close(trial_graphics::HudScale()*game.width/640,game.height/480);
        }
    }
    constexpr float expected[]={0,4.f/3.f,16.f/9.f,21.f/9.f,32.f/9.f,16.f/10.f};
    for(unsigned selected:{1u,2u,3u,4u,5u}){
        TrialAspectRatio=selected;trial_graphics::Initialize();
        assert(TrialAspectRatio==selected);
        check_close(trial_graphics::AspectRatio(),expected[selected]);
        auto draft=trial_graphics::Snapshot();std::string error;assert(trial_graphics::Validate(draft,app::Desktop(),error));
    }
    for(unsigned invalid:{6u,0xffffffffu}){
        auto draft=trial_graphics::Snapshot();draft.aspect=invalid;std::string error;
        assert(!trial_graphics::Validate(draft,app::Desktop(),error));
    }
    app::settings=saved;app::desktop=desktop;TrialAspectRatio=aspect;trial_graphics::Initialize();
}
int main() {
    app::file=std::filesystem::temp_directory_path()/
        (L"uc-graphics-fixture-"+std::to_wstring(GetCurrentProcessId())+L".toml");
    app::settings.join_ip="retained";
    trial_graphics::Initialize();assert(trial_graphics::Widescreen());
    CheckGameViewWidth();
    CheckAspectSettings();
    assert(!TrialWorldTextures&&!trial_graphics::WorldTextures());
    assert(!TrialHdUi&&!trial_graphics::HdUi()&&!trial_graphics::Snapshot().hd_ui);
    assert(TrialReticleScale==100&&trial_graphics::ReticleScale()==100);
    assert(TrialWeaponFovScale==100&&trial_graphics::WeaponFovScale()==100);
    assert(TrialFmvScale==0&&trial_graphics::FmvScale()==0&&trial_graphics::Snapshot().fmv_scale==0);
    assert(trial_graphics::MsaaSamples()==1&&!trial_graphics::MsaaSupported(4));
    assert(trial_graphics::SceneryLighting()&&trial_graphics::CharacterLighting());
    assert(trial_graphics::CharacterQuality()==0&&trial_graphics::RagdollMode()==0);
    assert(TrialDeathConvulsions&&trial_graphics::DeathConvulsions()&&trial_graphics::Snapshot().death_convulsions);
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
    draft.death_convulsions=false;assert(trial_graphics::DeathConvulsions());
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
    assert(saved.find("death_convulsions=0")!=std::string::npos&&!trial_graphics::DeathConvulsions());
    trial_graphics::Initialize();assert(!trial_graphics::Snapshot().death_convulsions&&!trial_graphics::DeathConvulsions());
    assert(saved.find("msaa_samples=4")!=std::string::npos&&trial_graphics::MsaaSamples()==4);
    assert(saved.find("ragdoll_amount=2")!=std::string::npos&&!trial_graphics::SceneryLighting()&&!trial_graphics::CharacterLighting());
    assert(trial_graphics::CharacterQuality()==1&&trial_graphics::RagdollMode()==2);
    draft=trial_graphics::Snapshot();draft.settings.render_percent=50;
    assert(trial_graphics::Apply(draft,message)&&app::display_posts==1);
    assert(message.find("redraws")!=std::string::npos);
    draft.aspect=1;assert(trial_graphics::Apply(draft,message));
    assert(message.find("Restart")!=std::string::npos&&trial_graphics::Widescreen());
    assert(TrialAspectRatio==1);trial_graphics::Initialize();assert(!trial_graphics::Widescreen());
    draft.aspect=5;assert(trial_graphics::Apply(draft,message));
    assert(message.find("Restart")!=std::string::npos&&!trial_graphics::Widescreen());
    assert(TrialAspectRatio==5&&Read().find("aspect_ratio=5\n")!=std::string::npos);
    trial_graphics::Initialize();
    assert(trial_graphics::Widescreen()&&trial_graphics::AspectRatio()==16.f/10.f);
    saved=Read();auto before=trial_graphics::Snapshot();
    app::fail_save=true;draft.settings.width=1920;draft.settings.height=1080;draft.aspect=2;draft.msaa=2;
    draft.scenery=true;draft.characters=true;draft.character_quality=0;draft.ragdolls=0;draft.hd_ui=false;draft.reticle_scale=50;draft.weapon_fov=50;
    draft.fmv_scale=0;
    draft.death_convulsions=true;
    assert(!trial_graphics::Apply(draft,message));app::fail_save=false;
    assert(Read()==saved&&app::settings.width==before.settings.width&&TrialAspectRatio==before.aspect);
    assert(!TrialDeathConvulsions&&!trial_graphics::DeathConvulsions()&&!trial_graphics::Snapshot().death_convulsions);
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
        if(which==4)bad.aspect=6;
        if(which==5)bad.settings.display_mode=app::DisplayMode(3);
        if(which==6)bad.settings.height=99999;
        if(which==7)bad.msaa=3;
        if(which==8)bad.character_quality=2;
        if(which==9)bad.ragdolls=3;
        if(which==10)bad.fmv_scale=3;
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
        draft.death_convulsions=enabled;
        assert(trial_graphics::Apply(draft,message));
        assert(trial_graphics::SceneryLighting()==enabled&&trial_graphics::CharacterLighting()==enabled);
        assert(trial_graphics::CharacterQuality()==draft.character_quality&&trial_graphics::RagdollMode()==draft.ragdolls);
        assert(TrialDeathConvulsions==enabled&&trial_graphics::DeathConvulsions()==enabled);
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
    for(unsigned scale:{0u,1u,2u,0u}){
        draft=trial_graphics::Snapshot();draft.fmv_scale=scale;
        const unsigned posts=app::display_posts;
        assert(trial_graphics::Apply(draft,message));
        assert(TrialFmvScale==scale&&trial_graphics::FmvScale()==scale&&app::display_posts==posts);
        assert(Read().find("fmv_scale="+std::to_string(scale))!=std::string::npos);
    }
    for(unsigned scale:{3u,0xffffffffu}){
        draft=trial_graphics::Snapshot();draft.fmv_scale=scale;
        assert(!trial_graphics::Apply(draft,message));assert(trial_graphics::FmvScale()==0);
    }
    TrialFmvScale=1;trial_graphics::Initialize();assert(trial_graphics::FmvScale()==1);
    TrialFmvScale=2;trial_graphics::Initialize();assert(trial_graphics::FmvScale()==2);
    TrialFmvScale=3;trial_graphics::Initialize();
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
    for(unsigned p:{0u,1u,0u}){
        auto colors=trial_graphics::Snapshot(p);const auto other=trial_graphics::Snapshot(1-p);
        colors.reticle_colors=p?std::array<uint32_t,4>{9,8,7,6}:std::array<uint32_t,4>{1,2,3,4};
        assert(trial_graphics::Snapshot(p).reticle_colors!=colors.reticle_colors||p==0);
        assert(trial_graphics::Apply(colors,message,p));
        for(unsigned s=0;s<4;++s)assert(trial_graphics::ReticleColor(p,s)==colors.reticle_colors[s]);
        assert(trial_graphics::Snapshot(1-p).reticle_colors==other.reticle_colors);
        const auto saved_colors=colors.reticle_colors;const auto text=Read();
        assert(text.find(std::string(p?"player2_":"")+"reticle_normal_color="+std::to_string(saved_colors[0]))!=std::string::npos);
        colors.reticle_colors.fill(0);app::fail_save=true;assert(!trial_graphics::Apply(colors,message,p));app::fail_save=false;
        assert(Read()==text&&trial_graphics::Snapshot(p).reticle_colors==saved_colors);
        colors.reticle_colors[0]=10;assert(!trial_graphics::Apply(colors,message,p));
        trial_graphics::Initialize();assert(trial_graphics::Snapshot(p).reticle_colors==saved_colors);
        assert(trial_graphics::Snapshot(1-p).reticle_colors==other.reticle_colors);
    }
    const auto before_colors=trial_graphics::Snapshot();
    TrialReticleColors[1][2]=0xffffffff;trial_graphics::Initialize();
    assert(trial_graphics::ReticleColor(1,2)==0&&trial_graphics::Snapshot().fov==before_colors.fov);
    assert(trial_graphics::Snapshot().reticle_colors==before_colors.reticle_colors);
    assert(trial_graphics::ReticleColor(2,0)==0&&trial_graphics::ReticleColor(0,4)==0);
    std::filesystem::remove(app::file);
    std::puts("graphics settings: staged apply, per-player FOV migration/isolation, persistence rollback, validation, aspect restart and shared display routing passed");
}

#include "xbe_coop_resources.h"
#include "xbe_coop.h"
#include "xbe_coop_turret_assets.h"
#include "cpu/x86_ops.h"
#include "kernel/kernel.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#ifndef UC_BURNER_RELEASE
#include <cstdio>
#endif
#include <cstring>
#include <filesystem>
#include <iterator>
#include <xmmintrin.h>

namespace {
thread_local bool loading=false;
thread_local unsigned loaded=0;
thread_local unsigned effects_seen=0;
constexpr const char* roots[]={
    "assets_characters\\cha_multiplayer\\cha_multiplayer_tzero\\cha_multiplayer_tzero_skin.amf",
    "assets_characters\\cha_multiplayer\\cha_multiplayer_burners\\cha_multiplayer_avenger_skin.amf",
    "assets_characters\\cha_multiplayer\\cha_multiplayer_burners\\cha_multiplayer_banger_skin.amf",
    "assets_characters\\cha_multiplayer\\cha_multiplayer_burners\\cha_multiplayer_magnum_skin.amf",
    "assets_characters\\cha_multiplayer\\cha_multiplayer_burners\\cha_multiplayer_prophet_skin.amf",
    "assets_playerarms\\arm_multiplayer_burner_arms_skin.amf",
    "fx\\fx_shadows\\fx_shadows_blob02.tga",
    "particles\\gasexplosion01.tga",
    "particles\\shapes\\ring.tga",
    "assets_characters\\cha_multiplayer\\cha_multiplayer_tzero\\cha_multiplayer_tzero_shield.amf",
    "assets_characters\\cha_cops\\cha_cop_tazer_skin.amf",
    "assets_characters\\cha_multiplayer\\cha_multiplayer_tzero\\cha_multiplayer_tzero_handgun_skin.amf",
    "assets_characters\\cha_patriotassault\\cha_patriotassault_assaultriffle_skin.amf",
    "assets_characters\\cha_burnerbanger\\cha_burnerbanger_shotgun_skin.amf",
    "assets_characters\\cha_saintavenger\\cha_saintavenger_machinepistol_skin.amf",
    "assets_characters\\cha_patriotmagnum\\cha_patriotmagnum_magnum_skin.amf",
    "assets_characters\\cha_burnerbrute\\cha_burnerbrute_cleaver_skin.amf",
    "assets_characters\\cha_saintprophet\\cha_saintprophet_saw_skin.amf",
    "assets_characters\\cha_patriotgrenade\\cha_patriotgrenade_grenadelauncher_skin.amf",
    "assets_characters\\cha_multiplayer\\cha_multiplayer_molotov_skin.amf",
    "assets_characters\\cha_multiplayer\\cha_multiplayer_nailbomb_skin.amf",
    "assets_characters\\cha_multiplayer\\cha_multiplayer_smokegrenade_skin.amf",
    "assets_characters\\cha_multiplayer\\cha_multiplayer_riotgrenade_skin.amf",
    "assets_characters\\cha_multiplayer\\cha_multiplayer_cop_shotgun_skin.amf",
    "assets_characters\\cha_multiplayer\\cha_multiplayer_cop_assaultrifle_skin.amf",
    "assets_coop\\medical\\eqp_hypospray_anim_heal_partner.kfa"};
constexpr const char* effects[]={
    "multiplayer\\spawn01.simpleparticlesystem",
    "multiplayer\\death_distortion.simpleparticlesystem"};
static_assert(std::size(roots)<32);
constexpr unsigned all_roots=(1u<<std::size(roots))-1;
constexpr unsigned all_effects=(1u<<std::size(effects))-1;
#ifndef UC_BURNER_RELEASE
struct PreloadTrace {
    const unsigned& parsed;
    ~PreloadTrace(){
        static unsigned records=0;
        if(!trial_coop::Available()||!trial_coop::Requested()||records>=4)return;
        ++records;
        std::fprintf(stderr,"[coop-resources] roots=%03X/%03X effects=%X/%X parsed=%X/%X ready=%u\n",
            loaded,all_roots,effects_seen,all_effects,parsed,all_effects,unsigned(trial_coop::ResourcesReady()));
        std::fflush(stderr);
    }
};
#endif
uint32_t Read(const uint8_t* b,uint32_t a){uint32_t v;std::memcpy(&v,b+a,4);return v;}
void Write(uint8_t* b,uint32_t a,uint32_t v){std::memcpy(b+a,&v,4);}
bool SameName(const uint8_t* b,uint32_t name,const char* expected){
    const size_t length=std::strlen(expected);
    if(!TrialGuestReadable(name,uint32_t(length+1))||b[name+length])return false;
    for(size_t n=0;n<length;++n){
        char value=char(b[name+n]);
        if(value>='A'&&value<='Z')value+=char('a'-'A');
        if(value=='/')value='\\';
        if(value!=expected[n])return false;
    }
    return true;
}
}

void TrialCoopPreload(X86Context& c,uint8_t* b){
    if(loading)return;
    TrialCoopTurretAssetsReset();
    trial_coop::ResourcesReady(false);
    if(!trial_coop::Requested()||!b||c.esp<0x18000)return;
    loaded=0;effects_seen=0;
    unsigned parsed_effects=0;
#ifndef UC_BURNER_RELEASE
    PreloadTrace trace{parsed_effects};
#endif
    for(const char* name:{"D:\\coopcharacters.lst","D:\\coopinjection.lst"}){
        const auto path=kernel::ResolvePath(name,0);
        std::error_code error;const auto size=std::filesystem::file_size(path,error);
        if(error||size<0xc8||size>16*1024*1024)return;
    }
    const uint32_t scratch=(c.esp-64)&~15u;
    if(!TrialGuestReadable(scratch-0x6000,0x6040))return;
    struct Restore {
        X86Context& ctx;X86Context original;unsigned csr;
        ~Restore(){ctx=original;_mm_setcsr(csr);loading=false;}
    } restore{c,c,_mm_getcsr()};
    loading=true;_mm_setcsr(0x1f80);
    // The ordinary LST loader resolves resources and closes its archive before
    // the campaign provider opens. It never starts a multiplayer scene.
    for(const char* name:{"coopcharacters","coopinjection"}){
        std::memcpy(b+scratch,name,std::strlen(name)+1);c.esp=scratch;
        kernel::CallGuest(c,0x54b70,{scratch});
    }
    if(loaded!=all_roots||effects_seen!=all_effects)return;
    // Particle files use the original extension callback, whose AL=1 only says
    // it recognized the extension. Require actual parsed entries after the LST
    // closes, using the same registry lookup as 23DF30 before its dereference.
    constexpr uint32_t registry=0x51cb78;
    if(!TrialGuestReadable(registry,0x10)||Read(b,registry)!=0x4830cc||
       !TrialGuestReadable(Read(b,registry+8),0x192))return;
    for(unsigned i=0;i<std::size(effects);++i){
        const auto* name=effects[i];
        std::memcpy(b+scratch,name,std::strlen(name)+1);c.esp=scratch;c.ecx=registry;
        const uint32_t parsed=kernel::CallGuest(c,0x23de20,{scratch});
        if(TrialGuestReadable(parsed,0x170))parsed_effects|=1u<<i;
    }
    trial_coop::ResourcesReady(parsed_effects==all_effects);
}
void TrialCoopResourceLoaded(const X86Context& c,const uint8_t* b){
    if(!loading||!b||!TrialGuestReadable(c.esp,0x18))return;
    // 8DB16 follows platform GetResource; 2A7B50's name/flags record is at +0x10.
    const uint32_t name=Read(b,c.esp+0x10);
    if(TrialGuestReadable(c.eax,4))
        for(unsigned i=0;i<std::size(roots);++i)
            if(SameName(b,name,roots[i]))loaded|=1u<<i;
    for(unsigned i=0;i<std::size(effects);++i)
        if(SameName(b,name,effects[i]))effects_seen|=1u<<i;
}
TrialCoopArmsScope::TrialCoopArmsScope(const X86Context& c,uint8_t* b){
    if(!trial_coop::Requested()||!trial_coop::ResourcesReady()||!b||
       !TrialGuestReadable(c.ecx,0x36c)||Read(b,c.ecx)!=0x46fbe8u)return;
    const uint32_t slot=Read(b,c.ecx+0x368);
    if(slot>=2||trial_coop::Selected(slot)==trial_coop::Character::TZero)return;
    // C87B0 consumes this name synchronously only when creating the arms.
    base_=b;original_=Read(b,0x51be00);Write(b,0x51be00,0x45bf50);
}
TrialCoopArmsScope::~TrialCoopArmsScope(){if(base_)Write(base_,0x51be00,original_);}

#include "firstperson.h"
#include "../firstperson_lighting.h"
#include "player_audio.h"
#include "player_footsteps.h"
extern "C" {
#include "../muzzle_placement.h"
#include "../texture_resources.h"
#include "../particle_quad_batch.h"
#include "../checkpoint.h"
}
#include <DirectXMath.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
using namespace DirectX;
static void require(bool ok,const char *what) {if(!ok)throw std::runtime_error(what);}
static XMMATRIX matrix(const float *p) {XMFLOAT4X4 m;memcpy(&m,p,64);return XMLoadFloat4x4(&m);}
static void store(float *p,FXMMATRIX m) {XMFLOAT4X4 v;XMStoreFloat4x4(&v,m);memcpy(p,&v,64);}

static bool supported_item(uint32_t item){return item==0||item==1||item==2||item==11||item==17;}
static unsigned equipment_slot(uint32_t item){require(supported_item(item),"Unsupported equipment slot");return item==2?0:item==17?1:item==1?2:item==0?3:4;}
static std::unique_ptr<NativeFirstpersonEquipment> load_equipment(NativeRenderer &renderer,
    const std::filesystem::path &game,const std::filesystem::path &assets,uint32_t item,
    const UCMainplayerItemSpec &spec) {
    auto result=std::make_unique<NativeFirstpersonEquipment>();result->spec=spec;
    require(uc_weapon_initialize(&result->state)==UC_MAINPLAYER_OK,"Native weapon construction");
    require(uc_mainplayer_presentation_resource(item,&result->resource)==UC_ANIMATION_OK,"Native weapon definition");
    UCMainplayerPresentationAction idle{};
    require(uc_mainplayer_presentation_action(item,0,0,0,0,0,&idle)==UC_ANIMATION_OK,"Native Idle action");
    const char *archive_name=item==0?"shield.lst":item==1?"taser.lst":item==2?"pistol.lst":item==11?"1-01.ent":"policeshotgun.lst";
    uint8_t hash[32];auto weapon=read_archive(game/archive_name,hash);
    uint8_t level_hash[32];auto level_clips=item<=1?read_archive(game/"1-01.ent",level_hash):decltype(weapon)(nullptr,uc_archive_close);
    const UCArchive *clips=item<=1?level_clips.get():weapon.get();
    result->gun.load_action(renderer,weapon.get(),hash,assets/(std::string(archive_name)+".ucmat"),
        result->resource.model,idle.animation_catalog,idle.alias,clips);
    auto sleeves=read_archive(game/"armssleeved.lst",hash);
    result->arms.load_action(renderer,sleeves.get(),hash,assets/"armssleeved.lst.ucmat",
        "Assets_PlayerArms\\ARM_SleeveArms_Skin.AMF",idle.animation_catalog,idle.alias,clips);
    result->clipbank.load(clips,idle.animation_catalog,result->gun.source.animation_template);
    require(uc_player_shield_bind(&result->gun.source,&result->shield_bone)!=0&&(item!=0||result->shield_bone>=0),"Authored shield surface tag");
    require(uc_muzzle_bind(&result->gun.source,&result->muzzle_bone),"Authored muzzle tag");
    // CC6B7 loads a separate model; C08F0 resolves its DummyLightFlare,
    // not a tag on the sleeves. Share the completed controller samples.
    if(result->resource.flashlight_mode==1){
        uint8_t light_hash[32];auto light_archive=read_archive(game/"1-01.ent",light_hash);
        result->flashlight_model=std::make_unique<NativeAnimatedModel>();
        result->flashlight_model->load_action(renderer,light_archive.get(),light_hash,assets/"1-01.ent.ucmat",
            "Assets_PlayerWeaponsAndEquipment\\EQP_Flashlight\\EQP_Flashlight_Skin.amf",idle.animation_catalog,idle.alias,clips);
        require(uc_flashlight_bind(&result->flashlight_model->source,&result->flashlight_bone)&&result->flashlight_bone>=0,"Authored flashlight flare tag");
    }
    result->appearance.resize(result->arms.materials.size(),{0,0}); // Fresh static IFL index0.
    for(const auto *model:{&result->gun,&result->arms})for(const auto &m:model->materials){
        const bool shield_face=item==0&&model==&result->gun&&m.state.alpha_prepass&&m.state.blend_mode==1;
        require(m.supported()&&(!m.state.blend_mode||shield_face||item==11),"Initial weapon needs an unsupported material/pass");
    }
    return result;
}

void NativeFirstperson::load(NativeRenderer &renderer,const std::filesystem::path &game,
    const std::filesystem::path &assets,const UCLevel &level,const UCLevelRestart &restart,const UCCheckpointSnapshot *saved) {
    require(!equipped,"First-person owner already loaded");
    require(mission_flags?mission_flag_count==level.field:!mission_flag_count,"Player mission flag extent");mission_level=&level;
    UCMainplayerProperties properties{};bool found=false;
    for(uint32_t i=0;i<level.entity_count;i++)if(!strcmp(level.entities[i].class_name,"Mainplayer")) {
        require(!found&&uc_mainplayer_properties(&level,i,&properties)==UC_MAINPLAYER_OK,"Unsupported Mainplayer properties");found=true;
    }
    require(found,"Missing Mainplayer");
    // Normal fresh1-01 route:58770 record+74=0,+78=-1;647B0 upgrades/class=0.
    UCMainplayerResolveInput resolve{};resolve.parsed_level_number=1;
    UCMainplayerItemSpec items[27]{};uint8_t upgrades[36]{};
    for(unsigned i=0;i<27;i++)require(uc_mainplayer_item_spec(i,0,upgrades,items+i)==UC_MAINPLAYER_OK,"Native item definition");
    UCMainplayerResetInput reset{};reset.no_saved_override=1;reset.items=items;reset.item_count=27;
    const uint32_t requests[]={2,17,20,1,13,14};
    for(unsigned i=0;i<6;i++) {
        uint32_t result;require(uc_mainplayer_resolve_item(requests[i],&resolve,&result)==UC_MAINPLAYER_OK,"Native item resolution");
        memcpy(reset.resolved_items+i,&result,4);
    }
    require(uc_mainplayer_reset(&properties,&reset,&inventory)==UC_MAINPLAYER_OK,"Native fresh inventory");
    require(uc_player_health_set(&inventory.health,&health_maximum,inventory.health)==1,"Native health high-water publication");
    if(saved){
        uint32_t extra[27]{};
        for(unsigned i=0;i<saved->count;i++)require(supported_item(saved->items[i].item),"Saved equipment is not supported by this native build");
        require(!uc_checkpoint_restore(saved,items,&inventory,extra),"Invalid saved player state");
        require(uc_player_health_set(&inventory.health,&health_maximum,inventory.health)==1,"Saved health high-water publication");
        require(inventory.requested_item>=0&&supported_item((uint32_t)inventory.requested_item),"Saved weapon is not supported by this native build");
    }
    require(uc_mainplayer_events_init_original(&player_events)==UC_ANIMATION_OK,"Native player event registration");
    equipment[0]=load_equipment(renderer,game,assets,2,items[2]);
    equipment[1]=load_equipment(renderer,game,assets,17,items[17]);
    equipment[2]=load_equipment(renderer,game,assets,1,items[1]);
    equipment[3]=load_equipment(renderer,game,assets,0,items[0]);
    if(saved&&inventory.owned[11])equipment[4]=load_equipment(renderer,game,assets,11,items[11]);
    {
        uint8_t hash[32];auto archive=read_archive(game/"1-01.ent",hash);const char*name="FX\\FX_Shield\\FX_shield_bullet.tga";UCTextureResources*raw=nullptr;
        require(!uc_texture_resources_create(archive.get(),1,&name,1,&raw),"Shield mark texture");
        std::unique_ptr<UCTextureResources,decltype(&uc_texture_resources_close)> textures(raw,uc_texture_resources_close);
        const auto*t=uc_texture_resources_at(raw,0);require(t&&t->image&&!t->sequence,"Shield mark image");shield_mark_texture=renderer.upload(*t->image);
        std::vector<NativeVertex> vertices(300*4);std::vector<uint32_t> indices(300*6);
        for(unsigned n=0;n<300;n++){const uint32_t v=n*4;uint32_t q[6]={v,v+1,v+2,v,v+2,v+3};memcpy(indices.data()+n*6,q,sizeof q);}
        shield_mark_mesh=renderer.upload(vertices.data(),vertices.size(),indices.data(),indices.size(),true);
    }
    switching.inventory=&inventory;switching.action=&action;switching.control=&equip_control;
    switching.special_action=-1;
    require(saved||inventory.requested_item==17,"Unsupported fresh equipped item");commit_equipment();
    auto table=read_file(assets/"animation-coefficients.bin",16384);
    require(uc_animation_blend_math_load(table.data(),table.size(),&math)==UC_ANIMATION_OK,"Native animation coefficients");
    const float clocks[4]={0,0,0,0};
    require(uc_firstperson_player_construct(&motion,clocks)==UC_MOTION_OK,"Native motion construction");
    uc_firstperson_player_activate(&motion);
    memcpy(actor.body,restart.transform,64);actor.angles[1]=restart.yaw;actor.eye_offset[1]=170;
    prior_position[0]=actor.body[12];prior_position[1]=actor.body[14];
    require(uc_firstperson_camera_initialize(actor.body,53,&camera,&camera_input)==UC_CAMERA_OK,"Native player camera construction");
    uc_camera_push_initialize(&camera_push);
}
void NativeFirstperson::enable_preview_throwables(NativeRenderer&r,const std::filesystem::path&game,const std::filesystem::path&assets,uint32_t count){
    require(equipped&&!equipment[4]&&count>0&&count<=10,"Invalid preview throwable loadout");
    UCMainplayerItemSpec specs[27]{};uint8_t upgrades[36]{};
    for(unsigned i=0;i<27;i++)require(uc_mainplayer_item_spec(i,0,upgrades,specs+i)==UC_MAINPLAYER_OK,"Preview item definition");
    require(inventory.requested_item!=-1&&specs[11].category==1&&specs[11].clip_size==10,"Molotov definition/selection");
    auto item=load_equipment(r,game,assets,11,specs[11]);
    require(uc_mainplayer_record_item(&inventory,11,specs,27)==UC_MAINPLAYER_OK,"Molotov inventory registration");
    equipment[4]=std::move(item);inventory.owned[11]=1;inventory.clip[11]=count;inventory.category_item[1]=11;
}
bool NativeFirstperson::pickup_molotov(NativeRenderer&r,const std::filesystem::path&game,const std::filesystem::path&assets){
    if(!equipped||inventory.clip[11]>=10)return false;
    if(!equipment[4])enable_preview_throwables(r,game,assets,1);
    else{inventory.owned[11]=1;++inventory.clip[11];inventory.category_item[1]=11;}
    return true;
}
static bool ordinary_equipment(const NativeFirstperson &p){
    if(!p.equipped||p.action.input_flags||p.inventory.owned[26]||
       ((p.equip_control.select_empty||p.equip_control.select_previous)&&p.action.state!=8)||
       p.switching.lowered||p.switching.raising||p.switching.restore_lowered||
       (p.action.state!=0&&p.action.state!=7&&p.action.state!=8)||p.action.animation>4||p.reload_overlay.clip)return false;
    for(unsigned i=0;i<5;i++){
        const auto &e=p.equipment[i];const unsigned item=i==0?2:i==1?17:i==2?1:i==3?0:11;
        if(!p.inventory.owned[item])continue;
        if(!e||e->state.alternate||p.inventory.clip[item]>e->spec.clip_size||
           !std::isfinite(e->reload_progress)||e->reload_progress<0||e->reload_progress>=1)return false;
    }
    return true;
}
static NativeFirstperson &player(void *p){return *static_cast<NativeFirstperson*>(p);}
static int switch_preflight(void *c,const UCPlayerEquipmentTransition *,uint32_t){return ordinary_equipment(player(c));}
static void switch_unreachable(void *){throw std::runtime_error("Unadmitted active equipment service");}
static void switch_state(void *c,uint32_t state,uint32_t stage){
    require(uc_player_equipment_idle_transition(&player(c).action,state,stage)==UC_MAINPLAYER_OK,"Native passive state transition");
}
static void switch_action(void *c,uint32_t id){player(c).apply_action(id);}
static void switch_sound(void *c){
    auto &p=player(c);if(p.audio)p.audio->switch_weapon((p.inventory.active_item==0||p.equip_control.select_empty)&&!p.equip_control.select_previous);
}
static void reload_sound(void *c){
    auto &p=player(c);if(p.audio)p.audio->play((uint32_t)p.inventory.active_item,"Load");
}
static uint8_t switch_done(void *c){
    auto *current=player(c).animations[0].current;
    return (uint8_t)(!current||!(current->timing.flags&0x40000000u));
}
static void switch_commit(void *c){player(c).commit_equipment();}
static uint8_t switch_pending(void *c){return player(c).equipped->state.pending_reload;}
static void switch_clear(void *c){require(uc_player_equipment_clear(&player(c).action_reset)==UC_MAINPLAYER_OK,"Native action reset");}
static uint8_t switch_reload_gate(void *c){
    auto &p=player(c);
    if(p.inventory.active_item==11){require(!p.equipped->state.pending_reload,"Molotov reload state");return 0;}
    if(p.inventory.active_item<=1){
        // Taser: whole43C00 full category2 is quiet. Shield persistent selection
        // disables reload as a PC policy; original41FA0 can instead inspect the
        // previous gun and requires its separate input/permission path.
        require(p.inventory.clip[p.inventory.active_item]==1&&p.equipped->spec.clip_size==1&&!p.equipped->state.pending_reload,"Unsupported secondary reload state");return 0;
    }uint32_t amount=0;const auto &spec=p.equipped->spec;
    require(uc_weapon_reload_amount((uint32_t)p.inventory.active_item,&spec,p.inventory.clip[p.inventory.active_item],
        p.inventory.ammo[spec.ammo_category],&amount)==UC_MAINPLAYER_OK,"Native switch reload amount");
    UCMainplayerReloadGate gate{};gate.item=(uint32_t)p.inventory.active_item;gate.reload_amount=(int32_t)amount;
    gate.pending=p.equipped->state.pending_reload;
    require(uc_weapon_can_fire(gate.item,p.inventory.clip[gate.item],&gate.can_primary)==UC_MAINPLAYER_OK,"Native reload primary availability");
    UCMainplayerReloadDecision decision{};
    require(uc_mainplayer_reload_gate(&gate,&decision)==UC_ANIMATION_OK&&!decision.requested,"Unadmitted reload during switch");
    return decision.requested;
}
static void switch_reevaluate(void *c){
    auto &p=player(c);
    // Complete D1AE0 admitted path: channel12 edge0 makes C5510 quiet, then
    // CF770 without a reload request, then category0 D1A20 -> BEB90 bit0 clear.
    require(!switch_reload_gate(c),"Unadmitted switch reevaluation");
    if(p.inventory.active_item<=1||p.inventory.active_item==11){require(!p.action.input_flags,"Unsupported secondary trigger during switch");return;} // Resolved inactive utility trigger during equipment update.
    UCMainplayerFireGate gate{};gate.item=(uint32_t)p.inventory.active_item;gate.input_flags=p.action.input_flags;
    UCMainplayerFireDecision decision{};
    require(!gate.input_flags&&uc_mainplayer_fire_gate(&gate,&decision)==UC_ANIMATION_OK&&!decision.allowed&&!decision.misfire,
        "Unadmitted fire during switch");
}
static void switch_tail(void *c){
    // BE870 nonlowered ordinary active item, channel16 edge0: no effects.
    require(ordinary_equipment(player(c)),"Unadmitted aim tail during switch");
}
static void switch_reload(void *c){player(c).update_reload();}
static const UCPlayerEquipmentServices switch_hooks={switch_preflight,switch_unreachable,switch_unreachable,
    switch_state,switch_action,switch_sound,switch_done,switch_commit,switch_pending,switch_reload,
    switch_clear,switch_reload_gate,switch_reevaluate,switch_tail};
bool NativeFirstperson::select_weapon(NativeRenderer &,uint32_t item) {
    if(!accepts_input())return false;
    if(preview_shot_pending||!supported_item(item)||!inventory.owned[item])return false;
    if(taser_phase&&supported_item(item)&&inventory.active_item!=(int32_t)item){taser_phase=0;apply_action(0);}
    if(preview_firing&&supported_item(item)&&inventory.active_item!=(int32_t)item)finish_preview_fire();
    if(shield_bashing&&inventory.active_item!=(int32_t)item){shield_bashing=false;action.state=action.stage=action.input_flags=0;apply_action(0);}
    if(!ordinary_equipment(*this)||(action.state!=0&&action.state!=7)||
       inventory.active_item==(int32_t)item||reload_requested)return false;
    const auto *next=equipment[equipment_slot(item)].get();
    if(!inventory.clip[item]&&!next->state.pending_reload)return false;
    uint32_t amount=0;const auto &spec=equipped->spec;
    if((inventory.active_item==2||inventory.active_item==17)&&uc_weapon_reload_amount((uint32_t)inventory.active_item,&spec,inventory.clip[inventory.active_item],
       inventory.ammo[spec.ammo_category],&amount)!=UC_MAINPLAYER_OK)return false;
    // Exclude C4CB0's automatic selection owner; actual42DA0 admits either
    // nonzero clip or a positive reload amount for these ordinary owned items.
    if(!inventory.clip[inventory.active_item]&&!amount&&inventory.active_item!=11)return false;
    const auto old_request=inventory.requested_item;inventory.requested_item=(int32_t)item;
    if(item<=1||item==11||inventory.active_item==0||inventory.active_item==11){
        // Explicit PC persistent slot, not C4CB0 category2 quick-use/restore.
        // CF8A0 still performs the real Lower/commit/Raise with special=-1.
        switching.special_action=-1;action.state=8;action.stage=0;shield_temporary=false;return true;
    }
    UCPlayerEquipmentSwitchInput input{};
    // Exact42DA0 owned/nonzero-clip-or-reload branch; no category cycling.
    input.current_available=(uint8_t)(inventory.owned[inventory.active_item]&&(inventory.clip[inventory.active_item]||amount));
    uint8_t requested=0;
    auto status=uc_player_equipment_request(&switching,&input,&switch_hooks,this,&requested);
    if(status!=UC_MAINPLAYER_OK||!requested){inventory.requested_item=old_request;return false;}
    return true;
}
static uint32_t reload_amount(void *c){
    auto &p=player(c);const auto &spec=p.equipped->spec;uint32_t amount=0;
    require(uc_weapon_reload_amount((uint32_t)p.inventory.active_item,&spec,p.inventory.clip[p.inventory.active_item],
        p.inventory.ammo[spec.ammo_category],&amount)==UC_MAINPLAYER_OK,"Native reload amount");return amount;
}
static int reload_preflight(void *c,const UCPlayerReload *r){
    auto &p=player(c);return ordinary_equipment(p)&&r->item==(uint32_t)p.inventory.active_item;
}
static UCAnimationController *const *reload_controllers(void *c,uint32_t *count){
    auto &p=player(c);*count=2;
    for(unsigned i=0;i<2;i++){
        require(p.animations[i].current!=nullptr,"Reload current controller missing");
        p.reload_controllers[i]=&p.animations[i].current->timing;
    }
    return p.reload_controllers;
}
static const UCPlayerReloadServices reload_hooks={reload_preflight,switch_action,reload_amount,
    switch_reevaluate,reload_sound,reload_controllers};
bool NativeFirstperson::request_reload(NativeRenderer &){
    if(!accepts_input())return false;
    if(inventory.active_item<=1||inventory.active_item==11)return false;
    if(preview_shot_pending)return false;
    if(preview_firing&&reload_amount(this))finish_preview_fire();
    if(!ordinary_equipment(*this)||action.state!=0||action.animation!=0||reload_requested||!reload_amount(this))return false;
    reload_requested=true;return true;
}
bool NativeFirstperson::add_camera_push(const UCCameraPushInput &input){
    return equipped&&uc_camera_push_add(&camera_push,&input)==UC_CAMERA_PUSH_OK;
}
void NativeFirstperson::finish_preview_fire(){
    if(!preview_firing)return;
    preview_firing=false;equipped->state.firing=0;
    action.state=action.stage=0;action.input_flags&=~2u;action.ready_latch=action.ready_pulse=0;
    apply_action(0);
}
void NativeFirstperson::update_preview_fire(float delta){
    require(std::isfinite(delta)&&delta>0&&delta<=1&&!preview_shot_pending,"Invalid preview firing tick");
    preview_clock+=delta;
    if(inventory.active_item==0){update_shield_bash();return;}
    if(inventory.active_item==1){update_taser_presentation();return;}
    if(inventory.active_item==11){update_molotov_presentation(delta);return;}
    if(preview_firing&&(action.ready_pulse||switch_done(this)))finish_preview_fire();
    if(!fire_held||fire_needs_release||!random||!preview_shot||!updates||preview_firing||
       action.state!=0||action.animation!=0||reload_requested||equipped->state.pending_reload||
       !inventory.clip[inventory.active_item])return;
    require(random->first<250&&random->second<250,"Invalid shared preview random owner");
    auto &weapon=equipped->state;const auto item=(uint32_t)inventory.active_item;
    uint8_t ready=0;
    require(uc_weapon_ready(&weapon,preview_clock,&ready)==UC_MAINPLAYER_OK,"Preview weapon deadline");
    if(!ready)return;
    weapon.alternate=uint8_t(item==17&&shotgun_secondary);
    // Explicit PC admission. The full CFBC0 input/utility/aim and CBA20 seven
    // effect services are deliberately not called with placeholder callbacks.
    require(uc_weapon_reset_shots(&weapon)==UC_MAINPLAYER_OK,"Preview shot reset");
    apply_action(3);
    UCWeaponCadenceResult cadence{};
    require(uc_weapon_cadence(item,inventory.ammo[equipped->spec.ammo_category],preview_clock,delta,
        &weapon,&cadence)==UC_MAINPLAYER_OK&&cadence.ready,"Preview native cadence");
    if(cadence.reset_fire_input)fire_needs_release=true;
    uint8_t fired=0;
    require(uc_weapon_shot(item,preview_clock,delta,&inventory.clip[item],&weapon,&fired)==UC_MAINPLAYER_OK&&fired,
        "Preview native ammo debit");
    action.state=4;action.stage=2;action.input_flags|=2;action.ready_latch=action.ready_pulse=0;
    preview_firing=preview_shot_pending=true;preview_shot_item=item;++preview_shots;
    if(audio)audio->fire(item,weapon.alternate);
}
void NativeFirstperson::update_taser_presentation(){
    // PC held-trigger presentation using CD430's authored11/13/12 actions.
    // Source melee queries, Arc sound parameter/loop, NPC stun and quick-use
    // restoration are not completed here. No ammo, bullet or muzzle request.
    if(action.state!=0||reload_requested)return;
    if(!taser_phase){
        if(!fire_held||fire_needs_release||!updates||action.animation!=0)return;
        apply_action(11);taser_phase=11;++taser_cycles;return;
    }
    const bool done=switch_done(this)!=0;
    if(taser_phase==12){if(done){taser_phase=0;apply_action(0);}return;}
    if(!fire_held||fire_needs_release){
        if(taser_phase!=11||done){apply_action(12);taser_phase=12;}return;
    }
    if(done){apply_action(13);taser_phase=13;}
}
void NativeFirstperson::update_molotov_presentation(float delta){
    // PC trigger admission around D1B60's actual7/8/2 actions, capped charge,
    // prior Throw marker and completion fallback. No roll/quick-use/script
    // interruption or full ThrownWeapon factory is claimed.
    if(!throw_stage){
        if(!fire_held||fire_needs_release||!updates||action.state||action.animation||!preview_throw||!inventory.clip[11])return;
        throw_charge=0;throw_emitted=throw_frame_ready=false;apply_action(7);throw_stage=1;action.state=9;return;
    }
    const bool done=switch_done(this)!=0;
    if(throw_stage==1){if(done)throw_stage=2;return;}
    if(throw_stage==2){
        throw_charge=std::min(.5f,throw_charge+delta);if(fire_held)return;
        apply_action(8);throw_stage=3;throw_capture_pending=true;return;
    }
    if(throw_stage==3){
        if(!throw_emitted&&throw_frame_ready&&(player_events.published[UC_MAINPLAYER_EVENT_THROW]||done)){
            require(inventory.clip[11]>0,"Molotov count before publication");
            if(preview_throw(preview_throw_context,throw_frame,throw_pitch,throw_charge)){--inventory.clip[11];++preview_throws;}
            else ++throw_rejections;
            throw_emitted=true;
        }
        if(done&&throw_emitted){apply_action(2);throw_stage=4;}return;
    }
    if(done){throw_stage=0;action.state=action.stage=0;apply_action(0);}
}
static void shield_sound(void *c,uint8_t active){auto &p=player(c);if(p.audio)p.audio->shield(active!=0);}
static void shield_switch(void *c){auto &p=player(c);require(uc_player_equipment_update(&p.switching,&switch_hooks,c)==UC_MAINPLAYER_OK,"Native temporary shield switch");}
bool NativeFirstperson::update_shield_input(){
    // PC binding and interruption admission; D0C30's utility/script/automatic
    // selection paths are not silently completed. Defer through reload, while
    // Fire/Taser presentation can finish into the actual ordinary switch owner.
    if(inventory.active_item==11||throw_stage)return false;
    if(!shield_held&&!shield_temporary)return false;
    if(shield_bashing){
        if(shield_held||!shield_temporary)return false;
        // PC hold release interrupts the presentation before a later MeleeHit.
        shield_bashing=shield_bash_pending=false;
        action.state=action.stage=action.input_flags=0;apply_action(0);
    }
    if(preview_firing&&shield_held)finish_preview_fire();
    if(taser_phase&&shield_held){taser_phase=0;apply_action(0);}
    if(!ordinary_equipment(*this)||(action.state!=0&&action.state!=8)||reload_requested||
       equipped->state.pending_reload||!inventory.clip[inventory.active_item])return false;
    if(shield_held){
        // RMB also takes over a manual key4 selection. Keep the return in the
        // same requested/previous fields consumed by original42270; saving0
        // here would make the subsequent release select the shield again.
        if(inventory.requested_item==0){
            const int32_t restore=inventory.active_item?inventory.active_item:inventory.previous_item;
            require(restore>0&&supported_item((uint32_t)restore)&&inventory.owned[restore],"Shield hold needs a prior item");
            inventory.requested_item=restore;
        }
        if(inventory.active_item==0)shield_temporary=true;
    }
    uint8_t requested=0;const int old=inventory.active_item;
    const UCPlayerShieldServices services={shield_sound,switch_state,shield_switch};
    require(uc_player_shield_input(&switching,(uint8_t)shield_held,&services,this,&requested)==UC_MAINPLAYER_OK,"Native shield hold input");
    if(requested){if(old!=0)shield_temporary=true;else if(!shield_held)shield_temporary=false;}
    return requested!=0;
}
bool NativeFirstperson::shield_blocks(const float incoming[3]) const {
    uint8_t blocked=0;
    return equipped&&uc_player_shield_blocks(inventory.active_item,&equip_control,shield_impact,actor.body,incoming,&blocked)==UC_MAINPLAYER_OK&&blocked;
}
int NativeFirstperson::shield_frame(float out[16])const{
    if(!equipped)return 1;const auto&g=equipped->gun;
    return uc_muzzle_world(equipped->shield_bone,g.source.bone_count,g.prepared.world,g.prepared.world_valid,object,out)?0:1;
}
int NativeFirstperson::shield_surface(float point[3]){
    float frame[16];if(!random||shield_frame(frame))return 1;
    return uc_player_shield_surface(&shield_marks,frame,random,point);
}
int NativeFirstperson::shield_input_response(float&forward,float&side){
    if(uc_player_shield_recover(&shield_recoil,&shield_feedback))return 1;
    if(shield_recoil.horizontal!=0||shield_recoil.vertical!=0){forward=shield_recoil.vertical;side=shield_recoil.horizontal;}
    return 0;
}
void NativeFirstperson::update_shield_bash(){
    // PC click admission around D0E20's actual Fire -> done/Idle ordering and
    // prior MeleeHit pulse. No fabricated C57C0 actor damage completion.
    if(shield_bashing){
        if(switch_done(this)){shield_bashing=false;action.state=action.stage=action.input_flags=0;apply_action(0);return;}
        if(player_events.published[UC_MAINPLAYER_EVENT_MELEE_HIT])shield_bash_pending=true;
        if(action.ready_pulse)action.ready_latch=1;
        return;
    }
    if(action.state||action.animation||!fire_held||fire_needs_release||!updates||reload_requested)return;
    apply_action(3);action.state=5;action.stage=1;action.input_flags|=2;action.ready_latch=0;
    shield_bashing=true;fire_needs_release=true;++shield_bashes;
}
void NativeFirstperson::complete_preview_shot(const float eye_frame[16]){
    if(!preview_shot_pending)return;
    // Consume this request once even if a downstream query fails after debit.
    preview_shot_pending=false;
    require(preview_shot&&random&&preview_shot(preview_shot_context,preview_shot_item,eye_frame+12,eye_frame+8),
        "Preview static shot query failed after ammo debit");
    secondary_shots+=equipped->state.alternate!=0;
    equipped->state.alternate=0; // Secondary mode belongs to the PC input adapter; passive switch/reload admits inactive fire only.
    ++preview_queries;
    auto sample=[&](float low,float high){int32_t word=0;
        require(!uc_game_random_next(random,&word),"Preview shared random draw");
        return ((double)word*((double)high-low))*0x1p-31+low;};
    // CBA20's resolved ordinary CameraPush payload with an explicit PC eye-axis
    // adapter in place of the not-yet-live tag0/effect entity factory.
    float fraction=(camera.fov-10.f)/43.f;if(fraction<=.05f)fraction=.05f;
    const float radius=(preview_shot_item==2?5.f:20.f)*fraction;
    UCCameraPushInput push{};push.force=(float)sample(preview_shot_item==2?.075f:.16f,
        preview_shot_item==2?.125f:.21f)*fraction;
    push.fade_out=1;push.minimum_range=1000;push.maximum_range=1010;
    for(unsigned i=0;i<3;i++)push.position[i]=(float)(sample(-radius,radius)+(double)(eye_frame[8+i]*500.f))+actor.body[12+i];
    require(add_camera_push(push),"Preview CameraPush capacity");
}
void NativeFirstperson::update_reload(){
    UCPlayerReload binding{(uint32_t)inventory.active_item,&action,&equipped->state,&equipped->reload_progress,
        &motion.input.angular_input,&switching.lowering_mode,&reload_overlay};
    uint8_t applied=0;
    require(uc_player_reload_update(&binding,&reload_hooks,this,&applied)==UC_ANIMATION_OK&&applied,"Native reload owner");
    // Whole CA930/D0C30 quiet paths are qualified for no utility/shield input,
    // no selected utility, fresh melee26 unowned and original shield mode1.
    // D0C30 returns0, so the extra D276A ammo transfer loop is not entered.
    require(!inventory.owned[26],"Unsupported reload melee interruption owner");
}
void NativeFirstperson::commit_equipment() {
    auto next_inventory=inventory;auto next_control=equip_control;
    require(uc_mainplayer_equipment_commit(&next_inventory,&next_control)==UC_MAINPLAYER_OK,"Native equipment commit");
    const uint32_t item=(uint32_t)next_inventory.active_item;
    require(supported_item(item),"Unsupported native resource commit");
    auto *next=equipment[equipment_slot(item)].get();require(next!=nullptr,"Weapon resources not loaded");
    int32_t reset=-1;
    require(uc_mainplayer_presentation_reset_action(0,(int32_t)item,&reset)==UC_ANIMATION_OK&&reset==0,"Native equipment Idle");
    UCMainplayerPresentationAction idle{};
    require(uc_mainplayer_presentation_action(item,0,0,0,0,0,&idle)==UC_ANIMATION_OK,"Native Idle action");
    NativeFirstpersonAnimations next_animations[2];
    for(auto &animation:next_animations)animation.apply(next->clipbank,idle);
    // Existing C8930 passive composition: no attachment, input-resource, light
    // or sound owner is fabricated. Old prior events were consumed before the
    // delayed CF8A0 commit. New banks remain alive through every borrowing ref.
    for(unsigned i=0;i<2;i++){events[i]={};animations[i]=std::move(next_animations[i]);bounds[i]={};}
    player_events.pending=0;memset(player_events.published,0,sizeof(player_events.published));
    equipped=next;resource=next->resource;inventory=next_inventory;equip_control=next_control;action.animation=0;
    equipped->gun.reset_presentation();equipped->arms.reset_presentation();
    if(equipped->flashlight_model)equipped->flashlight_model->reset_presentation();presentation_history=false;
    if(mission_flags)require(mission_level&&!uc_mission_equipment_flags(mission_level,mission_flags,mission_flag_count,inventory.active_item),"Equipment mission flag mapping");
}
void NativeFirstperson::apply_action(uint32_t id,int32_t repeats,int32_t blend){
    require(equipped&&supported_item((uint32_t)inventory.active_item),"First-person equipment change needs resource ownership");
    UCMainplayerPresentationAction request{};
    require(uc_mainplayer_presentation_action((uint32_t)inventory.active_item,id,flashlight_requested,flashlight_compatible(),repeats,blend,&request)==UC_ANIMATION_OK,
        "Unsupported first-person action");
    for(auto &animation:animations)animation.apply(equipped->clipbank,request);
    action.animation=id;
}
bool NativeFirstperson::flashlight_compatible()const{
    int32_t item=inventory.active_item;if(item==0||item==26)item=inventory.previous_item;
    UCMainplayerPresentationResource r{};return item>=0&&uc_mainplayer_presentation_resource(uint32_t(item),&r)==UC_ANIMATION_OK&&r.flashlight_mode!=0;
}
void NativeFirstperson::set_flashlight(bool enabled){
    if(flashlight_requested==enabled)return;flashlight_requested=enabled;
    // Switching/reload/fire retain their current authored action. Idle refreshes
    // immediately; later source action changes choose the flashlight alias.
    if(equipped&&action.animation==0&&action.state==0)apply_action(0);
}
bool NativeFirstperson::flashlight(UCNativeControlSpot &out,const float rendered_camera[16],const float object_view[16],float fraction,float plane[4])const{
    if(!equipped||!updates||!flashlight_requested||!flashlight_compatible()||!accepts_input())return false;
    const bool gun=resource.flashlight_mode==2;
    if(!gun&&!equipped->flashlight_model)return false;
    const auto &model=gun?equipped->gun:*equipped->flashlight_model;int32_t bone=gun?equipped->muzzle_bone:equipped->flashlight_bone;
    float pose[1][16],world[16],beam[16];uint8_t valid=1;
    if(bone>=0&&!model.presentation_bone(uint32_t(bone),fraction,pose[0]))return false;
    store(world,matrix(object_view)*matrix(rendered_camera));
    if(!uc_muzzle_world(bone<0?-1:0,1,pose,&valid,world,beam))return false;
    const float color[4]={210.f*(1.f/255.f),230.f*(1.f/255.f),250.f*(1.f/255.f),1};
    if(!uc_spot_light_build(beam+12,beam+8,1500.f,.4f,color,1,&out))return false;
    if(plane){memcpy(plane,beam+8,12);plane[3]=0.f-((beam[14]*beam[10]+beam[13]*beam[9])+beam[12]*beam[8]);}return true;
}
UCFirstpersonInputMovement NativeFirstperson::before_physics(NativeRenderer &renderer,float forward,float side,bool crouched,float delta) {
    require(equipped&&supported_item((uint32_t)inventory.active_item),"First-person equipment change needs resource ownership");
    const bool controlled=accepts_input();
    if(!controlled){forward=side=0;fire_held=shield_held=reload_requested=false;}
    auto &weapon_state=equipped->state;const auto &weapon_spec=equipped->spec;
    uint8_t commit=0;
    require(uc_mainplayer_presentation_reload_gate(player_events.published[UC_MAINPLAYER_EVENT_RELOAD],
        weapon_state.pending_reload,&commit)==UC_ANIMATION_OK,"Native prior reload marker gate");
    if(commit)require(uc_weapon_reload_commit((uint32_t)inventory.active_item,&weapon_spec,
        &inventory.clip[inventory.active_item],&inventory.ammo[weapon_spec.ammo_category],&weapon_state)==UC_MAINPLAYER_OK,
        "Native reload ammunition commit");
    action.ready_pulse=player_events.published[UC_MAINPLAYER_EVENT_READY_TO_FIRE];
    if(reload_requested){
        reload_requested=false;
        if(reload_amount(this))switch_state(this,7,0); // Explicit PC R admission.
    }
    // Original prior published reload marker must run against OLD equipment.
    // Only afterward may CF8A0 commit a requested item and publish its Raise.
    const bool shield_updated=controlled&&update_shield_input();
    if(!shield_updated&&action.state==8)require(uc_player_equipment_update(&switching,&switch_hooks,this)==UC_MAINPLAYER_OK,
        "Unsupported active equipment transition");
    else if(action.state==7)update_reload();
    if(controlled)update_preview_fire(delta);
    UCFirstpersonInputResolved input{{forward,side},delta,(unsigned)crouched,0};UCFirstpersonInputMovement resolved{};
    require(uc_firstperson_input_movement_step(&motion.input,&input,&resolved)==UC_MOTION_OK,"Native movement presentation");
    movement=resolved.blend;
    // PC raw-mouse adaptation changes angles separately; the absent stick has zero shaped axes.
    const float axes[2]={0,0};
    require(uc_firstperson_input_spring_targets(motion.weapon.angular,axes,0)==UC_MOTION_OK,"Native look targets");
    UCFirstpersonBobInput bob{{actor.body[12],actor.body[14]},{prior_position[0],prior_position[1]},delta,movement,(unsigned)crouched};
    unsigned due=0;require(uc_firstperson_bob_step(&motion.bob,&bob,&due)==UC_MOTION_OK,"Native completed-displacement bob");
    pending_footsteps+=due;
    if(due&&footsteps){footsteps->step(actor.body,motion.bob.side);pending_footsteps-=due;}
    NativeAnimatedModel *models[]={&equipped->gun,&equipped->arms};
    for(unsigned i=0;i<2;i++) {
        animations[i].update(renderer,*models[i],equipped->clipbank,math,delta,events[i],i==1?equipped->flashlight_model.get():nullptr);
        const auto &m=*models[i];
        require(uc_model_bounds(&m.source,m.prepared.world,m.source.bone_count,m.prepared.world_valid,bounds+i)==UC_MODEL_OK,"Native model draw bounds");
    }
    require(uc_mainplayer_events_collect(&player_events,events)==UC_ANIMATION_OK&&
        uc_mainplayer_events_publish(&player_events)==UC_ANIMATION_OK,"Native player event publication");
    reload_pulses+=player_events.published[UC_MAINPLAYER_EVENT_RELOAD];
    // Remaining refs are retained until the next animation update and reported.
    // Unsupported effect types remain visible; arms have no local player subscriptions.
    if(audio){
        //20BC40 publishes the Mainplayer owner for the weapon state. C87B0's
        // separate arms proxy retains1F13A0's null owner (1FB5E0 copies+8 only).
        if(inventory.active_item==2||inventory.active_item==17)audio->dispatch(events[0],this,action.state,inventory.active_item,inventory.previous_item);
        // Tazer Hold/Release uses a multi-track parameter program, not the
        // immediate one-shot backend. Keep these refs reported as unhandled.
        audio->dispatch(events[1],nullptr,action.state,inventory.active_item,inventory.previous_item);
        audio->collect();
    }
    unhandled_weapon_events+=events[0].count;unhandled_arm_events+=events[1].count;
    if(!controlled)resolved.movement[0]=resolved.movement[1]=0;
    return resolved;
}
int NativeFirstperson::apply_camera_feedback(){
    if(!feedback||!feedback_clock||!feedback_auxiliary)return 1;
    struct Context{NativeFirstperson*p;int failure=0;} context{this};
    UCSceneCamera projection{};projection.auxiliary=feedback_auxiliary;const float*out=nullptr;
    UCSceneCameraWorldServices h{&context,[](void*){return 1;},
        [](void*q,uint32_t player,const float*position,uint32_t kind)->double{auto&c=*(Context*)q;auto&p=*c.p;float value=0;
            if(uc_feedback_sample(uc_feedback_manager_queue(p.feedback),uc_feedback_manager_observers(p.feedback),player,position,kind,*p.feedback_clock,p.feedback_mode,&value)){c.failure=2;return NAN;}return value;},
        [](void*q,float amplitude,const float*world,float*scratch){auto&c=*(Context*)q;if(uc_feedback_perturb(c.p->feedback_random,amplitude,world,scratch))c.failure=2;}};
    if(uc_scene_camera_world(&projection,camera.world,camera_world,&feedback_world_flags,&feedback_bypass,&h,&out)||context.failure)return 2;
    if(out!=camera_world)memcpy(camera_world,out,64);return 0;
}
void NativeFirstperson::after_physics(const float body[3],float eye,float pitch,float yaw,bool crouched,float delta) {
    if(presentation_history){memcpy(previous_object,object,64);memcpy(previous_camera,camera_world,64);memcpy(previous_anchor,look_anchor,64);}
    prior_position[0]=actor.body[12];prior_position[1]=actor.body[14];
    memcpy(actor.body+12,body,12);actor.eye_offset[1]=eye;actor.angles[0]=pitch;actor.angles[1]=yaw;
    require(!uc_player_hit_angles(&impact,&impact_rate,&shield_recoil,delta,actor.angles),"Player hit angle publication");
    UCFirstpersonLookInput look{};float next_body[16];
    require(uc_firstperson_look_base(&actor,next_body,look.base)==UC_MOTION_OK,"Native player look base");memcpy(actor.body,next_body,64);
    memcpy(look_anchor,look.base,64);
    complete_preview_shot(look.base);
    if(throw_capture_pending){throw_capture_pending=false;memcpy(throw_frame,look.base,64);throw_pitch=pitch;throw_frame_ready=true;}
    if(shield_bash_pending){
        shield_bash_pending=false;
        if(preview_melee){float end[3];for(unsigned i=0;i<3;i++)end[i]=look.base[12+i]+look.base[8+i]*200.f;
            require(preview_melee(preview_melee_context,look.base+12,end),"Preview melee query failed");++shield_melee_queries;}
    }
    // Actual Mainplayer+C0/+C4 are null: CA620 preserves this matrix. Its aim/hit
    // queries remain absent, so this is only the passive first-person owner.
    look.delta=delta;look.movement=movement;look.crouched=crouched;
    look.bob_phase=motion.bob.phase;look.bob_amount=motion.bob.amount;look.mirrored=motion.bob.step_count&1u;
    UCHandgunPlacementInput placement{};
    require(uc_firstperson_look_sway(&motion.view,&look,placement.look)==UC_MOTION_OK,"Native player view sway");
    memcpy(camera_input.player_frame,placement.look,64);
    // Actual CA890 uses the resolved actor body, then1CD590 publishes camera+280.
    // Render interpolation never advances this retained effect history.
    require(uc_camera_push_tick(&camera_push,delta,actor.body,camera_input.extra)==UC_CAMERA_PUSH_OK,"Native camera push tick");
    require(uc_firstperson_camera_follow(&camera,&camera_input,camera_world)==UC_CAMERA_OK,"Native player camera follow");
    if(feedback)require(!apply_camera_feedback(),"Native camera feedback getter");
    placement.item=inventory.active_item;placement.delta=delta;placement.crouched=crouched;
    placement.aiming=aim_held&&accepts_input()&&!shield_held&&!reload_requested&&(action.state==0||action.state==4)&&
        (inventory.active_item==2||inventory.active_item==17);
    placement.bob_phase=look.bob_phase;placement.bob_amount=look.bob_amount;placement.mirrored=look.mirrored;placement.movement=movement;
    placement.recoil_pitch=motion.input.recoil[0].current;placement.recoil_yaw=motion.input.recoil[1].current;
    placement.angular_input=motion.input.angular_input;
    memcpy(placement.base_offset,resource.base_offset,12);memcpy(placement.aimed_offset,resource.aimed_offset,12);
    require(uc_handgun_placement_step(&motion.weapon,&placement,object)==UC_MOTION_OK,"Native equipped weapon placement");
    if(!presentation_history){memcpy(previous_object,object,64);memcpy(previous_camera,camera_world,64);memcpy(previous_anchor,look_anchor,64);presentation_history=true;}
    ++updates;
}
void NativeFirstperson::render_matrices(const float render_look[16],float aspect,float rendered_camera[16],float weapon_matrix[16],float object_view[16],float fraction) const {
    require(updates!=0,"First-person render before completed update");
    // PC presentation rebase: retain the completed camera/eye-frame offset while
    // the replacement controller interpolates body/eye and raw mouse look.
    // Pivot at the eye, never at the feet (which rotates eye height on mouse look).
    // No animation, camera history or bob advances in repeated render calls.
    auto anchor_inverse=XMMatrixInverse(nullptr,matrix(look_anchor));
    store(rendered_camera,matrix(camera_world)*anchor_inverse*matrix(render_look));
    store(object_view,matrix(object)*XMMatrixInverse(nullptr,matrix(camera_world)));
    if(fraction!=1) {
        float old_view[16],old_camera[16],current_camera[16],blended_camera[16];
        store(old_view,matrix(previous_object)*XMMatrixInverse(nullptr,matrix(previous_camera)));
        native_interpolate_transform(old_view,object_view,fraction,object_view);
        store(old_camera,matrix(previous_camera)*XMMatrixInverse(nullptr,matrix(previous_anchor)));
        store(current_camera,matrix(camera_world)*anchor_inverse);
        native_interpolate_transform(old_camera,current_camera,fraction,blended_camera);
        store(rendered_camera,matrix(blended_camera)*matrix(render_look));
    }
    float projection[16],sliced[16];
    require(uc_firstperson_projection(resource.field_of_view,aspect,25,50000,0,1,projection)==0&&
        uc_firstperson_depth_slice(projection,.1f,.15f,sliced)==0,"Native weapon projection");
    store(weapon_matrix,matrix(object_view)*matrix(sliced));
}
void NativeFirstperson::draw(NativeRenderer &renderer,const float weapon_matrix[16],const float object_view[16],float inverse_exposure,float fraction,NativeModelLighting*lighting,const NativeLightingFrame*private_frame) {
    require(equipped!=nullptr,"First-person draw without equipment");
    if(!accepts_input())return; // PC cutscene view: retain resources, hide hands/weapon.
    NativeAnimatedModel *models[]={&equipped->gun,&equipped->arms};float depth[2];
    for(unsigned i=0;i<2;i++) {
        const auto &b=bounds[i];
        depth[i]=(b.center[0]*object_view[2]+b.center[1]*object_view[6])+(b.center[2]*object_view[10]+object_view[14]);
    }
    const float white[4]={1,1,1,1};const auto &appearance=equipped->appearance;
    NativeTextureContext textures{0,appearance.data(),appearance.size()};
    const unsigned first=depth[1]<depth[0]?1:0;
    for(unsigned i=0;i<2;i++){unsigned index=first^i;if(index==0&&inventory.active_item==11&&(!inventory.clip[11]||(throw_stage==3&&throw_emitted)))continue;models[index]->present(renderer,fraction);if(lighting&&private_frame)models[index]->draw_lit(renderer,*lighting,*private_frame,object_view,weapon_matrix,white,index?&textures:nullptr,inverse_exposure,true);else models[index]->draw(renderer,weapon_matrix,white,index?&textures:nullptr,inverse_exposure,true);}
    if(flashlight_requested&&resource.flashlight_mode==1&&equipped->flashlight_model){
        auto &light=*equipped->flashlight_model;light.present(renderer,fraction);
        if(lighting&&private_frame)light.draw_lit(renderer,*lighting,*private_frame,object_view,weapon_matrix,white,nullptr,inverse_exposure,true);
        else light.draw(renderer,weapon_matrix,white,nullptr,inverse_exposure,true);
    }
    if(inventory.active_item==0&&shield_marks.count){
        const auto&gun=equipped->gun;float pose[1][16],frame[16];const uint8_t valid=1;
        require(equipped->shield_bone>=0&&gun.presentation_bone(uint32_t(equipped->shield_bone),fraction,pose[0])&&
            uc_muzzle_world(0,1,pose,&valid,weapon_matrix,frame),"Shield mark rendered pose");
        // Existing PC weapon material path uses unit ambient. The source2A0
        // material-lightness owner is not yet reproduced; placement/marks are exact.
        const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
        UCParticleQuad geometry[128];UCParticleQuadVertex packed[512];NativeVertex vertices[1200]{};
        for(unsigned first_mark=0;first_mark<shield_marks.count;first_mark+=128){unsigned count=std::min(128u,shield_marks.count-first_mark);
            require(!uc_player_shield_geometry(&shield_marks,first_mark,count,identity,1,inventory.active_item,action.state,0,geometry)&&
                !uc_particle_quad_batch(geometry,count,packed),"Shield mark geometry");
            for(unsigned n=0;n<count*4;n++){auto&v=vertices[first_mark*4+n];memcpy(v.position,packed[n].position,12);memcpy(v.uv,packed[n].uv,8);v.color=packed[n].color;}
        }
        renderer.update(shield_mark_mesh,vertices,1200);
        NativeDrawState state{};state.blend_mode=1;state.write_alpha=false;state.inverse_exposure=inverse_exposure;state.vertex_exposure=true;
        renderer.draw(shield_mark_mesh,frame,white,shield_mark_texture.Get(),0,shield_marks.count*6,state);
    }
}

void NativeFirstperson::draw_lit(NativeRenderer&r,NativeModelLighting&lighting,const float clip[16],const float world[16],float inverse_exposure,float fraction){
    require(std::isfinite(inverse_exposure)&&inverse_exposure>0,"Private model exposure");
    UCFirstpersonLighting source;uc_firstperson_lighting_neutral(&source);
    NativeLightingDirectional lights[3];for(unsigned i=0;i<3;i++){memcpy(lights[i].direction,source.direction[i],12);memcpy(lights[i].color,source.color[i],16);lights[i].intensity=1;lights[i].source_index=i;}
    NativeLightingFrame frame;frame.owner=this;frame.directionals=lights;frame.directional_count=3;frame.exposure=1.f/inverse_exposure;
    for(unsigned i=0;i<4;i++)frame.ambient[i]=source.ambient[i]*inverse_exposure;
    draw(r,clip,world,inverse_exposure,fraction,&lighting,&frame);
}

/* Native scene integration/inspection milestone, not the playable game yet.
 * Runtime dependencies: ordinary Win32/D3D11, native asset modules and zlib.
 * No original game program, guest memory, hardware interpreter or GPU packets. */
#include "assets.h"
#include "firstperson.h"
#include "player_mission.h"
#include "player_audio.h"
#include "character_targets.h"
#include "character_dialogue.h"
#include "character_speech.h"
#include "dispatch.h"
#include "subtitles.h"
#include "mission_objective.h"
#include "objective_messages.h"
#include "player_footsteps.h"
#include "integration_scene.h"
#include "frame_timing.h"
#include "display.h"
#include "frontend.h"
#include "save_game.h"
#include "graphics_settings.h"
#include "movie.h"
#include "input_bindings.h"
#include "menu_cursor.h"
#include "integration_weapon_effects.h"
#include "integration_impacts.h"
#include "integration_lighting.h"
#include "integration_shadows.h"
#include "scene_ambient.h"
#include "model_lighting.h"
#include "model_shadow_casters.h"
#include "integration_actors.h"
#include "throwables.h"
#include "level_particles.h"
#include "basic_prop.h"
#include "light_fitting.h"
#include "rigid_prop.h"
#include "mission_world.h"
#include "mission_zones.h"
#include "mission_template.h"
#include "cleaver_template.h"
#include "mission_character.h"
#include "ragdoll_world.h"
#include "npc_gun.h"
#include "mission_lightflash.h"
#include "mission_explosion.h"
#include "mission_explosion_visual.h"
#include "explosion_debris.h"
#include "camera_feedback.h"
#include "mission_sound.h"
#include "mission_fire_damage.h"
#include "mission_fire.h"
#include "../molotov_receiver.h"
#include "../scripted_molotov.h"
#include "../player_pain.h"
#include "mission_bullets.h"
#include "bullet_tracers.h"
#include "../audio_material_route.h"
#include "../npc_gun_controller.h"
#include "../npc_melee_controller.h"
#include "npc_throw_handler.h"
#include "npc_thrown_weapon.h"
#include "npc_thrown_particles.h"
#include "player_shield_spark.h"
#include "medic_weapon.h"
#include "heartbeat_monitors.h"
#include "../npc_thrown_effects.h"
#include "../npc_flight.h"
#include "../bullet_scenery.h"
#include "../bullet_actor.h"
#include "../weapon_bullet_emit.h"
#include "../point_light_registry.h"
#include "../npc_replication.h"
#include "../npc_throw_trajectory.h"
#include "../blazer_throw.h"
#include "character_ai_phase.h"
#include "character_ai_maintenance.h"
#include "character_cover.h"
#include "linear_impulse.h"
#include "character_footsteps.h"
#include "../character_hazards.h"
#include "../character_unobserved.h"
#include "../character_attention.h"
#include "mission_handoff.h"
#include "solid_block.h"
#include "hud.h"
#include "hud_heartbeat_draw.h"
#include "hud_reticle.h"
extern "C" {
#include "../scene_asset.h"
#include "../game_difficulty.h"
#include "../character_sight.h"
#include "../scene_models.h"
#include "../archive_reader/archive_reader.h"
#include "../material_asset.h"
#include "../texture_asset/texture_asset.h"
#include "../level_asset.h"
#include "../collision_asset.h"
#include "../collision_asset_query.h"
#include "../player_controller.h"
#include "../environment_fog.h"
#include "../firstperson_projection.h"
}
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include <shellapi.h>
#include <algorithm>
#include <chrono>
#include <climits>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
using std::isfinite;
#include "../../src/pc_mouse_rotation.h"
using namespace DirectX;

struct SceneOwner { UCScene value{}; ~SceneOwner() { uc_scene_free(&value); } };
struct SceneModelsOwner { UCSceneModels value{}; ~SceneModelsOwner(){uc_scene_models_free(&value);} };
struct LevelOwner { UCLevel value{}; ~LevelOwner() { uc_level_free(&value); } };
struct LevelLogicOwner { UCLevelLogic value{}; ~LevelLogicOwner(){uc_level_logic_free(&value);} };
struct CollisionOwner { UCCollisionAsset value{}; ~CollisionOwner() { uc_collision_free(&value); } };
struct RagdollAssetOwner {UCRagdollAsset value{};~RagdollAssetOwner(){uc_ragdoll_asset_free(&value);}};
struct SceneDraw { const UCSceneBatch *batch; BoundingBox bounds; };
static HWND app_window;
static NativeDisplay display;
static NativeFrontend frontend;
static std::unique_ptr<NativeSaveStore> save_store;
static std::optional<NativeSaveRecord> selected_save;
static unsigned save_slot=1;
static bool resume_requested=false;
static void refresh_saves(){
    std::array<std::string,3> summaries;
    selected_save.reset();
    for(unsigned slot=1;slot<=3;slot++){
        bool available=false;
        try{
            auto saved=save_store->load(slot);available=saved.has_value();
            summaries[slot-1]=saved?"Westside - checkpoint "+std::to_string(saved->checkpoint.number)+
                " - "+std::to_string(saved->completed_objectives.size())+" objectives":"Empty";
            if(slot==save_slot)selected_save=std::move(saved);
        }catch(const std::exception &){summaries[slot-1]="Unreadable save - preserved";}
        if(slot==save_slot)frontend.set_save_slot(slot,available,summaries[slot-1],std::filesystem::exists(save_store->path(slot)));
    }
    frontend.set_save_summaries(summaries);
}
static NativeMenuCursor menu_cursor;
static NativeGraphicsSettings graphics;
static NativeFrontendEvent frontend_event;
static bool frontend_enabled=false,mission_active=false,movie_playing=false,movie_skip=false;
static bool settings_dirty=false;
static NativeInputBindings bindings;
static int binding_capture=-1;
static bool display_failed=false,menu_open=false;
static bool paused=false, focused=false, resize_pending=false;
static bool tone_preview=true,tone_available=false;
static bool medic_sequence_check=false,mission_placement_check=false,fire_check=false;
static bool lighting_enabled=true,shadows_enabled=true,lighting_available=false;
static int64_t mouse_x=0, mouse_y=0;
static bool absolute_mouse_valid=false;
static LONG absolute_mouse_x=0,absolute_mouse_y=0;
static uint32_t current_weapon=17,pending_weapon=UINT32_MAX;
static bool pending_reload=false;
static bool pending_flashlight=false,pending_secondary=false;
static bool fire_held=false,fire_pressed=false;
static bool shield_held=false,aim_held=false;
static NativeOrdersInput orders_input;
static void raw_motion(LONG x,LONG y){if(!orders_input.motion(x,y)){mouse_x+=x;mouse_y+=y;}}
static bool explosion_preview=false,explosion_requested=false;
static int wheel_remainder=0;
static bool gameplay_input(){return focused&&!paused&&!menu_open&&!movie_playing&&(!frontend_enabled||(mission_active&&!frontend.active()));}
static HCURSOR visible_pointer(){
    if(frontend_enabled&&frontend.screen()==NativeFrontend::Screen::loading)return nullptr;
    // Cursor allocation must not unwind through a Win32 window callback.
    if(focused&&(paused||(frontend_enabled&&frontend.active()))&&!movie_playing)
        try{return menu_cursor.get(app_window);}catch(const std::exception &){}
    return LoadCursor(nullptr,IDC_ARROW);
}
static void pointer_mode() {
    bool capture=gameplay_input();
    if(capture) {
        RECT r; GetClientRect(app_window,&r); MapWindowPoints(app_window,nullptr,(POINT*)&r,2);
        ClipCursor(&r); SetCapture(app_window); SetCursor(nullptr);
    } else { ClipCursor(nullptr); ReleaseCapture(); SetCursor(visible_pointer()); }
    mouse_x=mouse_y=0;
    absolute_mouse_valid=false;
    pending_weapon=UINT32_MAX;pending_reload=false;pending_flashlight=pending_secondary=false;wheel_remainder=0;
    fire_held=fire_pressed=shield_held=aim_held=false;
    orders_input.cancel();
    explosion_requested=false;
}
static void menu_event(NativeFrontendEvent event){
    if(frontend_event.action==NativeFrontendAction::none)frontend_event=event;
    pointer_mode();
}
static void menu_mouse(HWND window,LPARAM position,bool click){
    RECT client{};GetClientRect(window,&client);
    menu_event(frontend.mouse((float)(short)LOWORD(position),(float)(short)HIWORD(position),
        (unsigned)std::max(1L,client.right),(unsigned)std::max(1L,client.bottom),click));
}
static bool capture_binding(unsigned key){
    if(binding_capture<0)return false;
    if(key==VK_ESCAPE){binding_capture=-1;frontend.set_status("Binding cancelled.");return true;}
    if(!bindings.bind(static_cast<NativeInputBindings::Action>(binding_capture),key)){
        frontend.set_status("That input is reserved or cannot be held. Choose another; Esc cancels.");return true;
    }
    binding_capture=-1;frontend.set_status(bindings.save()?"Binding saved.":"Binding applied; unable to save settings.");pointer_mode();return true;
}
static void input_press(unsigned key){
    using Action=NativeInputBindings::Action;
    if(!gameplay_input())return;
    if(key==bindings.key(Action::fire)){fire_held=fire_pressed=true;}
    else if(key==bindings.key(Action::shield_hold))shield_held=true;
    else if(key==bindings.key(Action::reload))pending_reload=true;
    else if(key==bindings.key(Action::aim))aim_held=true;
    else if(key==bindings.key(Action::secondary))pending_secondary=!pending_secondary;
    //Orders remain unavailable until the manager is connected; do not capture look.
    else if(key==bindings.key(Action::weapon_next)||key==bindings.key(Action::weapon_previous))
        pending_weapon=(pending_weapon!=UINT32_MAX?pending_weapon:current_weapon)==2?17u:2u;
    else{
        const Action actions[]={Action::pistol,Action::shotgun,Action::taser,Action::shield_select,Action::molotov};
        constexpr uint32_t items[]={2,17,1,0,11};
        for(unsigned i=0;i<5;i++)if(key==bindings.key(actions[i])){pending_weapon=items[i];break;}
    }
}
static void input_release(unsigned key){
    if(key==bindings.key(NativeInputBindings::Action::fire))fire_held=false;
    if(key==bindings.key(NativeInputBindings::Action::shield_hold))shield_held=false;
    if(key==bindings.key(NativeInputBindings::Action::aim))aim_held=false;
}
static LRESULT CALLBACK window_proc(HWND window,UINT message,WPARAM w,LPARAM l) {
    switch(message) {
    case WM_ACTIVATEAPP: focused=w!=0; if(app_window) pointer_mode(); return 0;
    case WM_ACTIVATE: focused=LOWORD(w)!=WA_INACTIVE;if(app_window)pointer_mode();return 0;
    case WM_SIZE: resize_pending=true; return 0;
    case WM_ENTERMENULOOP: menu_open=true;pointer_mode();return 0;
    case WM_EXITMENULOOP: menu_open=false;pointer_mode();return 0;
    case WM_COMMAND:
        if(LOWORD(w)==NativeDisplay::fpsCounterCommand){display.command(LOWORD(w));settings_dirty=true;return 0;}
        if(display.command(LOWORD(w))){display_failed=!display.apply(window);resize_pending=settings_dirty=true;return 0;}
        break;
    case WM_GETMINMAXINFO: {
        auto limits=reinterpret_cast<MINMAXINFO*>(l);
        RECT minimum_rect{0,0,320,240},maximum_rect{0,0,4096,4096};
        AdjustWindowRect(&minimum_rect,WS_OVERLAPPEDWINDOW,FALSE);AdjustWindowRect(&maximum_rect,WS_OVERLAPPEDWINDOW,FALSE);
        limits->ptMinTrackSize={minimum_rect.right-minimum_rect.left,minimum_rect.bottom-minimum_rect.top};
        limits->ptMaxTrackSize={maximum_rect.right-maximum_rect.left,maximum_rect.bottom-maximum_rect.top};return 0;
    }
    case WM_MOVE: if(app_window) pointer_mode(); return 0;
    case WM_SETCURSOR:
        if(LOWORD(l)==HTCLIENT){SetCursor(gameplay_input()?nullptr:visible_pointer());return TRUE;}
        break;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if(message==WM_SYSKEYDOWN&&(w==VK_F4||w==VK_SPACE))break;
        if(!(l&(1ll<<30))) {
            if(capture_binding((unsigned)w))return 0;
            if(movie_playing&&(w==VK_ESCAPE||w==VK_RETURN||w==VK_SPACE)){movie_skip=true;return 0;}
            if(w==VK_F5){display.command(NativeDisplay::fpsCounterCommand);settings_dirty=true;return 0;}
            if(w==VK_F6&&tone_available){tone_preview=!tone_preview;settings_dirty=true;return 0;}
            if(w==VK_F7&&lighting_available){lighting_enabled=!lighting_enabled;settings_dirty=true;return 0;}
            if(w==VK_F8&&lighting_available){shadows_enabled=!shadows_enabled;settings_dirty=true;return 0;}
            if(w==VK_F11||w==VK_F9){
                const unsigned id=w==VK_F11?NativeDisplay::fullscreenCommand:display.width==1280?NativeDisplay::resolution1080:display.width==1920?NativeDisplay::resolution1440:NativeDisplay::resolution720;
                display.command(id);display_failed=!display.apply(window);resize_pending=settings_dirty=true;return 0;
            }
            if(frontend_enabled&&frontend.active()){menu_event(frontend.key((unsigned)w));return 0;}
            if(w==VK_ESCAPE){paused=!paused;if(frontend_enabled)frontend.show(paused?NativeFrontend::Screen::pause:NativeFrontend::Screen::closed);pointer_mode();return 0;}
            if(gameplay_input()&&w==VK_F10&&explosion_preview){explosion_requested=true;return 0;}
            input_press((unsigned)w);
        }
        if(message==WM_SYSKEYDOWN)return 0;
        break;
    case WM_KEYUP:case WM_SYSKEYUP:input_release((unsigned)w);break;
    case WM_MOUSEWHEEL:
        if(binding_capture>=0){if(GET_WHEEL_DELTA_WPARAM(w))capture_binding(GET_WHEEL_DELTA_WPARAM(w)>0?NativeInputBindings::wheel_up:NativeInputBindings::wheel_down);return 0;}
        if(frontend_enabled&&frontend.active()){menu_event(frontend.key(GET_WHEEL_DELTA_WPARAM(w)>0?VK_UP:VK_DOWN));return 0;}
        if(gameplay_input()) {
            wheel_remainder+=GET_WHEEL_DELTA_WPARAM(w);
            while(wheel_remainder>=WHEEL_DELTA||wheel_remainder<=-WHEEL_DELTA) {
                const unsigned key=wheel_remainder>0?NativeInputBindings::wheel_up:NativeInputBindings::wheel_down;
                input_press(key);input_release(key);
                wheel_remainder+=wheel_remainder>0?-WHEEL_DELTA:WHEEL_DELTA;
            }
        }
        return 0;
    case WM_LBUTTONDOWN:
        if(capture_binding(VK_LBUTTON))return 0;
        if(movie_playing){movie_skip=true;return 0;}
        if(frontend_enabled&&frontend.active()){menu_mouse(window,l,true);return 0;}
        input_press(VK_LBUTTON);return 0;
    case WM_MOUSEMOVE:
        if(frontend_enabled&&frontend.active()&&!movie_playing&&binding_capture<0)menu_mouse(window,l,false);
        break;
    case WM_LBUTTONUP:input_release(VK_LBUTTON);return 0;
    case WM_RBUTTONDOWN:
        if(capture_binding(VK_RBUTTON))return 0;
        if(frontend_enabled&&frontend.active()&&!movie_playing){menu_event(frontend.key(VK_ESCAPE));return 0;}
        input_press(VK_RBUTTON);return 0;
    case WM_RBUTTONUP:input_release(VK_RBUTTON);return 0;
    case WM_MBUTTONDOWN:if(!capture_binding(VK_MBUTTON))input_press(VK_MBUTTON);return 0;
    case WM_MBUTTONUP:input_release(VK_MBUTTON);return 0;
    case WM_XBUTTONDOWN:{const unsigned key=GET_XBUTTON_WPARAM(w)==XBUTTON1?VK_XBUTTON1:VK_XBUTTON2;if(!capture_binding(key))input_press(key);return TRUE;}
    case WM_XBUTTONUP:input_release(GET_XBUTTON_WPARAM(w)==XBUTTON1?VK_XBUTTON1:VK_XBUTTON2);return TRUE;
    case WM_INPUT: {
        RAWINPUT input{}; UINT size=sizeof(input);
        if(GetRawInputData((HRAWINPUT)l,RID_INPUT,&input,&size,sizeof(RAWINPUTHEADER))!=(UINT)-1 &&
           input.header.dwType==RIM_TYPEMOUSE && gameplay_input()) {
            const auto &motion=input.data.mouse;
            if(motion.usFlags&MOUSE_MOVE_ABSOLUTE) {
                // Absolute raw devices (remote desktops/tablets) report a
                // normalized screen position; re-anchor on every capture.
                bool virtual_screen=(motion.usFlags&MOUSE_VIRTUAL_DESKTOP)!=0;
                LONG x=MulDiv(motion.lLastX,GetSystemMetrics(virtual_screen?SM_CXVIRTUALSCREEN:SM_CXSCREEN),65535);
                LONG y=MulDiv(motion.lLastY,GetSystemMetrics(virtual_screen?SM_CYVIRTUALSCREEN:SM_CYSCREEN),65535);
                if(absolute_mouse_valid)raw_motion(x-absolute_mouse_x,y-absolute_mouse_y);
                absolute_mouse_x=x;absolute_mouse_y=y;absolute_mouse_valid=true;
            } else {
                absolute_mouse_valid=false;raw_motion(motion.lLastX,motion.lLastY);
            }
        }
        break;
    }
    case WM_DESTROY: focused=false; pointer_mode(); PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(window,message,w,l);
}
static void sync_graphics(NativeRenderer &renderer){
    auto &s=graphics.value;
    s.width=display.width;s.height=display.height;s.fullscreen=display.fullscreen;s.fps_counter=display.fps_counter;
    s.msaa=renderer.msaa_samples();s.anisotropy=renderer.anisotropy();s.vsync=renderer.vsync();
    s.lighting=lighting_enabled;s.shadows=shadows_enabled;s.color=tone_preview;
    frontend.set_settings(s);
    if(settings_dirty&&frontend_enabled){if(!graphics.save())frontend.set_status("Settings applied; unable to save native-settings.ini.");settings_dirty=false;}
}
static NativeFrontendAction apply_frontend_event(NativeRenderer &renderer){
    using Action=NativeFrontendAction;
    const auto event=frontend_event;frontend_event={};
    try{
        switch(event.action){
        case Action::select_save_slot:
            if(event.value>=1&&event.value<=3){save_slot=event.value;refresh_saves();}
            return Action::none;
        case Action::rebind:
            if(event.value<(unsigned)NativeInputBindings::Action::count){binding_capture=(int)event.value;frontend.set_status("Press a key, mouse button or wheel direction. Esc cancels.");}
            return Action::none;
        case Action::controls_reset:
            bindings.reset_defaults();frontend.set_status(bindings.save()?"Default controls restored.":"Defaults applied; unable to save settings.");return Action::none;
        case Action::mouse_sensitivity:
            bindings.sensitivity=std::clamp(event.value,25u,200u);
            frontend.set_status(bindings.save()?"Mouse sensitivity saved.":"Sensitivity applied; unable to save settings.");return Action::none;
        case Action::resolution:{
            const unsigned old_width=display.width,old_height=display.height;
            try{renderer.resize(event.value*16/9,event.value);}catch(...){display.width=old_width;display.height=old_height;throw;}
            display.width=event.value*16/9;display.height=event.value;
            if(!display.apply(app_window))throw std::runtime_error("Could not resize the game window.");
            resize_pending=true;break;}
        case Action::fullscreen:
            display.fullscreen=event.value!=0;
            if(!display.apply(app_window)){display.fullscreen=!display.fullscreen;throw std::runtime_error("Could not change fullscreen mode.");}
            resize_pending=true;break;
        case Action::msaa:renderer.set_msaa(event.value);break;
        case Action::anisotropy:renderer.set_anisotropy(event.value);break;
        case Action::vsync:renderer.set_vsync(event.value!=0);break;
        case Action::frame_limit:graphics.value.frame_limit=event.value;break;
        case Action::fps_counter:display.fps_counter=event.value!=0;break;
        case Action::lighting:lighting_enabled=event.value!=0;break;
        case Action::shadows:shadows_enabled=event.value!=0;break;
        case Action::color:tone_preview=event.value!=0;break;
        case Action::resume:frontend.show(NativeFrontend::Screen::closed);paused=false;pointer_mode();return event.action;
        default:sync_graphics(renderer);return event.action;
        }
        settings_dirty=true;frontend.set_status({});
    }catch(const std::exception &e){frontend.set_status(e.what());}
    sync_graphics(renderer);return Action::none;
}
static bool pump_window(NativeRenderer &renderer){
    MSG message{};while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){
        if(message.message==WM_QUIT)return false;
        TranslateMessage(&message);DispatchMessageW(&message);
    }
    if(display_failed)throw std::runtime_error("Display mode change failed");
    if(resize_pending){if(renderer.width!=display.width||renderer.height!=display.height)renderer.resize(display.width,display.height);pointer_mode();resize_pending=false;}
    return true;
}
static bool play_movie(NativeRenderer &renderer,const std::filesystem::path &path){
    NativeMovie movie;movie.open(renderer,path);movie_playing=true;movie_skip=false;pointer_mode();
    NativeFramePacer pacer;bool running=true;
    while(!movie.finished()&&!movie_skip){
        pacer.wait(graphics.value.frame_limit);
        if(!pump_window(renderer)){running=false;break;}
        movie.pause(!focused||IsIconic(app_window));
        if(!focused||IsIconic(app_window)){MsgWaitForMultipleObjectsEx(0,nullptr,50,QS_ALLINPUT,MWMO_INPUTAVAILABLE);continue;}
        movie.step();const float clear[]={0,0,0,1};renderer.begin(clear);renderer.resolve_scene();movie.draw(renderer);renderer.present();
    }
    movie.close();movie_playing=false;movie_skip=false;pointer_mode();return running;
}
static bool present_loading(NativeRenderer &renderer,bool first=false){
    if(!frontend_enabled)return true;
    static std::chrono::steady_clock::time_point started;
    if(first)started=std::chrono::steady_clock::now();
    if(!pump_window(renderer))return false;
    const float clear[]={0,0,0,1};renderer.begin(clear);renderer.resolve_scene();
    frontend.draw_loading(renderer,std::chrono::duration<float>(std::chrono::steady_clock::now()-started).count());
    renderer.present();return true;
}
static bool run_frontend(NativeRenderer &renderer,const std::filesystem::path &movies,bool intro){
    mission_active=false;paused=false;frontend_event={};frontend.show(NativeFrontend::Screen::closed);pointer_mode();
    if(intro)for(const char *name:{"dolby","eidoslogo","rocklogo","intro_12_edit"})
        if(!play_movie(renderer,movies/(std::string(name)+".wmv")))return false;
    frontend.show(intro?NativeFrontend::Screen::splash:NativeFrontend::Screen::main);
    sync_graphics(renderer);pointer_mode();SetWindowTextW(app_window,L"Urban Chaos: Riot Response");
    NativeMovie background;std::string movie_name;NativeFramePacer pacer;
    while(true){
        pacer.wait(graphics.value.frame_limit);
        if(!pump_window(renderer))return false;
        const auto action=apply_frontend_event(renderer);
        if(action==NativeFrontendAction::quit)return false;
        if(action==NativeFrontendAction::start_mission||action==NativeFrontendAction::resume_mission){
            resume_requested=action==NativeFrontendAction::resume_mission;
            if(resume_requested&&!selected_save){frontend.set_status("No saved checkpoint in this slot.");continue;}
            background.close();frontend.show(NativeFrontend::Screen::closed);pointer_mode();
            if(!resume_requested&&!play_movie(renderer,movies/"intro_1.wmv"))return false;
            frontend.begin_loading(std::filesystem::path(UC_NATIVE_GAME_DATA),renderer);pointer_mode();
            frontend_event={};return present_loading(renderer,true);
        }
        const std::string desired=frontend.screen()==NativeFrontend::Screen::splash?
            (movie_name.empty()?"gamelogo":movie_name=="splash"||background.finished()?"splash":"gamelogo"):"fe_bk1";
        if(desired!=movie_name){background.open(renderer,movies/(desired+".wmv"),desired!="gamelogo");movie_name=desired;}
        background.pause(!focused||IsIconic(app_window));
        if(!focused||IsIconic(app_window)){MsgWaitForMultipleObjectsEx(0,nullptr,50,QS_ALLINPUT,MWMO_INPUTAVAILABLE);continue;}
        background.step();const float clear[]={0,0,0,1};renderer.begin(clear);renderer.resolve_scene();
        frontend.draw(renderer,graphics.value,background.texture());renderer.present();
    }
}
static ArchiveOwner load_scene(const std::filesystem::path &path,SceneOwner &scene,uint8_t hash[32],UCSceneModels *models=nullptr,BytesOwner *retained=nullptr) {
    auto archive=read_archive(path,hash);auto raw=archive.get();UCArchiveStatus status;
    size_t selected=find_resource(raw,path.filename().string().c_str());
    if(selected==SIZE_MAX) throw std::runtime_error("No embedded scene entry");
    BytesOwner bytes;
    auto &payload=retained?retained->value:bytes.value;
    status=uc_archive_read(raw,selected,UC_AR_LOCAL_PAYLOAD,&payload);
    if(status!=UC_AR_OK) throw std::runtime_error(std::string("Scene resource: ")+uc_archive_status(status));
    UCSceneStatus parsed=models?uc_scene_load_models(payload.data,payload.size,&scene.value,models):
        uc_scene_load(payload.data,payload.size,&scene.value);
    if(parsed!=UC_SCENE_OK) throw std::runtime_error(std::string("Scene layout: ")+uc_scene_status(parsed));
    return archive;
}
static UCLevelRestart load_restart(std::filesystem::path path,LevelOwner &level,int32_t number=-1,const uint8_t *expected_hash=nullptr) {
    path.replace_extension(".ent");uint8_t hash[32];auto archive=read_archive(path,hash);auto *raw=archive.get();
    if(expected_hash&&std::memcmp(expected_hash,hash,32))throw std::runtime_error("Save belongs to a different level revision; the save was preserved.");
    size_t selected=find_resource(raw,path.filename().string().c_str());
    BytesOwner bytes;
    if(selected==SIZE_MAX||uc_archive_read(raw,selected,UC_AR_LOCAL_PAYLOAD,&bytes.value)!=UC_AR_OK)
        throw std::runtime_error("Cannot read level entities");
    if(uc_level_load(bytes.value.data,bytes.value.size,&level.value)!=UC_LEVEL_OK)throw std::runtime_error("Unsupported level entity layout");
    UCLevelRestart restart{};
    if(uc_level_restart(&level.value,number,&restart)!=UC_LEVEL_OK)throw std::runtime_error("No supported restart point");
    return restart;
}
static void load_collision(const UCArchive *archive,const std::filesystem::path &path,CollisionOwner &collision) {
    auto name=path.stem().string()+".geohk";size_t selected=find_resource(archive,name.c_str());BytesOwner bytes;
    if(selected==SIZE_MAX||uc_archive_read(archive,selected,UC_AR_LOCAL_PAYLOAD,&bytes.value)!=UC_AR_OK)
        throw std::runtime_error("Cannot read world collision");
    if(uc_collision_load(bytes.value.data,bytes.value.size,&collision.value)!=UC_COLLISION_OK)
        throw std::runtime_error("Unsupported world collision layout");
}
static void write_bmp(const std::filesystem::path &path,unsigned width,unsigned height,const std::vector<uint8_t> &rgba) {
    BITMAPFILEHEADER file{}; BITMAPINFOHEADER info{};
    file.bfType=0x4D42; file.bfOffBits=sizeof(file)+sizeof(info); file.bfSize=file.bfOffBits+(DWORD)rgba.size();
    info.biSize=sizeof(info); info.biWidth=(LONG)width; info.biHeight=-(LONG)height;
    info.biPlanes=1; info.biBitCount=32; info.biCompression=BI_RGB;
    /* The HWND swapchain presents opaque RGB. Internal render-target alpha is
     * not image transparency; preserve it in GPU checks, not BMP previews. */
    auto bgra=rgba; for(size_t i=0;i<bgra.size();i+=4){std::swap(bgra[i],bgra[i+2]);bgra[i+3]=255;}
    std::ofstream out(path,std::ios::binary); out.exceptions(std::ios::badbit|std::ios::failbit);
    out.write((char*)&file,sizeof(file)); out.write((char*)&info,sizeof(info)); out.write((char*)bgra.data(),bgra.size());
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,LPWSTR,int show) {
    bool headless=false,overview=false,walk_check=false,idle_check=false,firstperson=false,integration=false,integration_check=false;
    bool shake_check=false,shake_sampled=false,shake_faded=false,shake_released=false;float shake_peak=0;
    bool profile_cpu=false,npc943_check=false,particle_push_check=false,dispatch_check=false,natural_trigger_check=false,weapon_secondary_check=false;
    bool lighting_preview=false,benchmark=false,secondary_check=false,equipment_preview=false,equipment_check=false;
    bool handoff_check=false,mission_preview=false,save_roundtrip_check=false,npc_hit_check=false;
    std::filesystem::path checkpoint_check_root;
    bool resolution_override=false;
    try {
        int argc=0; auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);
        if(!argv) throw std::runtime_error("Cannot read launch arguments");
        if(argc==1)frontend_enabled=mission_preview=equipment_preview=lighting_preview=integration=firstperson=true;
        std::filesystem::path level=std::filesystem::path(UC_NATIVE_GAME_DATA)/"1-01.geo", output;
        for(int i=1;i<argc;++i) {
            if(!wcscmp(argv[i],L"--npc-hit-check")&&i+1<argc){
                npc_hit_check=handoff_check=mission_preview=headless=equipment_preview=lighting_preview=integration=firstperson=true;output=argv[++i];
                SetUnhandledExceptionFilter([](EXCEPTION_POINTERS*p)->LONG{
                    CONTEXT c=*p->ContextRecord;fprintf(stderr,"Native fault %08lx address=%p access=%llu target=%llx\n",p->ExceptionRecord->ExceptionCode,p->ExceptionRecord->ExceptionAddress,
                        (unsigned long long)p->ExceptionRecord->ExceptionInformation[0],(unsigned long long)p->ExceptionRecord->ExceptionInformation[1]);
                    for(unsigned frame=0;frame<24&&c.Rip;++frame){DWORD64 base=0;auto*f=RtlLookupFunctionEntry(c.Rip,&base,nullptr);
                        fprintf(stderr,"frame %u rva=%llx image=%llx\n",frame,(unsigned long long)(c.Rip-base),(unsigned long long)base);
                        if(f){PVOID handler=nullptr;DWORD64 establisher=0;RtlVirtualUnwind(UNW_FLAG_NHANDLER,base,c.Rip,f,&c,&handler,&establisher,nullptr);}
                        else{SIZE_T read=0;if(!ReadProcessMemory(GetCurrentProcess(),(void*)c.Rsp,&c.Rip,sizeof c.Rip,&read)||read!=sizeof c.Rip)break;c.Rsp+=8;}}
                    fflush(stderr);return EXCEPTION_EXECUTE_HANDLER;});
            }
            else if(!wcscmp(argv[i],L"--check")&&i+1<argc) { headless=true; output=argv[++i]; }
            else if(!wcscmp(argv[i],L"--walk-check")&&i+1<argc) { headless=walk_check=true; output=argv[++i]; }
            else if(!wcscmp(argv[i],L"--idle-check")&&i+1<argc) { headless=idle_check=true; output=argv[++i]; }
            else if(!wcscmp(argv[i],L"--level")&&i+1<argc) level=argv[++i];
            else if(!wcscmp(argv[i],L"--overview"))overview=true;
            else if(!wcscmp(argv[i],L"--no-tone-preview"))tone_preview=false;
            else if(!wcscmp(argv[i],L"--first-person"))firstperson=true;
            else if(!wcscmp(argv[i],L"--integration"))integration=firstperson=true;
            else if(!wcscmp(argv[i],L"--integration-check")&&i+1<argc) {
                headless=integration=firstperson=integration_check=true;output=argv[++i];
            }
            else if(!wcscmp(argv[i],L"--lighting-preview"))lighting_preview=integration=firstperson=true;
            else if(!wcscmp(argv[i],L"--equipment-preview"))equipment_preview=lighting_preview=integration=firstperson=true;
            else if(!wcscmp(argv[i],L"--mission-preview"))mission_preview=equipment_preview=lighting_preview=integration=firstperson=true;
            else if(!wcscmp(argv[i],L"--frontend"))frontend_enabled=mission_preview=equipment_preview=lighting_preview=integration=firstperson=true;
            else if(!wcscmp(argv[i],L"--mission-handoff-check")&&i+1<argc){
                handoff_check=mission_preview=headless=equipment_preview=lighting_preview=integration=firstperson=true;output=argv[++i];
            }
            else if(!wcscmp(argv[i],L"--save-roundtrip-check")&&i+1<argc){
                save_roundtrip_check=handoff_check=mission_preview=headless=equipment_preview=lighting_preview=integration=firstperson=true;
                checkpoint_check_root=argv[++i];output=checkpoint_check_root/"fresh";
            }
            else if(!wcscmp(argv[i],L"--mission-medic-check")&&i+1<argc){
                medic_sequence_check=handoff_check=mission_preview=headless=equipment_preview=lighting_preview=integration=firstperson=true;output=argv[++i];
            }
            else if(!wcscmp(argv[i],L"--mission-placement-check")&&i+1<argc){
                mission_placement_check=handoff_check=mission_preview=headless=equipment_preview=lighting_preview=integration=firstperson=true;output=argv[++i];
            }
            else if(!wcscmp(argv[i],L"--mission-fire-check")&&i+1<argc){
                fire_check=handoff_check=mission_preview=headless=equipment_preview=lighting_preview=integration=firstperson=true;output=argv[++i];
            }
            else if(!wcscmp(argv[i],L"--explosion-preview"))explosion_preview=equipment_preview=lighting_preview=integration=firstperson=true;
            else if(!wcscmp(argv[i],L"--secondary-check")&&i+1<argc){
                secondary_check=lighting_preview=headless=integration=firstperson=integration_check=true;output=argv[++i];
            }
            else if(!wcscmp(argv[i],L"--equipment-check")&&i+1<argc){
                equipment_check=equipment_preview=secondary_check=lighting_preview=headless=integration=firstperson=integration_check=true;output=argv[++i];
            }
            else if((!wcscmp(argv[i],L"--lighting-check")||!wcscmp(argv[i],L"--benchmark-lighting"))&&i+1<argc) {
                benchmark=!wcscmp(argv[i],L"--benchmark-lighting");
                lighting_preview=headless=integration=firstperson=integration_check=true;output=argv[++i];
            }
            else if(!wcscmp(argv[i],L"--profile-cpu"))profile_cpu=true;
            else if(!wcscmp(argv[i],L"--npc943-check"))npc943_check=true;
            else if(!wcscmp(argv[i],L"--particle-push-check"))particle_push_check=true;
            else if(!wcscmp(argv[i],L"--shake-check"))shake_check=true;
            else if(!wcscmp(argv[i],L"--dispatch-check"))dispatch_check=true;
            else if(!wcscmp(argv[i],L"--natural-trigger-check"))natural_trigger_check=true;
            else if(!wcscmp(argv[i],L"--weapon-secondary-check"))weapon_secondary_check=true;
            else if(!wcscmp(argv[i],L"--no-lights"))lighting_enabled=false;
            else if(!wcscmp(argv[i],L"--no-shadows"))shadows_enabled=false;
            else if(!wcscmp(argv[i],L"--fullscreen"))display.fullscreen=true;
            else if(!wcscmp(argv[i],L"--resolution")&&i+1<argc){
                resolution_override=true;
                const auto resolution=argv[++i];
                if(!wcscmp(resolution,L"1280x720"))display.command(NativeDisplay::resolution720);
                else if(!wcscmp(resolution,L"1920x1080"))display.command(NativeDisplay::resolution1080);
                else if(!wcscmp(resolution,L"2560x1440"))display.command(NativeDisplay::resolution1440);
                else {LocalFree(argv);throw std::invalid_argument("Resolution must be 1280x720, 1920x1080 or 2560x1440");}
            }
            else { LocalFree(argv); throw std::runtime_error("Use --integration, --lighting-preview, --fullscreen, --resolution 1280x720|1920x1080|2560x1440, --no-lights, --no-shadows, --integration-check/--lighting-check/--benchmark-lighting <directory>, --level <archive>, --overview, --first-person, --no-tone-preview, --check/--idle-check/--walk-check <directory>"); }
        }
        LocalFree(argv);
        if((walk_check||idle_check)&&overview)throw std::runtime_error("Player check requires the original checkpoint");
        if(walk_check&&idle_check)throw std::runtime_error("Select one player check trajectory");
        if(integration&&headless&&!integration_check&&!handoff_check)throw std::runtime_error("Use --integration-check for the combined preview");
        if(frontend_enabled&&headless)throw std::runtime_error("Frontend requires an interactive window");
        if(firstperson&&(overview||level.stem()!=L"1-01"))throw std::runtime_error("First-person integration currently requires fresh Westside");
        wchar_t executable[32768];DWORD length=GetModuleFileNameW(nullptr,executable,32768);
        if(!length||length==32768)throw std::runtime_error("Cannot locate native assets");
        const auto package_directory=std::filesystem::path(executable).parent_path();
        if(save_roundtrip_check){
            save_store=std::make_unique<NativeSaveStore>(checkpoint_check_root/"saves");
            if(std::filesystem::exists(save_store->path(1)))throw std::runtime_error("Save check needs a fresh output directory");
        }
        if(frontend_enabled){
            graphics.load(package_directory);
            bindings.load(package_directory);frontend.set_bindings(bindings);
            save_store=std::make_unique<NativeSaveStore>(NativeSaveStore::default_directory());refresh_saves();
            if(!resolution_override){display.width=graphics.value.width;display.height=graphics.value.height;}
            display.fullscreen=display.fullscreen||graphics.value.fullscreen;display.fps_counter=graphics.value.fps_counter;
            tone_preview=tone_preview&&graphics.value.color;lighting_enabled=lighting_enabled&&graphics.value.lighting;shadows_enabled=shadows_enabled&&graphics.value.shadows;
        }
        unsigned width=display.width,height=display.height;
        if(!headless) {
            SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
            if(!frontend_enabled)display.create_menu();
            WNDCLASSW wc{}; wc.lpfnWndProc=window_proc; wc.hInstance=instance; wc.lpszClassName=L"UrbanChaosNativeScene";
            wc.hCursor=LoadCursor(nullptr,IDC_ARROW); if(!RegisterClassW(&wc)) throw std::runtime_error("Window registration failed");
            RECT rect{0,0,(LONG)width,(LONG)height}; AdjustWindowRect(&rect,WS_OVERLAPPEDWINDOW,FALSE);
            app_window=CreateWindowW(wc.lpszClassName,L"Urban Chaos: Riot Response",WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT,CW_USEDEFAULT,rect.right-rect.left,rect.bottom-rect.top,nullptr,nullptr,instance,nullptr);
            if(!app_window) throw std::runtime_error("Native window creation failed");
            if(!display.apply(app_window))throw std::runtime_error("Display mode setup failed");
            RAWINPUTDEVICE mouse{1,2,0,app_window};
            if(!RegisterRawInputDevices(&mouse,1,sizeof(mouse))) throw std::runtime_error("Mouse registration failed");
            ShowWindow(app_window,show); focused=GetForegroundWindow()==app_window; pointer_mode();
        }
        NativeRenderer renderer;renderer.open(app_window,width,height);
        if(frontend_enabled){
            frontend.load(level.parent_path(),renderer);
            renderer.set_anisotropy(graphics.value.anisotropy);renderer.set_vsync(graphics.value.vsync);
            if(renderer.supports_msaa(graphics.value.msaa))renderer.set_msaa(graphics.value.msaa);
            sync_graphics(renderer);
        }
        bool first_frontend=true;
        for(;;){
        bool return_to_menu=false;
        if(frontend_enabled){
            refresh_saves();
            if(!run_frontend(renderer,package_directory/"native-movies",first_frontend))break;
            first_frontend=false;
        }
        SceneOwner owned;SceneModelsOwner model_metadata;BytesOwner geometry_bytes;uint8_t archive_hash[32];
        auto archive=load_scene(level,owned,archive_hash,lighting_preview?&model_metadata.value:nullptr,
            lighting_preview?&geometry_bytes:nullptr);const UCScene &scene=owned.value;
        const auto restored_record=resume_requested?selected_save:std::nullopt;
        const auto *restored=restored_record?&*restored_record:nullptr;
        LevelOwner entities;auto restart=load_restart(level,entities,restored?restored->checkpoint.number:-1,restored?restored->level_hash.data():nullptr);
        LevelLogicOwner level_logic;
        if(integration&&uc_level_logic_load(&entities.value,&level_logic.value)!=UC_LEVEL_OK)
            throw std::runtime_error("Unsupported authored level trigger/navigation data");
        std::vector<uint8_t> mission_flags(entities.value.field,0);
        //57140 fresh world: three uses for each medic, flags16/17 reflect availability.
        if(integration)for(int32_t number:{16,17})if(uc_mission_flag_set(entities.value.remap,mission_flags.data(),mission_flags.size(),number,1))
            throw std::runtime_error("Missing medic availability flag mapping");
        UCFogProperties fog_properties{};bool fog_present=false;
        for(uint32_t i=0;i<entities.value.entity_count;++i)if(!strcmp(entities.value.entities[i].class_name,"FogController")) {
            if(fog_present)throw std::runtime_error("Multiple fog controllers require native entity selection");
            if(uc_environment_fog_properties(&entities.value,i,&fog_properties)!=UC_FOG_OK)
                throw std::runtime_error("Unsupported authored fog properties");
            fog_present=true;
        }
        /* Select the sole authored fog profile. The integration preview feeds
         * the connected exposure publication; script-driven selection is pending. */
        auto fog_table=std::make_unique<UCFogTable>();ComPtr<ID3D11ShaderResourceView> fog_lookup;
        CollisionOwner collision;load_collision(archive.get(),level,collision);
        float ray_from[]={restart.transform[12],restart.transform[13]+170,restart.transform[14]};
        float ray_to[]={restart.transform[12],restart.transform[13]-2000,restart.transform[14]};
        uc_collision_to_physics(ray_from,ray_from);uc_collision_to_physics(ray_to,ray_to);UCCollisionRayHit ground{};
        int ground_hit=uc_collision_raycast(&collision.value,ray_from,ray_to,&ground);
        if(ground_hit<0)throw std::runtime_error("Invalid native ground query");
        if(!present_loading(renderer))break;
        renderer.profile_cpu=profile_cpu;NativeFrameTiming timing;if(benchmark)timing.open(renderer,360);
        using ProfileClock=std::chrono::steady_clock;std::vector<std::array<double,5>> profile_rows;double profile_world=0,profile_physics=0;
        auto material_path=std::filesystem::path(executable).parent_path()/"native-assets"/(level.stem().string()+".ucmat");
        std::unique_ptr<NativeIntegrationScene> integrated_scene;
        if(integration) {
            integrated_scene=std::make_unique<NativeIntegrationScene>(entities.value,material_path.parent_path());
            tone_available=integrated_scene->available();
        }
        auto materials=load_materials(material_path,archive_hash,archive.get(),scene.material_names,scene.material_count,UC_NATIVE_MATERIAL_GEOMETRY_STATIC,true,renderer);
        /* An explicitly bundled inspection profile, never a live exposure default.
         * Header: UCTPRV01, exact GEO SHA256, then planar R/G/B256-byte tables. */
        auto tone_path=material_path;tone_path.replace_extension(".tone-preview");
        std::vector<uint8_t> tone_reference;ComPtr<ID3D11ShaderResourceView> tone_lookup;
        if(std::filesystem::exists(tone_path)) {
            tone_reference=read_file(tone_path,808);
            if(tone_reference.size()!=808||memcmp(tone_reference.data(),"UCTPRV01",8)||memcmp(tone_reference.data()+8,archive_hash,32))
                throw std::runtime_error("Invalid colour preview or wrong level identity");
            tone_lookup=renderer.upload_tone(tone_reference.data()+40,768);tone_available=true;
        }
        std::vector<NativeVertex> vertices(scene.vertex_count);
        for(size_t i=0;i<vertices.size();++i){memcpy(vertices[i].position,scene.vertices[i].position,12);memcpy(vertices[i].uv,scene.vertices[i].uv,8);vertices[i].color=scene.vertices[i].source_color;}
        auto mesh=renderer.upload(vertices.data(),vertices.size(),scene.indices,scene.index_count);
        if(!present_loading(renderer))break;
        std::unique_ptr<NativeIntegrationLighting> lighting;
        std::unique_ptr<NativeIntegrationShadows> shadows;
        std::unique_ptr<NativeSceneAmbient> scene_ambient;
        std::unique_ptr<NativeModelLighting> model_lighting;
        NativeModelShadowCasters model_casters;
        if(lighting_preview) {
            scene_ambient=std::make_unique<NativeSceneAmbient>(entities.value,geometry_bytes.value.data,geometry_bytes.value.size);
            lighting=std::make_unique<NativeIntegrationLighting>(renderer,scene,model_metadata.value,materials,
                geometry_bytes.value.data,geometry_bytes.value.size,entities.value,archive.get(),material_path.parent_path());
            shadows=std::make_unique<NativeIntegrationShadows>(renderer,scene,mesh,materials,
                std::max(1u,lighting->counts().shadow_eligible));
            model_lighting=std::make_unique<NativeModelLighting>(renderer,*lighting,material_path.parent_path());
            lighting_available=true;
        }
        std::vector<SceneDraw> batches;
        /* B4CA0 maps descriptor+16 to runtime mask bit2; BB7C0 requires it. */
        for(uint32_t i=0;i<scene.batch_count;++i) {
            const auto &batch=scene.batches[i];
            if(batch.light_index!=0xffff||!batch.flags[1]||!batch.index_count)continue;
            XMVECTOR minimum=XMVectorReplicate(FLT_MAX),maximum=XMVectorReplicate(-FLT_MAX);
            for(uint32_t j=0;j<batch.index_count;++j) {
                const auto p=scene.vertices[scene.indices[batch.first_index+j]].position;
                auto point=XMVectorSet(p[0],p[1],p[2],0);minimum=XMVectorMin(minimum,point);maximum=XMVectorMax(maximum,point);
            }
            SceneDraw draw{&batch,{}};BoundingBox::CreateFromPoints(draw.bounds,minimum,maximum);
            /* Conservative one-world-unit margin covers float rounding. These meshes never deform. */
            draw.bounds.Extents.x+=1;draw.bounds.Extents.y+=1;draw.bounds.Extents.z+=1;batches.push_back(draw);
        }
        if(batches.empty()) throw std::runtime_error("Scene has no supported base geometry");
        if(!present_loading(renderer))break;
        size_t textured_batches=0,untextured_batches=0,transparent_batches=0;
        for(const auto &draw:batches) {
            const auto &material=materials[draw.batch->material_index];
            if(material.texture)++textured_batches;
            if(material.state.untextured_material)++untextured_batches;
            if(material.state.blend_mode)++transparent_batches;
        }
        float midpoint[3],radius=1;
        for(unsigned i=0;i<3;++i) { midpoint[i]=(scene.bounds_min[i]+scene.bounds_max[i])*.5f; radius=std::max(radius,scene.bounds_max[i]-scene.bounds_min[i]); }
        XMFLOAT3 center(midpoint),position{};
        position={center.x,center.y+radius*.45f,center.z-radius*.65f};
        float inspection_pitch=-.605544f,inspection_yaw=0,speed=radius*.18f;
        if(!overview) {
            /* Ordinary standing reset: D2270 height170, CC2F0/BCFA0 eye offset.
             * Intro scripts/camera overrides and gameplay movement are pending. */
            position={restart.transform[12],restart.transform[13]+170, restart.transform[14]};
            inspection_pitch=0;inspection_yaw=restart.yaw;speed=400;
        }
        UCPlayer player{};float body[3];uc_collision_to_physics(restart.transform+12,body);uc_player_reset(&player,body);
        // Session banks/device outlive the player that borrows them.
        std::unique_ptr<NativePlayerAudio> player_audio;
        std::unique_ptr<NativePlayerFootsteps> player_footsteps;
        std::unique_ptr<NativeFirstperson> player_view;
        NativeHud hud;
        NativeHeartbeat heartbeat;
        NativeReticle reticle;
        UCGameRandom session_random{};
        UCVectorRandom vector_random{};
        std::unique_ptr<NativeIntegrationWeaponEffects> weapon_effects;
        std::unique_ptr<NativeIntegrationImpacts> impacts;
        std::unique_ptr<NativeIntegrationActors> actors;
        std::unique_ptr<NativeIntegrationThrowables> throwables;
        std::unique_ptr<NativeLevelParticles> level_particles;
        struct BarrierOwner {
            RagdollAssetOwner ragdoll_asset;
            std::unique_ptr<NativeRagdollWorld> ragdoll_world;
            std::vector<std::unique_ptr<NativeSolidBlock>> bodies;
            std::vector<std::unique_ptr<NativeRigidProp>> props;
            std::vector<std::unique_ptr<NativeBasicProp>> static_props;
            std::vector<std::unique_ptr<NativeLightFitting>> light_fittings;
            NativeCharacterWorld character_world;
            NativeCharacterNavigationManager navigation;
            NativeNavigationBoxes navigation_boxes{navigation};
            NativeCharacterDoors doors;
            NativeCharacterFactions factions;
            std::unique_ptr<NativeCharacterAIPhase> ai;
            NativeCharacterAIMaintenance ai_maintenance;
            //52FD0 starts the actual glass/segment observer registry empty.
            //GlassPane factories must register their real observer before they
            //can be admitted; unsupported factories already stop construction.
            std::map<void*,int(*)(void*,const UCBulletSegment*)> segment_observers;
            std::unique_ptr<NativeCharacterCoverPoints> cover_points;
            std::vector<std::unique_ptr<NativeMissionCoverPoint>> covers;
            NativeDialogueText dialogue_text;
            std::unique_ptr<NativeCharacterDialogueAudio> dialogue_audio;
            NativeSubtitles subtitles;
            NativeObjectiveMessages objective_messages;
            NativeHeartbeatMonitors heartbeat_monitors;
            NativeObjectiveSavedStatus objective_saved;
            std::vector<std::unique_ptr<NativeMissionObjective>> objectives;
            std::unique_ptr<UCCharacterHazards,decltype(&uc_character_hazards_close)> hazards{nullptr,uc_character_hazards_close};
            std::vector<std::unique_ptr<NativeMissionCharacter>> characters;
            std::vector<std::unique_ptr<NativeMissionZone>> zones;
            std::vector<std::unique_ptr<NativeCharacterSpeech>> speeches;
            int32_t active_dispatch=-1;
            std::vector<std::unique_ptr<NativeDispatch>> dispatches;
            std::vector<std::unique_ptr<NativeTriggeredCameraShake>> camera_shakes;
            std::unique_ptr<UCParticleRegistry,decltype(&uc_particle_registry_close)> npc_flare_registry{nullptr,uc_particle_registry_close};
            std::vector<std::unique_ptr<NativeNpcGun>> guns;
            std::vector<std::unique_ptr<NativeMissionLightFlash>> gun_flashes;
            std::vector<std::unique_ptr<NativeMissionExplosion>> explosions;
            std::vector<std::unique_ptr<NativeMissionExplosionVisual>> explosion_children;
            std::vector<std::unique_ptr<NativeMissionSound>> sounds;
            std::vector<std::unique_ptr<NativeMissionFireDamage>> fire_spheres;
            std::vector<std::unique_ptr<NativeMissionFire>> fires;
            NativeExplosionDebris explosion_debris;
            NativeCameraFeedback *camera_feedback=nullptr;
            NativePlayerCamera *player_camera=nullptr;
            NativeIntegrationScene *scene=nullptr;
            UCPlayerPain pain{};
            struct CharacterSound {int32_t id;std::string bank;std::unique_ptr<NativePlayerAudio::MissionSource> source;};
            std::vector<CharacterSound> character_sounds;
            std::map<uint32_t,std::array<double,2>> profile_entities;
            std::vector<std::array<double,3>> profile_slow_steps;
            std::vector<std::array<double,19>> profile_props;
            std::vector<std::array<double,14>> profile_prop_boxes;
            void record_prop(const NativeRigidProp&p,unsigned stage){
                if(profile_props.size()>=100000)return;
                std::array<double,19> row{double(p.entity.source),double(world->frame()),double(stage)};
                for(unsigned i=0;i<16;i++)row[i+3]=p.entity.transform[i];profile_props.push_back(row);
                for(const auto&b:p.boxes()){
                    std::array<double,14> box{row[0],row[1],row[2]};
                    for(unsigned i=0;i<3;i++){box[3+i]=b.center[i];box[6+i]=b.half[i];}
                    for(unsigned i=0;i<4;i++)box[9+i]=b.rotation[i];box[13]=b.radius;profile_prop_boxes.push_back(box);
                }
            }
            std::map<void*,std::unique_ptr<NativePlayerAudio::MissionSource>> loop_sources;
            std::map<std::string,std::unique_ptr<NativePlayerAudio::MissionSource>> world_sounds;
            NativeBulletTracers bullet_tracers;
            std::unique_ptr<NativeMissionBullets> bullets;
            UCCollisionResult bullet_collision{};
            int32_t bullet_target=-1;const void*bullet_actor=nullptr;const UCBulletState*query_bullet=nullptr;
            UCCharacterHitStatistics hit_statistics{nullptr,UINT32_MAX,UINT32_MAX};
            uint64_t npc_bullet_contacts=0,npc_head_contacts=0;
            int32_t checked_npc=-1;float checked_health=0;unsigned checked_body_hits=0,checked_corpse_hits=0,checked_blasts=0,checked_stuns=0;
            uint32_t restrained_bodies=0;
            UCWeaponBulletRequest checked_bullet{};
            float primary_accuracy=1;
            uint32_t bullet_player=UINT32_MAX;
            //Explicit fresh PC profile: ordinary offline session, blood on,
            //no progression181 unlock. This does not read or alter saved data.
            uint8_t bullet_progression=0,blood_enabled=1,bullet_session_mode=0;
            std::vector<std::unique_ptr<NativeNpcThrownWeapon>> npc_projectiles;
            std::vector<std::unique_ptr<NativePlayerShieldSpark>> shield_sparks;
            std::vector<std::unique_ptr<NativeMedicWeapon>> medic_weapons;
            int32_t medical_heals[2]={3,3};
            float concussion_attenuation=1; //Actual fresh4E7D38 shared audio scalar.
            std::unique_ptr<NativeNpcThrownParticles> npc_thrown_particles;
            NativeNpcThrownServices npc_projectile_services{};
            UCGunFlareServices npc_effects{};
            NativeNpcThrowServices npc_throw{};
            UCNpcGunControllerServices npc_gun{};
            UCGameDifficulty difficulty{};
            float throw_channel=.5f; //64B20 ->166B0/166C0, distinct profile+14C.
            float brute_channel=.5f; //Same constructor, distinct profile+144.
            std::string mission_name;
            float published_camera[16]{};
            std::vector<std::unique_ptr<NativeLinearImpulse>> impulses;
            NativeMissionWorld *world=nullptr;
            UCVectorRandom *vector_random=nullptr;
            NativeRenderer *renderer=nullptr;const UCCollisionAsset *collision=nullptr;
            NativeLevelParticles *particles=nullptr;
            NativeIntegrationLighting *lighting=nullptr;UCGameRandom *random=nullptr;
            NativeMissionPlayer *player=nullptr;NativeFirstperson *firstperson=nullptr;
            NativeMissionHandoff *handoff=nullptr;
            NativePlayerAudio *audio=nullptr;
            NativeIntegrationImpacts *impacts=nullptr;
            struct MolotovTemplate:NativeMolotovTemplate {BarrierOwner*owner;explicit MolotovTemplate(BarrierOwner*p):owner(p){}};
            std::vector<std::unique_ptr<MolotovTemplate>> molotov_templates;
            std::vector<std::unique_ptr<NativeCleaverTemplate>> cleaver_templates;
            ArchiveOwner archive{nullptr,uc_archive_close};uint8_t archive_hash[32]{};
            UCArchiveLookup texture_lookup{};
            std::filesystem::path catalog,audio_stream;
            float radius=.05f;uint32_t crt_seed=1;
            NativeMissionCharacter*character(int32_t id){
                auto*t=uc_entity_event_lookup(&world->registry(),id);if(!t)return nullptr;
                for(auto&c:characters)if(t==&c->entity.identity)return c.get();return nullptr;
            }
            NativeNpcThrownWeapon*projectile(int32_t id){
                auto*t=uc_entity_event_lookup(&world->registry(),id);if(!t)return nullptr;
                for(auto&p:npc_projectiles)if(t==&p->entity.identity)return p.get();return nullptr;
            }
            NativeNpcGun*gun(int32_t id){
                auto*t=uc_entity_event_lookup(&world->registry(),id);if(!t)return nullptr;
                for(auto&g:guns)if(t==&g->entity.identity)return g.get();return nullptr;
            }
            void bind_gun_controller(){
                auto&h=npc_gun;h.owner=this;h.random=random;
                h.difficulty=[](void*q,double*out){return uc_game_difficulty_value(&static_cast<BarrierOwner*>(q)->difficulty,out);};
                h.resolve=[](void*q,int32_t id,const UCEntityFreshView**out){auto&s=*static_cast<BarrierOwner*>(q);
                    auto*t=uc_entity_event_lookup(&s.world->registry(),id);auto&c=s.world->context();*out=t?c.resolve(c.owner,t):nullptr;return t&&!*out?1:0;};
                h.boolean=[](void*q,int32_t id,const char*name,unsigned*out){auto&s=*static_cast<BarrierOwner*>(q);auto h=s.target_services();int32_t value=0;
                    //E1C36 is the sole "is character" publisher. The admitted
                    //player/prop/base getters do not inherit CharacterE1A50.
                    if(!std::strcmp(name,"is character")){*out=s.character(id)!=nullptr;return 0;}
                    int found=h.property(h.owner,id,name,&value);if(found<0)return 1;*out=found?unsigned(value!=0):0;return 0;};
                h.vector=[](void*q,const UCEntityFreshView*v,const char*name,float*out,unsigned*found){auto&s=*static_cast<BarrierOwner*>(q);
                    if(!v||!v->id||uc_entity_event_lookup(&s.world->registry(),*v->id)!=v->target.identity)return 1;
                    if(!std::strcmp(name,"target position")){int r=s.target_position(*v->id,true,out);if(r<0)return 1;*found=unsigned(r!=0);return 0;}
                    //E1A50/C36B0 do not handle the exact "head position" key.
                    //29810 then uses entity translation, not the "head pos" getter.
                    if(!std::strcmp(name,"head position")&&(s.character(*v->id)||v==&s.player->entity.view)){*found=0;return 0;}
                    return 1;};
                h.weapon_type=[](void*q,int32_t id,int32_t*out){auto*g=static_cast<BarrierOwner*>(q)->gun(id);if(!g)return 1;*out=g->state().weaponD8;return 0;};
                h.matrix=[](void*q,int32_t id,float*out){auto*g=static_cast<BarrierOwner*>(q)->gun(id);return g?g->gun_matrix(out):1;};
                h.fire_position=[](void*q,int32_t id,float*out){auto*g=static_cast<BarrierOwner*>(q)->gun(id);return g?g->fire_position(out):1;};
                h.query=[](void*q,const float*from,const float*to,uint16_t group,const UCEntityFreshView**out,unsigned*found){auto&s=*static_cast<BarrierOwner*>(q);
                    s.refresh_collision();UCCollisionResult hit{};int32_t id=0;const void*actor=nullptr;
                    int r=s.character_world.query(from,to,9,group,0x4000,0,hit,id,&actor);if(r<0)return 1;*found=unsigned(r!=0);*out=nullptr;
                    if(actor){auto*t=uc_entity_event_lookup(&s.world->registry(),id);auto&c=s.world->context();*out=t?c.resolve(c.owner,t):nullptr;if(!*out)return 1;}return 0;};
                h.segment_clear=[](void*q,const float*from,const float*to,unsigned*out){uint8_t clear=0;auto&s=*static_cast<BarrierOwner*>(q);
                    if(uc_character_hazards_segment_clear(s.hazards.get(),from,to,0,&clear))return 1;*out=clear;return 0;};
                h.shot=[](void*q,int32_t id,const float*direction,float scale,unsigned hit){auto*g=static_cast<BarrierOwner*>(q)->gun(id);return g?g->shoot(direction,scale,hit):1;};
                h.reload_sound=[](void*q,int32_t id){auto*g=static_cast<BarrierOwner*>(q)->gun(id);return g?g->reload_sound():1;};
                h.emote=[](void*q,int32_t id,unsigned code,unsigned priority){auto*c=static_cast<BarrierOwner*>(q)->character(id);if(!c||!c->dialogue())return 1;
                    c->dialogue()->say(int32_t(code),int32_t(priority));return 0;};
                h.remove_weapon=[](void*q,int32_t id){auto&s=*static_cast<BarrierOwner*>(q);auto*g=s.gun(id);if(!g)return 1;auto&e=g->entity;e.flags|=1;
                    if(e.flags&8){auto&r=*s.world->context().registration;*r.creation={0,1,e.parent};if(r.insert_record(r.owner,&e.view.target,id,1))return 1;e.flags&=~8u;}return 0;};
                h.delta=[](void*q){auto*s=static_cast<BarrierOwner*>(q);return double(*s->world->script_services().delta);};
            }
            int play_sound(int32_t id,const char*bank,const char*name,bool stop=false){
                if(!bank||!name||!uc_entity_event_lookup(&world->registry(),id))return 1;
                auto found=std::find_if(character_sounds.begin(),character_sounds.end(),[&](const auto&v){return v.id==id&&v.bank==bank;});
                if(found==character_sounds.end()){
                    if(stop)return 0;size_t index=SIZE_MAX;if(!audio||uc_archive_lookup(&texture_lookup,bank,&index)||index==SIZE_MAX)return 1;
                    character_sounds.push_back({id,bank,std::make_unique<NativePlayerAudio::MissionSource>(*audio,archive.get(),index,audio_stream)});found=character_sounds.end()-1;}
                if(stop)found->source->stop(name);else found->source->play(name);return 0;
            }
            int world_sound(const char*bank,const char*event,uint32_t material,const float point[3]){
                if(!bank||!point||!audio)return 1;
                for(unsigned i=0;i<3;i++)if(!std::isfinite(point[i]))return 1;
                auto&source=world_sounds[bank];if(!source){size_t index=SIZE_MAX;
                    if(uc_archive_lookup(&texture_lookup,bank,&index)||index==SIZE_MAX)return 1;
                    source=std::make_unique<NativePlayerAudio::MissionSource>(*audio,archive.get(),index,audio_stream);}
                //Same PC nonspatial output as mission sounds. The actual event,
                //material fallback, bank selection and PCM playback are retained;
                //original source pooling, attenuation and Doppler remain separate.
                if(event){source->play(event);return 0;}
                UCAudioWorldMaterialServices h{source.get(),
                    [](void*q,const float*,float)->void*{return q;},
                    [](void*,void*q,const void*,const char*name,uint32_t)->uint8_t{return static_cast<NativePlayerAudio::MissionSource*>(q)->play(name);}};
                return uc_audio_world_material_request(source.get(),point,material,nullptr,0,&h);
            }
// Health services borrow real actor state seeded by Root::common.
int character_health(NativeMissionCharacter&actor,UCCharacterHealth&state,uint32_t flags,float damage,UCCharacterHealthServices&h){
    if(!handoff||!player)return 1;
    struct Context{BarrierOwner*s;NativeMissionCharacter*c;} context{this,&actor};h.owner=&context;h.gore_disabled=!blood_enabled;
    h.boolean=[](void*q,const char*name,uint8_t*out){int32_t value=0;int found=static_cast<Context*>(q)->c->property(name,&value);if(found<0)return 1;*out=found&&value;return 0;};
    h.body=[](void*q,UCCharacterHealthBody kind,uint8_t*out){return static_cast<Context*>(q)->c->health_body(kind,out);};
    h.stat=[](void*,const char*,float){return 0;}; //Original6BA40 is RET.
    h.counter=[](void*q,unsigned offset,int eligible){auto&s=*static_cast<Context*>(q)->s;s.hit_statistics.values=s.handoff->statistics;return uc_character_hit_count(&s.hit_statistics,s.world->frame(),offset,eligible);};
    h.counter_value=[](void*q,unsigned offset,int32_t*out){if(offset!=8)return 1;*out=(int32_t)static_cast<Context*>(q)->s->handoff->statistics[2];return 0;};
    h.increment_kills=[](void*q){++static_cast<Context*>(q)->s->handoff->statistics[1];return 0;};
    h.player=[](void*q,int32_t*out){*out=static_cast<Context*>(q)->s->player->entity.id;return 0;};
    h.target=[](void*q,int32_t*out){*out=-1;return static_cast<Context*>(q)->c->property("target ID",out)<0?1:0;};
    h.player_integer=[](void*q,int32_t id,const char*name,int32_t*out){auto&s=*static_cast<Context*>(q)->s;if(id!=s.player->entity.id)return 1;*out=0;return s.player->target_property(name,*out)<0?1:0;};
    h.player_scalar=[](void*q,int32_t id,const char*name,double*out){auto&s=*static_cast<Context*>(q)->s;if(id!=s.player->entity.id)return 1;*out=0;return s.player->target_scalar_property(name,*out,s.handoff->session_mode,0)<0?1:0;};
    h.dialogue=[](void*q,uint32_t code,int32_t priority){auto*c=static_cast<Context*>(q)->c;if(!c->dialogue())return 1;c->dialogue()->say(int32_t(code),priority);return 0;};
    h.headshot_text=[](void*q,char*out){auto&s=*static_cast<Context*>(q)->s;int32_t goal;if(s.headshot_goal(&goal))return 1;std::snprintf(out,16,"%d/%d",int32_t(s.handoff->statistics[2]),goal);return 0;};
    h.headshot_goal=[](void*q,int32_t*out){return static_cast<Context*>(q)->s->headshot_goal(out);};
    h.message=[](void*q,int32_t type,const char*title,const char*text,const char*value,int32_t icon){int r=static_cast<Context*>(q)->s->objective_messages.add(type,title,text,value,icon);if(r)throw std::runtime_error("Headshot HUD message failed");return r;};
    h.sound=[](void*q,const char*bank,const char*event){auto&c=*static_cast<Context*>(q);int r=c.s->play_sound(c.c->entity.id,bank,event);if(r)throw std::runtime_error("Headshot sound failed");return r;};
    h.effect=[](void*q,const char*bone,const char*asset){auto&c=*static_cast<Context*>(q);return c.s->character_effect(*c.c,bone,asset);};
    h.set_health=[](void*q,float value){return static_cast<Context*>(q)->c->set_health(value);};
    h.indicator=[](void*q,int32_t id,float amount,uint8_t explosive){auto&c=*static_cast<Context*>(q);struct Packet{float amount;uint8_t explosive,pad[3];} packet{amount,explosive,{}};
        uint32_t sender=uint32_t(c.c->root_id());uint8_t handled=0;return uc_entity_event_send(&c.s->world->registry(),id,35,&packet,sizeof packet,&sender,&handled);};
    h.enemy_type=[](void*q,int32_t*out){*out=-1;return static_cast<Context*>(q)->c->property("dda enemy type",out)<0?1:0;};
    h.dda_kill=[](void*q,int32_t type){auto&s=*static_cast<Context*>(q)->s;if(s.handoff->adaptive_disabled)return 0;if(s.difficulty.mode>3||(type!=0&&type!=2))return 1;
        const float rates[]={1,1,1.6000000238418579f,2};float&value=type==0?s.brute_channel:s.throw_channel;
        const float step=type==0?.699999988079071f/15.f:.4000000059604645f/12.f;value=std::min(1.f,value+step*rates[s.difficulty.mode]);return 0;}; //362B0 ->16720.
    // Other damage classes retain mandatory callbacks when their actual owners are connected.
    const int result=uc_character_health(&state,flags,damage,&h);
    if(result)throw std::runtime_error("NPC health failed flags="+std::to_string(flags)+" hp="+std::to_string(*h.health)+" mission="+mission_name+" headshots="+std::to_string(handoff->statistics[2])+" kills="+std::to_string(handoff->statistics[1]));
    return result;
}
int headshot_goal(int32_t*out){
    static const char*missions[]={"12-01","1-01","2-01","3-01","4-01","5-01","6-01","8-01","9-01","11-01","13-01"};
    static const int32_t goals[]={5,20,20,30,30,30,40,40,40,40,30}; //589C0 column1.
    for(unsigned i=0;i<11;i++)if(mission_name==missions[i]){*out=goals[i];return 0;}return 1;
}
int character_died(NativeMissionCharacter&actor,int32_t killer,bool gore){
    struct Packet{int32_t actor,killer;uint8_t gore,pad[3];} packet{actor.entity.id,killer,uint8_t(gore),{}};
    uint32_t sender=uint32_t(actor.root_id());
    return uc_entity_listener_broadcast(world->listeners(),&world->registry(),26,&packet,sizeof packet,&sender);
}

            int character_effect(NativeMissionCharacter&c,const char*bone,const char*asset){
                float local[16];if(!c.posed_bone(bone,local)||!impacts)return 1;
                XMFLOAT4X4 a,b,frame;std::memcpy(&a,local,64);std::memcpy(&b,c.entity.transform,64);
                XMStoreFloat4x4(&frame,XMLoadFloat4x4(&a)*XMLoadFloat4x4(&b));
                return impacts->fixed_particle(asset,&frame._11);
            }
            int character_bullet(NativeMissionCharacter&actor,const UCCharacterBullet&b,const UCCharacterBulletImpact&p,float amount,const UCEntityFreshView&source){
                struct Context{BarrierOwner*s;NativeMissionCharacter*c;const UCEntityFreshView*v;const UCCharacterBullet*b;} context{this,&actor,&source,&b};
                UCCharacterBulletServices h{};h.owner=&context;
                h.heavy_weapon=[](void*,int32_t item,uint8_t*out){*out=item==24||item==25;return 0;}; //431A0
                h.boolean=[](void*q,int32_t id,const char*name,uint8_t fallback,uint8_t*out){auto&c=*static_cast<Context*>(q);auto&s=*c.s;int32_t value=0;
                    if(id==s.player->entity.id&&!strcmp(name,"is sniping")){
                        // C36B0 fresh pistol/shotgun domain; neither has a scope.
                        if(s.firstperson->inventory.active_item!=2&&s.firstperson->inventory.active_item!=17)return 1;*out=0;return 0;}
                    auto*actor=s.character(id);if(!actor&&id==c.c->root_id())actor=c.c;if(!actor)return 1;
                    int found=actor->property(name,&value);if(found<0)return 1;*out=found?uint8_t(value!=0):fallback;return 0;};
                h.resolve=[](void*q,int32_t id,uint8_t*out){auto&s=*static_cast<Context*>(q)->s;*out=uc_entity_event_lookup(&s.world->registry(),id)!=nullptr;return 0;};
                h.player_index=[](void*q,int32_t id,int32_t*out){auto&c=*static_cast<Context*>(q);if(id!=*c.v->id)return 1;
                    *out=*c.v->target.parent_id==c.s->player->entity.id?0:-1;return 0;};
                h.source_owner=[](void*q,int32_t id,int32_t*out,uint8_t*found){auto&c=*static_cast<Context*>(q);if(id!=*c.v->id)return 1;
                    *out=c.b->owner;*found=1;return 0;}; //3F240 owner -> bulletE0.
                h.friendly=[](void*q,int32_t id,uint8_t*out){*out=static_cast<Context*>(q)->c->damage_friendly(id);return 0;};
                h.target=[](void*q,int32_t id){return static_cast<Context*>(q)->c->damage_target(id);};
                h.effect=[](void*q,int32_t id,const char*bone,const char*asset){auto&c=*static_cast<Context*>(q);return id==c.c->entity.id?c.s->character_effect(*c.c,bone,asset):1;};
                h.health=[](void*q,float value,int32_t kind){return static_cast<Context*>(q)->c->set_health(value,kind);};
                h.profile=[](void*q,uint32_t offset,uint8_t*out){auto&s=*static_cast<Context*>(q)->s;
                    if(offset==0x185){*out=0;return 0;} //Fresh profile: no incendiary upgrade.
                    if(offset==0x32){*out=s.blood_enabled;return 0;}return 1;};
                h.injure=[](void*q,uint32_t flags,float damage,const float*point,int32_t kind,int32_t owner,uint8_t check,uint8_t*out){
                    return static_cast<Context*>(q)->c->injure(flags,damage,point,kind,owner,check,out);};
                h.ray=[](void*q,const float*a,const float*b,uint32_t type,uint16_t group,uint32_t mask,float impulse,UCCharacterBloodHit*out,uint8_t*found){
                    auto&s=*static_cast<Context*>(q)->s;UCCollisionResult hit{};int32_t id;const void*actor=nullptr;
                    int result=s.character_world.query(a,b,type,group,mask,impulse,hit,id,&actor);if(result<0)return 1;*found=result!=0;
                    if(result){std::memcpy(out->point,hit.point,12);std::memcpy(out->normal,hit.normal,12);out->material=hit.material;out->actor=actor!=nullptr;}return 0;};
                h.random=[](void*q,int32_t low,int32_t high,int32_t*out){return int(uc_game_random_range(static_cast<Context*>(q)->s->random,low,high,out));};
                h.decal=[](void*q,const char*shader,const UCCharacterBloodHit*hit,float size,uint32_t mask,uint32_t flags){auto&s=*static_cast<Context*>(q)->s;
                    const float sizes[2]={size,size};return s.impacts->surface_decal(s.character_world,shader,hit->point,hit->normal,sizes,mask,flags,*s.vector_random);};
                return actor.bullet(b,p,amount,h);
            }
            int bullet_contact(const UCEntityFreshView&v,UCBulletState&b,UCBulletImpact&p){
                if(!bullet_actor)return 0;auto*c=character(bullet_target);if(!c){
                    // These existing PC geometry adapters retain scenery impacts.
                    // Their destructible/light-break gameplay is a separate owner.
                    if(particles&&particles->collision_owner(bullet_target))return 0;
                    for(const auto&owner:static_props)if(owner->entity.id==bullet_target)return 0;
                    for(const auto&owner:light_fittings)if(owner->entity.id==bullet_target)return 0;
                    for(const auto&owner:props)if(owner->entity.id==bullet_target)return 0;
                    for(const auto&owner:bodies)if(owner->entity.id==bullet_target)return 0;
                    return 1;
                }
                struct Contact{BarrierOwner*s;NativeMissionCharacter*c;const UCEntityFreshView*v;float damage=0;} context{this,c,&v};
                UCBulletActorServices h{};h.owner=&context;
                h.boolean=[](void*q,const char*name,uint8_t*out){int32_t value=0;int found=static_cast<Contact*>(q)->c->property(name,&value);
                    if(found<0)return 1;*out=found&&value;return 0;};
                h.damage=[](void*q,float value,const UCPlayerFireDamage*p){if(p->kind==8||p->kind==9)return 1;static_cast<Contact*>(q)->damage=value;return 0;}; //E0470 ordinary13C.
                h.react=[](void*q,const UCCharacterBullet*b,const UCCharacterBulletImpact*p){auto&c=*static_cast<Contact*>(q);
                    int r=c.s->character_bullet(*c.c,*b,*p,c.damage,*c.v);if(!r){++c.s->npc_bullet_contacts;c.s->npc_head_contacts+=p->material==9;}return r;};
                return uc_bullet_actor_contact(&v,&b,&c->entity.view,&p,&h);
            }
            void check_npc_hit(bool head,bool shotgun=false){
                if(shotgun&&firstperson->inventory.active_item!=17)throw std::runtime_error("Shotgun hit check weapon not equipped");
                refresh_collision(true);NativeMissionCharacter*c=character(checked_npc);
                if(!c)for(auto&candidate:characters){int32_t enemy=0;
                    if(candidate->ready()&&candidate->entity.actor.health9C>80&&candidate->property("enemy of player",&enemy)==1&&enemy){c=candidate.get();break;}}
                if(!c){std::string detail="NPC hit check has no living enemy";for(auto&candidate:characters){int32_t enemy=0;candidate->property("enemy of player",&enemy);
                    detail+=" id="+std::to_string(candidate->entity.id)+" hp="+std::to_string(candidate->entity.actor.health9C)+" enemy="+std::to_string(enemy)+" ready="+std::to_string(candidate->ready());}throw std::runtime_error(detail);}
                checked_npc=c->entity.id;checked_health=c->entity.actor.health9C;
                const NativeCharacterWorld::Capsule*shape=nullptr;
                for(const auto&part:character_world.capsules)if(part.entity==checked_npc){
                    const auto material=(part.filter.shape_user_data>>12)&0xffff; // metadata root material decoded by collision result.
                    if((head&&material==9)||(!head&&material!=9)){shape=&part;break;}}
                if(!shape)throw std::runtime_error("NPC hit check missing authored hit capsule");
                const auto&capsule=*shape->geometry;float point[3]={(capsule.a[0]+capsule.b[0])*50,(capsule.a[2]+capsule.b[2])*50,(capsule.a[1]+capsule.b[1])*50};
                UCWeaponBulletRequest r{};r.item=2;r.definition_kind=1;r.team=4;r.actor=uint32_t(player->entity.id);r.sentinel=-1;r.value20=80;r.scale24=r.value28=1;
                std::memcpy(r.origin,point,12);r.origin[0]-=float(capsule.radius*100+10);r.direction[0]=1;
                checked_bullet=r;
                bullet_player=uint32_t(player->entity.id);const auto before=npc_bullet_contacts,head_before=npc_head_contacts;
                const int result=shotgun?fire_primary(17,r.origin,r.direction):bullets->fire(r,bullet_player);
                if(result||npc_bullet_contacts<=before||(head&&npc_head_contacts<=head_before)||(!head&&bullet_collision.material==9))
                    throw std::runtime_error("NPC hit check ray did not reach requested body part");
                if(!head)++checked_body_hits;
            }
            void spawn_check_npc(uint32_t source){
                const int result=world->ensure(source,&checked_npc);
                if(result||checked_npc<0)throw std::runtime_error("NPC hit fixture construction failed source="+std::to_string(source)+" result="+std::to_string(result)+
                    " id="+std::to_string(checked_npc)+" failure_source="+std::to_string(world->failure_source())+" phase="+world->failure_phase());
            }
            void check_corpse_hit(){
                auto*c=character(checked_npc);if(!c||!(c->status()&1))throw std::runtime_error("Repeated-hit fixture needs dead character");
                const auto body_count=ragdoll_world->ragdoll_body_count();const auto health=c->entity.actor.health9C;
                if(bullets->fire(checked_bullet,uint32_t(player->entity.id))||ragdoll_world->ragdoll_body_count()!=body_count||c->entity.actor.health9C!=health)
                    throw std::runtime_error("Repeated bullet recreated or damaged retired character owner");
                ++checked_corpse_hits;
            }
            void check_npc_blast(bool stun=false){
                auto*c=character(checked_npc);if(!c||!c->ready()||c->entity.actor.health9C<=0)throw std::runtime_error("Blast fixture needs living enemy");
                // Inject an ordinary source-format listener9 packet through the
                // real broadcast/collision/damage path, not a direct health edit.
                UCExplosionBlast b{};std::memcpy(b.point,c->entity.transform+12,12);b.point[0]+=50;
                b.radius=500;b.damage=stun?0.f:2000.f;b.type=stun?2:0;b.blast=stun?0:1;b.owner=player->entity.id;b.team=b.parent=-1;
                auto h=explosion_services();if(h.broadcast(h.owner,b,player->entity.id))throw std::runtime_error("NPC blast listener failed");
                if(stun){
                    if(!(c->status()&4)||c->entity.actor.health9C<=0)throw std::runtime_error("Stun did not restrain living character");
                    const auto count=ragdoll_world->ragdoll_body_count();
                    if(checked_stuns&&count!=restrained_bodies)throw std::runtime_error("Repeated stun recreated articulated bodies");
                    restrained_bodies=count;++checked_stuns;
                }else ++checked_blasts;
            }
            int fire_primary(uint32_t item,const float*origin,const float*direction){
                if(!firstperson->equipped||!bullets||!camera_feedback||!lighting)return 1;
                // Publish the real unspread PC eye ray once for all pellets. Health
                // consumes its target/material/distance on the next mission tick.
                refresh_collision(true);float end[3];for(unsigned j=0;j<3;j++)end[j]=origin[j]+direction[j]*15000.f;
                UCCollisionResult hit{};int32_t actor=-1;const void*owner=nullptr;
                const int found=character_world.query(origin,end,9,player->entity.actor.group94,0x4000,0,hit,actor,&owner);
                if(found<0)return 1;UCFirstpersonAimCollision aim{};aim.entity=found&&owner?actor:-1;
                aim.distance=found?hit.distance:15000.f+.01f;aim.detail=found?hit.material:0;
                std::memcpy(aim.point,found?hit.point:end,12);if(player->publish_target(aim))return 1;
                uint32_t id=uint32_t(player->entity.id),spread_disabled=0,mode=handoff->session_mode,team=0;
                int32_t selected=-1,pellets=15;uintptr_t network=0;uint8_t upgrades[48]{};float target[3],unused[3]{};
                for(unsigned j=0;j<3;j++)target[j]=origin[j]+direction[j]*50000.f;
                struct Primary{BarrierOwner*s;const float*origin;} context{this,origin};
                UCWeaponBulletEmitView v{&id,&spread_disabled,player->entity.transform+12,&primary_accuracy,target,unused,&selected,&pellets,
                    &mode,&team,upgrades,&network,&firstperson->equipped->state};
                UCWeaponBulletEmitServices h{};h.context=&context;
                h.preflight=[](void*,uint32_t,const UCWeaponBulletEmitView*){return 1;};
                h.feedback=[](void*q,const float*p,float radius,float amplitude,float duration,uint32_t kind){return static_cast<Primary*>(q)->s->camera_feedback->spatial(p,radius,amplitude,duration,kind);};
                // PC free aim uses the published eye ray; original aim assist remains separate.
                h.tag0_origin=[](void*q,float*out){std::memcpy(out,static_cast<Primary*>(q)->origin,12);return 0;};
                h.bullet=[](void*q,const UCWeaponBulletRequest*r,uint32_t id){auto&s=*static_cast<Primary*>(q)->s;s.bullet_player=id;return s.bullets->fire(*r,id);};
                h.flash.context=this;h.flash.preflight=[](void*,uint32_t,const uint32_t*){return 1;};
                h.flash.create=[](void*q,const UCShotLightFlashRequest*r){if(static_cast<BarrierOwner*>(q)->lightflash(*r))throw std::runtime_error("Player shot light creation failed");};
                UCWeaponBulletEmitResult result{};return uc_weapon_bullet_emit(item,&v,random,&h,&result);
            }
            void bind_bullets(){
                UCBulletScenery h{};h.context=this;h.delta=world->script_services().delta;
                h.history=&ai_maintenance.history();h.local_player_id=&bullet_player;
                h.preflight=[](void*q,const UCEntityFreshView*v,const UCBulletState*b){auto&s=*static_cast<BarrierOwner*>(q);s.query_bullet=b;
                    return int(v&&v->id&&uc_entity_event_lookup(&s.world->registry(),*v->id)==v->target.identity&&s.impacts&&s.handoff&&!s.handoff->session_mode);};
                h.query=[](void*q,const float*from,const float*to,uint32_t type,uint32_t group,uint32_t mask,float impulse,const UCCollisionResult**out){auto&s=*static_cast<BarrierOwner*>(q);
                    s.refresh_collision(true);s.bullet_target=-1;s.bullet_actor=nullptr;
                    int r=s.character_world.query(from,to,type,group,mask,impulse,s.bullet_collision,s.bullet_target,&s.bullet_actor);
                    if(r<0)throw std::runtime_error("Player bullet collision query failed");
                    if(r&&s.bullet_actor&&uc_bullet_actor_ignored(s.query_bullet,s.bullet_target,s.bullet_player))r=0;
                    *out=r?&s.bullet_collision:nullptr;return r;};
                h.contact=[](void*q,const UCEntityFreshView*v,UCBulletState*b,UCBulletImpact*p){return static_cast<BarrierOwner*>(q)->bullet_contact(*v,*b,*p);};
                h.impact=[](void*q,const UCEntityFreshView*v,UCBulletState*b,const UCBulletImpact*impact){auto&s=*static_cast<BarrierOwner*>(q);
                    NativeBulletImpactServices effects{};effects.owner=&s;effects.blood_enabled=&s.blood_enabled;effects.session_mode=&s.bullet_session_mode;
                    effects.camera=[](void*q,float*out,uint8_t*present){std::memcpy(out,static_cast<BarrierOwner*>(q)->published_camera,64);*present=1;return 0;};
                    effects.sound=[](void*q,const char*bank,const char*event,uint32_t material,const float*point){return static_cast<BarrierOwner*>(q)->world_sound(bank,event,material,point);};
                    int result=s.impacts->bullet_impact(s.character_world,*v,*b,*impact,*s.random,*s.vector_random,effects);if(result)throw std::runtime_error("Bullet impact failed material="+std::to_string(impact->material)+" code="+std::to_string(result));return result;};
                h.segment=[](void*q,const UCBulletSegment*segment){auto&s=*static_cast<BarrierOwner*>(q);
                    for(const auto&observer:s.segment_observers)if(!observer.second||observer.second(observer.first,segment))throw std::runtime_error("NPC bullet segment observer failed");return 0;};
                h.flying_segment=[](void*q,const UCBulletFlyingSegment*segment){auto&s=*static_cast<BarrierOwner*>(q);const int32_t*ids=nullptr;uint32_t count=0;
                    if(uc_entity_names_view(s.world->names(),"RIOT_VAN",&ids,&count))return 1;
                    for(uint32_t i=0;i<count;i++){uint32_t sender=uint32_t(segment->id);uint8_t handled=0;
                        if(uc_entity_event_send(&s.world->registry(),ids[i],167,segment,sizeof(*segment),&sender,&handled))return 1;}return 0;};
                UCBulletTracerServices t{};t.owner=this;
                t.multiplayer=[](void*q,unsigned*out){auto&s=*static_cast<BarrierOwner*>(q);if(!s.handoff||s.handoff->session_mode)return 1;*out=0;return 0;};
                t.player_actor=[](void*q,uint32_t id,unsigned*exists,unsigned*is_player){auto&s=*static_cast<BarrierOwner*>(q);auto*p=uc_entity_event_lookup(&s.world->registry(),int32_t(id));
                    *exists=p!=nullptr;*is_player=p&&p==&s.player->entity.identity;return 0;};
                t.append=[](void*q,const float*from,const float*to,const float*color,float width){static_cast<BarrierOwner*>(q)->bullet_tracers.append(from,to,color,width);return 0;};
                bullets=std::make_unique<NativeMissionBullets>(*world,bullet_progression,h,t);
            }
            int lightflash(const UCShotLightFlashRequest&r){
                if(!lighting)return 1;NativeMissionLightFlashServices h{};h.owner=this;
                h.create=[](void*q,const float*p)->UCPointLightState*{auto&s=*static_cast<BarrierOwner*>(q);auto*light=s.lighting->create_dynamic(p,400);return light?&light->state:nullptr;};
                h.release=[](void*q,UCPointLightState*p){static_assert(offsetof(UCPointLightInstance,state)==0);auto&s=*static_cast<BarrierOwner*>(q);
                    return s.lighting->destroy_dynamic(reinterpret_cast<UCPointLightInstance*>(p))?0:1;};
                gun_flashes.push_back(std::make_unique<NativeMissionLightFlash>());return gun_flashes.back()->construct(*world,r,h);
            }
            int blast_obstruction(const float*from,const float*to,const void*target,int32_t group){
                int32_t target_id=-1;
                if(target==&player->entity.identity)target_id=player->entity.id;
                else for(const auto&c:characters)if(target==&c->entity.identity){target_id=c->entity.id;break;}
                if(target_id<0)return -1;
                UCExplosionObstructionServices query{};query.owner=this;
                query.query=[](void*q,const float*a,const float*b,uint32_t type,uint32_t region,uint32_t mask,float impulse,int32_t*id,uint8_t*actor){
                    auto&s=*static_cast<BarrierOwner*>(q);s.refresh_collision();UCCollisionResult result{};const void*owner=nullptr;
                    int hit=s.character_world.query(a,b,type,region,mask,impulse,result,*id,&owner);if(hit>0)*actor=owner!=nullptr;return hit;};
                query.multiplayer=[](void*q){auto&s=*static_cast<BarrierOwner*>(q);return s.handoff?int(s.handoff->session_mode!=0):-1;};
                return uc_explosion_obstructed(from,to,target_id,group,&query);
            }
            int bind_player_damage(){
                if(!handoff||!camera_feedback||!scene||!player_camera)return 1;uc_player_pain_init(&pain);
                NativePlayerImpactServices impact{};impact.owner=this;
                impact.statistic=[](void*q,unsigned offset){auto&s=*static_cast<BarrierOwner*>(q);if(!s.handoff||(offset!=0x28&&offset!=0x38))return 1;
                    ++s.handoff->statistics[offset/4];return 0;};
                impact.shield_sound=[](void*q,const char*event){auto&s=*static_cast<BarrierOwner*>(q);return s.play_sound(s.player->entity.id,"Banks\\Impacts.amp",event);};
                impact.shield_spark=[](void*q,const float*point,const float*rotation){auto&s=*static_cast<BarrierOwner*>(q);if(!s.npc_thrown_particles)return 1;
                    NativePlayerShieldSparkServices h{};h.owner=q;
                    h.feedback=[](void*q,const float*p,float radius,float amplitude,float duration,unsigned kind){auto&s=*static_cast<BarrierOwner*>(q);
                        return s.camera_feedback->spatial(p,radius,amplitude,duration,kind);};
                    auto spark=std::make_unique<NativePlayerShieldSpark>();auto*raw=spark.get();s.shield_sparks.push_back(std::move(spark));
                    return raw->construct(*s.world,s.player->entity.id,point,rotation,*s.npc_thrown_particles,h);};
                if(player->set_impact_services(impact))return 1;
                NativePlayerBlastServices blast{};blast.owner=this;blast.shared_attenuation=&concussion_attenuation;
                blast.enabled=[](void*q){auto&s=*static_cast<BarrierOwner*>(q);
                    if(!s.handoff||!s.player_camera)return -1;if(!s.handoff->checkpoints.ready(*s.world,&s.handoff->session_mode))return 0;
                    for(uint32_t i=0;i<s.player_camera->stack.count;i++)if(s.player_camera->stack.layers[i]->entity==&s.player->entity.identity)return 1;return 0;};
                blast.obstruction=[](void*q,const float*from,const float*to,const void*target,int32_t group){
                    return static_cast<BarrierOwner*>(q)->blast_obstruction(from,to,target,group);};
                blast.secondary_pain=[](void*q){return uc_player_pain_trigger(&static_cast<BarrierOwner*>(q)->pain,1);};
                blast.sound=[](void*q,const char*event){auto&s=*static_cast<BarrierOwner*>(q);return s.play_sound(s.player->entity.id,"Banks\\Player.amp",event);};
                blast.parameter=[](void*q,const char*event,const char*name,float value){auto&s=*static_cast<BarrierOwner*>(q);
                    for(auto&v:s.character_sounds)if(v.id==s.player->entity.id&&v.bank=="Banks\\Player.amp"){v.source->parameter(event,name,value);return 0;}return 1;};
                blast.bus_filter=[](void*q,const char*name,float cutoff,float attack,float hold,float release){auto&s=*static_cast<BarrierOwner*>(q);
                    if(!s.audio)return 1;s.audio->group_filter(name,cutoff,attack,hold,release,s.world->time());return 0;};
                blast.group_pitch=[](void*q,const char*name,float ratio){auto&s=*static_cast<BarrierOwner*>(q);if(!s.audio)return 1;s.audio->group_pitch(name,ratio);return 0;};
                if(player->set_blast_services(blast))return 1;
                UCPlayerDamageServices h{};h.owner=this;h.difficulty=&difficulty;
                h.multiplayer=[](void*q){auto&s=*static_cast<BarrierOwner*>(q);return s.handoff&&!s.handoff->session_mode?0:-1;};
                h.sound=[](void*q,unsigned channel){auto&s=*static_cast<BarrierOwner*>(q);
                    if(channel==0)return 0; //D3910 actor2D8 is NULL;216BD0 returns immediately.
                    return channel==1?s.play_sound(s.player->entity.id,"Banks\\Player.amp","GetShot"):1;};
                h.pain=[](void*q){return uc_player_pain_trigger(&static_cast<BarrierOwner*>(q)->pain,0);};
                h.feedback=[](void*q,float amplitude,float duration,uint32_t kind){auto&s=*static_cast<BarrierOwner*>(q);
                    //Actual D47E7 initializes player368 to0 for this single view.
                    return uc_feedback_enqueue(uc_feedback_manager_queue(s.camera_feedback->manager()),0,amplitude,duration,kind,s.world->time(),s.handoff->session_mode)==UC_FEEDBACK_OK?0:1;};
                h.enemy_type=[](void*q,int32_t id,int32_t*out){auto&s=*static_cast<BarrierOwner*>(q);*out=-1;
                    if(!uc_entity_event_lookup(&s.world->registry(),id))return 0;auto*c=s.character(id);if(!c)return 1;
                    return c->property("dda enemy type",out)<0?1:0;};
                h.adaptive_hit=[](void*q,int32_t type){auto&s=*static_cast<BarrierOwner*>(q);
                    return type==2?uc_player_damage_blazer_hit(s.difficulty.mode,0,&s.throw_channel):1;};
                h.delta=[](void*q)->double{return *static_cast<BarrierOwner*>(q)->world->script_services().delta;};
                return player->set_damage_services(h,&handoff->world_flags,0,
                    [](void*q){auto&s=*static_cast<BarrierOwner*>(q);auto&stack=s.player_camera->stack;
                        for(uint32_t i=0;i<stack.count;i++)if(stack.layers[i]->entity==&s.player->entity.identity)return 1;return 0;},
                    [](void*q){auto&s=*static_cast<BarrierOwner*>(q);return uc_player_pain_step(&s.pain,*s.world->script_services().delta,&s,
                        [](void*q,float start,float end,float amplitude){return static_cast<BarrierOwner*>(q)->scene->request_blur(start,end,amplitude);});});
            }
            NativeMissionExplosionServices explosion_services(){
                NativeMissionExplosionServices h{};h.owner=this;
                h.field_step=[](void*q,UCExplosionField&f,float dt){return native_prop_explosion_step(f,dt,static_cast<BarrierOwner*>(q)->props);};
                h.online=[](void*q){auto&s=*static_cast<BarrierOwner*>(q);return s.handoff&&!s.handoff->session_mode?0:-1;};
                h.broadcast=[](void*q,const UCExplosionBlast&b,int32_t id){auto&s=*static_cast<BarrierOwner*>(q);uint32_t sender=uint32_t(id);
                    return uc_entity_listener_broadcast(s.world->listeners(),&s.world->registry(),9,&b,sizeof b,&sender);};
                h.shake=[](void*q,const float*p,float radius,float amplitude,float duration,uint32_t kind){auto*s=static_cast<BarrierOwner*>(q);
                    return s->camera_feedback?s->camera_feedback->spatial(p,radius,amplitude,duration,kind):1;};
                h.area_effect=[](void*q,const float*b,uint32_t kind){auto&s=*static_cast<BarrierOwner*>(q);if(kind!=2)return 1;
                    //All38 authored fittings are immutable in this level owner.
                    //Check the superset, including unregistered/inactive slots:
                    //a miss proves59280 cannot reach any controller. A hit still
                    //requires its actual break coroutine; do not omit that work.
                    for(uint32_t i=0;i<s.world->source_count();i++){const auto*d=s.world->metadata(i).definition;
                        if(d&&d->class_name&&!std::strcmp(d->class_name,"LightFitting")){unsigned inside;
                            if(uc_light_fitting_area_contains(b,d->transform+12,&inside))return 1;
                            if(inside)throw std::runtime_error("Explosion reached light-fitting break coroutine");}}
                    return 0;};
                h.debris=[](void*q,NativeMissionExplosion&e,uint32_t n,float scale,float power,const char*texture){auto&s=*static_cast<BarrierOwner*>(q);
                    s.refresh_collision();return s.explosion_debris.explosion(e.state().parameters,e.entity.transform+12,n,scale,power,texture);};
                h.effect=[](void*q,NativeMissionExplosion&e,uint32_t helper){auto&s=*static_cast<BarrierOwner*>(q);s.refresh_collision();
                    if(helper==0xE9130)return s.impacts->explosion_scorch(s.character_world,e.entity.transform+12,*s.vector_random);
                    if(helper!=0xE98E0&&helper!=0xE9650)throw std::runtime_error("Explosion alternate particle owner unconnected");
                    struct Context{BarrierOwner*s;NativeMissionExplosion*e;} context{&s,&e};UCExplosionEffectServices effects{};effects.owner=&context;
                    effects.world=[](void*q,float*out){std::memcpy(out,static_cast<Context*>(q)->e->entity.transform,64);return 0;};
                    effects.particle=[](void*q,const char*asset,const float*frame){return static_cast<Context*>(q)->s->impacts->fixed_particle(asset,frame);};
                    effects.flash=[](void*q,const UCShotLightFlashRequest*r){return static_cast<Context*>(q)->s->lightflash(*r);};
                    effects.child=[](void*q,const UCExplosionVisualChild*r){auto&s=*static_cast<Context*>(q)->s;NativeExplosionVisualServices h{};h.owner=&s;
                        h.ring=[](void*q,NativeMissionEntity&e,const UCParticleAsset&a){auto*s=static_cast<BarrierOwner*>(q);return s->npc_thrown_particles?s->npc_thrown_particles->append_ring(e,a):1;};
                        s.explosion_children.push_back(std::make_unique<NativeMissionExplosionVisual>());return s.explosion_children.back()->construct(*s.world,*r,h);};
                    const auto&p=e.state().parameters;const uint32_t id=uint32_t(e.entity.id);
                    return helper==0xE98E0?uc_explosion_effect_particles(&id,&p.particle_type,&effects):uc_explosion_effect_flash(&id,&p.particle_type,&p.scale,&effects);};
                return h;
            }
            NativeNpcGunShotServices gun_shot_services(){
                NativeNpcGunShotServices h{};h.owner=this;h.player_id=player->entity.id;
                h.bullet=[](void*q,const UCWeaponBulletRequest&r,int32_t owner){auto&s=*static_cast<BarrierOwner*>(q);
                    if(!s.bullets||!s.player)return 1;s.bullet_player=uint32_t(s.player->entity.id);return s.bullets->fire(r,uint32_t(owner));};
                h.sound=[](void*q,NativeNpcGun&gun,const char*bank,const char*event,unsigned*found){auto&s=*static_cast<BarrierOwner*>(q);
                    if(s.play_sound(gun.entity.id,bank,event))return 1;*found=1;return 0;};
                h.lightflash=[](void*q,const UCShotLightFlashRequest&r){return static_cast<BarrierOwner*>(q)->lightflash(r);};
                return h;
            }
            struct ThrownContext {BarrierOwner*owner;NativeNpcThrownWeapon*weapon;};
            UCNpcThrownEffectsServices thrown_effects(ThrownContext&context){
                UCNpcThrownEffectsServices h{};h.owner=&context;
                h.asset=[](void*q,void*t,const char*name,const char**asset,unsigned*found){auto&c=*static_cast<ThrownContext*>(q);
                    return c.owner->npc_projectile_services.template_property(c.owner,static_cast<const UCEntityFreshView*>(t),name,asset,found);};
                h.particles=[](void*q,const char*asset,int32_t id,unsigned attached){auto&c=*static_cast<ThrownContext*>(q);
                    if(c.weapon->entity.id!=id||!c.owner->npc_thrown_particles||c.owner->npc_thrown_particles->append(asset,*c.weapon,attached!=0))
                        throw std::runtime_error(std::string("ThrownWeapon particle attachment: ")+(asset?asset:"missing template asset")+
                            " parent="+std::to_string(c.weapon->entity.parent)+" time="+std::to_string(c.owner->world->time())+
                            " templates="+std::to_string(c.owner->molotov_templates.size()));return 0;};
                h.time=[](void*q,float*out){*out=static_cast<ThrownContext*>(q)->owner->world->time();return 0;};
                h.camera_position=[](void*q,float*out){std::memcpy(out,static_cast<ThrownContext*>(q)->owner->published_camera+12,16);return 0;};
                h.light_destroy=[](void*q,void*p){auto*s=static_cast<ThrownContext*>(q)->owner;return s->lighting&&s->lighting->destroy_dynamic(static_cast<UCPointLightInstance*>(p))?0:1;};
                h.light_create=[](void*q,const float*position,void**out){auto*s=static_cast<ThrownContext*>(q)->owner;if(!s->lighting)return 1;
                    auto*p=s->lighting->create_dynamic(position,400);if(!p)return 1;p->state.enabled=1;p->state.shadows=0;
                    p->state.color[0]=1;p->state.color[1]=128.f/255;p->state.color[2]=22.f/255;p->state.color[3]=1;
                    p->state.multiplier=.66f;p->state.attenuation=250;p->state.dirty=1;p->state.specular=0;*out=p;return 0;};
                h.sound=[](void*q,const char*name,unsigned stop){auto&c=*static_cast<ThrownContext*>(q);
                    return c.owner->play_sound(c.weapon->entity.id,native_npc_throw_bank(c.weapon->state.weapon1EC),name,stop!=0);};
                h.random=[](void*q,float low,float high,double*out){auto*s=static_cast<ThrownContext*>(q)->owner;int32_t value;
                    if(uc_game_random_next(s->random,&value))return 1;*out=(double(value)*(double(high)-low))*0x1p-31+low;return 0;};
                h.flags=[](void*q,uint32_t flags){return static_cast<ThrownContext*>(q)->weapon->set_flags(flags);};
                h.molotov_event=[](void*q,void*t,const UCNpcMolotovExplosion*payload){auto&c=*static_cast<ThrownContext*>(q);auto*v=static_cast<UCEntityFreshView*>(t);
                    if(!v||!v->id||uc_entity_event_lookup(&c.owner->world->registry(),*v->id)!=v->target.identity)return 1;
                    uint8_t handled=0;uint32_t sender=uint32_t(c.weapon->entity.id);
                    return int(uc_entity_event_send(&c.owner->world->registry(),*v->id,210,payload,sizeof(*payload),&sender,&handled)||!handled);};
                h.physics_close=[](void*q){auto&c=*static_cast<ThrownContext*>(q);return uc_npc_replication_offline_admit(&c.weapon->state,nullptr,nullptr,nullptr);};
                return h;
            }
            int thrown_late(NativeNpcThrownWeapon&p){
                ThrownContext context{this,&p};UCNpcFlightServices h{};h.owner=&context;
                h.delta=world->script_services().delta;h.time=&world->time();
                h.displacement=[](void*q,const UCEntityFreshView*v,UCBulletState*b,float*out){auto&c=*static_cast<ThrownContext*>(q);auto&s=*c.owner;
                    if(v!=&c.weapon->entity.view||b!=&c.weapon->base)return 1;s.refresh_collision();
                    struct Collision {ThrownContext*context;UCCollisionResult hit{};} current{&c};
                    uint32_t player=uint32_t(s.player->entity.id);UCBulletScenery h{};h.context=&current;h.delta=s.world->script_services().delta;
                    h.history=&s.ai_maintenance.history();h.local_player_id=&player;
                    h.preflight=[](void*q,const UCEntityFreshView*v,const UCBulletState*b){auto&c=*static_cast<Collision*>(q)->context;
                        return int(v==&c.weapon->entity.view&&b==&c.weapon->base&&c.weapon->ready());};
                    h.query=[](void*q,const float*from,const float*to,uint32_t type,uint32_t group,uint32_t mask,float impulse,const UCCollisionResult**out){auto&c=*static_cast<Collision*>(q);
                        int32_t id=0;const void*actor=nullptr;int r=c.context->owner->character_world.query(from,to,type,group,mask,impulse,c.hit,id,&actor);
                        if(r<0)throw std::runtime_error("ThrownWeapon collision query");
                        if(actor)throw std::runtime_error("ThrownWeapon actor impact unconnected");*out=r?&c.hit:nullptr;return r;};
                    h.impact=[](void*q,const UCEntityFreshView*,UCBulletState*,const UCBulletImpact*impact){auto&c=*static_cast<Collision*>(q)->context;
                        auto h=c.owner->thrown_effects(c);return uc_npc_thrown_molotov_impact(&c.weapon->state,c.weapon->entity.id,0,-1,impact->point,impact->normal,&h);};
                    h.segment=[](void*q,const UCBulletSegment*segment){auto&s=*static_cast<Collision*>(q)->context->owner;
                        for(const auto&observer:s.segment_observers)if(!observer.second||observer.second(observer.first,segment))throw std::runtime_error("NPC bullet segment observer failed");return 0;};
                    h.flying_segment=[](void*q,const UCBulletFlyingSegment*segment){auto&s=*static_cast<Collision*>(q)->context->owner;
                        const int32_t*ids=nullptr;uint32_t count=0;if(uc_entity_names_view(s.world->names(),"RIOT_VAN",&ids,&count))return 1;
                        for(uint32_t i=0;i<count;i++){uint32_t sender=uint32_t(segment->id);uint8_t handled=0;
                            if(uc_entity_event_send(&s.world->registry(),ids[i],167,segment,sizeof(*segment),&sender,&handled))return 1;}return 0;};
                    return uc_bullet_scenery(&h,v,b,out);};
                h.tint=[](void*q,float*tint,unsigned write){auto*p=static_cast<ThrownContext*>(q)->weapon;
                    return write?p->set_model_tint(tint):(p->model_tint(tint)?0:1);};
                h.camera=[](void*q,float*out){std::memcpy(out,static_cast<ThrownContext*>(q)->owner->published_camera,64);return 0;};
                h.light=[](void*q,void*light,const float*matrix,float attenuation){auto&c=*static_cast<ThrownContext*>(q);if(c.weapon->state.effect248!=light||!light)return 1;
                    auto*p=static_cast<UCPointLightInstance*>(light);p->state.attenuation=attenuation;p->state.dirty=1;
                    std::memcpy(p->local_center,matrix+12,16);const float origin[4]={0,0,0,1};std::memcpy(p->world+12,origin,16);
                    return uc_point_light_geometry_update(&p->state,p->world,p->local_center,&p->geometry)?0:1;};
                h.multiplayer=[](void*q,unsigned*out){auto&c=*static_cast<ThrownContext*>(q);if(uc_npc_replication_offline_admit(&c.weapon->state,nullptr,nullptr,nullptr))return 1;*out=0;return 0;};
                h.intercept=[](void*q,int32_t id,float radius,unsigned only,float*direction,unsigned*out){auto&s=*static_cast<ThrownContext*>(q)->owner;
                    auto*t=uc_entity_event_lookup(&s.world->registry(),id);auto*v=t?s.world->context().resolve(s.world->context().owner,t):nullptr;
                    uint32_t player=uint32_t(s.player->entity.id);int hit=uc_npc_flight_intercept(&s.ai_maintenance.history(),id,v?v->transform+12:nullptr,&player,radius,only,direction);
                    if(hit<0)return 1;*out=unsigned(hit);return 0;};
                return uc_npc_flight_late(&p.state,&p.base,&p.entity.view,&h);
            }
            NativeCharacterAIMaintenance::Services maintenance_services(){return {this,
                [](void*p,int32_t id,uint8_t*found,uint32_t*status,uint8_t*controller){auto*c=static_cast<BarrierOwner*>(p)->character(id);
                    *found=c!=nullptr;*status=c?c->status():0;*controller=c&&c->has_controller();return 0;},
                [](void*p,int32_t id,uint8_t*out){auto*c=static_cast<BarrierOwner*>(p)->character(id);if(!c)return 1;*out=c->tasks_empty();return 0;},
                [](void*p,int32_t id,uint8_t*out){auto&s=*static_cast<BarrierOwner*>(p);auto*c=s.character(id);if(!c)return 1;
                    UCCharacterPlayerView view{};size_t count=0;
                    if(uc_entity_event_lookup(&s.world->registry(),s.player->entity.id)==&s.player->entity.identity){
                        std::memcpy(view.position,s.player->entity.view.transform+12,12);view.head_direction[1]=1;
                        if(s.player->target_vector_property("head direction",view.head_direction)<0)return 1;count=1;}
                    return uc_character_unobserved(c->entity.transform+12,&view,count,out);},
                [](void*p,int32_t id){auto*c=static_cast<BarrierOwner*>(p)->character(id);if(!c)return 1;c->request_retirement();return 0;}};}
            int sight_actor(int32_t id,UCCharacterSightActor &out){
                auto*t=uc_entity_event_lookup(&world->registry(),id);if(!t)return 0;
                auto*v=world->context().resolve(world->context().owner,t);if(!v||!v->transform||!v->target.parent_id)return -1;
                out={};out.identity=t;out.parent=*v->target.parent_id;out.group=v->actor?v->actor->group94:0;
                std::memcpy(out.position,v->transform+12,12);return 1;
            }
            int target_position(int32_t id,bool property,float*out){
                UCCharacterSightActor a{};int found=sight_actor(id,a);if(found!=1)return found;
                if(property){if(a.identity==&player->entity.identity){std::memcpy(out,firstperson->camera_input.player_frame+12,12);return 1;}
                    if(auto*c=character(id))return c->target_position(out)?1:-1;return -1;}
                std::memcpy(out,a.position,12);return 1;
            }
            NativeCharacterTargetServices target_services(){
                NativeCharacterTargetServices h{};h.owner=this;
                h.resolve=[](void*q,int32_t id,float*out){auto&s=*static_cast<BarrierOwner*>(q);UCCharacterSightActor a{};
                    int r=s.sight_actor(id,a);if(r==1)std::memcpy(out,a.position,12);return r;};
                h.property=[](void*q,int32_t id,const char*name,int32_t*out){auto&s=*static_cast<BarrierOwner*>(q);
                    auto*t=uc_entity_event_lookup(&s.world->registry(),id);if(!t)return 0;
                    for(const auto&c:s.characters)if(t==&c->entity.identity)return c->property(name,out);
                    if(t==&s.player->entity.identity)return s.player->target_property(name,*out);
                    return -1;};
                h.player=[](void*q){return static_cast<BarrierOwner*>(q)->player->entity.id;};return h;
            }
            int dialogue_zone(int32_t actor,int32_t*out){
                if(!out)return 1;UCCharacterZoneServices h{};h.owner=this;
                h.named=[](void*q,const char*n,const int32_t**ids,uint32_t*count){return uc_entity_names_view(static_cast<BarrierOwner*>(q)->world->names(),n,ids,count);};
                h.query=[](void*q,int32_t id,uint32_t event,int32_t actor,uint8_t*handled,uint8_t*reply){auto&s=*static_cast<BarrierOwner*>(q);
                    auto*t=uc_entity_event_lookup(&s.world->registry(),id);if(!t){*handled=0;return 0;}
                    for(auto&z:s.zones)if(t==&z->entity.identity)return z->query(event,actor,handled,reply);
                    return 1;};
                const int mode=uc_character_dialogue_zone(actor,&h);if(mode<0)return 1;*out=mode;return 0;
            }
            int visible(NativeMissionCharacter&observer,int32_t id,float cosine=-1,bool head=false,const float*offset=nullptr,bool include_characters=true){
                UCCharacterSightActor self{};if(sight_actor(observer.entity.id,self)!=1)return -1;
                UCCharacterSightServices h{};h.owner=this;
                h.resolve=[](void*q,int32_t id,UCCharacterSightActor*out){return static_cast<BarrierOwner*>(q)->sight_actor(id,*out);};
                h.point=[](void*q,const UCCharacterSightActor*a,const char*name,float*out){auto&s=*static_cast<BarrierOwner*>(q);
                    const bool head=!std::strcmp(name,"head pos");if(!head&&std::strcmp(name,"target position"))return -1;
                    if(a->identity==&s.player->entity.identity){
                        //C0EC0 -> C01C0(selector0): original+6B0 after D0642..D09BD.
                        std::memcpy(out,s.firstperson->camera_input.player_frame+12,12);return 1;}
                    for(const auto&c:s.characters)if(a->identity==&c->entity.identity)return (head?c->head_position(out):c->target_position(out))?1:-1;
                    return -1;};
                h.forward=[](void*q,const UCCharacterSightActor*a,float*out){auto&s=*static_cast<BarrierOwner*>(q);
                    auto*v=s.world->context().resolve(s.world->context().owner,(UCEntityEventTarget*)a->identity);
                    if(!v||!v->transform)return 1;std::memcpy(out,v->transform+8,12);return 0;};
                h.ray=[](void*q,const float*from,const float*to,uint32_t type,uint16_t group,uint32_t mask,UCCharacterSightActor*out){
                    auto&s=*static_cast<BarrierOwner*>(q);s.refresh_collision();UCCollisionResult hit{};int32_t id=0;const void*actor=nullptr;
                    int r=s.character_world.query(from,to,type,group,mask,0,hit,id,&actor);if(r<=0)return r;
                    *out={};if(actor&&s.sight_actor(id,*out)!=1)return -1;return 1;};
                const float zero[3]={};uint8_t result=0;
                return uc_character_sight(&self,id,cosine,uint8_t(head),offset?offset:zero,uint8_t(include_characters),&h,&result)?-1:result;
            }
            void refresh_collision(bool actor_hits=false){
                character_world.impulse_owner=this;
                character_world.apply_impulse=[](void*q,int32_t id,const float*momentum){auto&s=*static_cast<BarrierOwner*>(q);
                    for(const auto&p:s.props)if(p->entity.id==id)return p->bullet_impulse(momentum);
                    return 1;};
                auto&boxes=character_world.boxes;boxes.clear();
                character_world.capsules.clear();
                // Bullet queries own the newly connected actor recipients. The
                // existing thrown/movement adapters still admit static geometry.
                if(actor_hits)for(const auto&c:characters)c->append_collision(character_world);
                for(const auto&b:bodies)if(const auto*g=b->geometry())boxes.push_back({g,{7,0x38,0x00800000u|b->entity.actor.group94,1,0,0,0},b->entity.id,{g,1,1,0,0},&b->entity});
                auto append=[&](const auto&owners,uint32_t filter,bool combined){for(const auto&p:owners){
                    const auto&geometry=p->boxes();const auto&shapes=p->collision_shapes();
                    if(geometry.size()!=shapes.size())throw std::runtime_error("NPC collision metadata extent");
                    uint32_t root_material=0;
                    for(const auto&shape:shapes){uint32_t material;if(shape.family!=2||shape.metadata_size!=6)throw std::runtime_error("NPC collision metadata family");
                        std::memcpy(&material,shape.metadata+2,4);root_material|=material;}
                    for(size_t i=0;i<geometry.size();i++){const auto&shape=shapes[i];uint32_t material;
                        if(shape.family!=2||shape.metadata_size!=6)throw std::runtime_error("NPC collision metadata family");
                        std::memcpy(&material,shape.metadata+2,4);
                        //2A3000 builds one compound root, ORs its shape userdata,
                        //and2A8D00 publishes that root's material for every leaf.
                        const uint32_t type=combined&&geometry.size()>1?12:shape.root_kind;
                        if(combined)material=root_material;
                        boxes.push_back({&geometry[i],{type,material,filter|p->entity.actor.group94,1,0,0,0},p->entity.id,
                            {&geometry[i],1,combined?0u:1u,0,0},&p->entity});
                    }
                }};
                append(static_props,0x03800000,false);append(light_fittings,0x03800000,false);append(props,0x26800000,true);if(particles)particles->append_collision(character_world);
            }
            UCFireDamageServices fire_services(){auto&s=*this;
                    if(!s.handoff)throw std::runtime_error("Fire requires mission handoff");UCFireDamageServices f{};f.owner=&s;f.session_mode=&s.handoff->session_mode;
                    f.player_count=[](void*q)->int32_t{return static_cast<BarrierOwner*>(q)->player?1:-1;};
                    f.player=[](void*q,uint32_t i,UCFirePlayer*out){auto&s=*static_cast<BarrierOwner*>(q);if(i)return -1;
                        if(!uc_entity_event_lookup(&s.world->registry(),s.player->entity.id))return 0;
                        *out={s.player->entity.view.transform+12,s.player,s.player->entity.actor.group94};return 1;};
                    f.multiplayer=[](void*q){auto&s=*static_cast<BarrierOwner*>(q);return s.handoff&&!s.handoff->session_mode?0:-1;};
                    f.blocked=[](void*,const float*,const float*,void*,uint16_t){return -1;}; //Only online1A5C60 calls this branch.
                    f.damage=[](void*q,void*actor,float amount,const UCPlayerFireDamage*p){auto&s=*static_cast<BarrierOwner*>(q);
                        return actor==s.player?s.player->damage(amount,*p):1;};
                return f;
            }
            static int template_receive(void*q,const void*payload,uint32_t*sender){
                if(!payload||!sender)return 1;auto&t=*static_cast<MolotovTemplate*>(q);auto&s=*t.owner;
                UCMolotovReceiverServices h{};h.owner=&s;
                h.sender=[](void*q,int32_t id,float*out,unsigned*found){auto&s=*static_cast<BarrierOwner*>(q);
                    auto*t=uc_entity_event_lookup(&s.world->registry(),id);*found=t!=nullptr;if(!t)return 0;
                    auto*v=s.world->context().resolve(s.world->context().owner,t);if(!v||!v->transform)return 1;std::memcpy(out,v->transform,64);return 0;};
                h.particles=[](void*q,const char*asset,const float*frame){return static_cast<BarrierOwner*>(q)->impacts->fixed_particle(asset,frame);};
                h.sound=[](void*q,int32_t parent,const float*,const char*start,const char*stop,float seconds){auto&s=*static_cast<BarrierOwner*>(q);
                    s.sounds.push_back(std::make_unique<NativeMissionSound>());return s.sounds.back()->construct(*s.world,parent,*s.audio,s.archive.get(),s.texture_lookup,s.audio_stream,start,stop,seconds);};
                h.damage_sphere=[](void*q,int32_t parent,const float*,float radius,int32_t owner,float seconds,int32_t session,int32_t unique){auto&s=*static_cast<BarrierOwner*>(q);
                    auto f=s.fire_services();
                    s.fire_spheres.push_back(std::make_unique<NativeMissionFireDamage>());
                    return s.fire_spheres.back()->construct(*s.world,parent,s.navigation_boxes,f,radius,seconds,owner,session,unique);};
                h.scorch=[](void*q,const char*shader,const float*p,const float*n,float width,float height,uint32_t mask){auto&s=*static_cast<BarrierOwner*>(q);
                    if(std::strcmp(shader,"FX\\FX_Molotov_Scorch\\FX_Molotov_Scorch.shader"))return 1;const float size[]={width,height};s.refresh_collision();
                    return s.impacts->scorch(s.character_world,p,n,size,mask,*s.vector_random);};
                h.broadcast=[](void*q,uint32_t event,const UCMolotovBlast*b,int32_t id){auto&s=*static_cast<BarrierOwner*>(q);uint32_t sender=uint32_t(id);
                    return uc_entity_listener_broadcast(s.world->listeners(),&s.world->registry(),event,b,sizeof(*b),&sender);};
                return uc_molotov_receive(t.view()->transform,*t.view()->id,int32_t(*sender),static_cast<const UCNpcMolotovExplosion*>(payload),&h);
            }
            int ensure_template(NativeMissionWorld&mission,int32_t parent){
                const int32_t*ids=nullptr;uint32_t count=0;
                if(uc_entity_names_view(mission.names(),"TEMPLATE_MOLOTOV",&ids,&count))return 1;
                if(count)return 0;
                molotov_templates.push_back(std::make_unique<MolotovTemplate>(this));
                auto*p=molotov_templates.back().get();return p->construct_dynamic(mission,parent,p,template_receive);
            }
            int create_melee(NativeMissionWorld&w,NativeMissionCharacter&character,int32_t*out,uint32_t type,const float*pillbox_world=nullptr){
                if(!out)return 1;medic_weapons.push_back(std::make_unique<NativeMedicWeapon>());auto*p=medic_weapons.back().get();
                p->player_provider(this,[](void*q,int32_t*out){auto&s=*static_cast<BarrierOwner*>(q);if(!s.player||!s.player->player())return 1;*out=s.player->entity.id;return 0;});
                int result=p->construct(*renderer,w,character,archive.get(),archive_hash,catalog,texture_lookup,type,pillbox_world);
                if(result)throw std::runtime_error("Melee weapon factory type="+std::to_string(type)+" code="+std::to_string(result)+" actor="+std::to_string(character.entity.source));
                if(!result)*out=p->entity.id;return result;
            }
        } barrier_owner;
        // The world closes registrations before its borrowed bodies/flags die.
        NativeMissionPlayer mission_player;
        NativePlayerCamera gameplay_camera;
        std::unique_ptr<NativeMissionHandoff> handoff;
        const int32_t physical_controller_port=-1; //Current native input provider is keyboard/mouse.
        std::unique_ptr<NativeCameraFeedback> camera_feedback;
        NativeMissionWorld mission_world; //Detach shake receivers before their feedback manager closes.
        std::vector<const UCSolidBlockGeometry*> active_blocks;
        size_t active_barrier_count=0;
        if(integration){
            weapon_effects=std::make_unique<NativeIntegrationWeaponEffects>(renderer,level.parent_path(),material_path.parent_path(),material_path.parent_path());
            barrier_owner.npc_effects=weapon_effects->particle_service();
            barrier_owner.ai=std::make_unique<NativeCharacterAIPhase>(entities.value.remap,mission_flags.data(),mission_flags.size());
            barrier_owner.cover_points=std::make_unique<NativeCharacterCoverPoints>(level_logic.value);
            impacts=std::make_unique<NativeIntegrationImpacts>(renderer,level.parent_path(),material_path.parent_path(),material_path.parent_path());
            level_particles=std::make_unique<NativeLevelParticles>(renderer,level.parent_path(),material_path.parent_path(),entities.value,true);
            if(uc_vector_random_init(&vector_random))throw std::runtime_error("Native vector random initialization");
            barrier_owner.renderer=&renderer;barrier_owner.collision=&collision.value;
            barrier_owner.character_world.scenery=&collision.value;barrier_owner.world=&mission_world;
            barrier_owner.character_world.scenery_owner={&collision.value,1,1,0,0};
            barrier_owner.vector_random=&vector_random;
            barrier_owner.mission_name=level.stem().string(); //209540's retained level name, read by54BF0.
            barrier_owner.npc_throw.owner=&barrier_owner;
            barrier_owner.npc_throw.projectile=[](void*q,int32_t id,NativeNpcThrownView*out){auto&s=*static_cast<BarrierOwner*>(q);
                if(auto*p=s.projectile(id)){*out=p->view();return 1;}return uc_entity_event_lookup(&s.world->registry(),id)?-1:0;};
            barrier_owner.npc_throw.position=[](void*q,int32_t id,bool property,float*out){return static_cast<BarrierOwner*>(q)->target_position(id,property,out);};
            barrier_owner.npc_throw.incapacitated=[](void*q,int32_t id){auto&s=*static_cast<BarrierOwner*>(q);if(!uc_entity_event_lookup(&s.world->registry(),id))return 0;auto*c=s.character(id);if(!c)return -1;
                int32_t value;return c->property("incapacitated",&value)==1?value:-1;};
            barrier_owner.npc_throw.can_throw=[](void*q,int32_t actor,int32_t target,float speed,int32_t selection){auto&s=*static_cast<BarrierOwner*>(q);auto*c=s.character(actor);if(!c)return -1;
                float local[16],point[3];if(!c->posed_bone("gundummy",local)||s.target_position(target,true,point)!=1)return -1;
                return uc_npc_throw_can(selection,speed,local,c->entity.transform,point,c->entity.transform+8);};
            barrier_owner.npc_projectile_services.owner=&barrier_owner;
            barrier_owner.npc_projectile_services.physics_construct=[](void*,NativeNpcThrownWeapon&p){return uc_npc_replication_offline_admit(&p.state,nullptr,nullptr,nullptr);};
            barrier_owner.npc_projectile_services.physics_close=[](void*,NativeNpcThrownWeapon&p){return uc_npc_replication_offline_admit(&p.state,nullptr,nullptr,nullptr);};
            barrier_owner.npc_projectile_services.release_light=[](void*q,void*p){auto&s=*static_cast<BarrierOwner*>(q);
                return s.lighting&&s.lighting->destroy_dynamic(static_cast<UCPointLightInstance*>(p))?0:1;};
            barrier_owner.npc_projectile_services.template_property=[](void*q,const UCEntityFreshView*view,const char*name,const char**out,unsigned*found){auto&s=*static_cast<BarrierOwner*>(q);
                for(const auto&t:s.molotov_templates)if(t->view()==view){UCMolotovTemplateProperty value{};*found=t->property(name,value);if(*found){if(value.type!=4)return 1;*out=value.string;}return 0;}
                for(const auto&t:s.cleaver_templates)if(t->view()==view){*out=t->property(name);*found=*out!=nullptr;return 0;}return 1;};
            barrier_owner.npc_projectile_services.ignition=[](void*q,NativeNpcThrownWeapon&p){auto&s=*static_cast<BarrierOwner*>(q);BarrierOwner::ThrownContext c{&s,&p};auto h=s.thrown_effects(c);
                return uc_npc_thrown_ignition_effects(&p.state,p.entity.id,p.entity.transform,&h);};
            barrier_owner.npc_projectile_services.flight=[](void*q,NativeNpcThrownWeapon&p){auto&s=*static_cast<BarrierOwner*>(q);BarrierOwner::ThrownContext c{&s,&p};auto h=s.thrown_effects(c);
                return uc_npc_thrown_release_effects(&p.state,p.entity.id,&h);};
            barrier_owner.npc_projectile_services.explosion=[](void*q,NativeNpcThrownWeapon&p){auto&s=*static_cast<BarrierOwner*>(q);BarrierOwner::ThrownContext c{&s,&p};auto h=s.thrown_effects(c);
                return uc_npc_thrown_explosion_effects(&p.state,&p.base,&h);};
            barrier_owner.npc_projectile_services.sound_active=[](void*q,NativeNpcThrownWeapon&p,unsigned*out){auto&s=*static_cast<BarrierOwner*>(q);
                const char*bank=native_npc_throw_bank(p.state.weapon1EC);if(!bank||!out)return 1;*out=0;
                for(const auto&sound:s.character_sounds)if(sound.id==p.entity.id&&sound.bank==bank){*out=sound.source->busy();break;}return 0;};
            barrier_owner.npc_projectile_services.flight_late=[](void*q,NativeNpcThrownWeapon&p){return static_cast<BarrierOwner*>(q)->thrown_late(p);};
            barrier_owner.npc_projectile_services.owner_fuse_expired=[](void*q,NativeNpcThrownWeapon&p){
                auto&s=*static_cast<BarrierOwner*>(q);int32_t id=int32_t(p.base.actor);
                if(!uc_entity_event_lookup(&s.world->registry(),id))return 0;
                auto*actor=s.character(id);return actor?actor->held_molotov_burn():1;};
            // PC fallen-bottle pickup retains the real item, model and entity.
            barrier_owner.npc_projectile_services.pickup_floor=[](void*q,const float*from,const float*to,float*point,uint32_t*material){
                auto&s=*static_cast<BarrierOwner*>(q);s.refresh_collision();UCCollisionResult hit{};int32_t id=-1;
                int found=s.character_world.query(from,to,9,0,0x80,0,hit,id);
                if(found>0){std::memcpy(point,hit.point,12);*material=hit.material;}return found;};
            barrier_owner.npc_projectile_services.pickup=[](void*q,NativeNpcThrownWeapon&p,unsigned*taken){
                auto&s=*static_cast<BarrierOwner*>(q);*taken=0;if(p.state.weapon1EC!=11||!s.firstperson||!s.player)return 1;
                const float*player=s.player->entity.transform+12;const float*item=p.entity.transform+12;
                float x=player[0]-item[0],y=player[1]-item[1],z=player[2]-item[2];
                if((x*x+y*y)+z*z<75.f*75.f&&s.firstperson->inventory.health>0&&s.firstperson->accepts_input())
                    *taken=s.firstperson->pickup_molotov(*s.renderer,s.audio_stream.parent_path(),s.catalog.parent_path());
                return 0;};
            //216C60 material/default sound selection through the existing PC audio owner.
            barrier_owner.npc_projectile_services.land_sound=[](void*q,NativeNpcThrownWeapon&p)->int{
                auto&s=*static_cast<BarrierOwner*>(q);const char*bank=p.state.bank238;if(!bank||!s.audio)return 1;
                auto&source=s.world_sounds[bank];if(!source){size_t index=SIZE_MAX;
                    if(uc_archive_lookup(&s.texture_lookup,bank,&index)||index==SIZE_MAX)return 1;
                    source=std::make_unique<NativePlayerAudio::MissionSource>(*s.audio,s.archive.get(),index,s.audio_stream);}
                UCAudioMaterialActor actor{source.get()};char requested[128];UCAudioMaterialServices h{source.get(),
                    [](void*,const UCAudioMaterialActor*,const void*){return 1;},
                    [](void*q,UCAudioMaterialActor*)->void*{return q;},
                    [](void*,void*q,const void*,const char*name)->uint8_t{return static_cast<NativePlayerAudio::MissionSource*>(q)->play(name);}};
                return uc_audio_material_request(&actor,bank,p.state.material214,"LandOn",requested,sizeof requested,&h);};
            barrier_owner.npc_throw.restock=[](void*q,int32_t actor,int32_t weapon,bool secondary){auto&s=*static_cast<BarrierOwner*>(q);auto*c=s.character(actor);float local[16];
                if(!c||!c->posed_bone("gundummy",local))throw std::runtime_error("NPC restock attachment");
                NativeNpcThrownDescriptor d{};d.weapon=weapon;d.owner=actor;d.secondary=secondary;d.bank=native_npc_throw_bank(weapon);
                native_npc_throw_matrix(local,c->entity.transform,d.world);s.npc_projectiles.push_back(std::make_unique<NativeNpcThrownWeapon>());auto*p=s.npc_projectiles.back().get();
                if(p->construct(*s.renderer,*s.world,*c,s.archive.get(),s.archive_hash,s.catalog,s.texture_lookup,d,s.npc_projectile_services))throw std::runtime_error("NPC restock projectile construction");return p->entity.id;};
            barrier_owner.npc_throw.ignite=[](void*q,int32_t id,float seconds){auto*p=static_cast<BarrierOwner*>(q)->projectile(id);return p?p->ignite(seconds):1;};
            barrier_owner.npc_throw.set_fuse=[](void*q,int32_t id,float seconds){auto*p=static_cast<BarrierOwner*>(q)->projectile(id);if(!p)return 1;p->set_fuse(seconds);return 0;};
            barrier_owner.npc_throw.drop=[](void*q,int32_t id){auto*p=static_cast<BarrierOwner*>(q)->projectile(id);return p?p->drop():1;};
            barrier_owner.npc_throw.remove=[](void*q,int32_t id){auto*p=static_cast<BarrierOwner*>(q)->projectile(id);if(!p)return 1;p->entity.flags|=1;return p->remove();};
            barrier_owner.npc_throw.sound=[](void*q,int32_t id,const char*name){auto&s=*static_cast<BarrierOwner*>(q);auto*p=s.projectile(id);auto*c=p?s.character(p->entity.parent):nullptr;return c?s.play_sound(id,c->audio_bank_path(),name):1;};
            barrier_owner.npc_throw.launch=[](void*q,int32_t id,const float*point,uint32_t options,float speed,int32_t selection){auto&s=*static_cast<BarrierOwner*>(q);auto*p=s.projectile(id);if(!p)return 1;
                UCNpcThrowTrajectoryServices h{q,[](void*q,float a,float b){int32_t value;auto&s=*static_cast<BarrierOwner*>(q);if(uc_game_random_next(s.random,&value))throw std::runtime_error("NPC throw random");return (double(value)*(double(b)-a))*0x1p-31+a;},
                    [](void*q){int32_t value;auto&s=*static_cast<BarrierOwner*>(q);return uc_game_random_boolean(s.random,&value)?-1:value;},
                    [](void*q,const float*from,const float*to,uint32_t type,uint32_t region,uint32_t mask,float impulse){auto&s=*static_cast<BarrierOwner*>(q);s.refresh_collision();UCCollisionResult hit{};int32_t id=0;return s.character_world.query(from,to,type,region,mask,impulse,hit,id);}};
                UCNpcThrowTrajectory result{};if(uc_npc_throw_trajectory(p->entity.transform+12,point,selection,options,speed,&h,&result))return 1;return p->launch(result.velocity,result.delay,result.spin);};
            uc_game_difficulty_fresh(&barrier_owner.difficulty);
            if(restored){
                for(auto source:restored->completed_objectives)
                    if(source<0||uint32_t(source)>=entities.value.entity_count||std::strcmp(entities.value.entities[source].class_name,"Objective"))
                        throw std::runtime_error("Save contains invalid objective progress; the save was preserved.");
                barrier_owner.difficulty=restored->difficulty;barrier_owner.objective_saved.load_saved(restored->completed_objectives);
            }
            UCParticleRegistry*npc_flare_registry=nullptr;
            if(uc_particle_registry_create(1024,&npc_flare_registry))throw std::runtime_error("NPC effect registry initialization");
            barrier_owner.npc_flare_registry.reset(npc_flare_registry);
            UCCharacterHazards*hazards=nullptr;
            if(uc_character_hazards_create(&entities.value,&hazards))throw std::runtime_error("Native NPC hazard registry initialization");
            barrier_owner.hazards.reset(hazards);barrier_owner.impacts=impacts.get();
            barrier_owner.particles=level_particles.get();
            barrier_owner.lighting=lighting.get();barrier_owner.random=&session_random;
            barrier_owner.bind_gun_controller();
            barrier_owner.catalog=material_path.parent_path()/"1-01.ent.ucmat";
            barrier_owner.audio_stream=level.parent_path()/"5.saf";
            if(firstperson){barrier_owner.archive=read_archive(level.parent_path()/"1-01.ent",barrier_owner.archive_hash);
                BytesOwner ragdoll_bytes;const auto index=find_resource(barrier_owner.archive.get(),"ragdoll_havoc3.xml");
                if(index==SIZE_MAX||uc_archive_read(barrier_owner.archive.get(),index,UC_AR_LOCAL_PAYLOAD,&ragdoll_bytes.value)||
                    uc_ragdoll_asset_load(ragdoll_bytes.value.data,ragdoll_bytes.value.size,&barrier_owner.ragdoll_asset.value))
                    throw std::runtime_error("Original level ragdoll asset could not be loaded");
                //20959C passes -1700cm/s² to29E930;2A2870 converts to physics Z metres.
                const float gravity[]={0,0,-17};barrier_owner.ragdoll_world=std::make_unique<NativeRagdollWorld>(collision.value,gravity);
                //8DA10 language0 +1: English banks use tag1; common art keeps tag0.
                if(uc_archive_lookup_init(&barrier_owner.texture_lookup,barrier_owner.archive.get(),1))throw std::runtime_error("Mission archive lookup initialization");
                barrier_owner.npc_thrown_particles=std::make_unique<NativeNpcThrownParticles>(renderer,barrier_owner.archive.get(),mission_world,material_path.parent_path());}
            NativeMissionWorldServices services;services.owner=&barrier_owner;
            if(profile_cpu)services.profile_step=[](void*p,uint32_t source,double ms){auto&s=*static_cast<BarrierOwner*>(p);auto&v=s.profile_entities[source];v[0]+=ms;++v[1];if(ms>=1&&s.profile_slow_steps.size()<100000)s.profile_slow_steps.push_back({double(s.world->frame()),double(source),ms});};
            services.pre_world=[](void*owner,float previous_delta){auto&s=*static_cast<BarrierOwner*>(owner);
                if(s.ragdoll_world){s.refresh_collision();s.ragdoll_world->sync_collision(s.character_world);s.ragdoll_world->step(previous_delta);for(const auto&p:s.props)if(p->ready())p->sync_physics();}
                s.refresh_collision();s.navigation.update(previous_delta,[](void*q){if(static_cast<BarrierOwner*>(q)->navigation_boxes.refresh())throw std::runtime_error("Mission navigation box update failed");},&s);
                if(s.ai_maintenance.step(s.maintenance_services(),s.world->frame(),previous_delta))throw std::runtime_error("NPC maintenance update failed");
                if(uc_entity_event_lookup(&s.world->registry(),s.player->entity.id)==&s.player->entity.identity)
                    s.factions.refresh(s.target_services());
                NativeCharacterAIPhase::Services h{&s,[](void*p,int32_t id,bool expensive,uint8_t*found,uint32_t*flags){
                    auto*c=static_cast<BarrierOwner*>(p)->character(id);*found=c!=nullptr;*flags=0;if(!c)return 0;
                    int result=c->controller_update(expensive);*flags=c->controller_flags();if(result){NativeCharacterNavigationState state;c->navigation_state(state);
                        auto*b=static_cast<BarrierOwner*>(p)->world->binding(uint32_t(state.behaviour));uint32_t op=UINT32_MAX;
                        if(b&&b->view&&b->view->actor&&b->view->actor->wordC4)std::memcpy(&op,b->view->actor->wordC4,4);
                        throw std::runtime_error("NPC controller update failed source="+std::to_string(c->entity.source)+" behaviour="+std::to_string(state.behaviour)+" opcode="+std::to_string(op)+" code="+std::to_string(result));}return result;}};
                if(s.ai->tick(h)||s.ai_maintenance.finish(previous_delta))return 1;
                if(s.dialogue_audio)s.dialogue_audio->advance_emotes(previous_delta);return 0;};
            services.construct=[](void *owner,NativeMissionWorld &world,uint32_t index){
                auto &storage=*static_cast<BarrierOwner*>(owner);const auto &metadata=world.metadata(index);
                if(!metadata.definition||!metadata.definition->class_name)return 1;
                if(!strcmp(metadata.definition->class_name,"HotZone")||!strcmp(metadata.definition->class_name,"ColdBreathOnCharacters")){
                    storage.zones.push_back(std::make_unique<NativeMissionZone>());auto*p=storage.zones.back().get();
                    if(world.bind(index,{&p->entity.view,p,[](void*q){return static_cast<NativeMissionZone*>(q)->update();},
                        [](void*q){return static_cast<NativeMissionZone*>(q)->late();},[](void*q){return static_cast<NativeMissionZone*>(q)->close_detached();},nullptr}))return 1;
                    NativeColdBreathServices h{};h.owner=&storage;
                    h.count=[](void*q){return int32_t(static_cast<BarrierOwner*>(q)->ai->size());};
                    h.at=[](void*q,int32_t i,int32_t*out){return i<0||!out?1:static_cast<BarrierOwner*>(q)->ai->at(size_t(i),*out);};
                    h.health=[](void*q,int32_t id,double*out){auto*c=static_cast<BarrierOwner*>(q)->character(id);if(!out)return 1;
                        if(!c){*out=0;return 0;}return c->scalar_property("health",out)==1?0:1;};
                    h.random=[](void*q,float low,float high){int32_t v=0;if(uc_game_random_next(static_cast<BarrierOwner*>(q)->random,&v))throw std::runtime_error("Cold breath shared random");
                        return(double(v)*(double(high)-low))*0x1p-31+low;};
                    h.emit=[](void*q,int32_t id){auto&s=*static_cast<BarrierOwner*>(q);auto*c=s.character(id);
                        return c&&s.npc_thrown_particles?s.npc_thrown_particles->append_cold(*c):1;};
                    return p->construct(world,index,&h);
                }
                if(storage.handoff&&storage.handoff->supports(metadata.definition->class_name))return storage.handoff->construct(index);
                if(!strcmp(metadata.definition->class_name,"Objective")){
                    storage.objectives.push_back(std::make_unique<NativeMissionObjective>());auto*p=storage.objectives.back().get();
                    if(world.bind(index,{&p->entity.view,p,[](void*q){return static_cast<NativeMissionObjective*>(q)->update();},
                        [](void*q){return static_cast<NativeMissionObjective*>(q)->late();},[](void*q){return static_cast<NativeMissionObjective*>(q)->close_detached();},&p->entity.script}))return 1;
                    NativeObjectiveServices h{&storage,
                        [](void*q,bool*out){auto&s=*static_cast<BarrierOwner*>(q);if(!out||!s.handoff)return 1;
                            *out=s.handoff->checkpoints.ready(*s.world,&s.handoff->session_mode);return 0;},
                        [](void*q,int32_t type,const char*title,const char*text){return static_cast<BarrierOwner*>(q)->objective_messages.add(type,title,text);}};
                    return p->construct(world,index,storage.objective_saved,h);}
                if(!strcmp(metadata.definition->class_name,"CoverPoint")){
                    storage.covers.push_back(std::make_unique<NativeMissionCoverPoint>());auto*p=storage.covers.back().get();
                    if(world.bind(index,{&p->entity.view,p,[](void*q){return static_cast<NativeMissionCoverPoint*>(q)->update();},
                        [](void*q){return static_cast<NativeMissionCoverPoint*>(q)->late();},[](void*q){return static_cast<NativeMissionCoverPoint*>(q)->close_detached();},nullptr}))return 1;
                    return p->construct(world,index,*storage.cover_points);}
                if(!strcmp(metadata.definition->class_name,"LinearImpulse")){
                    storage.impulses.push_back(std::make_unique<NativeLinearImpulse>());auto*p=storage.impulses.back().get();
                    if(world.bind(index,{&p->entity.view,p,[](void*q){return static_cast<NativeLinearImpulse*>(q)->update();},
                        [](void*q){return static_cast<NativeLinearImpulse*>(q)->late();},[](void*q){return static_cast<NativeLinearImpulse*>(q)->close_detached();},&p->entity.script}))return 1;
                    NativeLinearImpulseServices h{&storage,
                        [](void*q,int32_t id,void**out){auto&s=*static_cast<BarrierOwner*>(q);if(!out)return 1;
                            for(const auto&prop:s.props)if(prop->entity.id==id&&prop->ready()){*out=prop.get();return 0;}return 1;},
                        [](void*,void*body,const float*momentum){return static_cast<NativeRigidProp*>(body)->impulse(momentum);}};
                    return p->construct(world,index,h);
                }
                if(!strcmp(metadata.definition->class_name,"FXFire")){
                    storage.fires.push_back(std::make_unique<NativeMissionFire>());
                    return storage.fires.back()->construct(world,index,storage.navigation_boxes,*storage.particles,storage.fire_services(),
                        *storage.audio,storage.archive.get(),storage.texture_lookup,storage.audio_stream);
                }
                if(!strcmp(metadata.definition->class_name,"Explosion")){
                    storage.explosions.push_back(std::make_unique<NativeMissionExplosion>());auto*p=storage.explosions.back().get();
                    if(world.bind(index,{&p->entity.view,p,[](void*q){return static_cast<NativeMissionExplosion*>(q)->update();},
                        [](void*){return 0;},[](void*q){return static_cast<NativeMissionExplosion*>(q)->close_detached();},nullptr,
                        [](void*q){return static_cast<NativeMissionExplosion*>(q)->remove();},
                        [](void*q){return static_cast<NativeMissionExplosion*>(q)->close_detached();}}))return 1;
                    return p->construct(world,index,*storage.audio,storage.archive.get(),storage.texture_lookup,storage.audio_stream,storage.explosion_services());
                }
                if(!strcmp(metadata.definition->class_name,"ThrownWeaponTemplateMolotov")){
                    storage.molotov_templates.push_back(std::make_unique<BarrierOwner::MolotovTemplate>(&storage));
                    auto*p=storage.molotov_templates.back().get();return p->construct(world,index,p,BarrierOwner::template_receive);
                }
                if(!strcmp(metadata.definition->class_name,"ThrowProjectile"))return storage.random?
                    native_construct_scripted_thrower(world,index,*storage.random,&storage,
                        [](void*q,int32_t parent,const float*frame,const float*target,float time,float&delay)->int32_t{
                            auto&s=*static_cast<BarrierOwner*>(q);UCScriptedMolotov launch{};
                            if(uc_scripted_molotov(frame+12,target,time,&delay,&launch))throw std::runtime_error("Scripted Molotov trajectory");
                            NativeNpcThrownDescriptor d{};std::memcpy(d.world,frame,64);d.bank=native_npc_throw_bank(11);
                            s.npc_projectiles.push_back(std::make_unique<NativeNpcThrownWeapon>());auto*p=s.npc_projectiles.back().get();
                            if(p->construct(*s.renderer,*s.world,parent,s.archive.get(),s.archive_hash,s.catalog,s.texture_lookup,d,s.npc_projectile_services))
                                throw std::runtime_error("Scripted Molotov construction");
                            uint32_t sender=uint32_t(parent);uint8_t handled=0;
                            if(uc_entity_event_send(&s.world->registry(),p->entity.id,217,&launch.fuse,4,&sender,&handled)||!handled)
                                throw std::runtime_error("Scripted Molotov ignition event");
                            const float flight[]={launch.velocity[0],launch.velocity[1],launch.velocity[2],launch.delay};handled=0;
                            if(uc_entity_event_send(&s.world->registry(),p->entity.id,221,flight,sizeof flight,&sender,&handled)||!handled)
                                throw std::runtime_error("Scripted Molotov launch event");return p->entity.id; }):1;
                if(!strcmp(metadata.definition->class_name,"Mainplayer")){
                    if(!storage.player||!storage.firstperson)return 1;
                    int r=storage.player->construct(world,index,*storage.firstperson);
                    if(!r)storage.factions.add(110,storage.player->entity.id); //D4EE0, fresh single-player profile.
                    return r;
                }
                if(!strcmp(metadata.definition->class_name,"TriggeredCameraShakeAndRumble")){
                    if(!storage.camera_feedback)return 1;
                    auto owned=std::make_unique<NativeTriggeredCameraShake>();auto*p=owned.get();storage.camera_shakes.push_back(std::move(owned));p->owners=&storage.camera_shakes;
                    if(world.bind(index,{&p->entity.view,p,[](void*q){return static_cast<NativeTriggeredCameraShake*>(q)->update();},
                        [](void*q){return static_cast<NativeTriggeredCameraShake*>(q)->late();},
                        [](void*q){return static_cast<NativeTriggeredCameraShake*>(q)->close_detached();},&p->entity.script,
                        [](void*q){return static_cast<NativeTriggeredCameraShake*>(q)->remove();},
                        [](void*q){auto*p=static_cast<NativeTriggeredCameraShake*>(q);auto&owners=*p->owners;
                            if(p->close_detached())return 1;
                            auto it=std::find_if(owners.begin(),owners.end(),[p](const auto&v){return v.get()==p;});
                            if(it==owners.end())return 1;owners.erase(it);return 0;}}))return 1;
                    return p->construct(world,index,storage.camera_feedback->manager(),storage.archive.get());
                }
                if(!strcmp(metadata.definition->class_name,"Dispatch")){
                    if(!storage.dialogue_audio||!storage.random||!storage.player||!storage.handoff)return 1;
                    auto owned=std::make_unique<NativeDispatch>();auto*p=owned.get();storage.dispatches.push_back(std::move(owned));
                    if(world.bind(index,{&p->entity.view,p,[](void*q){return static_cast<NativeDispatch*>(q)->update();},
                        NativeCharacterSpeech::late,[](void*q){return static_cast<NativeDispatch*>(q)->close_detached();},nullptr,
                        [](void*q){return static_cast<NativeDispatch*>(q)->remove();},
                        [](void*q){return static_cast<NativeDispatch*>(q)->close_detached();}}))return 1;
                    return p->construct(world,index,*storage.dialogue_audio,storage.dialogue_text,*storage.random,storage.subtitles,*storage.player,storage.active_dispatch,storage.handoff->world_flags);
                }
                if(!strcmp(metadata.definition->class_name,"CharacterSpeech")){
                    auto speech=std::make_unique<NativeCharacterSpeech>();auto*p=speech.get();storage.speeches.push_back(std::move(speech));
                    if(world.bind(index,{&p->entity.view,p,[](void*q){return static_cast<NativeCharacterSpeech*>(q)->update();},
                        NativeCharacterSpeech::late,[](void*q){return static_cast<NativeCharacterSpeech*>(q)->close_detached();},&p->entity.script}))return 1;
                    NativeCharacterSpeechServices h{};h.owner=&storage;
                    h.ready=[](void*q,int32_t id,uint8_t*out){auto&s=*static_cast<BarrierOwner*>(q);auto*c=s.character(id);if(!c)return 1;
                        const int32_t*ids=nullptr;uint32_t count=0;if(uc_entity_names_view(s.world->names(),"FRIENDLY_DEATH_CUTSCENE",&ids,&count))return 1;
                        *out=c->dialogue_ready(count!=0);return 0;};
                    h.playing=[](void*q,int32_t id,uint8_t*out){auto*c=static_cast<BarrierOwner*>(q)->character(id);if(!c)return 1;*out=c->dialogue_playing();return 0;};
                    h.request=[](void*q,int32_t id,int32_t speaker,const char*text,uint8_t look,uint8_t force){auto*c=static_cast<BarrierOwner*>(q)->character(id);return c&&c->dialogue_request(text,speaker,look!=0,force!=0)?0:1;};
                    h.status=[](void*q,int32_t id,uint32_t*out){auto*c=static_cast<BarrierOwner*>(q)->character(id);if(!c)return 1;*out=c->status();return 0;};
                    h.stop=[](void*q,int32_t id){auto*c=static_cast<BarrierOwner*>(q)->character(id);if(!c)return 1;c->dialogue_cancel();return 0;};
                    h.speaker_owner=[](void*q,int32_t id,int32_t*out){auto*c=static_cast<BarrierOwner*>(q)->character(id);return c&&c->dialogue_owner(*out)?0:1;};
                    h.cancel=h.stop;return p->construct(world,index,h);
                }
                if(!strcmp(metadata.definition->class_name,"MoveRunBehaviour")||!strcmp(metadata.definition->class_name,"MoveWalkBehaviour")||!strcmp(metadata.definition->class_name,"RandomSkinAndEquipment")||
                   !strcmp(metadata.definition->class_name,"DeleteMyself")||!strcmp(metadata.definition->class_name,"BurnersBlazerBehaviour")||
                   !strcmp(metadata.definition->class_name,"BurnersBruteBehaviour")||!strcmp(metadata.definition->class_name,"GenericExitCondition")||
                   !strcmp(metadata.definition->class_name,"FaceEntityBehaviour")||!strcmp(metadata.definition->class_name,"ScriptedBehaviour")||
                   !strcmp(metadata.definition->class_name,"PlayAnimBehaviour")||!strcmp(metadata.definition->class_name,"BurnersBruteAttackBehaviour")||
                   !strcmp(metadata.definition->class_name,"CharacterSpawnPoint")||!strcmp(metadata.definition->class_name,"Cop")||!strcmp(metadata.definition->class_name,"MedicForPlayersTeam")||
                   !strcmp(metadata.definition->class_name,"Named_Character")||!strcmp(metadata.definition->class_name,"CopGenericSpeech")||
                   !strcmp(metadata.definition->class_name,"CopNoSecondary")||!strcmp(metadata.definition->class_name,"NoCondition")||
                   !strcmp(metadata.definition->class_name,"CopMoveAndShoot")||!strcmp(metadata.definition->class_name,"PlayLoopedAnimBehaviour")||
                   !strcmp(metadata.definition->class_name,"GotLineOfSight")||!strcmp(metadata.definition->class_name,"FollowEntity")||!strcmp(metadata.definition->class_name,"NoBehaviour")||
                   !strcmp(metadata.definition->class_name,"WhenPlayerInRangeRunBehaviour")||!strcmp(metadata.definition->class_name,"WhenFlagSetRunBehaviour")||
                   !strcmp(metadata.definition->class_name,"CopAttackBehaviour")||!strcmp(metadata.definition->class_name,"BurnersBlazerAttackBehaviour")||
                   !strcmp(metadata.definition->class_name,"ShowHeartbeatMonitor")||!strcmp(metadata.definition->class_name,"MedicDisableAutoHeal")||
                   !strcmp(metadata.definition->class_name,"Medic")||!strcmp(metadata.definition->class_name,"MedicGenericSpeech")||!strcmp(metadata.definition->class_name,"MedicEnableAutoHeal")||
                   !strcmp(metadata.definition->class_name,"MakeInvulnerable")||
                   !strcmp(metadata.definition->class_name,"InteractiveScriptedBehaviour"))return native_construct_character_behaviour(world,index,storage.archive.get());
                if(!strcmp(metadata.definition->class_name,"CharacterBurnerBlazer")||!strcmp(metadata.definition->class_name,"Character")||!strcmp(metadata.definition->class_name,"CharacterCop")||!strcmp(metadata.definition->class_name,"CharacterMedic")){
                    if(!storage.firstperson||!storage.player||!storage.random||!storage.vector_random)return 1;
                    auto character=std::make_unique<NativeMissionCharacter>();auto*p=character.get();storage.characters.push_back(std::move(character));
                    NativeMissionCharacterServices h{};h.owner=&storage;h.collision=&storage.character_world;h.navigation=&storage.navigation;
                    h.ragdoll_world=storage.ragdoll_world.get();h.ragdoll_asset=&storage.ragdoll_asset.value;
                    h.blast_obstruction=[](void*q,const float*from,const float*to,const void*target,int32_t group){
                        return static_cast<BarrierOwner*>(q)->blast_obstruction(from,to,target,group);};
                    h.texture_lookup=&storage.texture_lookup;h.cover_points=storage.cover_points.get();
                    h.factions=&storage.factions;
                    h.health_policy=[](void*q,NativeMissionCharacter&c,UCCharacterHealth&s,uint32_t f,float d,UCCharacterHealthServices&h){return static_cast<BarrierOwner*>(q)->character_health(c,s,f,d,h);};
                    h.character_died=[](void*q,NativeMissionCharacter&c,int32_t killer,bool gore){return static_cast<BarrierOwner*>(q)->character_died(c,killer,gore);};
                    h.held_burn=[](void*q,NativeMissionCharacter&actor,bool completed)->int{
                        auto&s=*static_cast<BarrierOwner*>(q);if(!s.handoff)return 1;
                        if(completed){++s.handoff->statistics[1];return 0;} //1D08EC.
                        //ponytail: one real pelvis flame; moving paired limb emitters remain unported.
                        return s.play_sound(actor.entity.id,"Banks\\Objects\\Fire.amp","BodyStartBurning")||
                            s.character_effect(actor,"Bip01 Pelvis","AM-body-burn.simpleparticlesystem");};
                    h.dialogue_audio=storage.dialogue_audio.get();h.dialogue_text=&storage.dialogue_text;
                    h.dialogue_zone=[](void*q,NativeMissionCharacter&c,int32_t*out){return static_cast<BarrierOwner*>(q)->dialogue_zone(c.entity.id,out);};
                    h.subtitle=[](void*q,const NativeDialogueLine&line,int32_t id,int32_t*out){auto&s=*static_cast<BarrierOwner*>(q);auto*c=s.character(id);
                        return c?s.subtitles.add(line,*c,*s.world,*s.player,out):1;};
                    h.target_property=[](void*q,int32_t id,const char*name,int32_t*out){auto h=static_cast<BarrierOwner*>(q)->target_services();return h.property(h.owner,id,name,out);};
                    h.target_scalar_property=[](void*q,int32_t id,const char*name,double*out){auto&s=*static_cast<BarrierOwner*>(q);
                        auto*t=uc_entity_event_lookup(&s.world->registry(),id);if(!t)return 0;
                        if(auto*c=s.character(id))return c->scalar_property(name,out);
                        return t==&s.player->entity.identity&&s.handoff&&out?s.player->target_scalar_property(name,*out,s.handoff->session_mode,0):-1;}; //Fresh profile health tier0, shared with HUD.
                    h.target_matrix_property=[](void*q,int32_t id,const char*name,float*out){auto&s=*static_cast<BarrierOwner*>(q);
                        auto*t=uc_entity_event_lookup(&s.world->registry(),id);if(!t)return 0;
                        return t==&s.player->entity.identity?s.player->target_matrix_property(name,out):-1;};
                    h.target_vector_property=[](void*q,int32_t id,const char*name,float*out){auto&s=*static_cast<BarrierOwner*>(q);
                        auto*t=uc_entity_event_lookup(&s.world->registry(),id);if(!t)return 0;
                        if(t==&s.player->entity.identity){if(!std::strcmp(name,"target position")){std::memcpy(out,s.firstperson->camera_input.player_frame+12,12);return 1;}
                            return s.player->target_vector_property(name,out);}
                        if(auto*c=s.character(id)){if(!std::strcmp(name,"head pos"))return c->head_position(out)?1:-1;
                            if(!std::strcmp(name,"target position"))return c->target_position(out)?1:-1;}
                        return -1;};
                    h.visible=[](void*q,NativeMissionCharacter&actor,int32_t id){return static_cast<BarrierOwner*>(q)->visible(actor,id);};
                    h.visible_query=[](void*q,NativeMissionCharacter&actor,int32_t id,float cosine,bool head,const float*offset,bool characters){
                        return static_cast<BarrierOwner*>(q)->visible(actor,id,cosine,head,offset,characters);};
                    h.hazard_segment=[](void*q,const float*from,const float*to){uint8_t clear=0;
                        return uc_character_hazards_segment_clear(static_cast<BarrierOwner*>(q)->hazards.get(),from,to,0,&clear)?-1:int(clear);};
                    h.loop_sound=[](void*q,NativeMissionCharacter&actor,const void*bank,size_t size,const char*event){auto&s=*static_cast<BarrierOwner*>(q);
                        if(!s.audio)return 1;
                        if(!actor.entity.helper){NativeAudioBank descriptor;descriptor.load(bank,size);
                            if(descriptor.events().empty()&&descriptor.sounds().empty())actor.entity.helper=s.audio->create_empty_source(bank,size,actor.entity.transform+12,true);
                        }
                        if(!actor.entity.helper){
                            // Resolve the exact supplied authored bank, including populated PlayAnim banks.
                            size_t found=SIZE_MAX;
                            for(size_t i=0;i<uc_archive_count(s.archive.get());++i){auto*e=uc_archive_entry(s.archive.get(),i);
                                if(!e->name||!strstr((const char*)e->name,".amp"))continue;UCArchiveBytes bytes{};
                                if(uc_archive_read(s.archive.get(),i,UC_AR_LOCAL_PAYLOAD,&bytes))return 1;
                                bool match=bytes.size==size&&!memcmp(bytes.data,bank,size);uc_archive_bytes_free(&bytes);
                                if(match){found=i;break;}}
                            if(found==SIZE_MAX)return 1;
                            auto source=std::make_unique<NativePlayerAudio::MissionSource>(*s.audio,s.archive.get(),found,s.audio_stream);
                            actor.entity.helper=source.get();s.loop_sources.emplace(source.get(),std::move(source));
                        }
                        auto found=s.loop_sources.find(actor.entity.helper);if(found==s.loop_sources.end()){s.audio->play_empty_source((NativePlayerAudio::EmptySource*)actor.entity.helper,event);return 0;}
                        found->second->play(event);return 0;};
                    h.release_helper=[](void*q,void*helper){auto&s=*static_cast<BarrierOwner*>(q);if(!s.audio||!helper)return 1;
                        if(s.loop_sources.erase(helper))return 0;
                        s.audio->close_empty_source((NativePlayerAudio::EmptySource*)helper);return 0;};
                    h.difficulty=[](void*q,double*out){return uc_game_difficulty_value(&static_cast<BarrierOwner*>(q)->difficulty,out);};
                    h.npc_throw=&storage.npc_throw;
                    h.npc_gun=&storage.npc_gun;
                    h.throw_difficulty=[](void*q,float low,float high,double*out){auto&s=*static_cast<BarrierOwner*>(q);double value;
                        if(uc_blazer_throw_difficulty(&s.difficulty,s.throw_channel,&value))return 1;*out=value*((double)high-low)+low;return 0;};
                    h.throw_curve=[](void*q,float low,float middle,float high,double*out){auto&s=*static_cast<BarrierOwner*>(q);
                        return uc_blazer_throw_curve(&s.difficulty,s.throw_channel,low,middle,high,out);};
                    h.brute_difficulty=[](void*q,float low,float high,double*out){auto&s=*static_cast<BarrierOwner*>(q);double value;
                        if(uc_blazer_throw_difficulty(&s.difficulty,s.brute_channel,&value))return 1;*out=value*((double)high-low)+low;return 0;};
                    h.brute_range=[](void*q,float low,float high,float begin,float end,double*out){auto&s=*static_cast<BarrierOwner*>(q);
                        return uc_blazer_throw_range(&s.difficulty,s.brute_channel,low,high,begin,end,out);};
                    h.brute_curve=[](void*q,float low,float middle,float high,double*out){auto&s=*static_cast<BarrierOwner*>(q);
                        return uc_blazer_throw_curve(&s.difficulty,s.brute_channel,low,middle,high,out);};
                    h.world_flags=[](void*q,uint32_t*out){auto&s=*static_cast<BarrierOwner*>(q);if(!s.handoff)return 1;*out=s.handoff->world_flags;return 0;};
                    h.mission_ordinal=[](void*q,int32_t*out){auto&s=*static_cast<BarrierOwner*>(q);*out=uc_dialogue_mission_ordinal(s.mission_name.c_str());return 0;};
                    h.emote_distance=[](void*q,NativeMissionCharacter&c,double*out){auto&s=*static_cast<BarrierOwner*>(q);
                        *out=uc_dialogue_emote_distance(s.published_camera,c.entity.transform+12);return 0;};
                    h.character_sound=[](void*q,NativeMissionCharacter&c,const NativeAudioBank&,const char*name,bool stop){return static_cast<BarrierOwner*>(q)->play_sound(c.entity.id,c.audio_bank_path(),name,stop);};
                    h.controller_registration=[](void*q,int32_t id,bool add){auto&s=*static_cast<BarrierOwner*>(q);return add?s.ai->add(id):s.ai->remove(id);};
                    h.maintenance_registration=[](void*q,int32_t id){auto&s=*static_cast<BarrierOwner*>(q);return s.ai_maintenance.add(s.maintenance_services(),id);};
                    h.bullet_by=[](void*q,int32_t id,bool check,uint32_t*out){auto&s=*static_cast<BarrierOwner*>(q);
                        NativeCharacterAIMaintenance::ShotServices query{};query.owner=&s;query.first_player=s.player->entity.id;
                        query.actor=[](void*p,int32_t actor,float*position,float*forward){auto&s=*static_cast<BarrierOwner*>(p);
                            auto*t=uc_entity_event_lookup(&s.world->registry(),actor);if(!t)return 0;
                            auto*v=s.world->context().resolve(s.world->context().owner,t);if(!v||!v->transform)return -1;
                            std::memcpy(position,v->transform+12,12);std::memcpy(forward,v->transform+8,12);return 1;};
                        query.relation=[](void*p,int32_t actor,int32_t shooter,uint8_t*handled,uint8_t*result){auto*c=static_cast<BarrierOwner*>(p)->character(actor);
                            return c?c->bullet_relation(shooter,handled,result):1;};
                        return s.ai_maintenance.bullet_by(query,id,check,out);};
                    h.cop_gun=[](void*q,NativeMissionWorld&w,NativeMissionCharacter&character,int32_t*out){auto&s=*static_cast<BarrierOwner*>(q);
                        if(!out||!s.npc_flare_registry)return 1;s.guns.push_back(std::make_unique<NativeNpcGun>());auto*p=s.guns.back().get();
                        int result=p->construct(*s.renderer,w,character,s.archive.get(),s.archive_hash,s.catalog,s.texture_lookup,*s.random,*s.npc_flare_registry,&s.npc_effects);
                        if(!result){p->set_shot_services(s.gun_shot_services());*out=p->entity.id;}return result;};
                    h.medic_weapon=[](void*q,NativeMissionWorld&w,NativeMissionCharacter&c,int32_t*out){return static_cast<BarrierOwner*>(q)->create_melee(w,c,out,34);};
                    h.medic_pillbox=[](void*q,NativeMissionWorld&w,NativeMissionCharacter&c,const float*matrix,int32_t*out){return static_cast<BarrierOwner*>(q)->create_melee(w,c,out,34,matrix);};
                    h.brute_weapon=[](void*q,NativeMissionWorld&w,NativeMissionCharacter&c,int32_t*out){return static_cast<BarrierOwner*>(q)->create_melee(w,c,out,8);};
                    h.melee_weapon_type=[](void*q,int32_t id,int32_t*out){auto&s=*static_cast<BarrierOwner*>(q);if(!out)return 1;
                        auto*t=uc_entity_event_lookup(&s.world->registry(),id);for(auto&w:s.medic_weapons)if(t==&w->entity.identity)return w->property("weapon type",*out)==1?0:1;return 1;};
                    h.melee_weapon_flags=[](void*q,int32_t id,uint32_t flags){auto&s=*static_cast<BarrierOwner*>(q);auto*t=uc_entity_event_lookup(&s.world->registry(),id);
                        for(auto&w:s.medic_weapons)if(t==&w->entity.identity)return w->set_flags(flags);return 1;};
                    h.melee_eligible=[](void*q,NativeMissionCharacter&actor,const UCNpcMeleeController&melee,int32_t target,uint8_t*out){
                        auto&s=*static_cast<BarrierOwner*>(q);if(!out||actor.entity.id!=melee.actor)return 1;*out=0;
                        if(melee.weapon==-1||melee.detached)return 0;
                        auto*t=uc_entity_event_lookup(&s.world->registry(),target);if(!t)return 0;
                        auto&context=s.world->context();auto*v=context.resolve(context.owner,t);if(!v||!v->transform)return 1;
                        return uc_npc_melee_contact(melee.profile,actor.entity.transform+12,v->transform+12,actor.entity.transform+8,out);};
                    h.melee_strike=[](void*q,NativeMissionCharacter&actor,UCNpcMeleeController&melee){
                        auto&s=*static_cast<BarrierOwner*>(q);float point[3],amount;UCPlayerFireDamage hit{};
                        if(actor.entity.id!=melee.actor||s.target_position(melee.target,true,point)!=1||
                            uc_npc_melee_hit(&melee,actor.entity.transform+12,point,&amount,&hit))return 1;
                        if(melee.target!=s.player->entity.id)
                            throw std::runtime_error("NPC melee recipient damage is unconnected target="+std::to_string(melee.target));
                        if(s.player->damage(amount,hit))return 1;
                        return s.play_sound(melee.weapon,melee.type==8?"Banks\\Objects\\Cleaver.amp":"Banks\\Default.amp","Impact");};
                    h.medic_heals=[](void*q,int32_t named,int32_t*out){if(!out)return 1;*out=static_cast<BarrierOwner*>(q)->medical_heals[named==4];return 0;};
                    h.heartbeat_ready=[](void*q){return int(static_cast<BarrierOwner*>(q)->heartbeat_monitors.ready());};
                    h.heartbeat_show=[](void*q,int32_t id,const char*resource){return static_cast<BarrierOwner*>(q)->heartbeat_monitors.show(id,resource);};
                    h.heartbeat_join=[](void*q,int32_t id,const char*resource,const char*joined,const char*left){return static_cast<BarrierOwner*>(q)->heartbeat_monitors.join(id,resource,joined,left);};
                    h.heartbeat_remove=[](void*q,int32_t id){return static_cast<BarrierOwner*>(q)->heartbeat_monitors.remove(id);};
                    h.medic_set_heals=[](void*q,int32_t named,int32_t count){auto&s=*static_cast<BarrierOwner*>(q);unsigned index=named==4;s.medical_heals[index]=count;
                        auto&scripts=s.world->script_services();return scripts.logic?uc_mission_flag_set(scripts.logic->level->remap,scripts.flags,scripts.flag_count,int32_t(16+index),uint8_t(count>0)):1;};
                    h.ensure_molotov_template=[](void*q,NativeMissionWorld&w,int32_t parent){return static_cast<BarrierOwner*>(q)->ensure_template(w,parent);};
                    h.ensure_brute_template=[](void*q,NativeMissionWorld&w,int32_t parent){auto&s=*static_cast<BarrierOwner*>(q);
                        const int32_t*ids=nullptr;uint32_t count=0;if(uc_entity_names_view(w.names(),"TEMPLATE_CLEAVER",&ids,&count))return 1;
                        if(count)return 0;s.cleaver_templates.push_back(std::make_unique<NativeCleaverTemplate>());
                        return s.cleaver_templates.back()->construct(w,parent);};
                    h.player_id=[](void*q){return static_cast<BarrierOwner*>(q)->player->entity.id;};
                    h.doors=&storage.doors;
                    h.refresh_collision=[](void*q){static_cast<BarrierOwner*>(q)->refresh_collision();return 0;};
                    h.tags=[](void*q,int32_t id,const char*name,bool add){auto&s=*static_cast<BarrierOwner*>(q);
                        return add?uc_entity_names_bind(s.world->names(),name,id):uc_entity_names_remove(s.world->names(),name,id);};
                    h.faction=[](void*q,int32_t id,unsigned faction,bool add){auto&s=*static_cast<BarrierOwner*>(q);
                        if(add)s.factions.add(faction,id);else s.factions.remove(faction,id);return 0;};
                    h.hazard=[](void*q,const float*position,unsigned kind,uint8_t*hit){return uc_character_hazards_query(static_cast<BarrierOwner*>(q)->hazards.get(),position,kind,hit);};
                    h.animation_events=[](void*q,int32_t id,const UCAnimationEventQueue&events,const NativeAnimatedModel&,const float*frame,const NativeAudioBank&){
                        auto&s=*static_cast<BarrierOwner*>(q);auto*target=uc_entity_event_lookup(&s.world->registry(),id);
                        auto*view=target?s.world->context().resolve(s.world->context().owner,target):nullptr;
                        if(!view||!view->actor||!s.audio||!s.impacts)return 1;
                        NativeCharacterFootstep step{frame,view->actor->group94};
                        for(uint32_t i=0;i<events.count;i++){const auto&r=events.events[i];if(!r.owner||r.index>=r.owner->event_count)return 1;
                            const char*type=r.owner->events[r.index].type;
                            if(!_stricmp(type,"FootstepLeft"))step.left=1;else if(!_stricmp(type,"FootstepRight"))step.right=1;else return 1;}
                        if(!step.left&&!step.right)return 0;
                        const NativeMissionCharacter*character=nullptr;for(const auto&c:s.characters)if(c->entity.id==id)character=c.get();if(!character)return 1;
                        struct Context{BarrierOwner*owner;const NativeMissionCharacter*character;UCCollisionResult hit{};} context{&s,character};
                        NativeCharacterFootstepServices hooks{};hooks.owner=&context;
                        hooks.bone=[](void*p,const char*name,float*out){if(!static_cast<Context*>(p)->character->posed_bone(name,out))throw std::runtime_error("NPC foot bone pose missing");};
                        hooks.query=[](void*p,const float*from,const float*to,uint32_t type,uint32_t group,uint32_t mask,float impulse)->const UCCollisionResult*{
                            auto&c=*static_cast<Context*>(p);int32_t entity=0;int hit=c.owner->character_world.query(from,to,type,group,mask,impulse,c.hit,entity);
                            if(hit<0)throw std::runtime_error("NPC footstep world query failed");return hit?&c.hit:nullptr;};
                        hooks.footprint=[](void*p,const UCPlayerFootprint&mark){auto&s=*static_cast<Context*>(p)->owner;s.impacts->footprint(s.character_world,mark);};
                        native_character_footstep(step,hooks);s.audio->character_footstep(step.material);
                        return s.play_sound(id,character->audio_bank_path(),"RunFoley");
                    };
                    return p->construct(*storage.renderer,world,index,storage.archive.get(),storage.archive_hash,storage.catalog,
                        storage.firstperson->math,*storage.random,*storage.vector_random,h);
                }
                if(!strcmp(metadata.definition->class_name,"ParticleSystem"))return storage.particles->construct_particle(world,index);
                if(!strcmp(metadata.definition->class_name,"AnimatingProp")||!strcmp(metadata.definition->class_name,"AnimatingPropIgnorePVS"))return storage.particles->construct_prop(world,index);
                if(!strcmp(metadata.definition->class_name,"LightFitting")){
                    if(!storage.lighting||!storage.random)return 1;
                    auto prop=std::make_unique<NativeLightFitting>();auto *p=prop.get();storage.light_fittings.push_back(std::move(prop));
                    if(world.bind(index,{&p->entity.view,p,[](void *b){return static_cast<NativeLightFitting*>(b)->update();},
                        [](void *b){return static_cast<NativeLightFitting*>(b)->late();},
                        [](void *b){return static_cast<NativeLightFitting*>(b)->close_detached();},nullptr}))return 1;
                    return p->construct(*storage.renderer,world,index,storage.archive.get(),storage.archive_hash,storage.catalog,*storage.lighting,*storage.random);
                }
                if(!strcmp(metadata.definition->class_name,"BasicProp")){
                    auto prop=std::make_unique<NativeBasicProp>();auto *p=prop.get();storage.static_props.push_back(std::move(prop));
                    if(world.bind(index,{&p->entity.view,p,[](void *b){return static_cast<NativeBasicProp*>(b)->update();},
                        [](void *b){return static_cast<NativeBasicProp*>(b)->late();},
                        [](void *b){return static_cast<NativeBasicProp*>(b)->close_detached();},&p->entity.script}))return 1;
                    return p->construct(*storage.renderer,world,index,storage.archive.get(),storage.archive_hash,storage.catalog);
                }
                if(!strcmp(metadata.definition->class_name,"RigidBody")){
                    auto prop=std::make_unique<NativeRigidProp>();auto *p=prop.get();storage.props.push_back(std::move(prop));
                    NativeRigidPropServices hooks{};hooks.owner=&storage;hooks.physics=storage.ragdoll_world.get();
                    hooks.particle=[](void*q,NativeRigidProp&p,const char*asset,const char*bone,uint8_t hidden){auto&s=*static_cast<BarrierOwner*>(q);
                        return s.npc_thrown_particles?s.npc_thrown_particles->append(asset,p,bone,hidden):1;};
                    hooks.sound=[](void*q,NativeRigidProp&p,const char*event){return static_cast<BarrierOwner*>(q)->play_sound(p.entity.id,p.parameters().sound,event);};
                    hooks.explosion=[](void*q,NativeRigidProp&,const UCRigidPropExplosion&r){auto&s=*static_cast<BarrierOwner*>(q);
                        s.explosions.push_back(std::make_unique<NativeMissionExplosion>());
                        return s.explosions.back()->construct(*s.world,r,*s.audio,s.archive.get(),s.texture_lookup,s.audio_stream,s.explosion_services());};
                    p->set_services(hooks);
                    if(world.bind(index,{&p->entity.view,p,[](void *b){return static_cast<NativeRigidProp*>(b)->update();},
                        [](void *b){return static_cast<NativeRigidProp*>(b)->late();},
                        [](void *b){return static_cast<NativeRigidProp*>(b)->close_detached();},&p->entity.script,
                        [](void *b){return static_cast<NativeRigidProp*>(b)->retire();},
                        [](void *b){return static_cast<NativeRigidProp*>(b)->close_detached();}}))return 1;
                    const int result=p->construct(*storage.renderer,world,index,storage.archive.get(),storage.archive_hash,storage.catalog,*storage.collision);
                    if(!result&&storage.renderer->profile_cpu)storage.record_prop(*p,0);
                    return result;
                }
                if(strcmp(metadata.definition->class_name,"SolidCollisionBlock"))return 1;
                auto body=std::make_unique<NativeSolidBlock>();auto *p=body.get();storage.bodies.push_back(std::move(body));
                if(world.bind(index,{&p->entity.view,p,[](void *b){return static_cast<NativeSolidBlock*>(b)->update();},
                    [](void *b){return static_cast<NativeSolidBlock*>(b)->late();},
                    [](void *b){return static_cast<NativeSolidBlock*>(b)->close_detached();},&p->entity.script,
                    [](void *b){return static_cast<NativeSolidBlock*>(b)->retire();},
                    [](void *b){return static_cast<NativeSolidBlock*>(b)->close_detached();}}))return 1;
                return p->construct(world.context(),world.active_descriptor(),metadata,world.script_services(),&storage.radius,&world);
            };
            services.helper_tick=[](void*owner,void*helper,float delta){auto&s=*static_cast<BarrierOwner*>(owner);
                auto loop=s.loop_sources.find(helper);if(loop!=s.loop_sources.end()){loop->second->step(delta);return 0;}
                for(const auto&c:s.characters)if(helper&&c->entity.helper==helper){s.audio->tick_empty_source((NativePlayerAudio::EmptySource*)helper,delta);return 0;}
                return s.handoff?s.handoff->helper_tick(helper,delta):1;};
            services.trigger_body=[](void*owner,const UCEntityEventTarget*target,UCMissionTriggerBody*out){auto&s=*static_cast<BarrierOwner*>(owner);
                // Mainplayer458CA8 and Character/CharacterCop47BF50 use2187A0.
                // Borrow the registered actor40/88 fields; never infer a bound
                //from a visible first-person weapon or a collision capsule.
                if(!out||!target)return 1;
                NativeMissionEntity *body=s.player&&target==&s.player->entity.identity?&s.player->entity:nullptr;
                for(const auto&c:s.characters)if(target==&c->entity.identity)body=&c->entity;
                if(!body||uc_entity_event_lookup(&s.world->registry(),body->id)!=target)return 1;
                *out={body->view.transform+12,&body->actor.distance88};return 0;};
            services.character_query=[](void*owner,int32_t id){auto&s=*static_cast<BarrierOwner*>(owner);
                for(const auto&c:s.characters)if(c->entity.id==id&&uc_entity_event_lookup(&s.world->registry(),id)==&c->entity.identity)return 0;
                return 1;};
            services.character_status=[](void*owner,int32_t id,uint32_t*status){auto&s=*static_cast<BarrierOwner*>(owner);
                if(!status)return 1;for(const auto&c:s.characters)if(c->entity.id==id&&uc_entity_event_lookup(&s.world->registry(),id)==&c->entity.identity){*status=c->status();return 0;}return 1;};
            if(mission_world.load(entities.value,level_logic.value,mission_flags.data(),mission_flags.size(),&barrier_owner.crt_seed,services))
                throw std::runtime_error("Native mission registry initialization failed");
            auto&doors=barrier_owner.doors;doors.owner=&barrier_owner;
            doors.simple_door=[](void*q){auto&s=*static_cast<BarrierOwner*>(q);const int32_t*ids=nullptr;uint32_t count=0;
                if(uc_entity_names_view(s.world->names(),"SIMPLE_DOOR",&ids,&count))throw std::runtime_error("Door template lookup failed");return count?ids[0]:0;};
            doors.position=[](void*q,int32_t id)->const float*{auto&s=*static_cast<BarrierOwner*>(q);auto*t=uc_entity_event_lookup(&s.world->registry(),id);
                auto*v=t?s.world->context().resolve(s.world->context().owner,t):nullptr;if(!v)throw std::runtime_error("Door target missing");return v->transform+12;};
            doors.property=[](void*q,int32_t id,const char*,uint8_t fallback){auto&s=*static_cast<BarrierOwner*>(q);auto*t=uc_entity_event_lookup(&s.world->registry(),id);
                auto*v=t?s.world->context().resolve(s.world->context().owner,t):nullptr;
                if(!v||*v->source_index<0)throw std::runtime_error("Door property owner unconnected");
                const char*name=s.world->metadata((uint32_t)*v->source_index).definition->class_name;
                if((s.particles&&s.particles->collision_owner(id))||!strcmp(name,"SolidCollisionBlock")||!strcmp(name,"BasicProp")||!strcmp(name,"RigidBody")||!strcmp(name,"LightFitting"))return fallback;
                throw std::runtime_error("Door class properties unconnected");};
            doors.open=[](void*q,int32_t target,int32_t sender){auto&s=*static_cast<BarrierOwner*>(q);uint8_t payload[2]={1,0},handled=0;uint32_t from=(uint32_t)sender;
                if(uc_entity_event_send(&s.world->registry(),target,44,payload,sizeof payload,&from,&handled)||!handled)throw std::runtime_error("Door open receiver unconnected");};
            doors.cast_template=[](void*,const float*,const float*,int32_t)->int32_t{throw std::runtime_error("SimpleDoor body cast unconnected");};
            level_particles->prepare_props(renderer,material_path.parent_path(),mission_world,true,equipment_preview,equipment_preview);
            for(uint32_t index:{496u,555u,1174u,1179u,1185u,1189u,1191u,1193u}){
                if(index>=entities.value.entity_count||entities.value.entities[index].active||mission_world.ensure(index))
                    throw std::runtime_error("Native authored smoke/mist/barrel initialization failed");
            }
            // Source-immediate barriers and authored projectile templates.
            // Full automatic startup still needs the remaining factories.
            for(uint32_t i=0;i<entities.value.entity_count;i++){
                const auto &e=entities.value.entities[i];
                if(!e.active&&e.class_name&&(!strcmp(e.class_name,"SolidCollisionBlock")||
                    !strcmp(e.class_name,"ThrownWeaponTemplateMolotov"))&&mission_world.ensure(i))
                    throw std::runtime_error("Native authored barrier/template initialization failed");
            }
            active_blocks.reserve(barrier_owner.bodies.size());
        }
        if(!present_loading(renderer))break;
        struct ShotContext { NativeIntegrationScene *scene;const UCCollisionAsset *collision;
            NativeIntegrationWeaponEffects *muzzle;NativeIntegrationImpacts *impact;UCGameRandom *random;UCVectorRandom *vector;
            NativeIntegrationActors *actors=nullptr;
            BarrierOwner *mission=nullptr;
            uint64_t actor_hits=0,melee_static_hits=0,melee_actor_hits=0;
        } shot_context{integrated_scene.get(),&collision.value,weapon_effects.get(),impacts.get(),&session_random,&vector_random};
        if(firstperson) {
            player_audio=std::make_unique<NativePlayerAudio>(level.parent_path(),uint8_t{16},headless);
            player_footsteps=std::make_unique<NativePlayerFootsteps>(collision.value,*player_audio);
            player_view=std::make_unique<NativeFirstperson>();
            player_view->mission_flags=mission_flags.data();player_view->mission_flag_count=mission_flags.size();
            player_view->audio=player_audio.get();
            player_view->footsteps=player_footsteps.get();
            player_view->load(renderer,level.parent_path(),material_path.parent_path(),entities.value,restart,restored?&restored->checkpoint:nullptr);
            if(integration){
                barrier_owner.player=&mission_player;barrier_owner.firstperson=player_view.get();
                barrier_owner.audio=player_audio.get();player_audio->prepare_character_footsteps(level.parent_path());
                uint8_t globals_hash[32];auto globals=read_archive(level.parent_path()/"globallanguage.lst",globals_hash);
                for(unsigned i=0;i<2;i++){auto*a=i?barrier_owner.archive.get():globals.get();const char*name=i?"1-01.exttxt":"globals.exttxt";
                    BytesOwner bytes;size_t entry=find_resource(a,name);if(entry==SIZE_MAX||uc_archive_read(a,entry,UC_AR_LOCAL_PAYLOAD,&bytes.value))throw std::runtime_error("Mission dialogue text resource");
                    barrier_owner.dialogue_text.load(bytes.value.data,bytes.value.size,0,i!=0);}
                barrier_owner.dialogue_audio=std::make_unique<NativeCharacterDialogueAudio>(*player_audio,barrier_owner.texture_lookup,level.parent_path()/"5.saf",uint8_t{16});
                barrier_owner.subtitles.load(level.parent_path(),renderer);
                barrier_owner.objective_messages.load(level.parent_path(),renderer,barrier_owner.dialogue_text,*player_audio);
                barrier_owner.heartbeat_monitors.load(level.parent_path(),renderer,barrier_owner.dialogue_text,barrier_owner.archive.get(),mission_world,session_random,
                    [](void*q){static_cast<NativePlayerAudio*>(q)->heartbeat();},player_audio.get());
                barrier_owner.heartbeat_monitors.message_provider(&barrier_owner,[](void*q,const char*resource){auto&s=*static_cast<BarrierOwner*>(q);return s.handoff?s.handoff->hud_message(resource):1;});
                for(uint32_t i=0;i<entities.value.entity_count;i++)if(!strcmp(entities.value.entities[i].class_name,"Mainplayer")&&mission_world.ensure(i))
                    throw std::runtime_error("Native player mission registration failed");
                if(!mission_player.player())throw std::runtime_error("Native player mission slot remained inactive");
                if(gameplay_camera.construct(mission_world,mission_player,renderer.width,renderer.height))
                    throw std::runtime_error("Native gameplay camera registration failed");
                camera_feedback=std::make_unique<NativeCameraFeedback>(mission_world,vector_random,physical_controller_port);
                if(camera_feedback->bind(*player_view))throw std::runtime_error("Native camera feedback binding failed");
                barrier_owner.camera_feedback=camera_feedback.get();
                barrier_owner.player_camera=&gameplay_camera;barrier_owner.scene=integrated_scene.get();
                barrier_owner.explosion_debris.open(renderer,barrier_owner.character_world,barrier_owner.texture_lookup,session_random,material_path.parent_path());
                std::memcpy(barrier_owner.published_camera,gameplay_camera.entity.view.transform,64);
                UCBulletTracersServices tracer_audio{};tracer_audio.owner=&barrier_owner;
                tracer_audio.listener=[](void*q,float*out){auto&s=*static_cast<BarrierOwner*>(q);if(!s.audio)return 0;
                    std::memcpy(out,s.published_camera+12,12);return 1;};
                tracer_audio.flyby=[](void*q,const float*point,const float*velocity){
                    for(unsigned i=0;i<3;i++)if(!std::isfinite(velocity[i]))return 1;
                    return static_cast<BarrierOwner*>(q)->world_sound("Banks\\Impacts.amp","Entry",0,point);};
                barrier_owner.bullet_tracers.open(renderer,barrier_owner.texture_lookup,material_path.parent_path(),tracer_audio);
                barrier_owner.bind_bullets();
            }
            auto entity_path=level;entity_path.replace_extension(".ent");uint8_t entity_hash[32];
            auto entity_archive=read_archive(entity_path,entity_hash);hud.load(entity_archive.get(),renderer);
            hud.load_inventory(level.parent_path(),renderer);
            hud.update_bindings(renderer,bindings);
            reticle.load(entity_archive.get(),renderer);
            hud.initialize_health(player_view->inventory.health,0,0);
            // One shared game stream for this deterministic fresh inspection
            // session. Future shots/AI must borrow it, never reseed a HUD RNG.
            if(uc_game_random_init(&session_random,0))throw std::runtime_error("Native session random initialization");
            if(integration) {
                player_view->random=&session_random;player_view->preview_shot_context=&shot_context;
                player_view->preview_shot=[](void *context,uint32_t item,const float origin[3],const float direction[3]) {
                    auto &s=*static_cast<ShotContext*>(context);
                    if(s.mission){
                        int result=s.mission->fire_primary(item,origin,direction);if(result)throw std::runtime_error("Player bullet failure code="+std::to_string(result)+" target="+std::to_string(s.mission->bullet_target)+" material="+std::to_string(s.mission->bullet_collision.material));
                        if(!s.muzzle->shot(item))return false;
                        ++s.scene->shots;return true;
                    }
                    if(!s.scene->shot(*s.collision,item,origin,direction))return false;
                    if(!s.muzzle->shot(item))return false;
                    if(s.actors){
                        const float length=std::sqrt(direction[0]*direction[0]+direction[1]*direction[1]+direction[2]*direction[2]);
                        float end[3];for(unsigned i=0;i<3;i++)end[i]=origin[i]+direction[i]*(50000.f/length);
                        NativeActorHit hit;
                        if(s.actors->raycast(origin,end,hit)&&(!s.scene->last_hit||hit.fraction<s.scene->last_query.fraction)){
                            if(s.scene->last_hit)--s.scene->hits;
                            s.scene->last_hit=false;++s.actor_hits;
                        }
                    }
                    if(s.scene->last_hit)s.impact->hit(*s.collision,s.scene->last_query,item,*s.random,*s.vector);
                    return true;
                };
                if(mission_preview)shot_context.mission=&barrier_owner;
                player_view->preview_melee_context=&shot_context;
                player_view->preview_melee=[](void *context,const float origin[3],const float end[3]){
                    auto &s=*static_cast<ShotContext*>(context);float from[3],to[3];
                    uc_collision_to_physics(origin,from);uc_collision_to_physics(end,to);UCCollisionRayHit wall{};
                    const int result=uc_collision_raycast(s.collision,from,to,&wall);if(result<0)return false;
                    NativeActorHit hit;
                    if(s.actors&&s.actors->raycast(origin,end,hit)&&(!result||hit.fraction<wall.fraction))++s.melee_actor_hits;
                    else s.melee_static_hits+=result!=0;
                    return true;
                };
            }
            heartbeat.load(renderer);
        }
        if(!present_loading(renderer))break;
        if(equipment_preview&&!mission_preview){
            // Original immediate setpiece models in Idle. Real mission flags
            // expose a pending action until its animation/audio chain is owned.
            for(uint32_t index:{45u,348u})if(mission_world.ensure(index))
                throw std::runtime_error("Native authored vehicle initialization failed at "+std::to_string(index));
            // Deliberate test activation at their own authored spawns. Retail
            // Westside defers these actors until mission scripts activate them.
            const uint32_t selected[]={1058,971,751};
            actors=std::make_unique<NativeIntegrationActors>(renderer,level.parent_path(),material_path.parent_path(),entities.value,selected,3,
                mission_flags.data(),mission_flags.size());
            // Supported visual/PC-physics targets of zone1 controller724.
            // Canister605 keeps its explosion-capable metadata; damage and
            // explosion endpoints are not connected by this preview.
            // Explicit test activation; full mission startup remains incomplete.
            for(uint32_t index:{447u,48u,703u,605u,557u,769u})if(mission_world.ensure(index))
                throw std::runtime_error("Native authored prop initialization failed at "+std::to_string(index));
            for(uint32_t index:{511u,591u,853u,908u,950u})if(mission_world.ensure(index))
                throw std::runtime_error("Native authored barrier/car initialization failed at "+std::to_string(index));
            // Immediate steady on/off fixtures only. Flicker, broken states and
            // mission-driven transitions require their remaining controllers.
            for(uint32_t index:{1196u,1197u,1198u,1204u,1211u,1213u,1225u,1229u,1232u,1233u,
                1240u,1247u,1249u,1252u,1256u,1258u,1265u,1266u,1270u,1273u,1231u,1254u})
                if(index>=entities.value.entity_count||entities.value.entities[index].active||mission_world.ensure(index))
                    throw std::runtime_error("Native authored light fitting initialization failed at "+std::to_string(index));
            shot_context.actors=actors.get();
            throwables=std::make_unique<NativeIntegrationThrowables>(renderer,level.parent_path(),material_path.parent_path(),material_path.parent_path(),collision.value);
            player_view->enable_preview_throwables(renderer,level.parent_path(),material_path.parent_path(),10);
            player_view->preview_throw_context=throwables.get();
            player_view->preview_throw=[](void *context,const float frame[16],float aim_pitch,float charge){
                return static_cast<NativeIntegrationThrowables*>(context)->throw_molotov(frame,aim_pitch,charge);
            };
        }
        if(mission_preview){
            level_particles->prepare_helicopters(*player_audio,material_path.parent_path());
            handoff=std::make_unique<NativeMissionHandoff>(mission_world,mission_player,gameplay_camera,renderer,*player_audio,
                barrier_owner.archive.get(),barrier_owner.archive_hash,material_path.parent_path());barrier_owner.handoff=handoff.get();
            handoff->bind_checkpoint_state(barrier_owner.objective_saved,barrier_owner.difficulty);
            if(restored){
                handoff->checkpoints.selected=restored->checkpoint.number;handoff->snapshot=restored->checkpoint;
                handoff->snapshots=1;handoff->flashlight=restored->checkpoint.flag9;
                handoff->item_extra[22]=restored->checkpoint.extra22;
                std::memcpy(handoff->mission,restored->checkpoint.mission,sizeof handoff->mission);
                std::memcpy(handoff->statistics,restored->checkpoint.statistics,sizeof handoff->statistics);
            }
            handoff->load_feedback(level.parent_path(),barrier_owner.dialogue_text);
            if(barrier_owner.bind_player_damage())throw std::runtime_error("Native player damage binding failed");
            // Original immediate checkpoint registration order. Number0 starts
            //the intro; number1 must already exist when270 sends its operation.
            for(uint32_t i=0;i<entities.value.entity_count;i++){
                const auto&e=entities.value.entities[i];
                if(!e.active&&!std::strcmp(e.class_name,"RestartPoint")&&mission_world.ensure(i))
                    throw std::runtime_error("Mission checkpoint construction failed");
            }
            //211820 registers custom volumes after the ordinary source slots,
            //even when their script-controlled enable flag is initially clear.
            for(uint32_t i=0;i<level_logic.value.trigger_count;i++)
                if((level_logic.value.triggers[i].flags&32)&&mission_world.ensure_trigger(i))
                    throw std::runtime_error("Mission trigger construction failed at "+std::to_string(i));
            if(mission_world.complete_startup())throw std::runtime_error("Mission startup frame publication failed");
        }
        if(!present_loading(renderer))break;
        // Input, mission pitch reset and camera publication share the player's
        // actual angles. A second local pitch used to overwrite event0x22.
        float &pitch=player_view?player_view->actor.angles[0]:inspection_pitch;
        float &yaw=player_view?player_view->actor.angles[1]:inspection_yaw;
        unsigned movement_steps=0,equipment_seen=0,taser_ticks=0,taser_loop_ticks=0,shield_ticks=0;
        unsigned explosion_index=0;float explosion_cooldown=0;
        uint64_t recorded_snapshot=restored?1:0;
        std::optional<NativeSaveRecord> checkpoint_record=restored_record;
        auto save_checkpoint=[&](bool automatic){
            if(!save_store||!checkpoint_record){
                if(!automatic)frontend.set_status("No checkpoint reached yet. Progress saves at checkpoints.");
                return;
            }
            try{
                save_store->save(save_slot,*checkpoint_record);
                if(!automatic)frontend.set_status("Checkpoint saved to slot "+std::to_string(save_slot)+".");
            }catch(const std::exception &e){
                if(headless)throw;
                if(automatic){paused=true;frontend.show(NativeFrontend::Screen::pause);pointer_mode();}
                frontend.set_status(std::string("Save failed; checkpoint remains in memory. ")+e.what());
            }
        };
        UCExplosionField explosion_field{};explosion_field.done=1;
        const uint32_t explosion_sources[]={214,306,393,222,375,501};
        float accumulator=0,eye=170,previous_eye=170,presentation_eye=170,presentation_fraction=1;UCPlayer previous_player=player;
        unsigned movement_ramp_checks=0,movement_release_checks=0,movement_crouch_checks=0;
        auto step_player=[&](const UCPlayerInput &input) {
            auto effective=input;
            previous_player=player;previous_eye=eye;
            if(explosion_preview){
                explosion_cooldown=std::max(0.f,explosion_cooldown-1.f/30.f);
                if(explosion_requested&&explosion_cooldown==0){
                    const auto index=explosion_sources[explosion_index];
                    if(index>=entities.value.entity_count)throw std::runtime_error("Explosion preview descriptor missing");
                    impacts->preview_explosion(entities.value.entities[index]);
                    UCExplosionParameters parameters{};
                    if(uc_explosion_parameters(&entities.value.entities[index],&parameters))throw std::runtime_error("Explosion preview properties invalid");
                    explosion_field={};explosion_field.power=parameters.power;explosion_field.remaining=2;
                    uc_collision_to_physics(entities.value.entities[index].transform+12,explosion_field.center);
                    explosion_index=(explosion_index+1)%6;explosion_cooldown=12;
                }
                explosion_requested=false;
                if(!explosion_field.done&&native_prop_explosion_step(explosion_field,1.f/30.f,barrier_owner.props))
                    throw std::runtime_error("Explosion preview prop impulse failed");
            }
            if(integration){
                const auto world_start=profile_cpu?ProfileClock::now():ProfileClock::time_point{};
                if(mission_world.tick(1.f/30.f))throw std::runtime_error("Native mission operation failed: source "+std::to_string(mission_world.failure_source())+
                    " class "+mission_world.failure_class()+" phase "+mission_world.failure_phase()+" opcode "+std::to_string(mission_world.failure_opcode()));
                if(profile_cpu)profile_world+=std::chrono::duration<double,std::milli>(ProfileClock::now()-world_start).count();
                barrier_owner.refresh_collision();barrier_owner.explosion_debris.step(1.f/30.f);
                if(camera_feedback&&camera_feedback->step())throw std::runtime_error("Native camera feedback update failed");
                barrier_owner.subtitles.step(mission_world);
                for(auto i=barrier_owner.character_sounds.begin();i!=barrier_owner.character_sounds.end();)
                    if(!uc_entity_event_lookup(&mission_world.registry(),i->id))i=barrier_owner.character_sounds.erase(i);else{i->source->step(1.f/30.f);++i;}
                for(auto&sound:barrier_owner.world_sounds)if(sound.second)sound.second->step(1.f/30.f);
                if(player_audio)player_audio->step_groups(mission_world.time());
                barrier_owner.objective_messages.step(1.f/30.f);
                barrier_owner.heartbeat_monitors.step(1.f/30.f);
                if(handoff&&handoff->step_feedback(1.f/30.f))throw std::runtime_error("Checkpoint feedback step incomplete");
                if(handoff&&handoff->step_fade(1.f/30.f))throw std::runtime_error("Mission fade update failed");
                if(handoff&&handoff->snapshots!=recorded_snapshot){
                    NativeSaveRecord record;std::memcpy(record.level_hash.data(),barrier_owner.archive_hash,32);
                    record.checkpoint=handoff->snapshot;record.completed_objectives=barrier_owner.objective_saved.saved_completed();
                    record.difficulty=barrier_owner.difficulty;checkpoint_record=std::move(record);recorded_snapshot=handoff->snapshots;
                    if(frontend_enabled||save_roundtrip_check)save_checkpoint(true);
                }
                active_blocks.clear();for(const auto &body:barrier_owner.bodies)if(body->geometry())active_blocks.push_back(body->geometry());
                active_barrier_count=active_blocks.size();if(level_particles)level_particles->append_blocks(active_blocks);
                for(const auto &prop:barrier_owner.props)for(const auto &box:prop->boxes())active_blocks.push_back(&box);
                for(const auto &prop:barrier_owner.static_props)for(const auto &box:prop->boxes())active_blocks.push_back(&box);
                for(const auto &prop:barrier_owner.light_fittings)for(const auto &box:prop->boxes())active_blocks.push_back(&box);
            }
            if(actors)actors->step(renderer,1.f/30.f);
            if(player_view&&!player_view->accepts_input()){
                effective.forward=effective.right=0;effective.crouch=player.crouched!=0;
                pending_weapon=UINT32_MAX;pending_reload=false;pending_flashlight=pending_secondary=false;fire_pressed=false;
            }
            if(integration){player_view->set_fire(fire_held||fire_pressed);player_view->set_shield(shield_held);player_view->aim_held=aim_held&&player_view->accepts_input();fire_pressed=false;}
            if(player_view&&pending_weapon!=UINT32_MAX) {
                player_view->select_weapon(renderer,pending_weapon);
                pending_weapon=UINT32_MAX;
            }
            if(player_view&&pending_reload){player_view->request_reload(renderer);pending_reload=false;}
            if(player_view&&pending_secondary){
                if(player_view->flashlight_compatible())pending_flashlight=!pending_flashlight;
                else if(player_view->inventory.active_item==17)player_view->shotgun_secondary=!player_view->shotgun_secondary;
                pending_secondary=false;
            }
            if(player_view&&handoff){if(pending_flashlight)handoff->flashlight^=1;pending_flashlight=false;player_view->set_flashlight(handoff->flashlight!=0);}
            if(player_view){
                if(player_view->accepts_input()&&player_view->shield_input_response(effective.forward,effective.right))throw std::runtime_error("Shield input response incomplete");
                const auto resolved=player_view->before_physics(renderer,effective.forward,effective.right,player.crouched!=0,1.f/30.f);
                // Keyboard release stops the capsule immediately. Keep the original
                // smoother alive for weapon presentation without a movement tail.
                effective.forward=effective.forward!=0?resolved.movement[0]:0;
                effective.right=effective.right!=0?resolved.movement[1]:0;
            }
            const auto physics_start=profile_cpu?ProfileClock::now():ProfileClock::time_point{};
            auto status=uc_player_step_blocks(&collision.value,active_blocks.data(),active_blocks.size(),&effective,&player);
            if(profile_cpu)profile_physics+=std::chrono::duration<double,std::milli>(ProfileClock::now()-physics_start).count();
            if(status!=UC_PLAYER_OK)throw std::runtime_error("Native static-world controller could not resolve this position (status "+std::to_string(status)+")");
            if(particle_push_check&&player_view&&player_view->accepts_input()){
                if(input.forward==1&&!input.right&&effective.forward>0&&effective.forward<.99f&&!player.crouched)++movement_ramp_checks;
                if(!input.forward&&!input.right){
                    if(effective.forward!=0||effective.right!=0)throw std::runtime_error("Released movement still drives player physics");
                    ++movement_release_checks;
                }
                if(input.forward==1&&!input.right&&previous_player.crouched){
                    // Check the controller's requested motion; contact projection can
                    // add downhill velocity from gravity after this input is applied.
                    if(effective.forward>.50001f||std::abs(effective.right)>.30001f)throw std::runtime_error("Crouch reduction did not reach player physics");
                    ++movement_crouch_checks;
                }
            }
            for(const auto&prop:barrier_owner.props)if(prop->ready()&&prop->player_contact(player,effective))throw std::runtime_error("Player prop contact failed");
            float velocity[3];uc_collision_to_render(player.velocity,velocity);
            UCPlayerEyeState state=UC_PLAYER_EYE_STANDING;
            if(player.crouched)state=sqrtf(velocity[0]*velocity[0]+velocity[1]*velocity[1]+velocity[2]*velocity[2])>50?UC_PLAYER_EYE_CROUCH_MOVING:UC_PLAYER_EYE_CROUCH_IDLE;
            eye=uc_player_eye_height(eye,state,1.f/30.f);
            if(player_view) {
                float completed[3];uc_collision_to_render(player.position,completed);
                if(scene_ambient)scene_ambient->step(completed,1.f/30.f);
                if(integration&&mission_player.snapshot_motion())throw std::runtime_error("Player motion snapshot failed");
                player_view->after_physics(completed,eye,pitch,yaw,player.crouched!=0,1.f/30.f);
                if(integration&&mission_player.after_camera_response())throw std::runtime_error("Player concussion/pain response incomplete");
                if(integration)integrated_scene->publish_blur();
                if(weapon_effects)weapon_effects->step(*player_view,1.f/30.f,session_random);
                if(impacts)impacts->step(1.f/30.f,session_random);
                if(throwables)throwables->step(1.f/30.f,session_random);
                if(level_particles)level_particles->step(1.f/30.f,session_random);
                if(barrier_owner.npc_thrown_particles)barrier_owner.npc_thrown_particles->step(1.f/30.f,session_random);
                if(integration)barrier_owner.bullet_tracers.step(1.f/30.f);
                current_weapon=(uint32_t)player_view->inventory.active_item;
                equipment_seen|=1u<<current_weapon;
                taser_ticks+=current_weapon==1;
                taser_loop_ticks+=current_weapon==1&&player_view->current_animation()==13;
                shield_ticks+=current_weapon==0;
                hud.update_health(player_view->inventory.health,1.f/30.f);
                reticle.update(player_view->inventory,player_view->action,1.f/30.f,8);
                heartbeat.update({player_view->inventory.health,player_view->health_maximum,1,1},1.f/30.f,
                    session_random,[](void *owner){static_cast<NativePlayerAudio*>(owner)->heartbeat();},player_audio.get());
            }
            ++movement_steps;
            if(profile_cpu&&integration&&mission_world.frame()%30==0)for(const auto&p:barrier_owner.props)if(p->ready())barrier_owner.record_prop(*p,1);
        };
        if(player_view) {
            UCPlayerInput initial{};initial.yaw=yaw;initial.speed=2.5f;step_player(initial);
            previous_player=player;previous_eye=eye;
            uc_collision_to_render(player.position,body);position={body[0],body[1]+eye,body[2]};
            presentation_eye=eye;
        }
        if(!present_loading(renderer))break;
        if(walk_check||idle_check) {
            for(unsigned step=0;step<(walk_check?510u:120u);++step) {
                UCPlayerInput input{};input.yaw=yaw;input.speed=2.5f;
                if(walk_check) {
                    if(step>=90&&step<180)input.forward=1;
                    if(step>=180&&step<270)input.right=1;
                    if(step>=270&&step<360)input.forward=-1;
                    if(step>=360&&step<450)input.right=-1;
                    input.crouch=step>=450&&step<480;
                }
                step_player(input);
            }
            uc_collision_to_render(player.position,body);position={body[0],body[1]+eye,body[2]};
            presentation_eye=eye;
        }
        auto previous=std::chrono::steady_clock::now(); bool running=true; unsigned frames=0;
        const unsigned final_check_frame=equipment_check?719u:secondary_check?479u:359u;
        auto fps_start=previous;unsigned fps_frames=0;float shown_fps=0;
        uint64_t captured_shots=0;size_t shot_effect_pixels=0,impact_visible_pixels=0,actor_visible_pixels=0,throwable_visible_pixels=0,explosion_visible_pixels=0;
        unsigned control_events=0;UCPlayer control_body{};
        unsigned handoff_play_ticks=0,handoff_return_frame=0,handoff_items=0;size_t flashlight_pixels=0;
        int32_t chase_id=-1;float chase_origin[3]{},chase_distance=0;bool chase_spawn=false,chase_script=false,chase_deleted=false;
        std::ofstream chase_trace;
        if(mission_placement_check){std::filesystem::create_directories(output);chase_trace.open(output/"placement-route.csv");
            chase_trace<<"tick,source,status,blocker,phase,node,busy,x,y,z,goal_x,goal_y,goal_z,waypoint_x,waypoint_y,waypoint_z\n";}
        float dispatch_return[3]{};
        float handoff_origin[3]{},handoff_distance=0,fire_check_position[3]{};
        bool handoff_look=false,handoff_crouch=false;
        bool title_paused=!paused,title_focused=!focused;
        NativeFramePacer frame_pacer;bool audio_paused=false;
        if(!present_loading(renderer))break;
        if(frontend_enabled){mission_active=true;paused=false;frontend.show(NativeFrontend::Screen::closed);pointer_mode();}
        while(running) {
            if(frontend_enabled||dispatch_check)frame_pacer.wait(dispatch_check?60:graphics.value.frame_limit);
            MSG message{}; while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)) {
                if(message.message==WM_QUIT){running=false;break;} TranslateMessage(&message);DispatchMessageW(&message);
            }
            if(!running) break;
            if(frontend_enabled){
                const auto action=apply_frontend_event(renderer);
                if(action==NativeFrontendAction::save_checkpoint)save_checkpoint(false);
                if(action==NativeFrontendAction::quit)break;
                if(action==NativeFrontendAction::main_menu){return_to_menu=true;break;}
                hud.update_bindings(renderer,bindings);
            }
            if(player_audio&&!headless){const bool suspend=paused||!focused||IsIconic(app_window);
                if(suspend!=audio_paused){player_audio->pause(suspend);audio_paused=suspend;}}
            if(display_failed)throw std::runtime_error("Display mode change failed");
            if(resize_pending&&app_window) { if(renderer.width!=display.width||renderer.height!=display.height)renderer.resize(display.width,display.height);pointer_mode();resize_pending=false; }
            if(app_window&&IsIconic(app_window)){WaitMessage();previous=std::chrono::steady_clock::now();continue;}
            auto now=std::chrono::steady_clock::now(); float dt=std::min(.1f,std::chrono::duration<float>(now-previous).count());previous=now;
            const auto profile_start=profile_cpu?ProfileClock::now():ProfileClock::time_point{};profile_world=profile_physics=0;
            if(benchmark)timing.begin(renderer,frames);
            if(handoff_check){
                dt=1.f/30.f;presentation_fraction=1;UCPlayerInput input{};input.yaw=yaw;input.speed=2.5f;
                if(handoff->returns||(restored&&handoff->checkpoints.ready(mission_world,&handoff->session_mode))){
                    if(!handoff_play_ticks){std::memcpy(handoff_origin,player.position,sizeof(handoff_origin));handoff_return_frame=frames;}
                    input.forward=handoff_play_ticks<30?1.f:0.f;
                    if(handoff_play_ticks==30){const float old_pitch=pitch,old_yaw=yaw;
                        ucrr_pc_rotate(&pitch,&yaw,player_view->motion.pitch_limit.current,30,-10,100);
                        handoff_look=pitch!=old_pitch&&yaw!=old_yaw;input.yaw=yaw;}
                    if(npc943_check&&handoff_play_ticks==45){if(mission_world.ensure(943))throw std::runtime_error("NPC943 test spawn failed");}
                    if(particle_push_check&&handoff_play_ticks==60){if(mission_world.ensure(1309))throw std::runtime_error("Block1309 activation failed");}
                    if(particle_push_check&&handoff_play_ticks==61){
                        auto*b=mission_world.binding(1309);if(!b||!b->view)throw std::runtime_error("Block1309 missing before removal");
                        *b->view->target.flags|=1;
                    }
                    if(particle_push_check&&handoff_play_ticks==62){
                        if(mission_world.binding(1309))throw std::runtime_error("Block1309 still registered after removal");
                        for(const auto&b:barrier_owner.bodies)if(b->entity.source==1309&&(b->geometry()||b->entity.aux))throw std::runtime_error("Block1309 retained collision or actor resources");
                        std::ofstream(output/"block-removal.json")<<"{\"passed\":true,\"source\":1309,\"collision_removed\":true}\n";
                    }
                    if(particle_push_check&&handoff_play_ticks==300){
                        for(unsigned source:{447u,605u}){auto*b=mission_world.binding(source);if(!b||!b->view)throw std::runtime_error("Prop missing before removal: "+std::to_string(source));*b->view->target.flags|=1;}
                    }
                    if(particle_push_check&&handoff_play_ticks==301){
                        for(unsigned source:{447u,605u})if(mission_world.binding(source))throw std::runtime_error("Prop still registered after removal");
                        for(const auto&p:barrier_owner.props)if((p->entity.source==447||p->entity.source==605)&&(p->ready()||!p->boxes().empty()||p->entity.aux))throw std::runtime_error("Prop retained resources");
                        UCExplosionField field{};field.remaining=2;field.power=7500;std::memcpy(field.center,player.position,12);
                        if(native_prop_explosion_step(field,1.f/30.f,barrier_owner.props))throw std::runtime_error("Explosion after prop retirement failed");
                        std::ofstream(output/"prop-retirement.json")<<"{\"passed\":true,\"sources\":[447,605],\"explosion_after_retirement\":true}\n";
                    }
                    if(particle_push_check&&handoff_play_ticks==45){
                        const auto count=level_particles->counts().active;int32_t id=-1,again=-1;
                        if(mission_world.ensure(1177,&id))throw std::runtime_error("Deferred snow factory failed: "+std::string(mission_world.failure_phase()));
                        if(id<0||mission_world.ensure(1177,&again)||id!=again||
                           level_particles->counts().active!=count+1)throw std::runtime_error("Deferred snow activation check failed counts "+std::to_string(count)+"/"+std::to_string(level_particles->counts().active)+" ids "+std::to_string(id)+"/"+std::to_string(again));
                        auto at=std::find_if(barrier_owner.props.begin(),barrier_owner.props.end(),[](const auto&p){return p->ready()&&!p->parameters().unmovable&&!p->boxes().empty();});
                        if(at==barrier_owner.props.end()||(*at)->boxes().empty()||(*at)->parameters().unmovable)throw std::runtime_error("Push check box missing");
                        auto&p=**at;auto box=p.boxes()[0];XMFLOAT3 axis;
                        XMStoreFloat3(&axis,XMVector3Rotate(XMVectorSet(1,0,0,0),XMLoadFloat4((const XMFLOAT4*)box.rotation)));
                        UCPlayer probe{};for(unsigned k=0;k<3;k++)probe.position[k]=box.center[k];
                        probe.position[0]+=axis.x*(box.half[0]+box.radius+UC_PLAYER_CAPSULE_RADIUS+.0001f);
                        probe.position[1]+=axis.y*(box.half[0]+box.radius+UC_PLAYER_CAPSULE_RADIUS+.0001f);probe.position[2]-=.8f;
                        UCPlayerInput push{1,0,std::atan2(axis.x,-axis.y),2.5f,0};
                        if(uc_player_step_blocks(&collision.value,active_blocks.data(),active_blocks.size(),&push,&probe)!=UC_PLAYER_OK)
                            throw std::runtime_error("Push check player contact resolution failed");
                        float before[3];memcpy(before,p.entity.transform+12,12);
                        if(p.player_contact(probe,push))throw std::runtime_error("Push check contact failed");
                        barrier_owner.ragdoll_world->step(1.f/30.f);for(const auto&prop:barrier_owner.props)if(prop->ready())prop->sync_physics();
                        if(p.update())throw std::runtime_error("Push check update failed");
                        float dx=p.entity.transform[12]-before[0],dz=p.entity.transform[14]-before[2];
                        if(dx*dx+dz*dz<.01f)throw std::runtime_error("Player contact did not move retained box");
                        std::ofstream(output/"particle-push.json")<<"{\"passed\":true,\"particle_source\":1177,\"particle_id\":"<<id
                            <<",\"emitters_before\":"<<count<<",\"emitters_after\":"<<level_particles->counts().active
                            <<",\"prop_source\":"<<p.entity.source<<",\"prop_displacement_cm\":"<<std::hypot(dx,dz)<<",\"resolved_player_contact\":true}\n";
                    }
                    if(fire_check&&handoff_play_ticks==40){
                        const auto&t=level_logic.value.triggers[shake_check?362:5];float box[6];
                        if(t.kind!=(shake_check?17u:37u)||t.shape!=3||t.boxes.size!=sizeof box)throw std::runtime_error("Truck explosion trigger changed");
                        memcpy(box,t.boxes.data,sizeof box);float target[3];for(unsigned k=0;k<3;k++)target[k]=(box[k]+box[k+3])*.5f;
                        uc_collision_to_physics(target,player.position);memset(player.velocity,0,sizeof player.velocity);previous_player=player;
                    }
                    if(shake_check){for(const auto&shake:barrier_owner.camera_shakes)if(shake->entity.source==171){
                        const auto*observers=uc_feedback_manager_observers(camera_feedback->manager());bool registered=false;
                        for(uint32_t i=0;i<observers->position_count;i++)registered|=observers->position[i]==shake->observer();
                        if(registered!=shake->active())throw std::runtime_error("Shake receiver membership mismatch");
                        if(shake->active()){
                            double value=0;if(shake->observer()->sample(shake->observer()->owner,mission_player.entity.view.transform+12,&value)||!std::isfinite(value))throw std::runtime_error("Shake spatial receiver failed");
                            shake_sampled|=value>0;shake_peak=std::max(shake_peak,shake->amplitude());
                            shake_faded|=shake_peak>.2f&&shake->amplitude()<.1f;
                        }else if(shake_sampled)shake_released=true;
                        if(shake_released&&handoff_play_ticks<300&&shake->active())throw std::runtime_error("Completed shake restarted without a flag edge");
                    }}
                    if(shake_check&&handoff_play_ticks==300){
                        if(!shake_released||barrier_owner.camera_shakes.size()!=1)throw std::runtime_error("Shake did not finish before deletion check");
                        barrier_owner.camera_shakes.front()->entity.flags|=1;
                    }
                    if(shake_check&&handoff_play_ticks==304){
                        if(!barrier_owner.camera_shakes.empty()||uc_feedback_manager_observers(camera_feedback->manager())->position_count)throw std::runtime_error("Shake gameplay destruction retained receiver/owner");
                    }
                    if(fire_check&&handoff_play_ticks==250){
                        memcpy(fire_check_position,player.position,sizeof fire_check_position);
                        const auto&d=entities.value.entities[563];float point[3];memcpy(point,d.transform+12,12);point[0]+=10;
                        uc_collision_to_physics(point,player.position);memset(player.velocity,0,sizeof player.velocity);previous_player=player;
                    }
                    if(fire_check&&handoff_play_ticks==253){memcpy(player.position,fire_check_position,sizeof fire_check_position);memset(player.velocity,0,sizeof player.velocity);previous_player=player;}
                    if(handoff_play_ticks==60)pending_weapon=2;
                    if(handoff_play_ticks==140)pending_reload=true;
                    if(weapon_secondary_check){
                        aim_held=handoff_play_ticks>=80&&handoff_play_ticks<130;
                        if(handoff_play_ticks==95&&player_view->motion.weapon.translation[0].target>-10.f)throw std::runtime_error("Aim did not reach original aimed weapon offset: item="+std::to_string(player_view->inventory.active_item)+" state="+std::to_string(player_view->action.state)+" target="+std::to_string(player_view->motion.weapon.translation[0].target));
                        if(handoff_play_ticks==135&&std::fabs(player_view->motion.weapon.translation[0].target)>10.f)throw std::runtime_error("Aim release did not restore hip offset");
                        if(handoff_play_ticks==120||handoff_play_ticks==160||handoff_play_ticks==220||handoff_play_ticks==250)pending_secondary=true;
                        if(handoff_play_ticks==121&&!handoff->flashlight)throw std::runtime_error("Secondary did not enable pistol flashlight");
                        if(handoff_play_ticks==161&&handoff->flashlight)throw std::runtime_error("Secondary did not disable pistol flashlight");
                        if(handoff_play_ticks==221&&!player_view->shotgun_secondary)throw std::runtime_error("Secondary did not enable shotgun mode");
                        if(handoff_play_ticks==251&&(player_view->shotgun_secondary||!player_view->secondary_shots))throw std::runtime_error("Secondary shotgun shot/toggle failed");
                    }
                    if(npc_hit_check&&(handoff_play_ticks==120||handoff_play_ticks==160))pending_flashlight=true;
                    if(handoff_play_ticks==180)pending_weapon=17;
                    fire_held=(handoff_play_ticks>=100&&handoff_play_ticks<115)||(handoff_play_ticks>=230&&handoff_play_ticks<245);
                    input.crouch=handoff_play_ticks>=270&&handoff_play_ticks<300;
                    if(particle_push_check&&input.crouch)input.forward=1;
                    if(npc_hit_check&&handoff_play_ticks==5){auto effects=barrier_owner.explosion_services();
                        const float bounds[12]={-100000000.f,-100000000.f,-100000000.f,-99999000.f,-99999000.f,-99999000.f};
                        if(effects.area_effect(effects.owner,bounds,2))throw std::runtime_error("Explosion empty-area regression failed");}
                    // Explicit receiver fixture: use two real authored enemies after
                    // the opening police fight. This does not verify wave triggers.
                    if(npc_hit_check&&(handoff_play_ticks==60||handoff_play_ticks==200))barrier_owner.spawn_check_npc(handoff_play_ticks==60?356u:646u);
                    if(npc_hit_check&&(handoff_play_ticks==80||handoff_play_ticks==92))barrier_owner.check_npc_hit(handoff_play_ticks==92);
                    if(npc_hit_check&&handoff_play_ticks==82){auto*c=barrier_owner.character(barrier_owner.checked_npc);
                        if(!c||c->entity.actor.health9C>=barrier_owner.checked_health||c->entity.actor.health9C<=0)throw std::runtime_error("Body hit health check failed");}
                    if(npc_hit_check&&handoff_play_ticks==94){auto*c=barrier_owner.character(barrier_owner.checked_npc);
                        if(!barrier_owner.ragdoll_world||barrier_owner.ragdoll_world->ragdoll_body_count()<12)throw std::runtime_error("Headshot did not create articulated ragdoll bodies");
                        if(!c||c->entity.actor.health9C>0||!(c->status()&1)||!handoff->statistics[2]||!handoff->statistics[1])throw std::runtime_error("Headshot health/death/stat check failed hp="+std::to_string(c?c->entity.actor.health9C:-999)+" status="+std::to_string(c?c->status():0)+" headshots="+std::to_string(handoff->statistics[2])+" kills="+std::to_string(handoff->statistics[1]));}
                    if(npc_hit_check&&handoff_play_ticks==220)barrier_owner.check_npc_hit(true,true);
                    if(npc_hit_check&&(handoff_play_ticks==110||handoff_play_ticks==240))barrier_owner.check_corpse_hit();
                    if(npc_hit_check&&handoff_play_ticks==222){auto*c=barrier_owner.character(barrier_owner.checked_npc);
                        if(!barrier_owner.ragdoll_world||barrier_owner.ragdoll_world->ragdoll_body_count()<24)throw std::runtime_error("Second headshot did not retain independent ragdoll bodies");
                        if(!c||c->entity.actor.health9C>0||!(c->status()&1)||handoff->statistics[2]<2||handoff->statistics[1]<2)throw std::runtime_error("Shotgun headshot health/death/stat check failed");}
                    if(npc_hit_check&&handoff_play_ticks==260)barrier_owner.spawn_check_npc(474);
                    if(npc_hit_check&&(handoff_play_ticks==270||handoff_play_ticks==275))barrier_owner.check_npc_blast(true);
                    if(npc_hit_check&&handoff_play_ticks==280)barrier_owner.check_npc_blast();
                    if(npc_hit_check&&handoff_play_ticks==282){auto*c=barrier_owner.character(barrier_owner.checked_npc);
                        if(!c||c->entity.actor.health9C>0||!(c->status()&1)||barrier_owner.ragdoll_world->ragdoll_body_count()<36)
                            throw std::runtime_error("Explosion did not create independent ragdoll");
                        if(barrier_owner.ragdoll_world->ragdoll_body_count()!=barrier_owner.restrained_bodies)
                            throw std::runtime_error("Killing restrained character recreated its ragdoll");}
                    // Separate connection check:210770 sets a LOCAL authored flag,
                    // not a public remapped flag. Explicitly construct the authored
                    // medic; this does not verify its wave or spatial spawn trigger.
                    if(dispatch_check&&handoff_play_ticks>=900&&handoff_play_ticks<=904){
                        if(handoff_play_ticks==900){if(mission_world.ensure(373)||mission_world.ensure(425))throw std::runtime_error("Dispatch queue construction failed");}
                        else{NativeDispatch *first=nullptr,*second=nullptr;
                            for(auto&d:barrier_owner.dispatches){if(d->entity.source==373)first=d.get();if(d->entity.source==425)second=d.get();}
                            if(!first||!second)throw std::runtime_error("Dispatch queue owners missing");
                            if(handoff_play_ticks==901){if(!first->sent()||second->sent())throw std::runtime_error("Dispatch FIFO arbitration failed");first->entity.flags|=1;}
                            if(handoff_play_ticks==903){if(!second->sent()||first->completed()||uc_entity_event_lookup(&mission_world.registry(),first->entity.id))throw std::runtime_error("Dispatch delete/queue advance failed");handoff->world_flags|=0x400;}
                            if(handoff_play_ticks==904){if(second->completed()||barrier_owner.active_dispatch!=-1||uc_entity_event_lookup(&mission_world.registry(),second->entity.id))throw std::runtime_error("Dispatch mission cancellation failed");handoff->world_flags&=~0x400u;}
                        }
                    }
                    if(medic_sequence_check&&natural_trigger_check&&handoff_play_ticks%30==0){
                        std::ofstream trace(output/"dispatch-trace.csv",std::ios::app);
                        trace<<handoff_play_ticks<<","<<int(mission_flags[176]);
                        for(const auto&d:barrier_owner.dispatches)trace<<","<<d->entity.source<<":"<<d->sent()<<":"<<d->playing()<<":"<<d->completed();
                        trace<<"\n";
                    }
                    if(dispatch_check&&handoff_play_ticks==330)memcpy(dispatch_return,player.position,12);
                    if(dispatch_check&&handoff_play_ticks==420){memcpy(player.position,dispatch_return,12);memset(player.velocity,0,sizeof player.velocity);previous_player=player;}
                    if(natural_trigger_check){
                        const uint32_t trigger=mission_placement_check&&handoff_play_ticks==1?331u:
                            medic_sequence_check&&handoff_play_ticks==330?267u:
                            medic_sequence_check&&handoff_play_ticks==390?252u:UINT32_MAX;
                        if(trigger!=UINT32_MAX){const auto&t=level_logic.value.triggers[trigger];float box[6];
                            if(t.kind!=37||t.shape!=3||t.boxes.size<sizeof box)throw std::runtime_error("Natural test trigger changed");
                            memcpy(box,t.boxes.data,sizeof box);float target[3];for(unsigned k=0;k<3;k++)target[k]=(box[k]+box[k+3])*.5f;
                            uc_collision_to_physics(target,player.position);memset(player.velocity,0,sizeof player.velocity);previous_player=player;
                        }
                    }
                    if(medic_sequence_check&&!natural_trigger_check&&handoff_play_ticks==330){
                        if(mission_world.ensure(181))throw std::runtime_error("Medic sequence construction failed");
                        if(mission_flags.size()<=176)throw std::runtime_error("Medic sequence flag input failed");
                        mission_flags[176]=1;
                    }
                    if(mission_placement_check&&!natural_trigger_check&&handoff_play_ticks==1){
                        // Explicit authored activation; do not claim the spatial trigger.
                        if(mission_world.ensure(836,&chase_id)||chase_id<0)throw std::runtime_error("Chased medic836 construction failed");
                        auto*c=barrier_owner.character(chase_id);const auto*spawn=mission_world.metadata(328).definition;
                        if(!c||!spawn)throw std::runtime_error("Chased medic spawn328 missing");
                        std::memcpy(chase_origin,spawn->transform+12,12);
                    }
                    if(mission_placement_check&&natural_trigger_check&&chase_id<0){
                        if(uc_level_entity_lookup(&mission_world.slots(),836,&chase_id))throw std::runtime_error("Natural chased medic lookup failed");
                        const auto*spawn=mission_world.metadata(328).definition;std::memcpy(chase_origin,spawn->transform+12,12);
                    }
                    if(mission_placement_check&&chase_id>=0){
                        if(auto*c=barrier_owner.character(chase_id);c&&c->ready()){
                            //DF1A0 runs at first actor update. The same frame's
                            //late pass applies one freefall step (17m/s²,30Hz).
                            if(!chase_spawn){for(unsigned i=0;i<3;i++){
                                const float expected=chase_origin[i]-(i==1?1700.f/(30.f*30.f):0.f);
                                if(std::fabs(c->entity.transform[12+i]-expected)>.01f)
                                    throw std::runtime_error("Chased medic spawn328 axis="+std::to_string(i)+" actual="+std::to_string(c->entity.transform[12+i])+" expected="+std::to_string(expected));}chase_spawn=true;}
                            float d=0;for(unsigned i=0;i<3;i++){float v=c->entity.transform[12+i]-chase_origin[i];d+=v*v;}chase_distance=std::max(chase_distance,d);
                            if(handoff_play_ticks%30==0){NativeCharacterNavigationState n;
                                if(!c->navigation_state(n))throw std::runtime_error("Chased medic navigation snapshot unavailable");
                                chase_trace<<handoff_play_ticks<<','<<n.behaviour<<','<<n.status<<','<<n.blocker<<','<<n.phase<<','<<n.node<<','<<n.movement_busy;
                                for(unsigned i=0;i<3;i++)chase_trace<<','<<c->entity.transform[12+i];
                                for(float v:n.goal)chase_trace<<','<<v;for(float v:n.waypoint)chase_trace<<','<<v;chase_trace<<'\n';}
                        }
                        int32_t id=-1;if(uc_level_entity_lookup(&mission_world.slots(),838,&id))throw std::runtime_error("Chased medic script slot");
                        chase_script|=id>=0;chase_deleted|=!uc_entity_event_lookup(&mission_world.registry(),chase_id);
                    }
                    ++handoff_play_ticks;
                }
                step_player(input);uc_collision_to_render(player.position,body);presentation_eye=eye;position={body[0],body[1]+eye,body[2]};
                if(handoff_play_ticks){
                    float distance=0;for(unsigned i=0;i<3;i++){float d=player.position[i]-handoff_origin[i];distance+=d*d;}
                    handoff_distance=std::max(handoff_distance,distance);handoff_crouch|=player.crouched!=0;handoff_items|=1u<<current_weapon;
                }
            }else if(integration_check) {
                dt=1.f/30.f;
                if(explosion_preview&&frames==90)explosion_requested=true;
                presentation_fraction=1;
                // Bounded real integration trajectory: settle, fire, reload,
                // switch, fire, crouch/move, return. No synthetic owner success.
                if(frames==60)pending_reload=true;
                if(frames==150)pending_weapon=2;
                fire_held=(frames>=30&&frames<60)||(frames>=210&&frames<255);
                if(secondary_check){
                    if(frames==200)pending_weapon=1;
                    if(frames==315)pending_weapon=0;
                    if(frames==360)pending_reload=true;
                    if(frames==430)pending_weapon=17;
                    fire_held=(frames>=30&&frames<60)||(frames>=245&&frames<280)||(frames>=365&&frames<385);
                }
                if(equipment_check){
                    if(frames==430)pending_weapon=11;
                    if(frames>=465&&frames<475)fire_held=true;
                    if(frames>=575&&frames<585)fire_held=true;
                    if(frames==525)pending_weapon=17;
                    if(frames==550||frames==610||frames==670){
                        const auto &target=actors->actor((frames-550)/60);
                        const float dx=(target.low[0]+target.high[0])*.5f-position.x;
                        const float dy=(target.low[1]+target.high[1])*.5f-position.y;
                        const float dz=(target.low[2]+target.high[2])*.5f-position.z;
                        yaw=std::atan2(-dx,dz);pitch=std::atan2(dy,std::sqrt(dx*dx+dz*dz));
                    }
                }
                UCPlayerInput input{};input.yaw=yaw;input.speed=2.5f;
                input.crouch=frames>=270&&frames<300;
                if(frames>=300&&frames<315)input.right=1;
                if(frames>=315&&frames<330)input.right=-1;
                if(equipment_check){
                    // Exercise the real PC main-loop gate through its registered
                    // receiver. This explicit test sequence is not the intro.
                    const uint32_t event=frames==5?113u:frames==19?34u:frames==20?166u:0u;
                    if(event){uint32_t payload=0,sender=(uint32_t)mission_player.entity.id;uint8_t handled=0;
                        if(uc_entity_event_send(&mission_world.registry(),mission_player.entity.id,event,&payload,4,&sender,&handled)||!handled)
                            throw std::runtime_error("Native control event failed");
                        ++control_events;if(frames==5){control_body=player;pitch=.4f;}
                    }
                    if(frames>=5&&frames<20){input.forward=input.right=1;input.crouch=1;fire_held=true;pending_reload=true;pending_weapon=2;}
                }
                step_player(input);uc_collision_to_render(player.position,body);
                if(equipment_check&&frames>=5&&frames<20&&
                   (player_view->accepts_input()||player_view->preview_shots||player_view->inventory.active_item!=17||
                    player.crouched!=control_body.crouched||std::abs(player.position[0]-control_body.position[0])>.0001f||
                    std::abs(player.position[1]-control_body.position[1])>.0001f))
                    throw std::runtime_error("Native control suspension leaked player input");
                if(equipment_check&&frames==20&&(!player_view->accepts_input()||pitch!=0||control_events!=3))
                    throw std::runtime_error("Native control restoration failed");
                presentation_eye=eye;position={body[0],body[1]+eye,body[2]};
            } else if(!headless&&gameplay_input()) {
                auto clamp_count=[](int64_t v){return (long)std::clamp<int64_t>(v,-LONG_MAX,LONG_MAX);};
                if(!player_view||player_view->accepts_input())ucrr_pc_rotate(&pitch,&yaw,player_view?player_view->motion.pitch_limit.current:1.55f,clamp_count(mouse_x),clamp_count(mouse_y),(int)bindings.sensitivity);mouse_x=mouse_y=0;
                using InputAction=NativeInputBindings::Action;
                auto down=[](InputAction action){return (GetAsyncKeyState((int)bindings.key(action))&0x8000)!=0;};
                float forward=(float)(down(InputAction::forward)-down(InputAction::back)),side=(float)(down(InputAction::right)-down(InputAction::left));
                if(overview) {
                    float up=(float)(((GetAsyncKeyState('E')&0x8000)!=0)-((GetAsyncKeyState('Q')&0x8000)!=0));
                    float scale=dt*speed/std::max(1.f,sqrtf(forward*forward+side*side+up*up));
                    position.x+=(-sinf(yaw)*forward+cosf(yaw)*side)*scale;
                    position.z+=(cosf(yaw)*forward+sinf(yaw)*side)*scale;position.y+=up*scale;
                } else {
                    UCPlayerInput input{forward,side,yaw,2.5f,(uint8_t)down(InputAction::crouch)};
                    accumulator+=dt;
                    while(accumulator>=1.f/30.f){step_player(input);accumulator-=1.f/30.f;}
                    float interpolated[3],alpha=accumulator*30;
                    presentation_fraction=std::clamp(alpha,0.f,1.f);
                    for(unsigned i=0;i<3;++i)interpolated[i]=previous_player.position[i]+(player.position[i]-previous_player.position[i])*alpha;
                    uc_collision_to_render(interpolated,body);
                    presentation_eye=previous_eye+(eye-previous_eye)*alpha;
                    position={body[0],body[1]+presentation_eye,body[2]};
                }
            } else {accumulator=0;previous_player=player;previous_eye=eye;if(player_view)player_view->set_fire(false);}
            bool dynamic_post=integrated_scene&&tone_preview&&integrated_scene->available();
            if(dynamic_post)integrated_scene->begin(renderer,paused||(!headless&&!focused)?0:dt);
            XMVECTOR direction=XMVectorSet(-sinf(yaw)*cosf(pitch),sinf(pitch),cosf(yaw)*cosf(pitch),0);
            XMMATRIX view=XMMatrixLookToLH(XMLoadFloat3(&position),direction,XMVectorSet(0,1,0,0));
            float weapon_matrix[16]{},weapon_view[16]{},camera_frame[16]{};
            if(player_view) {
                if(integration&&(gameplay_camera.resize(renderer.width,renderer.height)||(!gameplay_camera.selected()&&!handoff)))
                    throw std::runtime_error("Missing selected gameplay camera");
                auto render_actor=player_view->actor;render_actor.angles[0]=pitch;render_actor.angles[1]=yaw;
                render_actor.eye_offset[1]=presentation_eye;
                render_actor.body[12]=position.x;render_actor.body[13]=position.y-presentation_eye;render_actor.body[14]=position.z;
                float render_body[16],render_look[16];
                if(uc_firstperson_look_base(&render_actor,render_body,render_look)!=UC_MOTION_OK)throw std::runtime_error("Native presentation body");
                player_view->render_matrices(render_look,(float)renderer.width/(float)renderer.height,camera_frame,weapon_matrix,weapon_view,presentation_fraction);
                std::memcpy(barrier_owner.published_camera,camera_frame,64);
                XMFLOAT4X4 camera;memcpy(&camera,camera_frame,64);view=XMMatrixInverse(nullptr,XMLoadFloat4x4(&camera));
            }
            float fov=overview?45.f:53.f,near_z=overview?radius*.0001f:25.f,far_z=overview?radius*4:50000.f;
            XMMATRIX projection;
            if(overview) projection=XMMatrixPerspectiveFovLH(XM_PIDIV4,(float)renderer.width/(float)renderer.height,near_z,far_z);
            else {
                /* Normal Camera activation D6540 uses53deg; original projection
                 * defaults are25/50000. Dynamic camera/script owners are pending. */
                XMFLOAT4X4 p;
                if(uc_firstperson_projection(fov,(float)renderer.width/(float)renderer.height,near_z,far_z,0,1,&p._11))
                    throw std::runtime_error("Unsupported native player projection");
                projection=XMLoadFloat4x4(&p);
            }
            if(handoff&&!gameplay_camera.selected()){
                auto*s=handoff->scene();auto*c=s?s->camera():nullptr;if(!c)throw std::runtime_error("Selected mission camera is missing");
                UCSceneView selected{};struct ViewOwner{NativeCutsceneCamera*c;UCSceneViewport*v;} owner{c,&gameplay_camera.viewport};
                UCSceneViewServices services{&owner,[](void*,const UCSceneProjection*){return 1;},[](void*p)->void*{return ((ViewOwner*)p)->v;},
                    [](void*p)->const float*{return ((ViewOwner*)p)->c->entity.transform;},[](void*,void*v){double values[5];if(uc_scene_viewport_values((UCSceneViewport*)v,values))throw std::runtime_error("Mission viewport");return (float)values[4];}};
                if(uc_scene_view_construct(&selected,&c->entity.projection,0,0,&services))throw std::runtime_error("Mission camera projection failed");
                view=XMLoadFloat4x4((const XMFLOAT4X4*)selected.inverse);projection=XMLoadFloat4x4((const XMFLOAT4X4*)selected.projection);
                position={selected.world[12],selected.world[13],selected.world[14]};
                std::memcpy(barrier_owner.published_camera,selected.world,64);
            }
            if(fog_present&&fog_properties.enabled&&(!fog_lookup||integration)) {
                XMFLOAT4X4 p;XMStoreFloat4x4(&p,projection);
                if(uc_environment_fog_build(&fog_properties,p._33,p._43,dynamic_post?integrated_scene->exposure():1,fog_table.get())!=UC_FOG_OK)
                    throw std::runtime_error("Unsupported native fog projection/profile");
                if(!fog_lookup)fog_lookup=renderer.upload_fog(fog_table->alpha,sizeof(fog_table->alpha));
            }
            XMFLOAT4X4 matrix;XMStoreFloat4x4(&matrix,view*projection);
            BoundingFrustum frustum(projection),world_frustum;frustum.Transform(world_frustum,XMMatrixInverse(nullptr,view));
            size_t geometry_visible_pixels=0;
            if(shadows)shadows->begin_frame();
            bool hide_actors=false,hide_throwables=false,hide_props=false;
            auto render=[&](bool cull,bool apply_fog=true,bool apply_tone=true,bool draw_player=true,bool draw_hud=true,bool advance=true,bool draw_effects=true,bool draw_muzzle=true,bool draw_level_sprites=true,bool allow_flashlight=true) {
                renderer.material_phase=movement_steps/30.f+(paused?0:accumulator);
                const float clear[]={0,0,0,1};renderer.begin(clear);size_t submitted=0;
                NativeLightingFrame light_frame;UCNativeControlSpot flashlight{};float flashlight_plane[4]{};
                if(lighting){
                    const float light_eye[]={position.x,position.y,position.z};float ambient[4]{};
                    const float exposure=dynamic_post?integrated_scene->exposure():1.f;
                    scene_ambient->sample(light_eye,exposure,ambient);
                    const bool flashlight_on=allow_flashlight&&draw_player&&player_view&&player_view->flashlight(flashlight,camera_frame,weapon_view,presentation_fraction,flashlight_plane);
                    light_frame=lighting->frame(light_eye,exposure,ambient,flashlight_on?&flashlight:nullptr,lighting_enabled,flashlight_plane);
                    if(shadows&&model_lighting&&shadows_enabled){
                        model_casters.begin();
                        if(!hide_props){
                            for(auto&p:barrier_owner.props)p->collect_shadows(renderer,*model_lighting,light_frame,model_casters,presentation_fraction);
                            for(auto&p:barrier_owner.static_props)p->collect_shadows(renderer,*model_lighting,light_frame,model_casters,presentation_fraction);
                            for(auto&p:barrier_owner.light_fittings)p->collect_shadows(renderer,*model_lighting,light_frame,model_casters,presentation_fraction);
                        }
                        if(!hide_actors){
                            if(actors)actors->collect_shadows(renderer,*model_lighting,light_frame,model_casters,presentation_fraction);
                            for(auto&c:barrier_owner.characters)c->collect_shadows(renderer,*model_lighting,light_frame,model_casters,presentation_fraction);
                        }
                        model_casters.submit(*shadows);
                    }
                    lighting->draw_prepass(renderer,&matrix._11,light_frame);
                    lighting->draw_native(renderer,&matrix._11,light_frame,shadows_enabled?NativeIntegrationShadows::prepare:nullptr,shadows.get());
                }
                // Qualified opaque materials use977A0 ambient,98DC0 lighting,
                // then96BE0 DST_COLOR base. Other material routes stay explicit.
                for(unsigned partition=0;partition<3;++partition) {
                    for(const auto &draw:batches) {
                        if(cull&&!world_frustum.Intersects(draw.bounds))continue;
                        auto batch=draw.batch;uint32_t color=(batch->material_index+1)*2654435761u;
                        const auto &material=materials[batch->material_index];
                        const unsigned material_partition=material.state.alpha_prepass?1u:material.state.blend_mode?2u:0u;
                        if(material_partition!=partition||(lighting&&lighting->alpha_only(batch->material_index)))continue;
                        float tint[]={.25f+(color&255)/510.f,.25f+((color>>8)&255)/510.f,.25f+((color>>16)&255)/510.f,1};
                        auto texture=material.texture.Get();if(material.supported())tint[0]=tint[1]=tint[2]=1;
                        auto draw_state=material.state;
                        if(dynamic_post)draw_state.inverse_exposure=integrated_scene->inverse_exposure();
                        if(lighting&&lighting->owns_material(batch->material_index)){
                            draw_state.blend_mode=3;draw_state.write_alpha=lighting->writes_alpha(batch->material_index);draw_state.inverse_exposure=1;draw_state.vertex_exposure=false;
                        }
                        renderer.draw(mesh,&matrix._11,tint,texture,batch->first_index,batch->index_count,draw_state);++submitted;
                    }
                    if(partition==0&&lighting)lighting->draw_alpha(renderer,mesh,&matrix._11,light_frame,
                        shadows_enabled?NativeIntegrationShadows::prepare:nullptr,shadows.get());
                    if(partition==1) {
                        if(!hide_props)for(const auto &prop:barrier_owner.props)prop->draw(renderer,&matrix._11,dynamic_post?integrated_scene->inverse_exposure():1.f,model_lighting.get(),&light_frame);
                        if(!hide_props)for(const auto &prop:barrier_owner.static_props)prop->draw(renderer,&matrix._11,dynamic_post?integrated_scene->inverse_exposure():1.f,model_lighting.get(),&light_frame);
                        if(!hide_props)for(const auto &prop:barrier_owner.light_fittings)prop->draw(renderer,&matrix._11,dynamic_post?integrated_scene->inverse_exposure():1.f,model_lighting.get(),&light_frame);
                        if(level_particles)level_particles->draw_props(renderer,&matrix._11,dynamic_post?integrated_scene->inverse_exposure():1.f,UINT32_MAX,model_lighting.get(),&light_frame);
                        if(actors&&!hide_actors)actors->draw(renderer,&matrix._11,dynamic_post?integrated_scene->inverse_exposure():1.f,presentation_fraction,model_lighting.get(),&light_frame);
                        if(!hide_actors)for(const auto&character:barrier_owner.characters)character->draw(renderer,&matrix._11,dynamic_post?integrated_scene->inverse_exposure():1.f,presentation_fraction,model_lighting.get(),&light_frame);
                        if(!hide_actors)for(const auto&gun:barrier_owner.guns)gun->draw(renderer,&matrix._11,dynamic_post?integrated_scene->inverse_exposure():1.f,presentation_fraction,model_lighting.get(),&light_frame);
                        if(!hide_actors)for(const auto&weapon:barrier_owner.medic_weapons)weapon->draw(renderer,&matrix._11,dynamic_post?integrated_scene->inverse_exposure():1.f,presentation_fraction,model_lighting.get(),&light_frame);
                        if(!hide_actors)for(const auto&item:barrier_owner.npc_projectiles)item->draw(renderer,&matrix._11,dynamic_post?integrated_scene->inverse_exposure():1.f,presentation_fraction,model_lighting.get(),&light_frame);
                        if(impacts&&draw_effects){XMFLOAT4X4 v,p;XMStoreFloat4x4(&v,view);XMStoreFloat4x4(&p,projection);
                            impacts->draw(renderer,&v._11,&p._11,dynamic_post?integrated_scene->inverse_exposure():1.f);}
                        if(throwables&&draw_effects&&!hide_throwables){XMFLOAT4X4 v,p;XMStoreFloat4x4(&v,view);XMStoreFloat4x4(&p,projection);
                            throwables->draw(renderer,&v._11,&p._11,dynamic_post?integrated_scene->inverse_exposure():1.f,presentation_fraction,model_lighting.get(),&light_frame);}
                        if(player_view&&draw_player){
                            if(model_lighting)player_view->draw_lit(renderer,*model_lighting,weapon_matrix,weapon_view,
                                dynamic_post?integrated_scene->inverse_exposure():1.f,presentation_fraction);
                            else player_view->draw(renderer,weapon_matrix,weapon_view,
                                dynamic_post?integrated_scene->inverse_exposure():1.f,presentation_fraction);}
                        if(weapon_effects&&draw_player&&draw_effects&&draw_muzzle)weapon_effects->draw(renderer,*player_view,weapon_view,camera_frame,
                            (float)renderer.width/(float)renderer.height,dynamic_post?integrated_scene->inverse_exposure():1.f,presentation_fraction,false);
                        if(weapon_effects&&draw_effects&&draw_muzzle){XMFLOAT4X4 v,p,c;XMStoreFloat4x4(&v,view);XMStoreFloat4x4(&p,projection);XMStoreFloat4x4(&c,XMMatrixInverse(nullptr,view));
                            if(!hide_actors)for(const auto&gun:barrier_owner.guns)gun->draw_effects(renderer,&matrix._11,dynamic_post?integrated_scene->inverse_exposure():1.f);
                            weapon_effects->draw_world(renderer,&v._11,&p._11,&c._11);}
                        if(headless&&!integration_check) {
                            geometry_visible_pixels=0;auto geometry=renderer.read_rgba();
                            for(size_t p=0;p<geometry.size();p+=4)if(geometry[p]||geometry[p+1]||geometry[p+2])++geometry_visible_pixels;
                            if(!geometry_visible_pixels)throw std::runtime_error("Native scene produced no geometry before fog");
                        }
                        /* 99240 applies depth fog before its later transparent list. */
                        if(apply_fog&&fog_lookup)renderer.fog(fog_lookup.Get(),fog_table->rgba);
                    }
                }
                // Explicit PC transparent-world phase. Authored emitters use
                // the same completed simulation and depth as the level.
                if(handoff&&handoff->scene())handoff->scene()->draw(renderer,&matrix._11,dynamic_post?integrated_scene->inverse_exposure():1.f,model_lighting.get(),&light_frame);
                renderer.resolve_scene();
                if(level_particles&&draw_level_sprites){XMFLOAT4X4 v,p;XMStoreFloat4x4(&v,view);XMStoreFloat4x4(&p,projection);
                    level_particles->draw(renderer,&v._11,&p._11);}
                if(barrier_owner.npc_thrown_particles&&draw_effects){XMFLOAT4X4 v,p;XMStoreFloat4x4(&v,view);XMStoreFloat4x4(&p,projection);
                    barrier_owner.npc_thrown_particles->draw(renderer,&v._11,&p._11);}
                if(integration&&draw_effects){XMFLOAT4X4 v,p;XMStoreFloat4x4(&v,view);XMStoreFloat4x4(&p,projection);
                    barrier_owner.explosion_debris.draw(renderer,&v._11,&p._11);}
                if(integration&&draw_effects){XMFLOAT4X4 v,p;XMStoreFloat4x4(&v,view);XMStoreFloat4x4(&p,projection);
                    barrier_owner.bullet_tracers.draw(renderer,&v._11,&p._11);}
                if(integration){XMFLOAT4X4 p;XMStoreFloat4x4(&p,projection);integrated_scene->draw_blur(renderer,p._33,p._43);}
                if(apply_tone&&dynamic_post)integrated_scene->finish(renderer,advance);
                else if(apply_tone&&!integration&&tone_preview&&tone_lookup)renderer.tone(tone_lookup.Get());
                // Native PC UI phase; original full HUD admission/history remains pending.
                if(player_view&&player_view->accepts_input()&&draw_hud){
                    hud.draw(renderer,player_view->inventory,player_view->equipped->spec);
                    heartbeat.draw(renderer,hud.textures[6].Get(),hud.health);
                    reticle.draw(renderer);
                }
                if(handoff){handoff->draw_feedback();handoff->draw_fade();}
                if(integration)barrier_owner.objective_messages.draw(renderer);
                if(integration)barrier_owner.subtitles.draw(renderer);
                if(integration&&draw_hud){const float color[]={1,1,1,1};barrier_owner.heartbeat_monitors.draw(renderer,48,36,color);}
                if(integration&&!headless&&display.fps_counter)barrier_owner.subtitles.draw_fps(renderer,shown_fps);
                return submitted;
            };
            const auto render_start=profile_cpu?ProfileClock::now():ProfileClock::time_point{};
            size_t submitted=render(true);
            if(profile_cpu){auto end=ProfileClock::now();profile_rows.push_back({double(frames),std::chrono::duration<double,std::milli>(render_start-profile_start).count(),std::chrono::duration<double,std::milli>(end-render_start).count(),profile_world,profile_physics});}
            if(frontend_enabled&&frontend.active()){
                frontend.draw(renderer,graphics.value);
                if(integration&&display.fps_counter)barrier_owner.subtitles.draw_fps(renderer,shown_fps);
            }
            if(handoff_check){
                if(frames==120){
                    std::filesystem::create_directories(output);
                    render(true,false,false,false,false,false);
                    write_bmp(output/"scenery-raw.bmp",renderer.width,renderer.height,renderer.read_rgba());
                    render(true,true,true,true,true,false);
                }
                if(fire_check&&(handoff_play_ticks==70||handoff_play_ticks==130||handoff_play_ticks==155||handoff_play_ticks==180||handoff_play_ticks==220||handoff_play_ticks==300)){
                    const auto saved_view=view;const auto saved_matrix=matrix;const auto saved_frustum=world_frustum;const auto saved_position=position;
                    const auto*t=entities.value.entities[556].transform;XMFLOAT3 at{t[12],t[13]+100,t[14]};
                    position={at.x+850,at.y+200,at.z+500};
                    view=XMMatrixLookAtLH(XMLoadFloat3(&position),XMLoadFloat3(&at),XMVectorSet(0,1,0,0));
                    XMStoreFloat4x4(&matrix,view*projection);frustum.Transform(world_frustum,XMMatrixInverse(nullptr,view));
                    render(true,true,true,false,false,false);
                    write_bmp(output/("van-"+std::to_string(handoff_play_ticks)+".bmp"),renderer.width,renderer.height,renderer.read_rgba());
                    std::vector<const UCSolidBlockGeometry*> boxes;level_particles->append_blocks(boxes);
                    std::ofstream trace(output/"van-poses.csv",std::ios::app);
                    for(auto*b:boxes){trace<<handoff_play_ticks;for(float f:b->center)trace<<','<<f;for(float f:b->rotation)trace<<','<<f;trace<<std::endl;}
                    view=saved_view;matrix=saved_matrix;world_frustum=saved_frustum;position=saved_position;
                    render(true,true,true,true,true,false);
                }
                if(npc_hit_check&&handoff_play_ticks==135){
                    if(!handoff->flashlight||!player_view->flashlight_compatible())throw std::runtime_error("Flashlight request did not reach compatible equipment");
                    render(true,true,true,true,true,false);const auto with=renderer.read_rgba();
                    render(true,true,true,true,true,false,true,true,true,false);const auto without=renderer.read_rgba();
                    for(size_t i=0;i<with.size();i+=4)flashlight_pixels+=memcmp(with.data()+i,without.data()+i,3)!=0;
                    if(flashlight_pixels<32)throw std::runtime_error("Connected flashlight did not change visible pixels");
                    write_bmp(output/"flashlight-on.bmp",renderer.width,renderer.height,with);
                    write_bmp(output/"flashlight-off.bmp",renderer.width,renderer.height,without);
                    render(true,true,true,true,true,false);
                }
                if(npc_hit_check&&(handoff_play_ticks==80||handoff_play_ticks==83||handoff_play_ticks==95||handoff_play_ticks==120||handoff_play_ticks==223||handoff_play_ticks==250||handoff_play_ticks==285||handoff_play_ticks==310)){
                    auto*c=barrier_owner.character(barrier_owner.checked_npc);
                    if(c){ // Inspection camera only: leave player, NPC and authored placement unchanged.
                        const auto saved_view=view;const auto saved_matrix=matrix;const auto saved_frustum=world_frustum;const auto saved_position=position;
                        const auto*t=c->entity.transform;XMFLOAT3 at{t[12],t[13]+80,t[14]};float best=-1;
                        barrier_owner.refresh_collision();
                        for(unsigned angle=0;angle<8;++angle){const float a=angle*XM_PIDIV4;XMFLOAT3 candidate{at.x+300*std::sin(a),at.y+40,at.z+300*std::cos(a)};
                            UCCollisionResult hit{};int32_t id=-1;int found=barrier_owner.character_world.query(&at.x,&candidate.x,9,0,0x80,0,hit,id);
                            if(found<0)throw std::runtime_error("NPC inspection camera query failed");const float distance=found?std::max(0.f,hit.distance-10):302.65f;
                            if(distance>best){best=distance;const float fraction=distance/302.65f;position={at.x+(candidate.x-at.x)*fraction,at.y+40*fraction,at.z+(candidate.z-at.z)*fraction};}}
                        view=XMMatrixLookAtLH(XMLoadFloat3(&position),XMLoadFloat3(&at),XMVectorSet(0,1,0,0));
                        XMStoreFloat4x4(&matrix,view*projection);frustum.Transform(world_frustum,XMMatrixInverse(nullptr,view));
                        render(true,true,true,false,true,false);std::filesystem::create_directories(output);
                        write_bmp(output/("npc-hit-"+std::to_string(handoff_play_ticks)+".bmp"),renderer.width,renderer.height,renderer.read_rgba());
                        view=saved_view;matrix=saved_matrix;world_frustum=saved_frustum;position=saved_position;render(true,true,true,true,true,false);
                    }
                }
                if(frames%120==0||frames==1||frames==30||frames==145||frames==150||frames==155||frames==210||frames==215||frames==220||(weapon_secondary_check&&(handoff_play_ticks==95||handoff_play_ticks==155))){std::filesystem::create_directories(output);write_bmp(output/("handoff-"+std::to_string(frames)+".bmp"),renderer.width,renderer.height,renderer.read_rgba());}
                ++frames;if(handoff_play_ticks<(mission_placement_check?900u:medic_sequence_check?(dispatch_check?905u:natural_trigger_check?900u:450u):330u)&&frames<1800)continue;
                if(handoff->takeovers!=(restored?0u:1u)||handoff->returns!=(restored?0u:1u)||!player_view->accepts_input()||!gameplay_camera.selected()||!handoff->snapshots||
                    handoff_play_ticks<330||handoff_distance<.01f||!handoff_look||!handoff_crouch||player.crouched||
                    (handoff_items&((1u<<2)|(1u<<17)))!=((1u<<2)|(1u<<17))||player_view->preview_shots<2||
                    player_view->preview_queries!=player_view->preview_shots||!player_view->reload_pulses)throw std::runtime_error("Mission handoff control acceptance failed");
                if(medic_sequence_check&&!dispatch_check){int32_t id=-1,allow=1,next=-1;
                    if(uc_level_entity_lookup(&mission_world.slots(),181,&id)||uc_level_entity_lookup(&mission_world.slots(),206,&next))throw std::runtime_error("Medic sequence source lookup failed");
                    auto*c=barrier_owner.character(id);
                    int found=c?c->property("allow heal",&allow):0;
                    if(!c||c->entity.actor.health9C!=600||c->entity.actor.maximumA0!=800||found!=1||allow||next==-1||
                        !barrier_owner.heartbeat_monitors.references(id))throw std::runtime_error("Medic sequence health/monitor acceptance failed: actor="+std::to_string(id)+
                            " health="+std::to_string(c?c->entity.actor.health9C:-1)+" max="+std::to_string(c?c->entity.actor.maximumA0:-1)+
                            " allow_found="+std::to_string(found)+" allow="+std::to_string(allow)+" next="+std::to_string(next)+
                            " references="+std::to_string(barrier_owner.heartbeat_monitors.references(id)));
                    std::ofstream medic_report(output/"medic.json");medic_report<<"{\"passed\":true,\"test_player_repositioned\":"<<(natural_trigger_check?"true":"false")<<",\"natural_spawn_verified\":"<<(natural_trigger_check?"true":"false")<<",\"injected_activation\":"<<(natural_trigger_check?"false":"true")<<",\"spatial_trigger_verified\":"<<(natural_trigger_check?"true":"false")<<",\"source\":181,\"health\":"<<c->entity.actor.health9C
                        <<",\"maximum\":"<<c->entity.actor.maximumA0<<",\"allow_heal\":"<<allow<<",\"next_source\":206,\"monitor_references\":"<<barrier_owner.heartbeat_monitors.references(id)<<"}\n";
                }
                    if(dispatch_check){
                        bool complete=false;for(const auto&d:barrier_owner.dispatches)if(d->entity.source==991)complete=d->sent()&&d->completed();
                        if(!complete)throw std::runtime_error("Natural Dispatch991 playback/completion did not finish");
                        if(!mission_flags[176]||!mission_flags[173])throw std::runtime_error("Dispatch991 did not release the waiting mission script");
                        int32_t radio=-1;if(uc_level_entity_lookup(&mission_world.slots(),991,&radio)||uc_entity_event_lookup(&mission_world.registry(),radio))throw std::runtime_error("Dispatch991 retained a live entity");
                        std::ofstream(output/"dispatch.json")<<"{\"passed\":true,\"source\":991,\"natural_trigger\":true,\"sent\":true,\"completed\":true,\"retired\":true,\"queue_order\":true,\"external_delete\":true,\"mission_cancel\":true,\"waiting_script_resumed\":true}\n";
                    }
                if(mission_placement_check){
                    std::ofstream report(output/"placement.json");report<<"{\"passed\":"<<((chase_spawn&&chase_distance>1&&chase_script&&chase_deleted)?"true":"false")
                        <<",\"source\":836,\"spawn_source\":328,\"injected_activation\":"<<(natural_trigger_check?"false":"true")<<",\"natural_trigger_verified\":"<<(natural_trigger_check?"true":"false")<<",\"test_player_repositioned\":"<<(natural_trigger_check?"true":"false")<<",\"movement_distance_squared\":"<<chase_distance
                        <<",\"spawn_verified\":"<<chase_spawn<<",\"script838\":"<<chase_script<<",\"retired\":"<<chase_deleted<<"}\n";
                    if(!chase_spawn||chase_distance<=1||!chase_script||!chase_deleted)throw std::runtime_error("Chased medic run/script/delete chain incomplete");
                }
                if(shake_check){
                    if(!shake_sampled||!shake_faded||!shake_released||!mission_flags[30]||!barrier_owner.camera_shakes.empty())throw std::runtime_error("Natural shake171 start/fade/release failed: owners="+std::to_string(barrier_owner.camera_shakes.size())+" sample="+std::to_string(shake_sampled)+" fade="+std::to_string(shake_faded)+" release="+std::to_string(shake_released)+" flag30="+std::to_string(mission_flags[30])+" peak="+std::to_string(shake_peak));
                    std::ofstream(output/"shake.json")<<"{\"passed\":true,\"source\":171,\"trigger\":362,\"test_player_repositioned\":true,\"registered_sample\":true,\"faded\":true,\"released\":true,\"no_retrigger\":true,\"gameplay_delete\":true,\"peak\":"<<shake_peak<<"}\n";
                }
                if(fire_check){
                    NativeMissionFire*fire=nullptr;for(auto&f:barrier_owner.fires)if(f->entity.source==563)fire=f.get();
                    if(!fire||fire->ticks<30||!fire->damage_calls||!barrier_owner.navigation_boxes.group(fire->entity.id))throw std::runtime_error("Truck fire gameplay acceptance failed");
                    bool explosion=false;for(auto&e:barrier_owner.explosions)if(e->entity.source==501)explosion=true;
                    if(!explosion)throw std::runtime_error("Real truck explosion501 did not execute");
                    std::vector<const UCSolidBlockGeometry*> van_boxes;level_particles->append_blocks(van_boxes);
                    if(van_boxes.size()!=1||std::find(active_blocks.begin(),active_blocks.end(),van_boxes[0])==active_blocks.end())
                        throw std::runtime_error("Overturned van missing from player collision");
                    auto*box=van_boxes[0];float from[3],to[3];memcpy(from,box->center,12);memcpy(to,box->center,12);from[0]-=10;to[0]+=10;
                    UCCollisionTriangleHit hit{};
                    if(uc_solid_block_cast(box,from,to,1,&hit)!=1)throw std::runtime_error("Overturned van does not block route ray");
                    if(level_particles->pending_prop_action(556)!=-1||fabsf(box->rotation[0])<.65f)
                        throw std::runtime_error("Van rollover did not reach its authored final pose");

                    std::ofstream report(output/"fire.json");report<<"{\"passed\":true,\"trigger\":"<<(shake_check?362:5)<<",\"test_player_repositioned\":true,\"explosion_source\":501,\"fire_source\":563,\"fire_ticks\":"<<fire->ticks<<",\"damage_calls\":"<<fire->damage_calls<<",\"fire_emitters\":"<<level_particles->counts().fire_active<<"}\n";
                }
                render(true,true,true,true,true,false);const auto pixels=renderer.read_rgba();
                render(true,true,true,true,false,false);const auto without_hud=renderer.read_rgba();
                render(true,true,true,false,true,false);const auto without_weapon=renderer.read_rgba();
                size_t hud_pixels=0,weapon_pixels=0;for(size_t p=0;p<pixels.size();p+=4){hud_pixels+=memcmp(pixels.data()+p,without_hud.data()+p,3)!=0;weapon_pixels+=memcmp(pixels.data()+p,without_weapon.data()+p,3)!=0;}
                write_bmp(output/"gameplay.bmp",renderer.width,renderer.height,pixels);
                write_bmp(output/"gameplay-no-hud.bmp",renderer.width,renderer.height,without_hud);
                write_bmp(output/"gameplay-no-weapon.bmp",renderer.width,renderer.height,without_weapon);
                if(particle_push_check){
                    if(!movement_ramp_checks||!movement_release_checks||movement_crouch_checks<10)throw std::runtime_error("Movement response checks were not exercised");
                    std::ofstream(output/"movement.json")<<"{\"passed\":true,\"ramp_checks\":"<<movement_ramp_checks
                        <<",\"release_checks\":"<<movement_release_checks<<",\"crouch_physics_checks\":"<<movement_crouch_checks<<"}\n";
                }
                if(hud_pixels<64||weapon_pixels<64)throw std::runtime_error("Mission handoff HUD/weapon pixels missing");
                std::ofstream report(output/"handoff.json");report<<"{\"passed\":true,\"frames\":"<<frames<<",\"return_frame\":"<<handoff_return_frame
                    <<",\"npc_hit_check\":"<<npc_hit_check<<",\"checked_npc\":"<<barrier_owner.checked_npc<<",\"checked_body_hits\":"<<barrier_owner.checked_body_hits
                    <<",\"npc_contacts\":"<<barrier_owner.npc_bullet_contacts<<",\"head_contacts\":"<<barrier_owner.npc_head_contacts<<",\"headshots\":"<<handoff->statistics[2]<<",\"kills\":"<<handoff->statistics[1]
                    <<",\"corpse_repeat_hits\":"<<barrier_owner.checked_corpse_hits<<",\"blast_packets\":"<<barrier_owner.checked_blasts
                    <<",\"stun_packets\":"<<barrier_owner.checked_stuns<<",\"flashlight_pixels\":"<<flashlight_pixels
                    <<",\"ragdoll_bodies\":"<<(barrier_owner.ragdoll_world?barrier_owner.ragdoll_world->ragdoll_body_count():0)
                    <<",\"gameplay_ticks\":"<<handoff_play_ticks<<",\"snapshots\":"<<handoff->snapshots<<",\"movement_distance_squared\":"<<handoff_distance
                    <<",\"mouse_look\":true,\"crouch\":true,\"items\":"<<handoff_items<<",\"shots\":"<<player_view->preview_shots<<",\"reload_events\":"<<player_view->reload_pulses
                    <<",\"hud_pixels\":"<<hud_pixels<<",\"weapon_pixels\":"<<weapon_pixels<<"}\n";
                if(handoff->close_fade())throw std::runtime_error("Mission fade shutdown failed");break;
            }
            if(benchmark)timing.end(renderer,frames);
            if(integration_check) {
                if(explosion_preview&&frames==105){
                    // Offscreen inspection camera only; never move the player
                    // or edit an authored spawn to manufacture a visible effect.
                    const auto saved_view=view;const auto saved_matrix=matrix;
                    const auto saved_frustum=world_frustum;const auto saved_position=position;
                    const float *point=entities.value.entities[explosion_sources[0]].transform+12;
                    position={point[0]-300,point[1]+150,point[2]-900};
                    view=XMMatrixLookAtLH(XMLoadFloat3(&position),XMLoadFloat3((const XMFLOAT3*)point),XMVectorSet(0,1,0,0));
                    XMStoreFloat4x4(&matrix,view*projection);frustum.Transform(world_frustum,XMMatrixInverse(nullptr,view));
                    render(true,true,true,false,true,false,true,false,false);const auto with=renderer.read_rgba();
                    render(true,true,true,false,true,false,false,false,false);const auto without=renderer.read_rgba();
                    for(size_t p=0;p<with.size();p+=4)explosion_visible_pixels+=memcmp(with.data()+p,without.data()+p,3)!=0;
                    std::filesystem::create_directories(output);
                    write_bmp(output/"explosion-on.bmp",renderer.width,renderer.height,with);
                    write_bmp(output/"explosion-off.bmp",renderer.width,renderer.height,without);
                    view=saved_view;matrix=saved_matrix;world_frustum=saved_frustum;position=saved_position;
                    render(true,true,true,true,true,false);
                }
                if(equipment_check&&(frames==480||frames==500||frames==580||frames==640||frames==700)){
                    render(true,true,true,true,true,false);const auto with=renderer.read_rgba();
                    hide_actors=true;render(true,true,true,true,true,false);auto without=renderer.read_rgba();
                    size_t changed=0;for(size_t p=0;p<with.size();p+=4)changed+=memcmp(with.data()+p,without.data()+p,3)!=0;
                    actor_visible_pixels=std::max(actor_visible_pixels,changed);hide_actors=false;
                    hide_throwables=true;render(true,true,true,true,true,false);without=renderer.read_rgba();
                    changed=0;for(size_t p=0;p<with.size();p+=4)changed+=memcmp(with.data()+p,without.data()+p,3)!=0;
                    throwable_visible_pixels=std::max(throwable_visible_pixels,changed);hide_throwables=false;
                    render(true,true,true,true,true,false);
                }
                if(!benchmark&&integrated_scene->shots!=captured_shots){
                    captured_shots=integrated_scene->shots;std::filesystem::create_directories(output);
                    auto with=renderer.read_rgba();render(true,true,true,true,true,false,false);auto without=renderer.read_rgba();
                    size_t changed=0;for(size_t p=0;p<with.size();p+=4)changed+=memcmp(with.data()+p,without.data()+p,3)!=0;
                    shot_effect_pixels=std::max(shot_effect_pixels,changed);
                    write_bmp(output/("shot-"+std::to_string(captured_shots)+".bmp"),renderer.width,renderer.height,with);
                    render(true,true,true,true,true,false,true,false);auto impact_only=renderer.read_rgba();
                    changed=0;for(size_t p=0;p<with.size();p+=4)changed+=memcmp(impact_only.data()+p,without.data()+p,3)!=0;
                    if(changed>impact_visible_pixels){impact_visible_pixels=changed;
                        write_bmp(output/"impact-only.bmp",renderer.width,renderer.height,impact_only);
                        write_bmp(output/"impact-off.bmp",renderer.width,renderer.height,without);}
                    render(true,true,true,true,true,false);
                }
                if((!benchmark&&(frames==25||frames==40||frames==120||frames==205||frames==220||frames==290||
                    (secondary_check&&frames==390)||(equipment_check&&(frames==460||frames==480||frames==500||frames==580||frames==640||frames==700))))||frames==final_check_frame) {
                    std::filesystem::create_directories(output);auto pixels=renderer.read_rgba();
                    write_bmp(output/("frame-"+std::to_string(frames)+".bmp"),renderer.width,renderer.height,pixels);
                    if(frames==final_check_frame) {
                        size_t level_sprite_pixels=0,prop_pixels=0;
                        if(!barrier_owner.props.empty()){
                            hide_props=true;render(true,true,true,true,true,false);const auto without_props=renderer.read_rgba();hide_props=false;
                            for(size_t p=0;p<pixels.size();p+=4)prop_pixels+=memcmp(pixels.data()+p,without_props.data()+p,3)!=0;
                            write_bmp(output/"props-off.bmp",renderer.width,renderer.height,without_props);
                        }
                        if(level_particles){
                            render(true,true,true,true,true,false,true,true,false);const auto without_sprites=renderer.read_rgba();
                            for(size_t p=0;p<pixels.size();p+=4)level_sprite_pixels+=memcmp(pixels.data()+p,without_sprites.data()+p,3)!=0;
                            write_bmp(output/"level-sprites-off.bmp",renderer.width,renderer.height,without_sprites);
                        }
                        render(true,true,true,true,false,false);auto without_hud=renderer.read_rgba();
                        render(true,true,true,false,true,false);auto without_weapon=renderer.read_rgba();
                        size_t hud_pixels=0,weapon_pixels=0;
                        for(size_t p=0;p<pixels.size();p+=4){hud_pixels+=memcmp(pixels.data()+p,without_hud.data()+p,3)!=0;weapon_pixels+=memcmp(pixels.data()+p,without_weapon.data()+p,3)!=0;}
                        if(hud_pixels<64||weapon_pixels<64||player_view->preview_shots<2||
                            player_view->preview_queries!=player_view->preview_shots||integrated_scene->shots!=player_view->preview_shots||
                            !player_audio->started()||current_weapon!=(secondary_check?17u:2u)||!player.grounded||
                            (secondary_check&&(equipment_seen!=((1u<<0)|(1u<<1)|(1u<<2)|(1u<<17)|(equipment_check?(1u<<11):0))||taser_ticks<30||!taser_loop_ticks||shield_ticks<40)))
                            throw std::runtime_error("Native integration scenario did not complete its connected components");
                        size_t light_pixels=0,shadow_pixels=0;
                        if(lighting&&lighting_enabled&&shadows_enabled) {
                            // Same camera, pose, exposure and history; all readback is outside timing.
                            lighting_enabled=false;render(true,true,true,true,true,false);auto off=renderer.read_rgba();
                            lighting_enabled=true;shadows_enabled=false;render(true,true,true,true,true,false);auto lit=renderer.read_rgba();
                            shadows_enabled=true;render(true,true,true,true,true,false);auto shadowed=renderer.read_rgba();
                            write_bmp(output/"comparison-off.bmp",renderer.width,renderer.height,off);
                            write_bmp(output/"comparison-lights.bmp",renderer.width,renderer.height,lit);
                            write_bmp(output/"comparison-shadows.bmp",renderer.width,renderer.height,shadowed);
                            for(size_t p=0;p<off.size();p+=4) {
                                light_pixels+=memcmp(off.data()+p,lit.data()+p,3)!=0;
                                shadow_pixels+=memcmp(lit.data()+p,shadowed.data()+p,3)!=0;
                            }
                        }
                        std::ofstream report(output/"integration-check.json");report.exceptions(std::ios::badbit|std::ios::failbit);
                        ComPtr<IDXGIDevice> graphics_device;ComPtr<IDXGIAdapter> adapter;DXGI_ADAPTER_DESC adapter_info{};
                        if(FAILED(renderer.device.As(&graphics_device))||FAILED(graphics_device->GetAdapter(&adapter))||
                           FAILED(adapter->GetDesc(&adapter_info)))throw std::runtime_error("Cannot identify graphics adapter");
                        D3D11_TEXTURE2D_DESC target_info{};renderer.color->GetDesc(&target_info);
                        report<<"{\"passed\":true,\"frames\":"<<frames+1<<",\"movement_steps\":"<<movement_steps
                            <<",\"width\":"<<renderer.width<<",\"height\":"<<renderer.height<<",\"msaa_samples\":"<<target_info.SampleDesc.Count
                            <<",\"adapter_vendor_id\":"<<adapter_info.VendorId<<",\"adapter_device_id\":"<<adapter_info.DeviceId
                            <<",\"shots\":"<<integrated_scene->shots<<",\"static_hits\":"<<integrated_scene->hits
                            <<",\"audio_voices_started\":"<<player_audio->started()<<",\"muted\":true"
                            <<",\"hud_pixels\":"<<hud_pixels<<",\"weapon_pixels\":"<<weapon_pixels
                            <<",\"final_item\":"<<current_weapon<<",\"pistol_ammo\":"<<player_view->inventory.clip[2]
                            <<",\"equipment_seen\":"<<equipment_seen<<",\"taser_ticks\":"<<taser_ticks<<",\"taser_loop_ticks\":"<<taser_loop_ticks
                            <<",\"shield_ticks\":"<<shield_ticks
                            <<",\"native_control_events\":"<<control_events
                            <<",\"shield_bashes\":"<<player_view->shield_bashes<<",\"shield_melee_queries\":"<<player_view->shield_melee_queries
                            <<",\"preview_actors\":"<<(actors?actors->size():0)<<",\"actor_hits\":"<<shot_context.actor_hits
                            <<",\"authored_barriers\":"<<barrier_owner.bodies.size()<<",\"active_barriers\":"<<active_barrier_count
                            <<",\"preview_props\":"<<barrier_owner.props.size()<<",\"active_world_colliders\":"<<active_blocks.size()<<",\"prop_visible_pixels\":"<<prop_pixels
                            <<",\"preview_static_props\":"<<barrier_owner.static_props.size()
                            <<",\"preview_light_fittings\":"<<barrier_owner.light_fittings.size()
                            <<",\"actor_visible_pixels\":"<<actor_visible_pixels<<",\"throwable_visible_pixels\":"<<throwable_visible_pixels
                            <<",\"melee_actor_hits\":"<<shot_context.melee_actor_hits<<",\"melee_static_hits\":"<<shot_context.melee_static_hits
                            <<",\"footstep_cycles\":"<<player_footsteps->counts().cycles<<",\"footstep_voices\":"<<player_footsteps->counts().started
                            <<",\"footstep_unsupported\":"<<player_footsteps->counts().unsupported
                            <<",\"shotgun_ammo\":"<<player_view->inventory.clip[17]<<",\"environment_frames\":"<<integrated_scene->frames()
                            <<",\"exposure\":"<<integrated_scene->exposure()<<",\"submitted_batches\":"<<submitted
                            <<",\"npc_damage\":false,\"original_shot_effects\":false,\"scripts_cutscene\":false,\"full_gameplay\":false"
                            <<",\"muzzle_shots\":"<<weapon_effects->counts().shots
                            <<",\"muzzle_particle_requests\":"<<weapon_effects->counts().particle_requests
                            <<",\"impact_hits\":"<<impacts->counts().hits<<",\"impact_particle_requests\":"<<impacts->counts().particle_requests
                            <<",\"decal_marks\":"<<impacts->counts().decal_marks
                            <<",\"preview_explosions\":"<<impacts->counts().explosion_visuals
                            <<",\"mission_explosion_activation\":false,\"explosion_inspection_pixels\":"<<explosion_visible_pixels;
                        report<<",\"shot_effect_pixels\":"<<shot_effect_pixels<<",\"impact_visible_pixels\":"<<impact_visible_pixels;
                        if(level_particles){const auto &c=level_particles->counts();
                            report<<",\"level_sprite_emitters\":"<<c.authored<<",\"level_sprite_active\":"<<c.active
                                <<",\"level_sprite_deferred\":"<<c.deferred<<",\"level_sprite_particles\":"<<c.live_particles
                                <<",\"level_sprite_draws\":"<<c.draws<<",\"level_sprite_visible_pixels\":"<<level_sprite_pixels
                                <<",\"fire_authored\":"<<c.fire_authored<<",\"fire_active\":"<<c.fire_active
                                <<",\"fire_deferred\":"<<c.fire_deferred<<",\"fire_particles\":"<<c.fire_live_particles
                                <<",\"animated_prop_authored\":"<<c.prop_authored<<",\"animated_prop_supported\":"<<c.prop_supported
                                <<",\"animated_prop_active\":"<<c.prop_active<<",\"animated_prop_particles\":"<<c.prop_live_particles
                                <<",\"animated_prop_markers\":"<<c.prop_markers<<",\"animated_prop_restarts\":"<<c.prop_restarts
                                <<",\"animated_prop_source_updates\":"<<c.prop_source_updates
                                <<",\"preview_vehicles\":"<<c.vehicle_active<<",\"pending_vehicle_actions\":"<<c.vehicle_pending
                                <<",\"deferred_prop_supported\":"<<c.deferred_prop_supported<<",\"deferred_prop_active\":"<<c.deferred_prop_active
                                <<",\"deferred_prop_pending\":"<<c.deferred_prop_pending;}
                        if(throwables){const auto &c=throwables->counts();
                            const auto &inv=player_view->inventory;
                            report<<",\"molotov_recorded\":"<<((inv.seen_items[11]&&std::find(inv.category_order[1],inv.category_order[1]+inv.category_count[1],11u)!=inv.category_order[1]+inv.category_count[1])?"true":"false");
                            report<<",\"molotov_count\":"<<player_view->inventory.clip[11]<<",\"throws\":"<<c.throws<<",\"throwable_hits\":"<<c.hits
                                <<",\"throwable_particle_requests\":"<<c.particle_requests<<",\"throwable_live_particles\":"<<c.live_particles;
                        }
                        if(lighting) {
                            const auto &c=lighting->counts();
                            report<<",\"point_lighting_enabled\":"<<(lighting_enabled?"true":"false")
                                <<",\"pc_shadows_enabled\":"<<(shadows_enabled?"true":"false")
                                <<",\"authored_lights\":"<<c.authored<<",\"point_lights\":"<<c.points
                                <<",\"legacy_preview_unsupported_lights\":"<<c.unsupported_lights<<",\"configured_lights\":"<<c.configured
                                <<",\"unmatched_light_entities\":"<<c.unmatched_entities<<",\"basic_material_slots\":"<<c.materials
                                <<",\"legacy_preview_unsupported_material_slots\":"<<c.unsupported_materials<<",\"light_memberships\":"<<c.memberships
                                <<",\"legacy_preview_excluded_specular_batches\":"<<c.excluded_specular_batches
                                <<",\"shadow_eligible_lights\":"<<c.shadow_eligible<<",\"drawn_lights\":"<<c.drawn_lights
                                <<",\"light_batches\":"<<c.drawn_batches<<",\"shadowed_lights\":"<<c.shadowed_lights
                                <<",\"shadow_requests\":"<<c.shadow_requests<<",\"shadow_unserved\":"<<c.shadow_unserved
                                <<",\"shadow_caster_batches\":"<<shadows->caster_batches()<<",\"shadow_excluded_batches\":"<<shadows->excluded_batches()
                                <<",\"shadow_cache_capacity\":"<<shadows->capacity()
                                <<",\"shadow_maps_rendered\":"<<shadows->maps_rendered()<<",\"shadow_cache_hits\":"<<shadows->cache_hits()
                                <<",\"shadow_indices_submitted\":"<<shadows->indices_submitted()
                                <<",\"same_frame_light_pixels\":"<<light_pixels<<",\"same_frame_shadow_pixels\":"<<shadow_pixels
                                <<",\"original_stencil_shadows\":false,\"native_specular_path_connected\":true";
                        }
                        report<<"}\n";
                        if(benchmark)timing.write(renderer,output/"frame-times.csv",90);
                        break;
                    }
                }
                ++frames;continue;
            }
            if(headless) {
                auto pixels=renderer.read_rgba(); size_t changed=0;
                render(false);if(pixels!=renderer.read_rgba())throw std::runtime_error("Frustum culling changed visible pixels");
                for(size_t p=0;p<pixels.size();p+=4) if(pixels[p]>40||pixels[p+1]>40||pixels[p+2]>40) ++changed;
                if(!changed) throw std::runtime_error("Native scene produced no visible geometry");
                std::filesystem::create_directories(output); write_bmp(output/"scene.bmp",renderer.width,renderer.height,pixels);
                size_t firstperson_pixels=0,hud_pixels=0;
                if(player_view) {
                    render(true,true,true,true,false);auto without_hud=renderer.read_rgba();
                    for(size_t p=0;p<pixels.size();p+=4)hud_pixels+=memcmp(pixels.data()+p,without_hud.data()+p,3)!=0;
                    if(hud_pixels<64)throw std::runtime_error("Native ammo HUD produced no visible pixels");
                    write_bmp(output/"scene-without-hud.bmp",renderer.width,renderer.height,without_hud);
                    render(true,true,true,false);auto without=renderer.read_rgba();
                    for(size_t p=0;p<pixels.size();p+=4)firstperson_pixels+=memcmp(pixels.data()+p,without.data()+p,3)!=0;
                    if(player_view->animations[0].current&&player_view->animations[0].current->timing.weight==1&&firstperson_pixels<64)throw std::runtime_error("Completed first-person pose produced no visible weapon/arms");
                    write_bmp(output/"scene-without-weapon.bmp",renderer.width,renderer.height,without);render(true);
                }
                if(tone_preview&&tone_lookup) {
                    render(true,true,true,true,false);auto toned_scene=renderer.read_rgba();
                    render(true,true,false,true,false);auto before_tone=renderer.read_rgba();
                    render(false,true,false,true,false);
                    if(before_tone!=renderer.read_rgba())throw std::runtime_error("Frustum culling changed pixels before colour preview");
                    for(size_t p=0;p<pixels.size();p+=4) {
                        for(unsigned c=0;c<3;++c)
                            if(toned_scene[p+c]!=tone_reference[40+c*256+before_tone[p+c]])
                                throw std::runtime_error("Scene colour preview differs from supplied lookup");
                        if(toned_scene[p+3]!=0)throw std::runtime_error("Scene colour preview alpha differs from original pass");
                    }
                    // The separate UI is identical with the colour preview on or off.
                    if(player_view&&player_view->accepts_input()){hud.draw(renderer,player_view->inventory,player_view->equipped->spec);
                        heartbeat.draw(renderer,hud.textures[6].Get(),hud.health);reticle.draw(renderer);}
                    before_tone=renderer.read_rgba();
                    write_bmp(output/"scene-before-tone.bmp",renderer.width,renderer.height,before_tone);
                }
                if(fog_lookup) {render(true,false);write_bmp(output/"scene-without-fog.bmp",renderer.width,renderer.height,renderer.read_rgba());}
                std::ofstream report(output/"scene-check.json"); report.exceptions(std::ios::badbit|std::ios::failbit);
                report.precision(9); /* Preserve float32 camera values for reference comparisons. */
                report<<"{\"passed\":true,\"vertices\":"<<scene.vertex_count<<",\"base_batches\":"<<batches.size()
                    <<",\"visible_pixels\":"<<changed<<",\"textured_batches\":"<<textured_batches
                    <<",\"untextured_batches\":"<<untextured_batches
                    <<",\"geometry_visible_pixels\":"<<geometry_visible_pixels
                    <<",\"unsupported_material_batches\":"<<batches.size()-textured_batches-untextured_batches
                    <<",\"transparent_batches\":"<<transparent_batches
                    <<",\"authored_fog\":"<<(fog_lookup?"true":"false")<<",\"fog_exposure\":1,\"dynamic_environment_owner\":false"
                    <<",\"fixed_tone_preview\":"<<(tone_preview&&tone_lookup?"true":"false")
                    <<",\"tone_preview_available\":"<<(tone_lookup?"true":"false")<<",\"tone_history_owner\":false"
                    <<",\"entity_records\":"<<entities.value.entity_count<<",\"checkpoint\":"<<restart.number
                    <<",\"collision_triangles\":"<<collision.value.triangle_count
                    <<",\"ground_hit\":"<<(ground_hit?"true":"false")<<",\"ground_triangle\":"<<ground.triangle
                    <<",\"submitted_batches\":"<<submitted<<",\"unculled_pixels_equal\":true"
                    <<",\"movement_steps\":"<<movement_steps<<",\"grounded\":"<<(player.grounded?"true":"false")
                    <<",\"eye_height_cm\":"<<eye
                    <<",\"player_physics_position\":["<<player.position[0]<<","<<player.position[1]<<","<<player.position[2]<<"]"
                    <<",\"camera\":\""<<(overview?"overview":"standing restart")<<"\""
                    <<",\"camera_position\":["<<position.x<<","<<position.y<<","<<position.z<<"]"
                    <<",\"camera_pitch_yaw\":["<<pitch<<","<<yaw<<"],\"viewport\":["<<renderer.width<<","<<renderer.height<<"]"
                    <<",\"vertical_fov_degrees\":"<<fov<<",\"near_z\":"<<near_z<<",\"far_z\":"<<far_z
                    <<",\"live_camera_owner\":"<<(player_view?"true":"false")
                    <<",\"passive_firstperson\":"<<(player_view?"true":"false")<<",\"firstperson_updates\":"<<(player_view?player_view->updates:0)
                    <<",\"firstperson_item\":"<<(player_view?player_view->inventory.active_item:-1)
                    <<",\"firstperson_pending_footsteps\":"<<(player_view?player_view->pending_footsteps:0)
                    <<",\"firstperson_reload_pulses\":"<<(player_view?player_view->reload_pulses:0)
                    <<",\"firstperson_unhandled_arm_events\":"<<(player_view?player_view->unhandled_arm_events:0)
                    <<",\"firstperson_unhandled_weapon_events\":"<<(player_view?player_view->unhandled_weapon_events:0)
                    <<",\"firstperson_clip_ammo\":"<<(player_view?player_view->inventory.clip[player_view->inventory.active_item]:0)
                    <<",\"ammo_hud\":"<<(player_view?"true":"false")
                    <<",\"health_hud\":"<<(player_view&&hud.health_initialized?"true":"false")
                    <<",\"heartbeat_waveform\":"<<(player_view?"true":"false")
                    <<",\"resident_heartbeat_audio\":"<<(player_view&&player_audio?"true":"false")
                    <<",\"weapon_reticle\":"<<(player_view?"true":"false")
                    <<",\"resident_weapon_audio\":"<<(player_audio?"true":"false")
                    <<",\"weapon_audio_requests\":"<<(player_audio?player_audio->requests():0)
                    <<",\"weapon_audio_voices\":"<<(player_audio?player_audio->started():0)
                    <<",\"weapon_audio_muted\":"<<(player_audio&&headless?"true":"false")
                    <<",\"actor_health_maximum\":"<<(player_view?player_view->health_maximum:0)
                    <<",\"hud_changed_pixels\":"<<hud_pixels
                    <<",\"health_hud_fraction\":"<<(player_view?hud.health.fraction:0)
                    <<",\"firstperson_changed_pixels\":"<<firstperson_pixels
                    <<",\"firstperson_idle_weight\":"<<(player_view&&player_view->animations[0].current?player_view->animations[0].current->timing.weight:0)
                    <<",\"firstperson_idle_phase\":"<<(player_view&&player_view->animations[0].current?player_view->animations[0].current->timing.phase:0)
                    <<",\"presentation_body_rebase\":"<<(player_view?"true":"false")
                    <<",\"complete_material_context\":false,\"visual_fidelity_verified\":false,\"animated_entities\":"<<(player_view?2:0)<<",\"gameplay\":false,\"guest_runtime\":false}\n";
                break;
            }
            renderer.present();++fps_frames;
            const auto presented=std::chrono::steady_clock::now();
            float fps_seconds=std::chrono::duration<float>(presented-fps_start).count();
            bool title_due=fps_seconds>=.5f;
            if(title_due){shown_fps=fps_frames/fps_seconds;fps_frames=0;fps_start=presented;}
            if((++frames)==1||title_due||paused!=title_paused||focused!=title_focused) {
                if(frontend_enabled){SetWindowTextW(app_window,L"Urban Chaos: Riot Response");}
                else if(lighting) {
                    wchar_t title[360];swprintf_s(title,L"Urban Chaos — Native %s | %s | %.1f FPS | RMB shield, LMB attack, 1–5 items | F6 exposure %s | F7 lights %s | F8 PC shadows %s | F9 resolution, F11 fullscreen | Esc pause",
                        mission_preview?L"Mission Test":equipment_preview?L"Equipment Test":L"Lighting Preview",paused?L"PAUSED":focused?L"ACTIVE":L"INACTIVE",shown_fps,tone_preview?L"ON":L"OFF",
                        lighting_enabled?L"ON":L"OFF",shadows_enabled?L"ON":L"OFF");
                    SetWindowTextW(app_window,title);
                } else if(integration) {
                    wchar_t title[360];swprintf_s(title,L"Urban Chaos — Integration Preview | %s | %.1f FPS | LMB fire, R reload, 1/2 weapons, 3 Taser, 4 shield, WASD, mouse, Ctrl crouch, Esc pause | F6 exposure/glow %s",
                        paused?L"PAUSED":focused?L"ACTIVE":L"INACTIVE",shown_fps,tone_preview?L"ON":L"OFF");
                    SetWindowTextW(app_window,title);
                } else {
                wchar_t title[360];swprintf_s(title,L"Urban Chaos — %s | Native %s prototype | WASD, mouse look, %s, Shift fast, Esc pause%s%s",
                    paused?L"PAUSED — press Esc to resume":focused?L"ACTIVE":L"INACTIVE — select this window",
                    player_view?L"first-person":overview?L"scene":L"movement",overview?L"Q/E height":L"Ctrl crouch",
                    player_view?L" | 1 pistol, 2 shotgun, 3 Taser, 4 shield, wheel firearms":L"",
                    tone_available?(tone_preview?L" | F6 fixed colour ON":L" | F6 fixed colour OFF"):L"");SetWindowTextW(app_window,title);
                }
                title_paused=paused;title_focused=focused;
            }
        }
        if(profile_cpu){std::ofstream m(output/"model-profile.csv");for(auto v:renderer.profile_model_ms)m<<v<<std::endl;}
        if(profile_cpu){std::ofstream e(output/"entity-profile.csv");e<<"source,total_ms,calls\n";for(const auto&v:barrier_owner.profile_entities)e<<v.first<<','<<v.second[0]<<','<<v.second[1]<<'\n';}
        if(profile_cpu){std::ofstream e(output/"slow-steps.csv");e<<"frame,source,ms\n";for(const auto&v:barrier_owner.profile_slow_steps)e<<uint32_t(v[0])<<','<<uint32_t(v[1])<<','<<v[2]<<'\n';}
        if(profile_cpu){std::ofstream f(output/"prop-transforms.csv");f.precision(10);for(const auto&r:barrier_owner.profile_props){for(unsigned i=0;i<r.size();i++)f<<(i?",":"")<<r[i];f<<'\n';}}
        if(profile_cpu){std::ofstream f(output/"prop-boxes.csv");f.precision(10);for(const auto&r:barrier_owner.profile_prop_boxes){for(unsigned i=0;i<r.size();i++)f<<(i?",":"")<<r[i];f<<'\n';}}
        if(profile_cpu){std::ofstream f(output/"cpu-profile.csv");f<<"frame,simulation_ms,render_ms,world_ms,player_physics_ms\n";for(const auto&r:profile_rows)f<<r[0]<<','<<r[1]<<','<<r[2]<<','<<r[3]<<','<<r[4]<<'\n';}
        if(fire_check){
            const auto before=level_particles->counts().fire_active;
            if(mission_world.close())throw std::runtime_error("Truck fire mission cleanup failed");
            if(shake_check&&uc_feedback_manager_observers(camera_feedback->manager())->position_count)throw std::runtime_error("Active shake receiver survived world shutdown");
            for(const auto&f:barrier_owner.fires)if(barrier_owner.navigation_boxes.group(f->entity.id))throw std::runtime_error("Fire routing obstacle survived shutdown");
            if(level_particles->counts().fire_active+barrier_owner.fires.size()!=before)throw std::runtime_error("Fire emitter shutdown incomplete");
            std::ofstream(output/"fire-cleanup.json")<<"{\"passed\":true,\"fires_closed\":"<<barrier_owner.fires.size()<<"}\n";
        }
        if(save_roundtrip_check&&!resume_requested){
            selected_save=save_store->load(1);if(!selected_save)throw std::runtime_error("Opening did not persist its checkpoint");
            resume_requested=true;output=checkpoint_check_root/"resume";continue;
        }
        if(!frontend_enabled||!return_to_menu)break;
        mission_active=false;paused=false;pointer_mode();
        }
        if(app_window){DestroyWindow(app_window);app_window=nullptr;} return 0;
    } catch(const std::exception &error) {
        ClipCursor(nullptr);ReleaseCapture();
        //Save terminal verification errors even when a fullscreen dialog is hidden.
        wchar_t failed_executable[MAX_PATH]{};
        if(GetModuleFileNameW(nullptr,failed_executable,MAX_PATH)){
            std::ofstream report(std::filesystem::path(failed_executable).parent_path()/"last-error.txt");
            report<<error.what()<<'\n';
        }
        if(headless) { fprintf(stderr,"Native scene: %s\n",error.what()); }
        else MessageBoxA(app_window,error.what(),"Native scene verification",MB_OK|MB_ICONERROR);
        if(app_window) DestroyWindow(app_window); return 1;
    }
}

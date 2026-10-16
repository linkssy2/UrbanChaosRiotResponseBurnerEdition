"""Build Urban Chaos against the complete pinned XboxXBERecomp runtime."""
from pathlib import Path
import hashlib,json,os,subprocess,struct,sys
sys.path.insert(0,str(Path(__file__).resolve().parent))
ROOT=Path(__file__).resolve().parents[1]
AREA=ROOT/'build/xbe-full-integration-20261014'
SDK=ROOT/'build/upstream-review-xboxxbe-20261014'
RELEASE=os.environ.get('UCRR_XBE_RELEASE')=='1'
assert os.environ.get('UCRR_SERVICES_HEAVY_MUTEX')=='owned'
report=json.loads((AREA/'integration-status.json').read_text())
assert report['tools_qualified']
env={k.upper():v for k,v in os.environ.items()}
vc=Path('C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/14.44.35207')
sdk=Path('C:/Program Files (x86)/Windows Kits/10');v='10.0.26100.0'
env['INCLUDE']=';'.join(map(str,[vc/'include',sdk/f'Include/{v}/ucrt',sdk/f'Include/{v}/shared',sdk/f'Include/{v}/um',sdk/f'Include/{v}/winrt']))
env['LIB']=';'.join(map(str,[vc/'lib/x64',sdk/f'Lib/{v}/ucrt/x64',sdk/f'Lib/{v}/um/x64']))
env['PATH']=str(vc/'bin/Hostx64/x64')+';'+str(sdk/f'bin/{v}/x64')+';'+env['PATH']
def save():(AREA/'integration-status.json').write_text(json.dumps(report,indent=2)+'\n')
def run(name,args):
 print(name,flush=True)
 with (AREA/(name+'.log')).open('w')as log:p=subprocess.run(list(map(str,args)),cwd=AREA,env=env,stdout=log,stderr=subprocess.STDOUT)
 report['steps'].append(dict(name=name,exit=p.returncode));save()
 assert p.returncode==0,(AREA/(name+'.log')).read_text()[-5000:]
try:
 report['runtime_built']=False
 report['live_status']='New executable pending checks and live verification'
 save()
 (AREA/'src/compat').mkdir(parents=True,exist_ok=True)
 from prepare_xbe_graphics_compat import prepare
 prepare()
 from prepare_xbe_visibility import prepare as prepare_visibility
 prepare_visibility(SDK, AREA)
 from prepare_xbe_inline_draws import prepare as prepare_inline_draws
 prepare_inline_draws(SDK, AREA)
 from prepare_xbe_resource_refs import prepare as prepare_resource_refs
 prepare_resource_refs(SDK, AREA)
 from prepare_xbe_shader_state import prepare as prepare_shader_state
 prepare_shader_state(SDK, AREA)
 from prepare_xbe_target60 import prepare as prepare_target60
 d3d_path=AREA/'src/compat/d3d8.cpp'
 d3d_path.write_text(prepare_target60(d3d_path.read_text(), AREA))
 from prepare_xbe_signed_lighting import prepare as prepare_signed_lighting
 renderer_path=AREA/'src/compat/d3d12.cpp'
 renderer_path.write_text(prepare_signed_lighting(renderer_path.read_text(), AREA))
 from prepare_xbe_depth_alias import prepare as prepare_depth_alias
 prepare_depth_alias(SDK, AREA)
 from prepare_xbe_smooth_fog import prepare as prepare_smooth_fog
 prepare_smooth_fog(SDK, AREA)
 from prepare_xbe_graphics_settings import prepare as prepare_graphics_settings
 settings_source,main_source,renderer_source=prepare_graphics_settings(
  (SDK/'runtime/app/settings.cpp').read_text(),(SDK/'runtime/app/main.cpp').read_text(),renderer_path.read_text(),AREA)
 (AREA/'src/compat/settings.cpp').write_text(settings_source)
 (AREA/'src/compat/main.cpp').write_text(main_source)
 from prepare_xbe_msaa import prepare as prepare_msaa
 renderer_source=prepare_msaa(renderer_source,AREA)
 from prepare_xbe_perf import prepare as prepare_perf
 renderer_source=prepare_perf(renderer_source,AREA)
 from prepare_xbe_shader_cache import prepare as prepare_shader_cache
 renderer_source=prepare_shader_cache(renderer_source,AREA)
 renderer_path.write_text(renderer_source)
 from prepare_xbe_filter_check import prepare as prepare_filter_check
 prepare_filter_check(AREA)
 # Let SDL select its supported Windows backends, including XInput Xbox pads.
 controller_source=(SDK/'runtime/core/input.cpp').read_text()
 for hint in ('SDL_HINT_JOYSTICK_WGI','SDL_HINT_JOYSTICK_RAWINPUT','SDL_HINT_XINPUT_ENABLED','SDL_HINT_JOYSTICK_DIRECTINPUT'):
  import re
  controller_source,n=re.subn(r'    SDL_SetHint\('+hint+r', "[01]"\);\n','',controller_source)
  assert n==1,hint
 for name in ('xbe_controller_style.h','xbe_controller_device.h'):
  (AREA/'src/compat'/name).write_text((ROOT/'scripts'/name).read_text())
 controller_source=controller_source.replace('#include "core/input.h"','#include "core/input.h"\n#include "xbe_controller_device.h"')
 controller_source=controller_source.replace('    SDL_SetHint(SDL_HINT_JOYSTICK_THREAD, "1");',
  '    SDL_SetHint("SDL_GAMECONTROLLER_USE_BUTTON_LABELS", "0"); // Keep physical face positions consistent.\n    SDL_SetHint(SDL_HINT_JOYSTICK_THREAD, "1");')
 controller_source=controller_source.replace('            p.connected = true;',
  '            p.connected = true;\n            if(port==0)trial_controller::family.store(trial_controller::Identify(SDL_GetRealGamepadType(pad)));')
 controller_source=controller_source.replace('        g_pads[i] = nullptr;',
  '        g_pads[i] = nullptr;\n        if(i==0)trial_controller::family.store(trial_controller::Xbox);')
 (AREA/'src/compat/input.cpp').write_text(controller_source)
 from prepare_xbe_input import prepare as prepare_input
 prepare_input()
 from prepare_xbe_event_callback import prepare as prepare_event_callback
 prepare_event_callback(SDK, AREA)
 from prepare_xbe_effect_settings import prepare as prepare_effect_settings
 prepare_effect_settings(AREA)
 from prepare_xbe_ragdoll_settings import prepare as prepare_ragdoll_settings
 prepare_ragdoll_settings(AREA)
 from prepare_xbe_intro_skip import prepare as prepare_intro_skip
 prepare_intro_skip(AREA)
 from prepare_xbe_health_pulse import prepare as prepare_health_pulse
 prepare_health_pulse(SDK, AREA)
 from prepare_xbe_pc_prompts import prepare as prepare_pc_prompts, prepare_guest as prepare_pc_prompt_guest
 prepare_pc_prompt_guest(AREA)
 from prepare_xbe_frontend import prepare as prepare_frontend
 prepare_frontend(AREA)
 from prepare_xbe_level_details import prepare as prepare_level_details
 prepare_level_details(AREA)
 from prepare_xbe_frontend_fonts import prepare as prepare_frontend_fonts
 prepare_frontend_fonts(AREA)
 from prepare_xbe_menu_style import prepare as prepare_menu_style
 prepare_menu_style(AREA)
 from prepare_xbe_coroutines import prepare as prepare_coroutines
 prepare_coroutines()
 from prepare_xbe_io_apc import prepare as prepare_io_apc
 prepare_io_apc(SDK, AREA)
 from prepare_xbe_language import prepare as prepare_language
 prepare_language(SDK, AREA)
 from prepare_xbe_heap_probe import prepare as prepare_heap_probe
 prepare_heap_probe(AREA)
 xbe=(AREA/'assets/Default.xbe').read_bytes()
 u=lambda o:struct.unpack_from('<I',xbe,o)[0]
 base=u(0x104); table=u(0x120)-base
 pins={0x423b41:'8b4424048d48f8f7d81bc023c18b0850ff5104c20400',
       0x3e5230:'51568b3538423f008b866c190000',
       0x42501d:'8b442404ff7424088bc883c0e4f7d91b',
       0x42506f:'e992fdffff56e83beaffff0fb6f0a120',
       0x42601c:'8b442404ff74240c8bc8ff74240c83c0',
       0x426190:'e908fcffff558becff7518d9451483ec',
       0x426d23:'8b4424048bc883c0f8f7d91bc923c851',
       0x427181:'ff7424108b442408ff7424108bc8ff74241083c0f8f7d91bc923c851e835fcffffc21000',
       0x426156:'e9eafaffff',
       0x3ef870:'83ec2c5355568b3538423f005733ed55',
       0x3e2c30:'8b4424048b0d38423f008bd0c1ea08',
       0x3e39f0:'53568b3538423f00578b7c24108b84be980f0000',
       0x3e9350:'8b0d38423f006a00e873520000',
       0x3e9370:'8b4424048b0d38423f008901c20400'}
 matched=set()
 for i in range(u(0x11c)):
  o=table+i*56;va,vs,raw=[u(o+j) for j in (4,8,12)]
  for address,expected in pins.items():
   if va<=address<va+vs:
    assert xbe[raw+address-va:raw+address-va+len(expected)//2].hex()==expected,hex(address)
    matched.add(address)
 assert matched==pins.keys()
 (AREA/'src/compat/device_refs.h').write_text('''#pragma once
#include <stdint.h>
static uint32_t uc_ds_ref(uint32_t &refs,bool add){
 if(add){if(refs!=UINT32_MAX)++refs;}else if(refs)--refs;
 return refs;
}
''')
 ds=(SDK/'runtime/xdk/dsound.cpp').read_text()
 from prepare_xbe_stream_factory import prepare as prepare_stream_factory
 ds=prepare_stream_factory(ds, AREA/'src/compat/test_stream_factory.cpp')
 from prepare_xbe_audio_rolloff import prepare as prepare_audio_rolloff
 ds=prepare_audio_rolloff(ds, AREA/'src/compat/test_audio_rolloff.cpp')
 (AREA/'src/compat/xbe_voice_mode.h').write_text((ROOT/'scripts/xbe_voice_mode.h').read_text())
 ds=ds.replace('#include "xdk/hle.h"','#include "xdk/hle.h"\n#include "xbe_voice_mode.h"',1)
 assert ds.count('uint32_t mode = 0;')==1
 ds=ds.replace('uint32_t mode = 0;','uint32_t mode = 0, pending_mode = UINT32_MAX;',1)
 old_commit='HLE_STDCALL(IDirectSound_CommitDeferredSettings, 1) { return 0; }'
 assert ds.count(old_commit)==1
 ds=ds.replace(old_commit,'''HLE_STDCALL(IDirectSound_CommitDeferredSettings, 1) {
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    for(auto& [id,v]:g_voices)uc_voice_mode_commit(v);
    return 0;
}''',1)
 ds=ds.replace('#include "xdk/hle.h"','#include "xdk/hle.h"\n#include "device_refs.h"\n#define SYM_IDirectSound_AddRef 00423B41',1)
 assert '#define SYM_IDirectSound_AddRef' in ds
 ds=ds.replace('uint32_t g_ds = 0;','uint32_t g_ds = 0, g_ds_refs = 0;',1)
 ds=ds.replace('    W32(a[1], g_ds);','    uc_ds_ref(g_ds_refs,true);\n    W32(a[1], g_ds);',1)
 old='HLE_STDCALL(IDirectSound_Release, 1) { return 0; }'
 assert ds.count(old)==1
 ds=ds.replace(old,'''// Urban Chaos .XTLID B0036: stdcall AddRef, one interface argument.
HLE_STDCALL(IDirectSound_AddRef, 1) {
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    return a[0] == g_ds && g_ds ? uc_ds_ref(g_ds_refs,true) : 0;
}
HLE_STDCALL(IDirectSound_Release, 1) {
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    // Keep the upstream process-owned audio device alive until shutdown.
    return a[0] == g_ds && g_ds ? uc_ds_ref(g_ds_refs,false) : 0;
}''',1)
 # GetVoiceProperties wrappers reach NV2A/APU objects absent from the host mixer.
 marker='BUFFER(Release, 1) {'
 assert ds.count(marker)==1
 ds=ds.replace(marker,'''// Public SetMode interfaces: (voice, DS3DMODE, immediate/deferred).
#define SYM_UrbanChaos_BufferMode 0042601C
#define SYM_UrbanChaos_StreamMode 00426190
static uint32_t VoiceMode(const Args& a) {
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    return uc_voice_mode(Find(a[0]),a[1],a[2]);
}
HLE_STDCALL(UrbanChaos_BufferMode, 3) { return VoiceMode(a); }
HLE_STDCALL(UrbanChaos_StreamMode, 3) { return VoiceMode(a); }
#define SYM_UrbanChaos_BufferVoiceProperties 0042501D
#define SYM_UrbanChaos_StreamVoiceProperties 0042506F
static uint32_t VoiceProperties(const Args& a) {
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    Voice* v = Find(a[0]);
    if (!v || !a[1]) return 0x80070057u;
    // shortcut: software mix bins are exact; hardware DSP/3D telemetry needs its native owner.
    std::memset(HostPtr(a[1]), 0, 0x5c);
    uint32_t n = v->nbins ? v->nbins : 2;
    W32(a[1], n);
    for (uint32_t i = 0; i < 8; ++i) {
        uint32_t bin = i < n ? (v->nbins ? v->bin[i] : i) : 0xffffffffu;
        W32(a[1] + 4 + i*8, bin);
        W32(a[1] + 8 + i*8, i < n ? uint32_t(v->bin_mb[bin]) : uint32_t(-10000));
    }
    return 0;
}
HLE_STDCALL(UrbanChaos_BufferVoiceProperties, 2) { return VoiceProperties(a); }
HLE_STDCALL(UrbanChaos_StreamVoiceProperties, 2) { return VoiceProperties(a); }
#define SYM_UrbanChaos_BufferPitch 00424F31
#define SYM_UrbanChaos_StreamPitch 0042503E
static uint32_t VoicePitch(const Args& a) {
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    Voice* v = Find(a[0]);
    int32_t pitch = int32_t(a[1]);
    if (!v || pitch < -8192 || pitch > 8191) return 0x80070057u;
    v->freq = uint32_t(std::lround(v->rate * std::exp2(pitch / 4096.0)));
    return 0;
}
HLE_STDCALL(UrbanChaos_BufferPitch, 2) { return VoicePitch(a); }
HLE_STDCALL(UrbanChaos_StreamPitch, 2) { return VoicePitch(a); }

'''+marker,1)
 from prepare_xbe_audio_cone import prepare as prepare_audio_cone
 ds=prepare_audio_cone(ds, AREA)
 from prepare_xbe_audio_factors import prepare as prepare_audio_factors
 ds=prepare_audio_factors(ds, AREA)
 from prepare_xbe_audio_pitch import prepare as prepare_audio_pitch
 ds=prepare_audio_pitch(ds, AREA)
 from prepare_xbe_stream_status import prepare as prepare_stream_status
 ds=prepare_stream_status(ds, AREA)
 from prepare_xbe_audio_routing import prepare as prepare_audio_routing
 ds=prepare_audio_routing(ds, AREA)
 from prepare_xbe_audio_loop import prepare as prepare_audio_loop
 ds,audio_output=prepare_audio_loop(ds,(SDK/'runtime/audio/xaudio2.cpp').read_text(),AREA)
 (AREA/'src/compat/xaudio2.cpp').write_text(audio_output)
 (AREA/'src/compat/dsound.cpp').write_text(ds)
 (AREA/'src/compat/test_device_refs.cpp').write_text('''#include "device_refs.h"
#include <cstdlib>
int main(){uint32_t r=0;if(uc_ds_ref(r,false)!=0||uc_ds_ref(r,true)!=1||uc_ds_ref(r,true)!=2||uc_ds_ref(r,false)!=1||uc_ds_ref(r,false)!=0)return 1;r=UINT32_MAX;if(uc_ds_ref(r,true)!=UINT32_MAX)return 2;return 0;}
''')
 (AREA/'src/common').mkdir(parents=True,exist_ok=True)
 from prepare_xbe_cursor import prepare as prepare_cursor
 from prepare_xbe_graphics_input import prepare as prepare_graphics_input
 for name in ('xbe_trial_menu_input.cpp','xbe_trial_hud.cpp','xbe_trial_graphics_ui.cpp','xbe_trial_graphics_ui.h',
              'xbe_controls.h','xbe_controls_backend.inc','xbe_controls_capture.h','xbe_controls_ui.inc','xbe_language_ui.inc'):
  source=(ROOT/'scripts'/name).read_text()
  if name=='xbe_trial_menu_input.cpp':
   source,renderer_source=prepare_pc_prompts(source,renderer_path.read_text(),AREA)
   renderer_path.write_text(renderer_source)
   source=prepare_graphics_input(prepare_cursor(source,AREA),AREA)
  (AREA/'src/common'/name).write_text(source)
 from prepare_xbe_conversation_skip import prepare as prepare_conversation_skip
 prepare_conversation_skip(AREA)
 info=(SDK/'templates/game/src/common/game_info.cpp').read_text()
 info=info.replace('#include "app/app.h"','#include "app/app.h"\n#include "xbe_graphics_settings.h"\nextern bool TrialInputMessage(void*, uint32_t, uintptr_t, intptr_t, intptr_t*);\nextern void TrialInputWindow(void*);\nextern void TrialInputPad(int, input::Pad&);\nextern void TrialHudUi(double);\nextern bool TrialShowFps;\nextern bool TrialSmoothFog;\nextern bool TrialDistanceFog;\nextern uint32_t TrialFogDistanceMode;')
 info=info.replace('.window = nullptr,','.window = TrialInputWindow,')
 info=info.replace('.message = nullptr,','.message = TrialInputMessage,').replace('.pad = nullptr,','.pad = TrialInputPad,')
 assert info.count('.ui = nullptr,')==1 and info.count('.options = {},')==1
 info=info.replace('.ui = nullptr,','.ui = TrialHudUi,').replace('.options = {},','.options = {{"fps_counter", &TrialShowFps, nullptr}, {"smooth_fog", &TrialSmoothFog, nullptr}, {"distance_fog", &TrialDistanceFog, nullptr}, {"fog_distance_mode", nullptr, &TrialFogDistanceMode}, {"aspect_ratio", nullptr, &TrialAspectRatio}, {"msaa_samples", nullptr, &TrialMsaaSamples}, {"scenery_lighting", &TrialSceneryLighting, nullptr}, {"character_lighting", &TrialCharacterLighting, nullptr}, {"character_quality", nullptr, &TrialCharacterQuality}, {"ragdoll_amount", nullptr, &TrialRagdollMode}},')
 assert info.count('.init = nullptr,')==1
 info=info.replace('.init = nullptr,','.init = trial_graphics::Initialize,')
 info=info.replace('#include "xbe_graphics_settings.h"','#include "xbe_graphics_settings.h"\n#include "xbe_language.h"')
 info=info.replace('{"ragdoll_amount", nullptr, &TrialRagdollMode}',
                   '{"ragdoll_amount", nullptr, &TrialRagdollMode}, {"language", nullptr, &TrialLanguage}')
 info=info.replace('.init = trial_graphics::Initialize,',
                   '.init = []{trial_graphics::Initialize();trial_language::Initialize();},')
 info=info.replace('Game Title','Urban Chaos XboxXBERecomp Integration').replace('L"Game"','L"UrbanChaosXbeTrial"').replace('"game",','"urban_chaos_xbe_trial",').replace('"Game"}}','"UrbanChaosXbeTrial"}}')
 if RELEASE:
  info=info.replace('L"Urban Chaos XboxXBERecomp Integration"','L"Urban Chaos: Riot Response - Burner Edition"')
  info=info.replace('.short_title = L"UrbanChaosXbeTrial"','.short_title = L"Burner Edition"')
 (AREA/'src/common/game_info.cpp').write_text(info)
 from prepare_xbe_frame_costs import prepare as prepare_frame_costs
 prepare_frame_costs(AREA)
 from prepare_xbe_release import prepare as prepare_release
 prepare_release(SDK, AREA)
 from prepare_xbe_release_assets import prepare as prepare_release_assets
 prepare_release_assets(SDK, AREA)
 (AREA/'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.25)
project(UrbanChaosXbeTrial LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include("'''+SDK.as_posix()+'''/cmake/XbeRecomp.cmake")
set(UC_ZLIB_DLL "C:/Users/Administrator/.cache/codex-runtimes/codex-primary-runtime/dependencies/native/git/mingw64/bin/zlib1.dll")
set(UC_ZLIB_INCLUDE "C:/Users/Administrator/.cache/codex-runtimes/codex-primary-runtime/dependencies/python/Lib/site-packages/lxml/includes/extlibs")
# The pinned runtime uses clang's GNU frontend/llvm-ar; the existing archive
# target expects cl/lib flags. Reuse its C sources with this toolchain instead.
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/zlib.def" "LIBRARY zlib1.dll\\nEXPORTS\\n uncompress2\\n zlibVersion\\n")
get_filename_component(trial_compiler_dir "${CMAKE_C_COMPILER}" DIRECTORY)
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/zlib-existing.lib"
    COMMAND "${trial_compiler_dir}/llvm-lib.exe" /nologo /machine:x64 "/def:${CMAKE_CURRENT_BINARY_DIR}/zlib.def" "/out:${CMAKE_CURRENT_BINARY_DIR}/zlib-existing.lib"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/zlib.def" VERBATIM)
add_custom_target(trial_zlib_import DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/zlib-existing.lib")
add_library(uc_native_texture STATIC "'''+(ROOT/'native/texture_asset/texture_asset.c').as_posix()+'''"
    "'''+(ROOT/'native/archive_reader/archive_reader.c').as_posix()+'''" "'''+(ROOT/'native/archive_lookup.c').as_posix()+'''")
add_dependencies(uc_native_texture trial_zlib_import)
target_include_directories(uc_native_texture PRIVATE "${UC_ZLIB_INCLUDE}" PUBLIC "'''+(ROOT/'native/archive_reader').as_posix()+'''")
target_link_libraries(uc_native_texture PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/zlib-existing.lib")
configure_file("${UC_ZLIB_DLL}" "${CMAKE_CURRENT_BINARY_DIR}/zlib1.dll" COPYONLY)
file(GLOB XBE_HOST_SOURCES CONFIGURE_DEPENDS src/common/*.cpp)
list(FILTER XBE_HOST_SOURCES EXCLUDE REGEX "/test_[^/]*[.]cpp$")
list(FILTER XBE_RUNTIME_SOURCES EXCLUDE REGEX "/runtime/xdk/dsound.cpp$")
list(FILTER XBE_RUNTIME_SOURCES EXCLUDE REGEX "/runtime/audio/xaudio2.cpp$")
list(FILTER XBE_RUNTIME_SOURCES EXCLUDE REGEX "/runtime/xdk/d3d8.cpp$")
list(FILTER XBE_RUNTIME_SOURCES EXCLUDE REGEX "/runtime/gpu/d3d12.cpp$")
list(FILTER XBE_RUNTIME_SOURCES EXCLUDE REGEX "/runtime/gpu/vertex.cpp$")
list(FILTER XBE_RUNTIME_SOURCES EXCLUDE REGEX "/runtime/gpu/shaders.cpp$")
list(FILTER XBE_RUNTIME_SOURCES EXCLUDE REGEX "/runtime/kernel/xboxkrnl.cpp$")
list(FILTER XBE_RUNTIME_SOURCES EXCLUDE REGEX "/runtime/core/input.cpp$")
list(APPEND XBE_HOST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/compat/input.cpp")
list(FILTER XBE_RUNTIME_SOURCES EXCLUDE REGEX "/runtime/core/log.cpp$")
list(FILTER XBE_RUNTIME_SOURCES EXCLUDE REGEX "/runtime/gpu/render_thread.cpp$")
list(FILTER XBE_RUNTIME_SOURCES EXCLUDE REGEX "/runtime/app/(settings|main|game_files)[.]cpp$")
list(APPEND XBE_HOST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/compat/game_files.cpp")
list(APPEND XBE_HOST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/compat/settings.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/compat/main.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/compat/xbe_graphics_settings.cpp")
list(APPEND XBE_HOST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/compat/xboxkrnl.cpp")
list(APPEND XBE_HOST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/compat/log.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/compat/render_thread.cpp")
list(APPEND XBE_HOST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/compat/control_indexed.c")
list(APPEND XBE_HOST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/compat/xbe_pc_prompt_art.cpp")
list(APPEND XBE_HOST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/compat/xbe_shader_cache.cpp")
list(APPEND XBE_HOST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/compat/xbe_frontend_fonts.cpp" "'''+(ROOT/'native/font.c').as_posix()+'''")
list(APPEND XBE_HOST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/compat/vertex.cpp")
list(APPEND XBE_HOST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/compat/shaders.cpp")
list(APPEND XBE_HOST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/compat/dsound.cpp")
list(APPEND XBE_HOST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/compat/xaudio2.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/compat/xbe_audio_loop.cpp")
list(APPEND XBE_HOST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/compat/d3d8.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/compat/d3d12.cpp")
add_executable(test_controller_device "'''+(ROOT/'scripts/test_xbe_controller_device.cpp').as_posix()+'''" src/compat/input.cpp "'''+(SDK/'runtime/core/log.cpp').as_posix()+'''")
target_include_directories(test_controller_device PRIVATE "'''+(SDK/'runtime').as_posix()+'''")
target_link_libraries(test_controller_device PRIVATE SDL3::SDL3)
add_executable(test_device_refs src/compat/test_device_refs.cpp)
add_executable(test_event_callback src/compat/test_event_callback.cpp)
target_include_directories(test_event_callback PRIVATE "'''+(SDK/'runtime').as_posix()+'''" generated/game)
add_executable(test_stream_factory src/compat/test_stream_factory.cpp)
add_executable(test_io_apc src/compat/test_io_apc.cpp)
target_include_directories(test_io_apc PRIVATE "'''+(SDK/'runtime').as_posix()+'''")
add_executable(test_heap_probe src/compat/test_heap_probe.cpp)
target_include_directories(test_heap_probe PRIVATE "'''+ROOT.as_posix()+'''" "'''+(SDK/'runtime').as_posix()+'''")
add_executable(test_visibility src/compat/test_visibility.cpp)
target_include_directories(test_visibility PRIVATE "'''+(SDK/'runtime').as_posix()+'''" generated/game)
add_executable(test_inline_draws src/compat/test_inline_draws.cpp)
target_include_directories(test_inline_draws PRIVATE "'''+(SDK/'runtime').as_posix()+'''" generated/game)
add_executable(test_resource_refs src/compat/test_resource_refs.cpp)
target_include_directories(test_resource_refs PRIVATE "'''+(SDK/'runtime').as_posix()+'''" generated/game)
add_executable(test_shader_state src/compat/test_shader_state.cpp)
target_include_directories(test_shader_state PRIVATE "'''+(SDK/'runtime').as_posix()+'''" generated/game)
add_executable(test_cursor src/compat/test_cursor.cpp)
target_include_directories(test_cursor PRIVATE "'''+ROOT.as_posix()+'''")
target_link_libraries(test_cursor PRIVATE user32 gdi32)
add_executable(test_target60 src/compat/test_target60.cpp)
target_include_directories(test_target60 PRIVATE "'''+ROOT.as_posix()+'''" "'''+(SDK/'runtime').as_posix()+'''" generated/game)
add_executable(test_signed_lighting src/compat/test_signed_lighting.cpp)
target_include_directories(test_signed_lighting PRIVATE src/compat "'''+(SDK/'runtime').as_posix()+'''" generated/game)
target_link_libraries(test_signed_lighting PRIVATE d3d12 dxgi d3dcompiler)
add_executable(test_depth_alias src/compat/test_depth_alias.cpp)
target_include_directories(test_depth_alias PRIVATE src/compat src "'''+(SDK/'runtime').as_posix()+'''")
target_link_libraries(test_depth_alias PRIVATE d3d12 dxgi d3dcompiler)
add_executable(test_msaa src/compat/test_msaa.cpp)
target_include_directories(test_msaa PRIVATE src/compat src "'''+(SDK/'runtime').as_posix()+'''")
target_link_libraries(test_msaa PRIVATE d3d12 dxgi d3dcompiler)
add_executable(test_perf src/compat/test_perf.cpp "'''+(SDK/'runtime/core/log.cpp').as_posix()+'''")
target_include_directories(test_perf PRIVATE "'''+(SDK/'runtime').as_posix()+'''")
target_compile_definitions(test_perf PRIVATE NDEBUG)
add_executable(test_frame_reuse src/compat/test_xbe_frame_reuse.cpp)
add_executable(test_readback_reuse src/compat/test_xbe_readback_reuse.cpp)
target_link_libraries(test_readback_reuse PRIVATE d3d12 dxgi)
add_executable(test_shader_cache src/compat/test_shader_cache.cpp)
target_include_directories(test_shader_cache PRIVATE src/compat "'''+(SDK/'runtime').as_posix()+'''")
target_link_libraries(test_shader_cache PRIVATE bcrypt d3d12 dxgi d3dcompiler)
add_executable(test_effect_settings src/compat/test_effect_settings.cpp)
target_include_directories(test_effect_settings PRIVATE src/common src/compat "'''+(SDK/'runtime').as_posix()+'''")
add_executable(test_ragdoll_settings src/compat/test_ragdoll_settings.cpp)
target_include_directories(test_ragdoll_settings PRIVATE src/common "'''+(SDK/'runtime').as_posix()+'''")
add_executable(test_audio_rolloff src/compat/test_audio_rolloff.cpp)
target_include_directories(test_audio_rolloff PRIVATE "'''+ROOT.as_posix()+'''" "'''+(SDK/'runtime').as_posix()+'''" generated/game)
add_executable(test_audio_cone src/compat/test_audio_cone.cpp)
target_include_directories(test_audio_cone PRIVATE "'''+ROOT.as_posix()+'''" "'''+(SDK/'runtime').as_posix()+'''" generated/game)
add_executable(test_audio_factors src/compat/test_audio_factors.cpp)
target_include_directories(test_audio_factors PRIVATE "'''+ROOT.as_posix()+'''" "'''+(SDK/'runtime').as_posix()+'''" generated/game)
add_executable(test_audio_pitch src/compat/test_audio_pitch.cpp)
target_include_directories(test_audio_pitch PRIVATE "'''+ROOT.as_posix()+'''" "'''+(SDK/'runtime').as_posix()+'''" generated/game)
add_executable(test_stream_status src/compat/test_stream_status.cpp)
add_executable(test_audio_loop src/compat/test_audio_loop.cpp src/compat/xbe_audio_loop.cpp)
add_executable(test_audio_routing src/compat/test_audio_routing.cpp)
target_include_directories(test_audio_routing PRIVATE "'''+ROOT.as_posix()+'''" "'''+(SDK/'runtime').as_posix()+'''" generated/game)
target_compile_definitions(test_audio_loop PRIVATE XBE_AUDIO_TRACE_TEST=1)
target_include_directories(test_stream_status PRIVATE "'''+ROOT.as_posix()+'''" "'''+(SDK/'runtime').as_posix()+'''" generated/game)
add_executable(test_recorded_draws "'''+(ROOT/'scripts/test_xbe_recorded_draws.cpp').as_posix()+'''" src/compat/control_indexed.c)
target_include_directories(test_recorded_draws PRIVATE "'''+ROOT.as_posix()+'''" "'''+(ROOT/'native').as_posix()+'''" "'''+(SDK/'runtime').as_posix()+'''" src/compat)
add_executable(test_voice_mode "'''+(ROOT/'scripts/test_xbe_voice_mode.cpp').as_posix()+'''")
add_executable(test_coroutines src/compat/test_coroutines.cpp)
target_include_directories(test_coroutines PRIVATE "'''+(SDK/'runtime').as_posix()+'''")
add_executable(test_pc_keyboard "'''+(ROOT/'scripts/test_keyboard_config.c').as_posix()+'''")
target_include_directories(test_pc_keyboard PRIVATE "'''+(ROOT/'third_party/xboxrecomp/src/input').as_posix()+'''")
target_include_directories(test_pc_keyboard PRIVATE "'''+(ROOT/'third_party/xboxrecomp/include').as_posix()+'''")
target_include_directories(test_pc_keyboard PRIVATE "'''+(ROOT/'third_party/xboxrecomp/src').as_posix()+'''")
target_link_libraries(test_pc_keyboard PRIVATE user32 xinput)
add_executable(test_pc_camera "'''+(ROOT/'scripts/test_pc_mouse_camera.c').as_posix()+'''")
add_executable(test_pc_memory "'''+(ROOT/'scripts/test_xbe_input_memory.cpp').as_posix()+'''")
add_executable(test_graphics_menu_context src/common/test_xbe_graphics_menu_context.cpp)
target_include_directories(test_graphics_menu_context PRIVATE src/common)
add_executable(test_health_pulse src/compat/test_health_pulse.cpp)
target_include_directories(test_health_pulse PRIVATE src src/compat "'''+ROOT.as_posix()+'''" "'''+(SDK/'runtime').as_posix()+'''")
add_executable(test_intro_skip src/compat/test_intro_skip.cpp)
add_executable(test_conversation_skip src/compat/test_conversation_skip.cpp)
target_include_directories(test_conversation_skip PRIVATE src/common "'''+(SDK/'runtime').as_posix()+'''" generated/game)
target_include_directories(test_intro_skip PRIVATE src src/compat "'''+ROOT.as_posix()+'''" "'''+(SDK/'runtime').as_posix()+'''")
add_executable(test_pc_prompts src/compat/test_pc_prompts.cpp src/compat/xbe_pc_prompt_art.cpp)
target_include_directories(test_pc_prompts PRIVATE src/compat "'''+ROOT.as_posix()+'''" "'''+(SDK/'runtime').as_posix()+'''")
target_link_libraries(test_pc_prompts PRIVATE gdi32 user32)
add_executable(test_prompt_handoff src/compat/test_prompt_handoff.cpp)
target_include_directories(test_prompt_handoff PRIVATE src "'''+(SDK/'runtime').as_posix()+'''")
add_executable(test_language src/compat/test_language.cpp)
target_include_directories(test_language PRIVATE "'''+ROOT.as_posix()+'''" src/common src/compat src "'''+(SDK/'runtime').as_posix()+'''" generated/game)
add_executable(test_release src/compat/test_release.cpp src/compat/log.cpp)
target_include_directories(test_release PRIVATE src src/compat "'''+(SDK/'runtime').as_posix()+'''")
target_compile_definitions(test_release PRIVATE UC_BURNER_RELEASE)
add_executable(test_release_assets src/compat/test_release_assets.cpp "'''+(SDK/'runtime/kernel/crypto.cpp').as_posix()+'''")
target_include_directories(test_release_assets PRIVATE src/compat "'''+(SDK/'runtime').as_posix()+'''" generated/game)
add_executable(test_filter src/compat/test_filter.cpp)
target_include_directories(test_filter PRIVATE src "'''+(SDK/'runtime').as_posix()+'''")
add_executable(test_frontend src/compat/test_frontend.cpp)
target_include_directories(test_frontend PRIVATE src src/common src/compat generated/game "'''+ROOT.as_posix()+'''" "'''+(SDK/'runtime').as_posix()+'''")
add_executable(test_level_details src/compat/test_level_details.cpp)
target_include_directories(test_level_details PRIVATE src/common "'''+(ROOT/'src').as_posix()+'''" "'''+(SDK/'runtime').as_posix()+'''")
add_executable(test_frontend_fonts src/compat/test_frontend_fonts.cpp src/compat/xbe_frontend_fonts.cpp src/compat/xbe_pc_prompt_art.cpp "'''+(ROOT/'native/font.c').as_posix()+'''"
    ${imgui_SOURCE_DIR}/imgui.cpp ${imgui_SOURCE_DIR}/imgui_draw.cpp ${imgui_SOURCE_DIR}/imgui_tables.cpp ${imgui_SOURCE_DIR}/imgui_widgets.cpp)
target_include_directories(test_frontend_fonts PRIVATE src/compat "'''+ROOT.as_posix()+'''" "'''+(SDK/'runtime').as_posix()+'''" ${imgui_SOURCE_DIR})
target_link_libraries(test_frontend_fonts PRIVATE uc_native_texture gdi32)
add_executable(test_menu_style src/compat/test_menu_style.cpp src/compat/xbe_frontend_fonts.cpp src/compat/xbe_pc_prompt_art.cpp "'''+(ROOT/'native/font.c').as_posix()+'''"
    ${imgui_SOURCE_DIR}/imgui.cpp ${imgui_SOURCE_DIR}/imgui_draw.cpp ${imgui_SOURCE_DIR}/imgui_tables.cpp ${imgui_SOURCE_DIR}/imgui_widgets.cpp)
target_include_directories(test_menu_style PRIVATE src/common src/compat "'''+ROOT.as_posix()+'''" "'''+(SDK/'runtime').as_posix()+'''" ${imgui_SOURCE_DIR})
target_link_libraries(test_menu_style PRIVATE uc_native_texture gdi32 user32)
add_executable(test_graphics_input src/compat/test_graphics_input.cpp)
target_include_directories(test_graphics_input PRIVATE src/common "'''+ROOT.as_posix()+'''" "'''+(SDK/'runtime').as_posix()+'''")
target_link_libraries(test_graphics_input PRIVATE user32 xinput)
add_executable(test_controls "'''+(ROOT/'scripts/test_xbe_controls.cpp').as_posix()+'''")
target_include_directories(test_controls PRIVATE "'''+ROOT.as_posix()+'''" "'''+(SDK/'runtime').as_posix()+'''")
target_link_libraries(test_controls PRIVATE user32 xinput)
add_executable(test_graphics_settings src/compat/test_graphics_settings.cpp)
target_include_directories(test_graphics_settings PRIVATE src/compat "'''+(SDK/'runtime').as_posix()+'''")
target_link_libraries(test_graphics_settings PRIVATE user32)
add_executable(test_target_resize src/compat/test_target_resize.cpp)
target_include_directories(test_target_resize PRIVATE src/compat "'''+(SDK/'runtime').as_posix()+'''")
add_executable(test_smooth_fog src/compat/test_smooth_fog.cpp "'''+(ROOT/'native/environment_fog.c').as_posix()+'''")
target_include_directories(test_smooth_fog PRIVATE src/compat src "'''+ROOT.as_posix()+'''" "'''+(SDK/'runtime').as_posix()+'''")
target_link_libraries(test_smooth_fog PRIVATE d3d12 dxgi d3dcompiler)
add_xbe_image(UrbanChaosXbeTrial Default.xbe game.toml 128 game)
option(UC_BURNER_RELEASE "Remove development captures and routine logging" OFF)
if(UC_BURNER_RELEASE)
    target_compile_definitions(UrbanChaosXbeTrial PRIVATE UC_BURNER_RELEASE)
endif()
target_compile_definitions(UrbanChaosXbeTrial PRIVATE UC_PRESENT_SURFACE_LAYOUT=2)
target_link_libraries(UrbanChaosXbeTrial PRIVATE SDL3::SDL3 bcrypt gdi32 xinput uc_native_texture)
target_include_directories(UrbanChaosXbeTrial PRIVATE "'''+ROOT.as_posix()+'''" src/compat src/common)
''')
 clang=Path('C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/Llvm/x64/bin')
 deps=['-DFETCHCONTENT_SOURCE_DIR_'+name.upper()+'='+str(AREA/'out/_deps'/(name+'-src')) for name in ('zydis','tomlplusplus','imgui','sdl3')]
 from setup_xbe_libusb import prepare as prepare_libusb
 deps+=prepare_libusb(AREA)
 run('configure-game-msvc',['cmake','-S',AREA,'-B',AREA/'out-msvc','-G','Ninja','-DUC_BURNER_RELEASE='+('ON' if RELEASE else 'OFF'),'-DCMAKE_C_COMPILER='+str(clang/'clang.exe'),'-DCMAKE_CXX_COMPILER='+str(clang/'clang++.exe'),'-DCMAKE_BUILD_TYPE=RelWithDebInfo','-DCMAKE_C_FLAGS=-march=x86-64-v2','-DCMAKE_CXX_FLAGS=-march=x86-64-v2',*deps])
 config=(AREA/'out-msvc/_deps/sdl3-build/include-config-relwithdebinfo/build_config/SDL_build_config.h').read_text()
 assert '#define HAVE_LIBUSB 1' in config,'Switch 2 USB driver would be silently omitted'
 run('build-game-msvc',['cmake','--build',AREA/'out-msvc','--target','UrbanChaosXbeTrial','test_device_refs','test_pc_keyboard','test_pc_camera','test_pc_memory','test_coroutines','test_voice_mode','test_stream_factory','test_io_apc','test_recorded_draws','test_visibility','test_inline_draws','test_resource_refs','test_shader_state','test_cursor','test_target60','test_signed_lighting','test_depth_alias','test_audio_rolloff','test_audio_cone','test_audio_factors','test_audio_pitch','test_stream_status','test_audio_loop','test_audio_routing','test_heap_probe','test_graphics_menu_context','test_graphics_input','test_graphics_settings','test_target_resize','test_smooth_fog','-j','4'])
 run('build-interface-fixtures',['cmake','--build',AREA/'out-msvc','--target','test_release_assets','test_language','test_release','test_prompt_handoff','test_event_callback','test_shader_cache','test_perf','test_controls','test_msaa','test_effect_settings','test_ragdoll_settings','test_filter','test_intro_skip','test_conversation_skip','test_health_pulse','test_pc_prompts','test_frontend','test_level_details','test_frontend_fonts','test_menu_style','-j','4'])
 run('build-controller-device',['cmake','--build',AREA/'out-msvc','--target','test_controller_device','-j','4'])
 run('test-controller-device',[AREA/'out-msvc/test_controller_device.exe'])
 run('test-language',[AREA/'out-msvc/test_language.exe'])
 run('test-release',[AREA/'out-msvc/test_release.exe'])
 run('test-release-assets',[AREA/'out-msvc/test_release_assets.exe',AREA/'assets/Default.xbe',ROOT/'game_files/Default.xbe'])
 run('test-event-callback',[AREA/'out-msvc/test_event_callback.exe'])
 run('build-frame-reuse',['cmake','--build',AREA/'out-msvc','--target','test_frame_reuse','test_readback_reuse','-j','4'])
 run('test-frame-reuse',[AREA/'out-msvc/test_frame_reuse.exe'])
 run('test-readback-reuse',[AREA/'out-msvc/test_readback_reuse.exe'])
 run('test-readback-reuse-hardware',[AREA/'out-msvc/test_readback_reuse.exe','hardware'])
 run('test-shader-cache',[AREA/'out-msvc/test_shader_cache.exe'])
 run('test-perf-disabled',[AREA/'out-msvc/test_perf.exe','disabled'])
 run('test-perf-enabled',[AREA/'out-msvc/test_perf.exe','enabled'])
 run('test-controls',[AREA/'out-msvc/test_controls.exe'])
 run('test-msaa',[AREA/'out-msvc/test_msaa.exe'])
 run('test-msaa-hardware',[AREA/'out-msvc/test_msaa.exe','hardware'])
 run('test-effect-settings',[AREA/'out-msvc/test_effect_settings.exe'])
 run('test-ragdoll-settings',[AREA/'out-msvc/test_ragdoll_settings.exe'])
 run('test-filter',[AREA/'out-msvc/test_filter.exe'])
 run('test-intro-skip',[AREA/'out-msvc/test_intro_skip.exe'])
 run('test-conversation-skip',[AREA/'out-msvc/test_conversation_skip.exe'])
 run('test-health-pulse',[AREA/'out-msvc/test_health_pulse.exe'])
 run('test-pc-prompts',[AREA/'out-msvc/test_pc_prompts.exe'])
 run('test-prompt-handoff',[AREA/'out-msvc/test_prompt_handoff.exe'])
 run('test-frontend',[AREA/'out-msvc/test_frontend.exe'])
 run('test-level-details',[AREA/'out-msvc/test_level_details.exe'])
 run('test-frontend-fonts',[AREA/'out-msvc/test_frontend_fonts.exe'])
 run('test-menu-style',[AREA/'out-msvc/test_menu_style.exe'])
 run('test-graphics-menu-context',[AREA/'out-msvc/test_graphics_menu_context.exe'])
 run('test-graphics-input',[AREA/'out-msvc/test_graphics_input.exe'])
 run('test-graphics-settings',[AREA/'out-msvc/test_graphics_settings.exe'])
 run('test-target-resize',[AREA/'out-msvc/test_target_resize.exe'])
 run('test-smooth-fog',[AREA/'out-msvc/test_smooth_fog.exe'])
 run('test-smooth-fog-hardware',[AREA/'out-msvc/test_smooth_fog.exe','hardware'])
 run('test-depth-alias',[AREA/'out-msvc/test_depth_alias.exe'])
 run('test-signed-lighting',[AREA/'out-msvc/test_signed_lighting.exe'])
 run('test-signed-lighting-hardware',[AREA/'out-msvc/test_signed_lighting.exe','hardware'])
 run('test-target60',[AREA/'out-msvc/test_target60.exe'])
 run('test-cursor',[AREA/'out-msvc/test_cursor.exe'])
 run('test-shader-state',[AREA/'out-msvc/test_shader_state.exe'])
 run('test-audio-pitch',[AREA/'out-msvc/test_audio_pitch.exe'])
 run('test-stream-status',[AREA/'out-msvc/test_stream_status.exe'])
 run('test-audio-loop',[AREA/'out-msvc/test_audio_loop.exe'])
 run('test-audio-routing',[AREA/'out-msvc/test_audio_routing.exe'])
 run('test-audio-cone',[AREA/'out-msvc/test_audio_cone.exe'])
 run('test-audio-factors',[AREA/'out-msvc/test_audio_factors.exe'])
 run('test-resource-refs',[AREA/'out-msvc/test_resource_refs.exe'])
 run('test-heap-probe',[AREA/'out-msvc/test_heap_probe.exe'])
 run('test-heap-probe-disabled',[AREA/'out-msvc/test_heap_probe.exe','disabled'])
 run('test-inline-draws',[AREA/'out-msvc/test_inline_draws.exe'])
 run('test-audio-rolloff',[AREA/'out-msvc/test_audio_rolloff.exe'])
 run('test-visibility',[AREA/'out-msvc/test_visibility.exe'])
 run('test-recorded-draws',[AREA/'out-msvc/test_recorded_draws.exe'])
 run('test-io-apc',[AREA/'out-msvc/test_io_apc.exe'])
 run('test-stream-factory',[AREA/'out-msvc/test_stream_factory.exe'])
 run('test-voice-mode',[AREA/'out-msvc/test_voice_mode.exe'])
 run('test-coroutines',[AREA/'out-msvc/test_coroutines.exe'])
 run('test-device-refs',[AREA/'out-msvc/test_device_refs.exe'])
 run('test-pc-keyboard',[AREA/'out-msvc/test_pc_keyboard.exe'])
 run('test-pc-camera',[AREA/'out-msvc/test_pc_camera.exe'])
 run('test-pc-memory',[AREA/'out-msvc/test_pc_memory.exe'])
 report['runtime_built']=True
 report['release_diagnostics_removed']=RELEASE
 report['runtime_sha256']=hashlib.sha256((AREA/'out-msvc/UrbanChaosXbeTrial.exe').read_bytes()).hexdigest()
finally:save()


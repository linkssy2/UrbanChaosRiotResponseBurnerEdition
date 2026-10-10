"""Original targeted-orders visibility + live production gesture/artwork, no game."""
import datetime,hashlib,json,os,shutil,subprocess,sys
from pathlib import Path
root=Path(__file__).resolve().parents[2];here=Path(__file__).resolve().parent
sys.path.insert(0,str(root/'scripts'))
import prepare_pc_orders_hud as hook
from prepare_hybrid_lighting import body
out=root/'build/hybrid-native-frontend-20261007/medic-cue-checks'/datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%d-%H%M%S-%f');out.mkdir(parents=True)
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
paths=[here/'test.c',here/'run.py',root/'scripts/prepare_pc_orders_hud.py',root/'scripts/test_pc_input_backend.c',root/'scripts/light_removal_checks/test.c',root/'src/recomp/gen/recomp_0012.c',root/'src/recomp/recomp_types.h',root/'third_party/xboxrecomp/include/host_controller.h']
paths += [root/'third_party/xboxrecomp/src/input'/p for p in ('xinput_device.c','host_keyboard_config.h','host_controller_config.h','xinput_poll_policy.h')]
paths += [root/'third_party/xboxrecomp/include/pc_mouse_state.h']
paths += [root/'third_party/xboxrecomp/src/nv2a/nv2a_pgraph_d3d11.c',root/'native/runtime/hud_keys.c']
paths += [root/'third_party/xboxrecomp/src/nv2a'/p for p in ('nv2a_pc_hud_art.h','nv2a_pc_hud_assets.h','nv2a_pc_hud.h')]
report={'passed':False,'scope':__doc__,'sources':{str(p.relative_to(root)):sha(p)for p in paths},'steps':[]}
def save():(out/'results.json').write_text(json.dumps(report,indent=2)+'\n')
def run(name,args):
    p=subprocess.run(list(map(str,args)),cwd=out,capture_output=True,text=True,timeout=180,env={k:v for k,v in os.environ.items()if not k.upper().startswith('UCRR_')})
    (out/f'{name}.log').write_text(p.stdout+p.stderr);report['steps'].append({'name':name,'returncode':p.returncode});save();print(name,p.returncode,flush=True)
    if p.returncode:raise RuntimeError((p.stdout+p.stderr)[-6000:])
    return p.stdout
try:
    for p in paths:
        d=out/'source'/p.relative_to(root);d.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(p,d)
    report['generator']=hook.self_check()
    renderer=(root/'third_party/xboxrecomp/src/nv2a/nv2a_pgraph_d3d11.c').read_text(encoding='utf-8')
    assert 'if(!tex&&pc_hud_stage[0]==2&&xbox_KeyboardMode()){\n            d3d8_texture_bindings_end();d3d8_combiners_set_nv097(NULL);return;' in renderer
    report['renderer_boundary']='Missing PC orders texture returns before sprite GPU submission, with texture transaction and raw combiner scope closed; source-pinned, not GPU failure-injected.'
    source=(root/'src/recomp/gen/recomp_0012.c').read_text();original=hook.body(source);candidate=hook.body(hook.transform(source))
    # Execute the exact selected visibility tail and original container traversal;
    # the upstream NPC/mission command discovery remains source-owned/unmodified.
    def gate(text,name):
        head=text[:text.index('loc_00158510: ;')].replace('void sub_00158510',f'static void {name}')
        start=text.index('loc_00158D11: ;');end=text.index('loc_00158D3D: ;')
        return head+text[start:end]+'\n}\n'
    (out/'hud.inc').write_text('static '+body(source,'00157300')+'\n'+gate(original,'reference_gate')+gate(candidate,'candidate_gate'))
    support=(root/'scripts/light_removal_checks/test.c').read_text();support=support[:support.index('typedef struct {uint32_t kind')]
    (out/'support.inc').write_text(support)
    inc=[out,root/'src/recomp',root/'third_party/xboxrecomp/src',root/'third_party/xboxrecomp/src/input',root/'third_party/xboxrecomp/src/nv2a']
    cm='cmake_minimum_required(VERSION 3.20)\nproject(pc_orders C)\nadd_executable(check "'+(here/'test.c').as_posix()+'")\n'
    cm+='target_include_directories(check PRIVATE '+' '.join('"'+p.as_posix()+'"'for p in inc)+')\n'
    cm+='target_compile_definitions(check PRIVATE RECOMP_GENERATED_CODE UCRR_GUEST_ADDRESS_SPLIT=1 _CRT_SECURE_NO_WARNINGS)\ntarget_compile_options(check PRIVATE /O2 /fp:strict)\ntarget_link_libraries(check PRIVATE xinput user32 gdi32)\n'
    (out/'CMakeLists.txt').write_text(cm)
    cmake=Path(os.environ['USERPROFILE'])/'.local/share/retcomm/toolchains/cmake-clang-v1/latest/bin/cmake.exe'
    run('configure',[cmake,'-S',out,'-B',out/'b','-G','Visual Studio 17 2022','-A','x64']);run('build',[cmake,'--build',out/'b','--config','Release','--parallel','2'])
    report['checks']=run('check',[out/'b/Release/check.exe',out/'heal-drag-pad.bmp'])
    report['exe_sha256']=sha(out/'b/Release/check.exe')
    assert all(sha(root/p)==h for p,h in report['sources'].items()),'Source drift'
    report['limitations']=['Runs the exact original 158D11 visibility tail and full 157300 sprite-container traversal with synthetic available/targeted HUD state. NPC rescue/eligibility discovery and final GPU draw are not simulated.','Production input publication, rebinding/cancellation and existing GDI atlas generation execute; no game was launched and no live scene visual claim is made.']
    report['passed']=True
except BaseException as e:report['error']=repr(e);raise
finally:save();print(out/'results.json')

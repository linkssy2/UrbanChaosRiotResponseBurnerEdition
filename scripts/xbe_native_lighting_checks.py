"""New-renderer pixel-lighting qualification. Default checks sources only.

--prepare writes the private fixture (original shader words never enter a public
output); --run compiles it and runs hardware/WARP math checks, with no game active.
These checks do not claim live visuals, frame rate, or vertex-shader migration.
"""
from pathlib import Path
import argparse
import datetime
import hashlib
import json
import os
import shutil
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
HERE=Path(__file__).resolve().parent
UPSTREAM=ROOT/'build/upstream-review-xboxxbe-20261014'
sys.path.insert(0,str(HERE))
import native_lighting_pixel_sources as original
import prepare_native_lighting_pixel as prior
import prepare_xbe_native_lighting as adapter


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prepare',action='store_true')
    parser.add_argument('--run',action='store_true')
    parser.add_argument('--virtual-probe',action='store_true',help='Also verify the four known disagreements against the uncorrected upstream compiler')
    args=parser.parse_args()
    paths=[Path(__file__),HERE/'xbe_native_lighting.h',HERE/'xbe_native_lighting_checks.cpp',
           HERE/'prepare_xbe_native_lighting.py',ROOT/'src/native_lighting_pixel_source.h',
           UPSTREAM/'runtime/gpu/shaders.cpp',UPSTREAM/'runtime/gpu/shaders.h']
    report={'source_checks':False,'gpu_verified':False,'sources':{
        str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},'steps':[]}
    rows=original.programs()
    header=(ROOT/'src/native_lighting_pixel_source.h').read_text()
    virtual_families=[]
    for row in rows:
        digest=hashlib.sha256(prior.key(row['words'])).digest()
        assert ','.join(f'0x{x:02x}'for x in digest) in header
        assert all(json.dumps(line+'\n') in header for line in original.formula(row['words']).splitlines())
        words=row['words']
        # Final E/F/G have three input bytes; the fourth byte is flags.
        virtual=any(((words[8]>>shift)&15)>=14 for shift in (0,8,16,24)) or any(((words[9]>>shift)&15)>=14 for shift in (8,16,24))
        if virtual:virtual_families.append(row['name'])
    assert len(rows)==42 and len(virtual_families)==12
    source=(UPSTREAM/'runtime/gpu/d3d12.cpp').read_text()
    patched=adapter.patch_source(source)
    assert patched.count('uc_xbe_lighting::Pixel(')==1
    try:adapter.patch_source(patched)
    except ValueError:pass
    else:raise AssertionError('Repeated patch accepted')
    assert source.replace(adapter.INCLUDE,adapter.INCLUDE+'\n#include "xbe_native_lighting.h"',1).replace(adapter.SEAM,adapter.REPLACEMENT,1)==patched
    fixed_shaders=adapter.patch_infinity_source(adapter.patch_sum_source((UPSTREAM/'runtime/gpu/shaders.cpp').read_text()))
    try:adapter.patch_sum_source(fixed_shaders)
    except ValueError:pass
    else:raise AssertionError('Repeated SUM correction accepted')
    assert fixed_shaders.count(adapter.SUM_REPLACEMENT)==1
    report.update(source_checks=True,eligible_families=42,held_families=[],virtual_families=virtual_families,
                  limitations=['Vertex replacement not implemented','GPU proof and live parity pending',
                               'D3D11 arithmetic fixture does not prove D3D12 gameplay pixels','No FPS claim'])
    if not(args.prepare or args.run):
        print(json.dumps(report,indent=2));return
    out=ROOT/'build/xbe-native-lighting-checks'/datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%d-%H%M%S-%f')
    out.mkdir(parents=True)
    shutil.copyfile(HERE/'xbe_native_lighting.h',out/'xbe_native_lighting.h')
    adapter.prepare_shaders(UPSTREAM/'runtime/gpu/shaders.cpp',out/'shaders_sum.cpp')
    assert (out/'shaders_sum.cpp').read_text()==fixed_shaders
    report['upstream_regression_probe']=args.virtual_probe
    report['corrected_reference_sha256']=hashlib.sha256(fixed_shaders.encode()).hexdigest()
    shutil.copyfile(ROOT/'src/native_lighting_pixel_source.h',out/'native_lighting_pixel_source.h')
    (out/'xbe_native_lighting_definitions.inc').write_text('static const uint32_t definitions[][60]={\n'+
        ',\n'.join('{'+','.join(f'0x{v:08x}'for v in row['words'])+'}'for row in rows)+'\n};\n')
    try:
        if args.run:
            vc=Path('C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/14.44.35207')
            compiler=Path('C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/Llvm/x64/bin/clang-cl.exe')
            sdk=Path('C:/Program Files (x86)/Windows Kits/10');version='10.0.26100.0'
            env={k.upper():v for k,v in os.environ.items()if not k.upper().startswith('UCRR_')}
            env['INCLUDE']=';'.join(map(str,[vc/'include']+[sdk/'Include'/version/p for p in ('ucrt','shared','um','winrt')]))
            env['LIB']=';'.join(map(str,[vc/'lib/x64',sdk/'Lib'/version/'um/x64',sdk/'Lib'/version/'ucrt/x64']))
            env['PATH']=str(vc/'bin/Hostx64/x64')+os.pathsep+env.get('PATH','')
            def run(name,command):
                p=subprocess.run(list(map(str,command)),cwd=out,env=env,capture_output=True,text=True,timeout=300)
                (out/(name+'.log')).write_text(p.stdout+p.stderr)
                report['steps'].append({'name':name,'exit':p.returncode})
                if p.returncode:raise RuntimeError((p.stdout+p.stderr)[-5000:])
                if name.startswith('gpu-'):report['steps'][-1]['result']=json.loads(p.stdout.strip().splitlines()[-1])
            variants=([('current',UPSTREAM/'runtime/gpu/shaders.cpp',4)] if args.virtual_probe else [])+[('sum-fixed',out/'shaders_sum.cpp',0)]
            for name,shaders,expected in variants:
                flags=[f'/DUC_XBE_LIGHTING_EXPECT_DISAGREEMENTS={expected}']
                run('compile-'+name,[compiler,'/nologo','/std:c++20','/EHsc','/MD','/O2','/W3',*flags,
                    '/D_CRT_SECURE_NO_WARNINGS','/DNOMINMAX',f'/I{out}',f'/I{UPSTREAM/"runtime"}',
                    HERE/'xbe_native_lighting_checks.cpp',shaders,
                    f'/Fe{out/("check-"+name+".exe")}','/link','d3d11.lib','d3dcompiler.lib','bcrypt.lib'])
                for device in ('warp','hardware'):run('gpu-'+name+'-'+device,[out/('check-'+name+'.exe'),device])
            report['gpu_verified']=True
            report['limitations']=['Vertex replacement not implemented','No live parity or FPS claim',
                'D3D11 arithmetic fixture does not prove D3D12 gameplay pixels','Finite arithmetic coverage; absolute tolerance 0.00002']
    finally:
        report['sources_unchanged']=all(hashlib.sha256((ROOT/p).read_bytes()).hexdigest()==h for p,h in report['sources'].items())
        (out/'results.json').write_text(json.dumps(report,indent=2)+'\n')
        print(out/'results.json')


if __name__=='__main__':main()

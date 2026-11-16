"""Solo weapon-wheel blur replaces only the final scene presentation draw."""
from pathlib import Path
from prepare_xbe_perf import once

METHOD = r'''
// UC_WEAPON_WHEEL_BLUR: original resolved source, sharp later ImGui, no extra copy.
bool D3D12Backend::BlitWeaponWheel(ID3D12Resource* src,DXGI_FORMAT fmt,const float uv[4]) {
    float constants[8];
    if(!uc_weapon_wheel_blur::Plan(guest_w_,guest_h_,uv,gamma_,constants))return false;
    auto* pso=EnsurePassPipeline(uc_weapon_wheel_blur::kShader,"psmain",kBgra,-1);
    if(!pso)return false; // original blit remains available after allocation/compile failure
    const uint32_t slot=AllocSrvSlots(2);
    D3D12_SHADER_RESOURCE_VIEW_DESC view{};view.Format=fmt;
    view.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
    view.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;view.Texture2D.MipLevels=1;
    dev_->CreateShaderResourceView(src,&view,Cpu(srv_heap_.Get(),slot,srv_inc_));
    view.Format=kBgra;
    dev_->CreateShaderResourceView(lut_.Get(),&view,Cpu(srv_heap_.Get(),slot+1,srv_inc_));
    const uint32_t samplers=EnsureBlitSamplers();
    const auto cb=Upload(256,256);std::memcpy(cb.cpu,constants,sizeof(constants));
    DrawFullScreen(pso,cb.gpu,slot,samplers);return true;
}
'''

def transform(source):
    if 'UC_WEAPON_WHEEL_BLUR' in source:
        assert source.count(METHOD)==1
        assert source.count('trial_weapon_wheel::SlowTimeActive()')==1
        return source
    source=once(source,'#include "xbe_coop_blur.h"',
        '#include "xbe_coop_blur.h"\n#include "xbe_weapon_wheel_blur.h"\n#include "xbe_weapon_wheel.h"')
    source=once(source,'    void RenderUi();','    void RenderUi();\n    bool BlitWeaponWheel(ID3D12Resource*,DXGI_FORMAT,const float[4]);')
    anchor='void D3D12Backend::BlitFrame(Target& t, const Surface& front) {'
    source=once(source,anchor,METHOD+'\n'+anchor)
    original='    Blit(read.res, read.format, uv, {.gamma = gamma_});'
    source=once(source,original,
        '    if(!trial_weapon_wheel::SlowTimeActive()||!BlitWeaponWheel(read.res,read.format,uv))\n'+original)
    return source

def prepare(area):
    area=Path(area);local=Path(__file__).parent;compat=area/'src/compat'
    path=compat/'d3d12.cpp';source=path.read_text();updated=transform(source)
    assert transform(updated)==updated
    for name in ('xbe_weapon_wheel_blur.h','test_xbe_weapon_wheel_blur_gpu.cpp'):
        (compat/name.replace('test_xbe_','test_')).write_text((local/name).read_text())
    path.write_text(updated)
    return {'wheel_blur':'solo only, existing resolved presentation source, gamma and crop retained, no extra pass/copy',
            'fixture':'test_weapon_wheel_blur_gpu.cpp'}

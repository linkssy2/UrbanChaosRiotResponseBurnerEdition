"""Queue an owned scene-only blur before the original co-op HUD pass."""
from pathlib import Path


def once(source, old, new):
    if new in source:
        assert source.count(new) == 1
        return source
    assert source.count(old) == 1, old[:120]
    return source.replace(old, new, 1)


RENDERER = r'''
void D3D12Backend::BlurScene(const State& s,unsigned mask) {
    if(!mask||(mask&~3u))return;
    Target* target=FindTarget(s.rt,false);
    if(!target||!target->drawn||target->depth||target->gw!=guest_w_||target->gh!=guest_h_||
       target->h<2||!EndSmoothFog(*target))return;
    auto& t=*target;
    auto* pso=EnsurePassPipeline(uc_coop_blur::kShader,"psmain",t.fmt,-1,uc_msaa::Samples(&t));
    if(!pso)return;
    // Feedback copies the resolved scene; neither half ever samples the image being written.
    ID3D12Resource* source=Feedback(t);if(!source)return;
    uint32_t descriptor=AllocSrvSlots(1);
    D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.Format=t.fmt;
    srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
    srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Texture2D.MipLevels=1;
    dev_->CreateShaderResourceView(source,&srv,Cpu(srv_heap_.Get(),descriptor,srv_inc_));
    Transition(t.res.Get(),t.state,D3D12_RESOURCE_STATE_RENDER_TARGET);
    SetBlitTarget(Cpu(rtv_heap_.Get(),t.view,rtv_inc_),t.w,t.h);
    for(unsigned slot=0;slot<2;++slot)if(mask&(1u<<slot)){
        uc_coop_blur::Region region;
        if(!uc_coop_blur::Plan(t.w,t.h,t.gw,t.gh,slot,region))continue;
        D3D12_VIEWPORT viewport{0,float(region.top),float(t.w),float(region.bottom-region.top),0,1};
        D3D12_RECT clip{0,LONG(region.top),LONG(t.w),LONG(region.bottom)};
        cl_->RSSetViewports(1,&viewport);cl_->RSSetScissorRects(1,&clip);
        Alloc constants=Upload(256,256);std::memcpy(constants.cpu,region.constants,sizeof(region.constants));
        DrawFullScreen(pso,constants.gpu,descriptor);
    }
    InvalidateMsaa(t);bound_rt_=bound_ds_=nullptr;
}
'''

THREADED = r'''
    void BlurScene(const State& s,unsigned mask) override {
        if(!mask||(mask&~3u))return;
        SendState(s);
        Cmd command{Op::BlurScene};command.a=mask;cur_->cmds.push_back(command);
        Use(s.rt.header);Written(s.rt);
        if(cur_->cmds.size()>=kBatchCommands)Submit();
    }
'''

BRIDGE = r'''
namespace d3d {
void TrialDownedSceneBlur(unsigned mask) {
    if(!mask||(mask&~3u))return;
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    if(!g_backend||Dev(kDevRenderTarget)!=Dev(XDK_CDevice_BackBuffer))return;
    Walk(Dev(kDevPush));
    g_backend->BlurScene(g_state,mask);
}
}
'''


def prepare(area):
    area=Path(area);local=Path(__file__).parent;compat=area/'src/compat'
    backend_path=area/'src/gpu/backend.h'
    backend=backend_path.read_text()
    backend=once(backend,'    virtual void Present(const Surface& front) = 0;',
                 '    virtual void Present(const Surface& front) = 0;\n    virtual void BlurScene(const State&,unsigned) {}')
    backend_path.write_text(backend)
    threaded_path=compat/'render_thread.cpp';threaded=threaded_path.read_text()
    threaded=once(threaded,'enum class Op : uint8_t { State, Draw, Clear, Copy, Present,',
                  'enum class Op : uint8_t { State, Draw, Clear, Copy, BlurScene, Present,')
    marker='    void Present(const Surface& front) override {'
    threaded=once(threaded,marker,THREADED+'\n'+marker)
    marker='            case Op::Present: inner_->Present(c.s1); break;'
    threaded=once(threaded,marker,'            case Op::BlurScene: inner_->BlurScene(state_,c.a); break;\n'+marker)
    threaded_path.write_text(threaded)
    d3d_path=compat/'d3d8.cpp';d3d=d3d_path.read_text()
    marker='// targets and presentation\n'
    d3d=once(d3d,marker,marker+BRIDGE+'\n')
    d3d_path.write_text(d3d)
    renderer_path=compat/'d3d12.cpp';renderer=renderer_path.read_text()
    renderer=once(renderer,'#include "gpu/backend.h"','#include "gpu/backend.h"\n#include "xbe_coop_blur.h"')
    renderer=once(renderer,'    void Present(const Surface& front) override;',
                  '    void Present(const Surface& front) override;\n    void BlurScene(const State&,unsigned) override;')
    marker='void D3D12Backend::BlitFrame(Target& t, const Surface& front) {'
    renderer=once(renderer,marker,RENDERER+'\n'+marker)
    renderer_path.write_text(renderer)
    for name in ('xbe_coop_blur.h','test_xbe_coop_blur_gpu.cpp'):
        (compat/('test_coop_blur_gpu.cpp' if name.startswith('test_') else name)).write_text((local/name).read_text())

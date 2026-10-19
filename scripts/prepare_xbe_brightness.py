"""Reduce the verified full-screen brightness readback, retaining the XBE consumer."""
from pathlib import Path
from prepare_xbe_perf import once

def restore_generated(area):
    """Remove the private candidate hook when returning to the default renderer."""
    path=Path(area)/'generated/game/x86_recomp.14.cpp'
    text=path.read_text()
    text=text.replace('extern "C" void TrialBrightnessComplete(X86Context&);\n','').replace('\tTrialBrightnessComplete(ctx);\n','')
    path.write_text(text)

def prepare(area):
    area=Path(area); compat=area/'src/compat'
    path=compat/'d3d8.cpp'; text=path.read_text()
    marker='// synchronisation, the renderer never runs behind'
    text=once(text, marker, r'''
namespace {
thread_local bool brightness_scope=false;
thread_local uint32_t brightness_surface=0;
}
X86_EXTERN_FUNC(__imp__sub_0008BBE0);
X86_FUNC(sub_0008BBE0) {
    struct Scope {
        bool previous=brightness_scope;
        uint32_t surface=brightness_surface;
        ~Scope() {
            std::lock_guard<std::recursive_mutex> lock(g_lock);
            // Any early exit must still honor the deferred surface lock.
            if(brightness_surface && g_backend) g_backend->Sync(brightness_surface,0);
            brightness_scope=previous; brightness_surface=surface;
        }
    } scope;
    brightness_scope=true; brightness_surface=0;
    __imp__sub_0008BBE0(ctx,base);
}
extern "C" void TrialBrightnessComplete(X86Context& ctx) {
    if(!brightness_surface || !g_backend) return;
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    const uint32_t data=brightness_surface;
    brightness_surface=0;
    // Admit only the exact full-screen grid; other viewports retain the full copy.
    const bool full=ctx.ebp==20 && ctx.edx==640 && ctx.ebx==480 &&
        R32(ctx.esp+0x14)==0 && R32(ctx.esp+0x24)==2560 && R32(ctx.esp+0x28)==data;
    g_backend->Sync(data,full ? 0x42524954u : 0);
}
''' + marker)
    text=once(text,'            g_backend->Sync(data, 0);',r'''            bool brightness_lock=false;
            if(brightness_scope && !brightness_surface) {
                for(unsigned i=0;i<16;++i)
                    if(R32(ctx.esp+i*4)==0x8bc06) brightness_lock=true;
            }
            if(brightness_lock) brightness_surface=data;
            else g_backend->Sync(data,0);''')
    path.write_text(text)
    path=area/'generated/game/x86_recomp.14.cpp'; text=path.read_text()
    # The generated header owns X86Context; insert our declaration after includes.
    text=text.replace('extern "C" void TrialBrightnessComplete(X86Context&);\n','').replace('\tTrialBrightnessComplete(ctx);\n','')
    at=text.index('X86_FUNC(')
    text=text[:at]+'extern "C" void TrialBrightnessComplete(X86Context&);\n'+text[at:]
    text=once(text,'\t// 0008BC8C cmp ebp, edx','\tTrialBrightnessComplete(ctx);\n\t// 0008BC8C cmp ebp, edx')
    path.write_text(text)
    original=text[text.index('X86_FUNC(__imp__sub_0008BBE0) {'):text.index('X86_WEAK_FUNC(sub_0008BBE0')]
    fixture=Path(__file__).with_name('test_xbe_brightness_guest.cpp').read_text()
    (compat/'test_brightness_guest.cpp').write_text(fixture.replace('/* ORIGINAL */',original))
    path=compat/'render_thread.cpp'; text=path.read_text()
    text=once(text,'void Sync(uint32_t data, uint32_t) override { ReadBack(data, false); }',
              'void Sync(uint32_t data, uint32_t mode) override { ReadBack(data, false, mode); }')
    text=once(text,'void ReadBack(uint32_t data, bool late) {','void ReadBack(uint32_t data, bool late, uint32_t mode=0) {')
    text=once(text,'        if (!written_.erase(data)) return;',
              '        if (!written_.contains(data)) return;\n        if(mode!=0x42524954u) written_.erase(data);')
    text=once(text,'        Cmd c{Op::Sync};\n        c.a = data;', '        Cmd c{Op::Sync};\n        c.a = data; c.b=mode;')
    text=once(text,'case Op::Sync: inner_->Sync(c.a, 0); break;', 'case Op::Sync: inner_->Sync(c.a, c.b); break;')
    path.write_text(text)
    path=compat/'d3d12.cpp'; text=path.read_text()
    text=once(text,'float4 psmain(O i) : SV_Target {\n    float4 c = 0;',r'''float4 psmain(O i) : SV_Target {
    if(opts.z==0) {
        uint sum=0;
        for(uint y=15;y<480;y+=30) for(uint x=20;x<640;x+=40) {
            uint3 rgb=(uint3)round(saturate(img.Load(int3(x,y,0)).rgb)*255.0);
            sum+=rgb.r+rgb.g+rgb.b;
        }
        return float4((sum>>16)&255u,(sum>>8)&255u,sum&255u,255u)/255.0;
    }
    float4 c = 0;''')
    text=once(text,'void D3D12Backend::Sync(uint32_t data, uint32_t) {','void D3D12Backend::Sync(uint32_t data, uint32_t mode) {')
    text=once(text,'    std::vector<uint8_t> px;\n    if (!ReadBack(src.res, t.gw, t.gh, px)) return;',r'''    std::vector<uint8_t> px;
    if(mode==0x42524954u && t.gw==640 && t.gh==480 && src.down) {
        D3D12_RESOURCE_STATES state=D3D12_RESOURCE_STATE_COPY_SOURCE;
        auto reduced=Shrink(src.res,state,kBgra,1,1,{.taps=0});
        if(reduced && ReadBack(reduced.Get(),1,1,px)) {
            auto* guest=static_cast<uint8_t*>(kernel::HostPtr(data));
            unsigned sum=unsigned(px[0])|(unsigned(px[1])<<8)|(unsigned(px[2])<<16);
            unsigned remainder=sum%768;
            // Preserve the exact sum, including remainder and the original accumulator.
            for(unsigned y=15;y<480;y+=30) for(unsigned x=20;x<640;x+=40)
                for(unsigned channel=0;channel<3;++channel) {
                    guest[size_t(y)*GuestPitch(640)+x*4+channel]=uint8_t(sum/768+(remainder!=0));
                    if(remainder) --remainder;
                }
            t.sig=MemSig(t.data,t.bytes); t.read.clear();
            // Partial publication must not satisfy another caller's full surface lock.
            if(diag::Perf()) LOG_FIRST(4,"brightness: GPU sum readback active");
            return;
        }
        // The reduction transitioned its input; restore it for the full-copy fallback.
        Transition(src.res,state,D3D12_RESOURCE_STATE_COPY_SOURCE);
    }
    if (!ReadBack(src.res, t.gw, t.gh, px)) return;''')
    path.write_text(text)
    begin=text.index('const char* kBlitHlsl = R"(')+len('const char* kBlitHlsl = R"(')
    shader=text[begin:text.index('\n)";',begin)]
    fixture=Path(__file__).with_name('test_xbe_brightness.cpp').read_text()
    (compat/'test_brightness.cpp').write_text(fixture.replace('/* SHADER */',shader))

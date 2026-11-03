"""Backport exact constant and command-state reuse from XboxXBERecomp.

Based on TGP482/XboxXBERecomp 908dc53a40d4544ab8900d3d9eb3bda174daf846
and 0fd00442660573dc342b41c11c6574318f0e8698 (MIT). This bounded adaptation
keeps the title's depth/fog, descriptor, geometry and upload-arena changes.
No draw, shader, resource transition, submission or pacing policy changes.
"""
from pathlib import Path
from prepare_xbe_perf import once, function
from prepare_xbe_prompt_handoff import between

MARKER = 'UC_EXACT_DRAW_BINDINGS'
STRUCT = '''// UC_EXACT_DRAW_BINDINGS: actual state of this command list, never guest-state hashes.
struct DrawBinds {
    ID3D12PipelineState* pso = nullptr;
    D3D_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    bool rasterizer = false;
    D3D12_VIEWPORT viewport{};
    D3D12_RECT scissor{};
    bool output = false;
    float blend[4]{};
    uint32_t stencil = 0;
    D3D12_GPU_VIRTUAL_ADDRESS vsc = 0, dc = 0;
    // GPU handles include the heap generation, unlike descriptor slot numbers.
    uint64_t views = 0, samplers = 0;
};

'''
FIELDS = '''    DrawBinds binds_;
    decltype(State::vsc) last_vsc_{};
    D3D12_GPU_VIRTUAL_ADDRESS last_vsc_gpu_ = 0;
    const State* last_vsc_state_ = nullptr;
    uint64_t last_vsc_revision_ = 0;
    DrawConstants last_dc_{};
    D3D12_GPU_VIRTUAL_ADDRESS last_dc_gpu_ = 0;
    void InvalidateDrawState(bool uploads = false);
'''
INVALIDATE = '''void D3D12Backend::InvalidateDrawState(bool uploads) {
    binds_ = {};
    if (uploads) {
        last_vsc_gpu_ = 0;
        last_vsc_state_ = nullptr;
        last_vsc_revision_ = 0;
        last_dc_gpu_ = 0;
    }
}

'''
OLD_GUARD = '''struct DrawEnd {
    Extension* e;
    ~DrawEnd() {
        if (e) e->EndDraw();
    }
};'''
NEW_GUARD = '''struct DrawEnd {
    Extension* e;
    DrawBinds* binds;
    D3D12_GPU_VIRTUAL_ADDRESS* vsc;
    D3D12_GPU_VIRTUAL_ADDRESS* dc;
    ~DrawEnd() {
        if (e) {
            e->EndDraw();
            // Also covers early exits and an extension removed before the next draw.
            *binds = {};
            *vsc = *dc = 0;
        }
    }
};'''
PSO = '''    if (ext_) InvalidateDrawState();
    if (pso != binds_.pso) cl_->SetPipelineState(pso);
    binds_.pso = pso;'''


REPLACEMENTS = [
        ('    void Draw(const State& s, uint32_t prim, uint32_t start, uint32_t count, const uint8_t* indices) override;',
         '    void Draw(const State& s, uint32_t prim, uint32_t start, uint32_t count, const uint8_t* indices, uint64_t vsc_revision = 0) override;'),
        ('void D3D12Backend::Draw(const State& s, uint32_t prim, uint32_t start, uint32_t count, const uint8_t* indices) {',
         'void D3D12Backend::Draw(const State& s, uint32_t prim, uint32_t start, uint32_t count, const uint8_t* indices, uint64_t vsc_revision) {'),
        ('DrawConstants& c, bool analytic_fog);', 'DrawConstants& c, bool analytic_fog, uint64_t vsc_revision);'),
        ('DrawConstants& c, bool analytic_fog) {', 'DrawConstants& c, bool analytic_fog, uint64_t vsc_revision) {'),
        ('    UploadConstants(s, sh, k, stages, size, c, analytic_fog);',
         '    UploadConstants(s, sh, k, stages, size, c, analytic_fog, vsc_revision);'),
        ('class D3D12Backend final : public Backend, public Context {',
         STRUCT + 'class D3D12Backend final : public Backend, public Context {'),
        ('    uint32_t last_texture_slot_=0;', '    uint32_t last_texture_slot_=0;\n' + FIELDS.rstrip()),
        ('    ID3D12GraphicsCommandList* List() override { return cl_.Get(); }',
         '    ID3D12GraphicsCommandList* List() override { InvalidateDrawState(); return cl_.Get(); }'),
        ('    void Unbind() override { bound_rt_ = bound_ds_ = nullptr; }',
         '    void Unbind() override { InvalidateDrawState(); bound_rt_ = bound_ds_ = nullptr; }'),
        ('void D3D12Backend::Begin() {', INVALIDATE + 'void D3D12Backend::Begin() {\n    InvalidateDrawState(true);'),
        ('void D3D12Backend::Submit(bool wait) {',
         'void D3D12Backend::Submit(bool wait) {\n    InvalidateDrawState(true);'),
        ('void D3D12Backend::SetFullViewport(uint32_t w, uint32_t h) {',
         'void D3D12Backend::SetFullViewport(uint32_t w, uint32_t h) {\n    InvalidateDrawState();'),
        ('void D3D12Backend::RenewDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) {',
         'void D3D12Backend::RenewDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) {\n    InvalidateDrawState();'),
        ('                                  std::optional<uint32_t> sampler_slot) {',
         '                                  std::optional<uint32_t> sampler_slot) {\n    InvalidateDrawState();'),
        ('void D3D12Backend::RenderUi() {', 'void D3D12Backend::RenderUi() {\n    InvalidateDrawState();'),
        ('void D3D12Backend::LoadSettings() {',
         'void D3D12Backend::LoadSettings() {\n    InvalidateDrawState(true);'),
        ('''void D3D12Backend::CopyByDraw(const Target& src_t, ID3D12Resource* src_res, D3D12_RESOURCE_STATES& src_state,
                              Target& dst_t, const CopyList& l) {''',
         '''void D3D12Backend::CopyByDraw(const Target& src_t, ID3D12Resource* src_res, D3D12_RESOURCE_STATES& src_state,
                              Target& dst_t, const CopyList& l) {
    InvalidateDrawState();'''),
        ('void D3D12Backend::BlitFrame(Target& t, const Surface& front) {',
         'void D3D12Backend::BlitFrame(Target& t, const Surface& front) {\n    InvalidateDrawState();'),
        (OLD_GUARD, NEW_GUARD),
        ('    DrawEnd draw_end{ext_};', '''    DrawEnd draw_end{ext_, &binds_, &last_vsc_gpu_, &last_dc_gpu_};
    if (ext_) InvalidateDrawState(true);'''),
    ]
REPLACEMENTS.append(('    if (!pso) return;\n    cl_->SetPipelineState(pso);',
                     '    if (!pso) return;\n' + PSO))

METHOD_CHANGES = [
    ('void D3D12Backend::UploadConstants(', [
        ('''    Alloc vc = Upload(sizeof(s.vsc), 256);
    std::memcpy(vc.cpu, s.vsc.data(), sizeof(s.vsc));
    Alloc dc = Upload(sizeof(DrawConstants), 256);''', '''    // Extensions may edit the uploaded vertex constants; always give them a fresh copy.
    // UC_REPLAY_VSC_REVISION: only replay supplies a nonzero, consumer-owned revision.
    const bool same = !ext_ && last_vsc_gpu_ &&
        ((vsc_revision && last_vsc_state_ == &s && last_vsc_revision_ == vsc_revision) ||
         !std::memcmp(last_vsc_.data(), s.vsc.data(), sizeof(s.vsc)));
    Alloc vc{};
    if (!same) {
        vc = Upload(sizeof(s.vsc), 256);
        std::memcpy(vc.cpu, s.vsc.data(), sizeof(s.vsc));
        std::memcpy(last_vsc_.data(), s.vsc.data(), sizeof(s.vsc));
        last_vsc_gpu_ = vc.gpu;
    }
    last_vsc_state_ = &s;
    last_vsc_revision_ = vsc_revision;'''),
        ('''    std::memcpy(dc.cpu, &c, sizeof(c));
    cl_->SetGraphicsRootConstantBufferView(0, vc.gpu);
    cl_->SetGraphicsRootConstantBufferView(1, dc.gpu);''', '''    // Compare the final bytes, AFTER fog, texture, fixed-function lighting and extension fills.
    if (!last_dc_gpu_ || std::memcmp(&last_dc_, &c, sizeof(c))) {
        Alloc dc = Upload(sizeof(c), 256);
        std::memcpy(dc.cpu, &c, sizeof(c));
        std::memcpy(&last_dc_, &c, sizeof(c));
        last_dc_gpu_ = dc.gpu;
    }
    if (ext_) InvalidateDrawState();
    if (last_vsc_gpu_ != binds_.vsc) cl_->SetGraphicsRootConstantBufferView(0, last_vsc_gpu_);
    if (last_dc_gpu_ != binds_.dc) cl_->SetGraphicsRootConstantBufferView(1, last_dc_gpu_);
    binds_.vsc = last_vsc_gpu_;
    binds_.dc = last_dc_gpu_;'''),
    ]),
    ('void D3D12Backend::SetTextureViewsAndSamplers(', [
        ('''    cl_->SetGraphicsRootDescriptorTable(2, Gpu(srv_heap_.Get(), slot, srv_inc_));
    cl_->SetGraphicsRootDescriptorTable(3, Gpu(smp_heap_.Get(), samplers, smp_inc_));''', '''    // Resolve BOTH allocators first: sampler renewal invalidates both root tables.
    const auto views = Gpu(srv_heap_.Get(), slot, srv_inc_);
    const auto sampling = Gpu(smp_heap_.Get(), samplers, smp_inc_);
    if (views.ptr != binds_.views) cl_->SetGraphicsRootDescriptorTable(2, views);
    if (sampling.ptr != binds_.samplers) cl_->SetGraphicsRootDescriptorTable(3, sampling);
    binds_.views = views.ptr;
    binds_.samplers = sampling.ptr;'''),
    ]),
    ('void D3D12Backend::SetViewportScissor(', [
        ('''    cl_->RSSetViewports(1, &vp);
''', ''''''),
        ('''    cl_->RSSetScissorRects(1, &rect);''', '''    // Per-view camera/scissor changes compare their final host-space state exactly.
    const bool same = binds_.rasterizer &&
        !std::memcmp(&vp, &binds_.viewport, sizeof(vp)) &&
        !std::memcmp(&rect, &binds_.scissor, sizeof(rect));
    if (same) return;
    cl_->RSSetViewports(1, &vp);
    cl_->RSSetScissorRects(1, &rect);
    binds_.rasterizer = true;
    binds_.viewport = vp;
    binds_.scissor = rect;'''),
    ]),
    ('void D3D12Backend::SetBlendFactorStencilRef(', [
        ('''    cl_->OMSetBlendFactor(bf);
    cl_->OMSetStencilRef(SimpleState(s, kMStencilRef) & 0xFF);''', '''    const uint32_t ref = SimpleState(s, kMStencilRef) & 0xFF;
    if (binds_.output && !std::memcmp(bf, binds_.blend, sizeof(bf)) && ref == binds_.stencil) return;
    cl_->OMSetBlendFactor(bf);
    cl_->OMSetStencilRef(ref);
    binds_.output = true;
    std::memcpy(binds_.blend, bf, sizeof(bf));
    binds_.stencil = ref;'''),
    ]),
    ('void D3D12Backend::DrawGeometry(', [
        ('''    cl_->IASetPrimitiveTopology(PrimitiveTopology(a));''', '''    const auto topology = PrimitiveTopology(a);
    if (topology != binds_.topology) cl_->IASetPrimitiveTopology(topology);
    binds_.topology = topology;'''),
    ]),
]


def validate(source):
    """Pin reset boundaries and reject new unreviewed direct state-writer methods."""
    import re
    assert source.count(STRUCT) == 1 and source.count(FIELDS.rstrip()) == 1
    for signature in ('void D3D12Backend::Begin(', 'void D3D12Backend::Submit(', 'void D3D12Backend::LoadSettings('):
        assert 'InvalidateDrawState(true);' in function(source, signature)
    for signature in ('void D3D12Backend::SetFullViewport(', 'void D3D12Backend::RenewDescriptorHeap(',
                      'void D3D12Backend::DrawFullScreen(', 'void D3D12Backend::RenderUi(',
                      'void D3D12Backend::CopyByDraw(', 'void D3D12Backend::BlitFrame('):
        assert 'InvalidateDrawState();' in function(source, signature)
    assert NEW_GUARD in source
    assert 'List() override { InvalidateDrawState(); return cl_.Get(); }' in source
    assert 'Unbind() override { InvalidateDrawState();' in source
    draw = function(source, 'void D3D12Backend::Draw(')
    assert 'DrawEnd draw_end{ext_, &binds_, &last_vsc_gpu_, &last_dc_gpu_};' in draw
    assert 'if (ext_) InvalidateDrawState(true);' in draw and PSO in draw
    assert 'uint64_t vsc_revision)' in draw
    assert 'UploadConstants(s, sh, k, stages, size, c, analytic_fog, vsc_revision);' in draw
    upload = function(source, 'void D3D12Backend::UploadConstants(')
    assert 'vsc_revision && last_vsc_state_ == &s && last_vsc_revision_ == vsc_revision' in upload
    setters = r'cl_->(?:SetPipelineState|SetGraphicsRootConstantBufferView|SetGraphicsRootDescriptorTable|IASetPrimitiveTopology|RSSetViewports|RSSetScissorRects|OMSetBlendFactor|OMSetStencilRef|SetDescriptorHeaps|SetGraphicsRootSignature)\('
    reviewed = {'Begin', 'RenewDescriptorHeap', 'SetFullViewport', 'DrawFullScreen', 'Pass',
                'UploadConstants', 'SetTextureViewsAndSamplers', 'SetViewportScissor',
                'SetBlendFactorStencilRef', 'DrawGeometry', 'Draw', 'CopyByDraw',
                'BlurScene', 'BlitFrame', 'RenderUi'}
    for match in re.finditer(setters, source):
        owners = list(re.finditer(r'^.*\bD3D12Backend::(\w+)\(', source[:match.start()], re.M))
        assert owners and owners[-1].group(1) in reviewed, 'unreviewed command-state writer'
    # These outside passes already enter through an invalidating viewport helper.
    for name, helper in [('Pass', 'SetFullViewport('), ('BlurScene', 'SetBlitTarget(')]:
        signature = 'void D3D12Backend::' + name + '('
        if signature in source:  # BlurScene is installed later in the build pipeline.
            body = function(source, signature)
            first = re.search(setters, body)
            assert first and body.index(helper) < first.start()


def transform(source):
    if MARKER in source:
        # Reconstructing and reapplying is also an idempotency/partial-patch check.
        baseline = undo(source)
        assert apply(baseline) == source, 'partial draw bindings preparation'
        validate(source)
        return source
    result = apply(source)
    validate(result)
    assert undo(result) == source
    return result


def apply(source):
    for signature, pairs in METHOD_CHANGES:
        old = function(source, signature)
        new = old
        for before, after in pairs:
            new = once(new, before, after)
        source = once(source, old, new)
    for before, after in REPLACEMENTS:
        source = once(source, before, after)
    return source


def undo(source):
    for before, after in reversed(REPLACEMENTS):
        source = once(source, after, before)
    for signature, pairs in reversed(METHOD_CHANGES):
        old = function(source, signature)
        new = old
        for before, after in reversed(pairs):
            if after:
                new = once(new, after, before)
            else:
                # The viewport call moves below the computed rect in this backport.
                new = once(new, '    D3D12_VIEWPORT vp{0, 0, W, H, 0, 1};\n',
                           '    D3D12_VIEWPORT vp{0, 0, W, H, 0, 1};\n' + before)
        source = once(source, old, new)
    return source


METHODS = ('void D3D12Backend::UploadConstants(', 'void D3D12Backend::SetTextureViewsAndSamplers(',
           'void D3D12Backend::SetViewportScissor(', 'void D3D12Backend::SetBlendFactorStencilRef(',
           'void D3D12Backend::UploadVertexBuffers(', 'void D3D12Backend::DrawGeometry(',
           'void D3D12Backend::DrawFullScreen(', 'void D3D12Backend::SetFullViewport(',
           'void D3D12Backend::Begin(', 'void D3D12Backend::RenewDescriptorHeap(')


DRAW_ARGUMENTS = 'const State& s, uint32_t prim, uint32_t start, uint32_t count, const uint8_t* indices'
REPLAY_REVISION = '''                // A changed block can straddle the constants' boundary.
                bool vsc_changed = false;
                const size_t vsc_first = At(state_, &state_.vsc), vsc_end = vsc_first + sizeof(state_.vsc);
'''
REPLAY_OVERLAP = '''                    vsc_changed |= size_t(head[0]) < vsc_end && size_t(head[0]) + head[1] > vsc_first;
'''
REPLAY_ADVANCE = '''                // Zero disables the shortcut after overflow; it is also the direct-draw fallback.
                if (vsc_changed && vsc_revision_) ++vsc_revision_;
'''
THREAD_REPLACEMENTS = [
    ('    void Draw(' + DRAW_ARGUMENTS + ') override {',
     '    void Draw(' + DRAW_ARGUMENTS + ', uint64_t = 0) override {'),
    ('                uint8_t* s = reinterpret_cast<uint8_t*>(&state_);',
     '                uint8_t* s = reinterpret_cast<uint8_t*>(&state_);\n' + REPLAY_REVISION.rstrip()),
    ('                    std::memcpy(s + head[0], p + 8, head[1]);',
     '                    std::memcpy(s + head[0], p + 8, head[1]);\n' + REPLAY_OVERLAP.rstrip()),
    ('                if (c.vs >= 0) state_.vs = b.vs[size_t(c.vs)];',
     REPLAY_ADVANCE + '                if (c.vs >= 0) state_.vs = b.vs[size_t(c.vs)];'),
    ('case Op::Draw: inner_->Draw(state_, c.a, c.b, c.c, c.d ? data : nullptr); break;',
     'case Op::Draw: inner_->Draw(state_, c.a, c.b, c.c, c.d ? data : nullptr, vsc_revision_); break;'),
    ('    State state_;', '    State state_;\n    uint64_t vsc_revision_ = 1; // UC_REPLAY_VSC_REVISION: never sent by the producer'),
]


def tracked_backend(source):
    old = '    virtual void Draw(' + DRAW_ARGUMENTS + ') = 0;'
    new = '    virtual void Draw(' + DRAW_ARGUMENTS + ', uint64_t vsc_revision = 0) = 0;'
    if new in source:
        assert source.count(new) == 1 and old not in source
        return source
    return once(source, old, new)


def tracked_replay(source):
    if '// UC_REPLAY_VSC_REVISION:' in source:
        for before, after in THREAD_REPLACEMENTS:
            source = once(source, after, before)
    for before, after in THREAD_REPLACEMENTS:
        source = once(source, before, after)
    return source


def fixture(before, after, threaded):
    text = Path(__file__).with_name('test_xbe_draw_bindings.cpp').read_text()
    text = once(text, '/* STRUCT */', STRUCT)
    text = once(text, '/* FIELDS */', FIELDS.replace('    void InvalidateDrawState(bool uploads = false);\n', ''))
    helpers = '\n'.join(function(before, s) for s in ('float Bits(', 'void Color(', 'D3D_PRIMITIVE_TOPOLOGY PrimitiveTopology('))
    text = once(text, '/* HELPERS */', helpers)
    for label, version in [('BEFORE', before), ('AFTER', after)]:
        methods = '\n\n'.join(function(version, s) for s in METHODS)
        # Only the host heap interface is substituted; allocation, retirement and
        # reset order execute the production function bodies.
        methods = methods.replace('ComPtr<ID3D12DescriptorHeap>', 'ComPtr<MockHeap>').replace(
            'ID3D12DescriptorHeap* heaps[]', 'MockHeap* heaps[]')
        # Count only the actual VSC comparison; final DrawConstants stay unmodified.
        if label == 'AFTER':
            methods = once(methods, 'std::memcmp(last_vsc_.data(), s.vsc.data(), sizeof(s.vsc))',
                           'CompareVsc(last_vsc_.data(), s.vsc.data(), sizeof(s.vsc))')
        invalidation = function(after, 'void D3D12Backend::InvalidateDrawState(')
        methods += '\n' + invalidation
        guard = NEW_GUARD if label == 'AFTER' else OLD_GUARD
        methods += '\n' + guard
        pso = PSO if label == 'AFTER' else '    cl_->SetPipelineState(pso);'
        methods += '\nvoid D3D12Backend::SetPipeline(ID3D12PipelineState* pso) {\n' + pso + '\n}\n'
        start = 'DrawEnd guard{ext_, &binds_, &last_vsc_gpu_, &last_dc_gpu_};\n    if(ext_)InvalidateDrawState(true);' if label == 'AFTER' else 'DrawEnd guard{ext_};'
        methods += '\nvoid D3D12Backend::ExtensionDraw(const State& s, bool early) {\n    ' + start + '''
    ext_->BeginDraw(s);
    if(early)return;
    Guest(s);
}
'''
        text = once(text, '/* ' + label + ' */', methods)
    for marker, value in (
        ('/* REPLAY_COMMAND_TYPES */', between(threaded, 'enum class Op :', '// the plain data in State')),
        ('/* REPLAY_STATE_RANGES */', between(threaded, 'struct Range {', 'using Clock =')),
        ('/* REPLAY_SEND_METHODS */', between(threaded, "    // a field of the render thread's State,", '    // resources by guest header:')),
        ('/* REPLAY_STATE_AND_DRAW */', between(threaded, '            case Op::State: {', '            case Op::Clear:')),
    ):
        text = once(text, marker, value)
    return text


def prepare(area):
    area = Path(area)
    path = area / 'src/compat/d3d12.cpp'
    original = path.read_text()
    result = transform(original)
    before = undo(result)
    assert transform(result) == result and transform(before) == result
    backend_path = area / 'src/gpu/backend.h'
    backend = tracked_backend(backend_path.read_text())
    assert tracked_backend(backend) == backend
    threaded_path = area / 'src/compat/render_thread.cpp'
    threaded = tracked_replay(threaded_path.read_text())
    assert tracked_replay(threaded) == threaded
    backend_path.write_text(backend)
    threaded_path.write_text(threaded)
    path.write_text(result)
    finalize(area)


def finalize(area):
    """Check the final interface and queue after every downstream preparer."""
    area = Path(area)
    renderer = (area / 'src/compat/d3d12.cpp').read_text()
    backend = (area / 'src/gpu/backend.h').read_text()
    threaded = (area / 'src/compat/render_thread.cpp').read_text()
    validate(renderer)
    assert tracked_backend(backend) == backend, 'a downstream preparer erased the tracked draw interface'
    assert tracked_replay(threaded) == threaded, 'a downstream preparer erased tracked replay'
    (area / 'src/compat/test_draw_bindings.cpp').write_text(fixture(undo(renderer), renderer, threaded))


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('area', type=Path)
    prepare(parser.parse_args().area)

"""Reuse one fully overwritten BGRA scratch image for blocking surface reads.

Run after frame-cost/MSAA/release preparation. The NOOVERWRITE ring is unchanged.
The witness extracts the real copy-source, Shrink, Blit and ReadBack methods.
"""
from pathlib import Path
from prepare_xbe_perf import once, function


DECL = '    CopySource PrepareCopySource(Target& t, const ShrinkOptions& o);'
NEW_DECL = '''    // One output allocation, shared only by serialized blocking readbacks.
    ComPtr<ID3D12Resource> sync_down_;
    uint32_t sync_down_w_ = 0, sync_down_h_ = 0;
    CopySource PrepareCopySource(Target& t, const ShrinkOptions& o, bool sync = false);'''
SIGNATURE = 'D3D12Backend::CopySource D3D12Backend::PrepareCopySource(Target& t, const ShrinkOptions& o)'
NEW_SIGNATURE = SIGNATURE[:-1] + ', bool sync)'
OLD_SHRINK = '        out.down = Shrink(read.res, *read.state, read.format, t.gw, t.gh, o);'
NEW_SHRINK = '''        ShrinkOptions reuse = o;
        if (sync) {
            if (sync_down_ && (sync_down_w_ != t.gw || sync_down_h_ != t.gh))
                CurrentFrame().garbage.push_back(std::move(sync_down_));
            reuse.into = sync_down_.Get();
            reuse.into_state = D3D12_RESOURCE_STATE_COPY_SOURCE;
        }
        out.down = Shrink(read.res, *read.state, read.format, t.gw, t.gh, reuse);
        if (sync && out.down && !sync_down_) {
            sync_down_ = out.down;
            sync_down_w_ = t.gw;
            sync_down_h_ = t.gh;
        }'''
OLD_CALL = '    CopySource src = PrepareCopySource(t, {});'
NEW_CALL = '    CopySource src = PrepareCopySource(t, {}, true);'


def transform(source):
    # Storage reuse is valid across source identity/format/scale changes because
    # the existing pass overwrites every RGBA sample of a fixed BGRA output.
    shrink = function(source, 'ComPtr<ID3D12Resource> D3D12Backend::Shrink(')
    assert 'Texture2DDesc(DXGI_FORMAT_B8G8R8A8_UNORM, w, h)' in shrink
    assert 'SetBlitTarget(rh, w, h);' in shrink
    assert 'Blit(src, fmt, uv, {.alpha = true, .taps = o.taps, .mul = o.mul});' in shrink
    assert 'Transition(down.Get(), down_state, D3D12_RESOURCE_STATE_COPY_SOURCE);' in shrink
    assert 'td.SampleDesc.Count = 1;' in function(source, 'D3D12_RESOURCE_DESC Texture2DDesc(')
    assert 'bool over = false;' in source
    full = function(source, 'void D3D12Backend::SetFullViewport(')
    assert 'D3D12_RECT scissor{0, 0, LONG(w), LONG(h)};' in full
    pipeline = function(source, 'D3D12_GRAPHICS_PIPELINE_STATE_DESC D3D12Backend::FullScreenPipelineDesc(')
    assert 'D3D12_COLOR_WRITE_ENABLE_ALL' in pipeline and 'BlendEnable' not in pipeline
    if NEW_DECL in source:
        assert source.count(NEW_DECL) == source.count(NEW_SHRINK) == source.count(NEW_CALL) == 1
        assert source.count(NEW_SIGNATURE) == 1
        return source
    source = once(source, DECL, NEW_DECL)
    source = once(source, SIGNATURE, NEW_SIGNATURE)
    source = once(source, OLD_SHRINK, NEW_SHRINK)
    return once(source, OLD_CALL, NEW_CALL)


def undo(source):
    source = once(source, NEW_DECL, DECL)
    source = once(source, NEW_SIGNATURE, SIGNATURE)
    source = once(source, NEW_SHRINK, OLD_SHRINK)
    return once(source, NEW_CALL, OLD_CALL)


def fixture(source):
    old = undo(source)
    assert transform(old) == source
    text = Path(__file__).with_name('test_xbe_sync_downsample.cpp').read_text()
    text = once(text, '/* BLIT_SHADER */', source[source.index('const char* kBlitHlsl'):source.index('const char* kDepthHlsl')])
    shared = '\n\n'.join(function(source, signature) for signature in (
        'D3D12_RESOURCE_DESC BufferDesc(', 'D3D12_RESOURCE_DESC Texture2DDesc(',
        'void D3D12Backend::SetFullViewport(', 'void D3D12Backend::SetBlitTarget(',
        'void D3D12Backend::DrawFullScreen(', 'void D3D12Backend::Blit(ID3D12Resource* src, DXGI_FORMAT fmt, const float uvrect[4], const BlitOptions& o)',
        'D3D12_GRAPHICS_PIPELINE_STATE_DESC D3D12Backend::FullScreenPipelineDesc(',
        'ID3D12PipelineState* D3D12Backend::EnsureBlitPipeline(',
        'ComPtr<ID3D12Resource> D3D12Backend::Shrink(', 'bool D3D12Backend::ReadBack('))
    text = once(text, '/* SHARED_METHODS */', shared.replace('D3D12Backend::', 'Base::'))
    for label, value in [('BEFORE', old), ('AFTER', source)]:
        text = once(text, '/* '+label+' */', function(value, 'D3D12Backend::CopySource D3D12Backend::PrepareCopySource('))
    return text


def prepare(area):
    compat = Path(area) / 'src/compat'
    path = compat / 'd3d12.cpp'
    source = transform(path.read_text())
    assert transform(source) == source
    path.write_text(source)
    (compat / 'test_xbe_sync_downsample.cpp').write_text(fixture(source))
    # Existing Sync byte/ordering witness models this boundary, not its GPU
    # implementation. Permit the new cache-admission flag without changing it.
    witness = compat / 'test_xbe_sync_pixels.inc'
    if witness.exists():
        text = witness.read_text()
        old = 'CopySource PrepareCopySource(Target&,std::nullptr_t)'
        new = 'CopySource PrepareCopySource(Target&,std::nullptr_t,bool=false)'
        if old in text:
            witness.write_text(once(text, old, new))


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('area', type=Path)
    prepare(parser.parse_args().area)

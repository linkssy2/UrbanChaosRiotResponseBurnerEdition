"""Reuse exact texture views and sampler inputs within their descriptor lifetimes."""
from pathlib import Path
from prepare_xbe_perf import once, function

_MARKER = 'UC_SRV_TUPLE_CACHE'
_RESET = '\n    texture_view_cache_.Reset(); // UC_SRV_TUPLE_CACHE'
_OLD_ALLOC = '''    const uint32_t slot=AllocSrvSlots(4);
    for(unsigned i=0;i<4;++i) {
        dev_->CreateShaderResourceView(stages.res[i],&stages.sv[i],Cpu(srv_heap_.Get(),slot+i,srv_inc_));
        last_texture_resources_[i]=stages.res[i];
    }'''
_LOOKUP = '''    const auto key=TextureViewCache::MakeKey(stages.res,stages.sv);
    const uint64_t hash=Hash(&key,sizeof(key));
    uint32_t slot=0;
    if(texture_view_cache_.Find(srv_heap_.Get(),key,hash,slot)) {
#ifndef UC_BURNER_RELEASE // UC_SRV_TUPLE_CACHE
        if(diag::Perf())++g_perf.srv_tuple_hits;
#endif
    } else {
#ifndef UC_BURNER_RELEASE // UC_SRV_TUPLE_CACHE
        if(diag::Perf())++g_perf.srv_tuple_misses;
#endif
        slot=AllocSrvSlots(4);
        for(unsigned i=0;i<4;++i)
            dev_->CreateShaderResourceView(stages.res[i],&stages.sv[i],Cpu(srv_heap_.Get(),slot+i,srv_inc_));
        texture_view_cache_.Store(srv_heap_.Get(),key,hash,slot);
    }
    for(unsigned i=0;i<4;++i)last_texture_resources_[i]=stages.res[i];'''


def _cache_changes():
    return [
        ('#include "core/diag.h"', '#include "core/diag.h"\n#include "xbe_srv_cache.h" // UC_SRV_TUPLE_CACHE'),
        ('    uint32_t last_texture_slot_=0;', '''    uint32_t last_texture_slot_=0;
    using TextureViewCache = uc_srv_cache::Cache<ID3D12Resource,ComPtr<ID3D12Resource>,D3D12_SHADER_RESOURCE_VIEW_DESC>;
    TextureViewCache texture_view_cache_;'''),
        ('    uint64_t draws = 0;', '    uint64_t draws = 0;\n    uint64_t srv_tuple_hits = 0, srv_tuple_misses = 0; // UC_SRV_TUPLE_CACHE'),
        (_OLD_ALLOC, _LOOKUP),
        ('        g_perf = Perf{};', '''        LOGR("perf: srv tuples %.1f hits %.1f misses/frame (adjacent fast path excluded)",
             p.srv_tuple_hits / n, p.srv_tuple_misses / n);
        g_perf = Perf{};''')]


def cache_transform(source):
    if _MARKER in source:
        for _, replacement in _cache_changes():
            assert source.count(replacement) == 1, 'partial texture cache preparation'
        assert source.count(_RESET) == 3
        return source
    for original, replacement in _cache_changes():
        source = once(source, original, replacement)
    reset = '    last_texture_heap_=nullptr;'
    assert source.count(reset) == 3
    return source.replace(reset, reset + _RESET)


def cache_undo(source):
    assert source.count(_RESET) == 3
    source = source.replace(_RESET, '')
    for original, replacement in reversed(_cache_changes()):
        source = once(source, replacement, original)
    return source


_SAMPLER_MARKER = 'UC_SAMPLER_INPUT_CACHE'
_SAMPLER_KEY = '''uc_sampler_inputs::Key MakeSamplerInputs(const State& s, const DrawTextures& stages,
                                             bool clamp, uint32_t aniso) {
    static_assert(kTssAddressU == 0 && kTssMaxAnisotropy == 8);
    uc_sampler_inputs::Key key;
    for (size_t i = 0; i < 4; ++i) {
        // The first nine words plus BorderColor are every StageSampler TSS input.
        std::memcpy(key.stage[i].data(), s.tss[i].data(), 9 * sizeof(uint32_t));
        key.stage[i][9] = s.tss[i][kTssBorderColor];
        key.stage[i][10] = stages.sv[i].Texture2D.MipLevels;
        key.stage[i][11] = stages.info[i].shadow != 0;
    }
    key.aniso = aniso;
    key.clamp = clamp;
    key.shadow_func = s.rs[kRsShadowFunc];
    return key;
}
'''
_SAMPLER_LOOKUP = '''    const auto inputs = MakeSamplerInputs(s, stages, clamp, aniso);
    uint32_t previous_slot;
    if (last_sampler_inputs_.Find(inputs, previous_slot)) {
#ifndef UC_BURNER_RELEASE // UC_SAMPLER_INPUT_CACHE
        if (diag::Perf()) ++g_perf.sampler_input_hits;
#endif
        return previous_slot;
    }
#ifndef UC_BURNER_RELEASE // UC_SAMPLER_INPUT_CACHE
    if (diag::Perf()) ++g_perf.sampler_input_misses;
#endif
'''


def _sampler_changes():
    return [
        ('#include "xbe_frame_reuse.h"',
         '#include "xbe_sampler_inputs.h" // UC_SAMPLER_INPUT_CACHE\n#include "xbe_frame_reuse.h"'),
        ('    std::unordered_map<uint64_t, uint32_t> samplers_;\n',
         '    std::unordered_map<uint64_t, uint32_t> samplers_;\n    uc_sampler_inputs::Cache last_sampler_inputs_;\n'),
        ('    uint64_t verts = 0;',
         '    uint64_t sampler_input_hits = 0, sampler_input_misses = 0; // UC_SAMPLER_INPUT_CACHE\n    uint64_t verts = 0;'),
        ('void D3D12Backend::CreateHeaps() {\n',
         'void D3D12Backend::CreateHeaps() {\n    last_sampler_inputs_.Reset(); // UC_SAMPLER_INPUT_CACHE\n'),
        ('    heap=std::move(replacement);\n',
         '    heap=std::move(replacement);\n    if (sampler) last_sampler_inputs_.Reset(); // UC_SAMPLER_INPUT_CACHE\n'),
        ('uint32_t D3D12Backend::EnsureStageSamplers(',
         _SAMPLER_KEY+'\nuint32_t D3D12Backend::EnsureStageSamplers('),
        ('    uint32_t aniso = app::Get().aniso;\n', '    uint32_t aniso = app::Get().aniso;\n'+_SAMPLER_LOOKUP),
        ('    if (it != samplers_.end()) return it->second;', '''    if (it != samplers_.end()) {
        last_sampler_inputs_.Store(inputs, it->second);
        return it->second;
    }'''),
        ('    samplers_[key] = base;\n', '    samplers_[key] = base;\n    last_sampler_inputs_.Store(inputs, base);\n'),
        ('        LOGR("perf: per frame %.0f draws', '''        LOGR("perf: sampler inputs %.1f hits %.1f misses/frame (exact adjacent inputs)",
             p.sampler_input_hits / n, p.sampler_input_misses / n);
        LOGR("perf: per frame %.0f draws'''),
    ]


def sampler_transform(source):
    if _SAMPLER_MARKER in source:
        for _, replacement in _sampler_changes():
            assert source.count(replacement) == 1, 'partial sampler cache preparation'
        return source
    for original, replacement in _sampler_changes():
        source = once(source, original, replacement)
    return source


def sampler_undo(source):
    for original, replacement in reversed(_sampler_changes()):
        source = once(source, replacement, original)
    return source


def sampler_fixture(source, shaders):
    baseline = sampler_undo(source)
    helpers = '\n\n'.join(function(baseline, name) for name in (
        'float Bits(', 'void Color(', 'D3D12_COMPARISON_FUNC Cmp(', 'D3D12_TEXTURE_ADDRESS_MODE Address('))
    helpers += '\n\n'+function(shaders, 'uint64_t Hash(const void* p, size_t n, uint64_t h) {').replace(
        'uint64_t Hash(', 'uint64_t gpu::Hash(', 1)
    constants = baseline[baseline.index('// D3DTEXF:'):baseline.index('D3D12_SAMPLER_DESC StageSampler(')]
    helpers += '\n'+constants+'\n'+function(baseline, 'D3D12_SAMPLER_DESC StageSampler(')
    fixture = Path(__file__).with_name('test_xbe_sampler_inputs.cpp').read_text()
    fixture = once(fixture, '/* HELPERS */', helpers)
    for label, version in [('BEFORE', baseline), ('AFTER', source)]:
        methods = '\n\n'.join(function(version, name) for name in (
            'void D3D12Backend::CreateHeaps(', 'void D3D12Backend::RenewDescriptorHeap(',
            'uint32_t D3D12Backend::EnsureBlitSamplers(', 'uint32_t D3D12Backend::EnsureStageSamplers('))
        # Replace only the mock interface name; allocation and retirement remain production code.
        methods = methods.replace('ComPtr<ID3D12DescriptorHeap>', 'ComPtr<MockHeap>').replace(
            'ID3D12DescriptorHeap* heaps[]', 'MockHeap* heaps[]')
        if label == 'AFTER':
            methods = function(source, 'uc_sampler_inputs::Key MakeSamplerInputs(')+'\n'+methods
        fixture = once(fixture, '/* '+label+' */', methods)
    return fixture


def prepare(area):
    area = Path(area)
    path = area / 'src/compat/d3d12.cpp'
    source = path.read_text()
    if 'last_texture_heap_' not in source:
        source = once(source, '    uint32_t AllocSrvSlots(uint32_t n);', '''    uint32_t AllocSrvSlots(uint32_t n);
    uint32_t TextureViewSlots(const DrawTextures& stages);
    ID3D12DescriptorHeap* last_texture_heap_=nullptr;
    std::array<ComPtr<ID3D12Resource>,4> last_texture_resources_;
    D3D12_SHADER_RESOURCE_VIEW_DESC last_texture_views_[4]{};
    uint32_t last_texture_slot_=0;''')
        # Submit(true), frame retirement and heap rollover each reset the arena.
        assert source.count('f.srv = 0;') == 3
        source = source.replace('f.srv = 0;', 'f.srv = 0;\n    last_texture_heap_=nullptr;')
        marker = 'void D3D12Backend::SetTextureViewsAndSamplers('
        start = source.index(marker)
        source = source[:start] + '''// Hold four resources so an address cannot be reused while its descriptor is cached.
uint32_t D3D12Backend::TextureViewSlots(const DrawTextures& stages) {
    bool same=last_texture_heap_==srv_heap_.Get();
    for(unsigned i=0;same && i<4;++i)
        same=last_texture_resources_[i].Get()==stages.res[i] &&
            !std::memcmp(&last_texture_views_[i],&stages.sv[i],sizeof(stages.sv[i]));
    if(same)return last_texture_slot_;
    const uint32_t slot=AllocSrvSlots(4);
    for(unsigned i=0;i<4;++i) {
        dev_->CreateShaderResourceView(stages.res[i],&stages.sv[i],Cpu(srv_heap_.Get(),slot+i,srv_inc_));
        last_texture_resources_[i]=stages.res[i];
    }
    std::memcpy(last_texture_views_,stages.sv,sizeof(last_texture_views_));
    last_texture_slot_=slot;last_texture_heap_=srv_heap_.Get();
    return slot;
}

''' + source[start:]
        old = function(source, marker)
        new = once(old, '    uint32_t slot = AllocSrvSlots(4);\n', '')
        new = once(new, '        dev_->CreateShaderResourceView(stages.res[i], &sv, Cpu(srv_heap_.Get(), slot + i, srv_inc_));\n', '')
        new = once(new, '    uint32_t samplers =', '    const uint32_t slot=TextureViewSlots(stages);\n    uint32_t samplers =')
        source = once(source, old, new)
    source = cache_transform(source)
    baseline = cache_undo(source)
    assert cache_transform(source) == source and cache_transform(baseline) == source
    source = sampler_transform(source)
    assert sampler_transform(source) == source and sampler_transform(sampler_undo(source)) == source
    assert cache_transform(source) == source  # The two independent transforms compose.
    path.write_text(source)
    scripts = Path(__file__).parent
    (area / 'src/compat/xbe_srv_cache.h').write_text((scripts / 'xbe_srv_cache.h').read_text())
    (area / 'src/compat/xbe_sampler_inputs.h').write_text((scripts / 'xbe_sampler_inputs.h').read_text())
    fixture = Path(__file__).with_name('test_xbe_draw_reuse.cpp').read_text()
    for label, version in [('BEFORE', baseline), ('AFTER', source)]:
        methods = '\n\n'.join(function(version, name) for name in (
            'uint32_t D3D12Backend::TextureViewSlots(', 'uint32_t D3D12Backend::AllocSrvSlots('))
        fixture = once(fixture, '/* ' + label + ' */', methods)
    shaders = (area / 'src/compat/shaders.cpp').read_text()
    (area / 'src/compat/test_sampler_inputs.cpp').write_text(sampler_fixture(source, shaders))
    hash_impl = function(shaders, 'uint64_t Hash(const void* p, size_t n, uint64_t h) {')
    hash_impl = hash_impl.replace('uint64_t Hash(', 'uint64_t ProductionHash(', 1)
    fixture = once(fixture, '/* HASH_IMPLEMENTATION */', hash_impl)
    (area / 'src/compat/test_draw_reuse.cpp').write_text(fixture)

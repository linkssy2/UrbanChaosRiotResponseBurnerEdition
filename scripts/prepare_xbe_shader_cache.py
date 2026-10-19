"""Reuse validated DXBC across launches, without changing shader options or draws."""
from pathlib import Path
import hashlib
from prepare_xbe_perf import function, once


def transform(source):
    if '#include "xbe_shader_cache.h"' in source:
        assert source.count('uc_xbe_shader_cache::Load(') == 1
        assert source.count('uc_xbe_shader_cache::Save(') == 1
        return source
    source = once(source, '#include "core/diag.h"', '#include "core/diag.h"\n#include "xbe_shader_cache.h"')
    source = once(source, '    LoadSettings();', '''    uc_xbe_shader_cache::Warm([this](const std::string& src,const std::string& entry,const std::string& profile,ID3DBlob* code){
        uint64_t seed=Hash(profile.data(),profile.size());
        seed=Hash(entry.data(),entry.size(),seed);
        blobs_[Hash(src.data(),src.size(),seed)]=code;
    });
    // Built-in presentation variants are known before any title draw.
    EnsureBlitPipeline(DXGI_FORMAT_B8G8R8A8_UNORM,false);
    EnsureBlitPipeline(DXGI_FORMAT_B8G8R8A8_UNORM,true);
    LoadSettings();''')
    source = once(source, '    uint64_t shaders = 0;\n', '    uint64_t shaders = 0;\n    uint64_t shader_disk_hits = 0;\n')
    old = function(source, 'ID3DBlob* D3D12Backend::Compile(')
    new = once(old, '    ComPtr<ID3DBlob> err;\n', '''    if (uc_xbe_shader_cache::Load(src, entry, profile, D3DCOMPILE_OPTIMIZATION_LEVEL1, 0, &code)) {
        ++g_perf.shader_disk_hits;
        blobs_[hash] = code;
        return code.Get();
    }
    ComPtr<ID3DBlob> err;
''')
    new = once(new, '''    PerfScope perf_compile{g_perf.compile};
    g_perf.shaders++;
    HRESULT hr = D3DCompile(src.data(), src.size(), nullptr, nullptr, nullptr, entry, profile,
                            D3DCOMPILE_OPTIMIZATION_LEVEL1, 0, &code, &err);
''', '''    g_perf.shaders++;
    HRESULT hr;
    {
        PerfScope perf_compile{g_perf.compile};
        hr = D3DCompile(src.data(), src.size(), nullptr, nullptr, nullptr, entry, profile,
                        D3DCOMPILE_OPTIMIZATION_LEVEL1, 0, &code, &err);
    }
''')
    new = once(new, '    blobs_[hash] = code;\n    return code.Get();\n}',
               '    uc_xbe_shader_cache::Save(src, entry, profile, D3DCOMPILE_OPTIMIZATION_LEVEL1, 0, code.Get());\n'
               '    blobs_[hash] = code;\n    return code.Get();\n}')
    source = once(source, old, new)
    source = once(source, '%llu shaders %llu psos this second",', '%llu shaders %llu psos this second, %llu shader disk hits",')
    source = once(source, '(unsigned long long)p.shaders, (unsigned long long)p.psos);',
                  '(unsigned long long)p.shaders, (unsigned long long)p.psos, (unsigned long long)p.shader_disk_hits);')
    return source


def prepare(source, area):
    area = Path(area)
    local = Path(__file__).parent
    prior = local.parent/'third_party/xboxrecomp/src/nv2a'
    # Keep the validated exact-key, bounded read, payload hash and atomic replace
    # implementation. Only the two C implicit conversions need C++ spellings.
    header = (prior/'nv2a_shader_disk_cache.h').read_text()
    header = once(header, 'const unsigned char *b=p;', 'const unsigned char *b=(const unsigned char*)p;')
    header = once(header, 'saved=malloc(n?n:1);', 'saved=(char*)malloc(n?n:1);')
    # Rebuildable cache: keep atomic replacement, without a forced disk flush on
    # the render thread. An interrupted/corrupt cache entry is just another miss.
    header = once(header, 'MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH', 'MOVEFILE_REPLACE_EXISTING')
    (area/'src/compat/nv2a_shader_disk_cache.h').write_text(header)
    (area/'src/compat/nv2a_shader_compiler_identity.h').write_text((prior/'nv2a_shader_compiler_identity.h').read_text())
    for name in ('xbe_shader_cache.h', 'xbe_shader_cache.cpp'):
        (area/'src/compat'/name).write_text((local/name).read_text())
    # Root signature/state construction changes invalidate driver blobs automatically.
    contract=hashlib.sha256(source.encode()).hexdigest()
    (area/'src/compat/xbe_pipeline_contract.h').write_text('#define UC_PIPELINE_CONTRACT "'+contract+'"\n')
    result = transform(source)
    assert transform(result) == result
    fixture = (local/'test_xbe_shader_cache.cpp').read_text()
    marker = '/* COMPILE_IMPLEMENTATION */'
    assert fixture.count(marker) == 1
    fixture = fixture.replace(marker, function(result, 'ID3DBlob* D3D12Backend::Compile('))
    (area/'src/compat/test_shader_cache.cpp').write_text(fixture)
    for desc in ('p','d'):
        result=result.replace('dev_->CreateGraphicsPipelineState(&'+desc+', IID_PPV_ARGS(&pso))',
            'uc_xbe_shader_cache::CreatePipeline(dev_.Get(),'+desc+',pso.GetAddressOf())')
    assert 'dev_->CreateGraphicsPipelineState(' not in result
    return result

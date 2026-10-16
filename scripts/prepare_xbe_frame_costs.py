"""Reuse exact fog qualifications, completed readbacks and unchanged draw uploads."""
from pathlib import Path
from prepare_xbe_perf import once, function

def transform(source):
    if '#include "xbe_frame_reuse.h"' in source:
        return source
    source=once(source,'#include "core/diag.h"','#include "core/diag.h"\n#include "xbe_frame_reuse.h"')
    source=once(source,'    Alloc Upload(uint64_t size, uint64_t align = 256);',
        '''    uc_frame_reuse::Qualification<PixelKey> fog_qualification_;
    uc_frame_reuse::Geometry<Alloc> geometry_reuse_;
    ComPtr<ID3D12Resource> sync_readback_;
    uint64_t sync_readback_bytes_=0;
    Alloc Upload(uint64_t size, uint64_t align = 256);''')
    source=once(source,'    if (smooth_tone) {\n        auto check=PixelHlsl(pk);\n        smooth_tone=SmoothToneShader(check);',
        '''    unsigned fog_bits=0;
    if(smooth_tone || analytic_fog)fog_bits=fog_qualification_.Get(sh.pkey,pk,[&]{
        auto original=PixelHlsl(pk),check=original;
        unsigned bits=SmoothToneShader(check)?1u:0u;
        check=std::move(original);
        return bits|(AnalyticFogShader(check)?2u:0u);
    });
    if (smooth_tone) {
        smooth_tone=(fog_bits&1)!=0;''')
    source=once(source,'        auto check=PixelHlsl(pk);analytic_fog=AnalyticFogShader(check);',
        '        analytic_fog=(fog_bits&2)!=0;')
    source=once(source,'        Alloc vb = Upload(b.bytes, 16);\n        std::memcpy(vb.cpu, b.src, b.bytes);',
        '''        Alloc vb=geometry_reuse_.Get(b.src,b.bytes,[&]{
            Alloc result=Upload(b.bytes,16);
            std::memcpy(result.cpu,b.src,b.bytes);return result;
        });''')
    source=once(source,'        Alloc ib = Upload(ibytes, 4);\n        std::memcpy(ib.cpu, a.index_src, ibytes);',
        '''        Alloc ib=geometry_reuse_.Get(a.index_src,ibytes,[&]{
            Alloc result=Upload(ibytes,16);
            std::memcpy(result.cpu,a.index_src,ibytes);return result;
        });''')
    # Both mid-frame synchronous submission and normal frame reuse invalidate offsets.
    assert source.count('f.used = 0;')==2
    source=source.replace('f.used = 0;', 'f.used = 0;\n    geometry_reuse_.Reset();')
    old=function(source,'bool D3D12Backend::ReadBack(')
    new=once(old,'    ComPtr<ID3D12Resource> rb;', '    // Previous synchronous readback completed before this buffer can be reused.\n    auto& rb=sync_readback_;')
    new=once(new,'''    if (FAILED(dev_->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &bd, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                             IID_PPV_ARGS(&rb)))) {
        return false;
    }''', '''    if(!rb || bytes>sync_readback_bytes_) {
        ComPtr<ID3D12Resource> replacement;
        if(FAILED(dev_->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&bd,
            D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&replacement))))return false;
        rb=std::move(replacement);sync_readback_bytes_=bytes;
    }''')
    new=once(new,'    rb->Map(0, nullptr, reinterpret_cast<void**>(&p));',
        '    D3D12_RANGE range{0,SIZE_T(bytes)};\n    if(FAILED(rb->Map(0,&range,reinterpret_cast<void**>(&p))))return false;')
    new=once(new,'    rb->Unmap(0, nullptr);', '    D3D12_RANGE written{0,0};\n    rb->Unmap(0,&written);')
    return once(source,old,new)

def prepare(area):
    area=Path(area); path=area/'src/compat/d3d12.cpp'
    source=transform(path.read_text());assert transform(source)==source
    path.write_text(source)
    local=Path(__file__).parent
    for name in ('xbe_frame_reuse.h','test_xbe_frame_reuse.cpp'):
        (area/'src/compat'/name).write_text((local/name).read_text())
    fixture=(local/'test_xbe_readback_reuse.cpp').read_text()
    fixture=once(fixture,'/* READBACK_IMPLEMENTATION */',function(source,'bool D3D12Backend::ReadBack('))
    (area/'src/compat/test_xbe_readback_reuse.cpp').write_text(fixture)

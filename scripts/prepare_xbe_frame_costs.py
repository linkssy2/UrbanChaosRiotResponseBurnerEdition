"""Reuse fog/readback resources and keep geometry snapshots in frame-owned uploads."""
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
    source=texture_size_scope(source);assert texture_size_scope(source)==source
    source=palette_initialization(source);assert palette_initialization(source)==source
    source=texture_dimensions(source);assert texture_dimensions(source)==source
    source=texture_target_eligibility(source);assert texture_target_eligibility(source)==source
    source=descriptor_lifetime(source);assert descriptor_lifetime(source)==source
    source=upload_spill(source);assert upload_spill(source)==source
    path.write_text(source)
    local=Path(__file__).parent
    for name in ('xbe_frame_reuse.h','xbe_upload_spill.h','test_xbe_frame_reuse.cpp'):
        (area/'src/compat'/name).write_text((local/name).read_text())
    fixture=(local/'test_xbe_readback_reuse.cpp').read_text()
    fixture=once(fixture,'/* READBACK_IMPLEMENTATION */',function(source,'bool D3D12Backend::ReadBack('))
    (area/'src/compat/test_xbe_readback_reuse.cpp').write_text(fixture)
    fixture=(local/'test_xbe_descriptor_lifetime.cpp').read_text()
    fixture=once(fixture,'/* DESCRIPTOR_IMPLEMENTATION */','\n'.join(function(source,name) for name in (
        'void D3D12Backend::RenewDescriptorHeap(', 'uint32_t D3D12Backend::AllocSrvSlots(')))
    (area/'src/compat/test_xbe_descriptor_lifetime.cpp').write_text(fixture)
    (area/'src/compat/test_xbe_palette_init.cpp').write_text(palette_fixture(source))
    (area/'src/compat/test_xbe_texture_constants.cpp').write_text(texture_dimension_fixture(source))
    name='test_xbe_texture_constants_model.inc'
    (area/'src/compat'/name).write_text((local/name).read_text())

DIMENSION_FIELD='    // Authored dimensions remain valid even when host binding fails.\n    uint32_t width[4], height[4];'
DIMENSION_DECL='    void FillTexelConstants(int i, const TextureDesc& d, const DrawTextures& stages, DrawConstants& c);'
DIMENSION_NEW_DECL=DIMENSION_DECL.replace('const TextureDesc& d,','uint32_t width, uint32_t height,')
DIMENSION_DESCRIBE='    if (!s.textures[size_t(stage)] || !Describe(s.textures[size_t(stage)], d)) return nullptr;'
DIMENSION_READ='''        TextureDesc d;
        if (s.textures[size_t(i)] && Describe(s.textures[size_t(i)], d)) FillTexelConstants(i, d, stages, c);'''
DIMENSION_REUSE='''        uint32_t width = stages.width[i], height = stages.height[i];
        // Extensions may mutate guest headers between binding and constants.
        if (ext_) {
            TextureDesc d;
            width = 0;
            if (s.textures[size_t(i)] && Describe(s.textures[size_t(i)], d)) {
                width = d.width; height = d.height;
            }
        }
        if (width) FillTexelConstants(i, width, height, stages, c);'''

def texture_dimensions(source):
    if DIMENSION_FIELD in source:
        assert source.count(DIMENSION_FIELD)==source.count(DIMENSION_REUSE)==1
        assert source.count(DIMENSION_NEW_DECL)==1
        return source
    import re
    source=once(source,'    ID3D12Resource* res[4];','    ID3D12Resource* res[4];\n'+DIMENSION_FIELD)
    source=once(source,DIMENSION_DECL,DIMENSION_NEW_DECL)
    old=function(source,'ID3D12Resource* D3D12Backend::BindTexture(')
    new=once(old,'    info = {};','    info = {};\n    stages.width[stage] = stages.height[stage] = 0;')
    new=once(new,DIMENSION_DESCRIBE,DIMENSION_DESCRIBE+'\n    stages.width[stage] = d.width;\n    stages.height[stage] = d.height;')
    source=once(source,old,new)
    old=function(source,'void D3D12Backend::FillTexelConstants(')
    assert set(re.findall(r'\bd\.(\w+)',old))=={'width','height'}
    new=old.replace('const TextureDesc& d,','uint32_t width, uint32_t height,').replace('d.width','width').replace('d.height','height')
    source=once(source,old,new)
    return once(source,DIMENSION_READ,DIMENSION_REUSE)

def texture_dimension_fixture(source):
    import re
    original=once(source,'\n'+DIMENSION_FIELD,'')
    original=once(original,DIMENSION_NEW_DECL,DIMENSION_DECL)
    original=once(original,'\n    stages.width[stage] = stages.height[stage] = 0;','')
    original=once(original,'\n    stages.width[stage] = d.width;\n    stages.height[stage] = d.height;','')
    original=once(original,DIMENSION_REUSE,DIMENSION_READ)
    old=function(original,'void D3D12Backend::FillTexelConstants(')
    new=once(old,'uint32_t width, uint32_t height,','const TextureDesc& d,')
    new=re.sub(r'\bwidth\b','d.width',new)
    new=re.sub(r'\bheight\b','d.height',new)
    original=once(original,old,new)
    assert texture_dimensions(original)==source
    fixture=Path(__file__).with_name('test_xbe_texture_constants.cpp').read_text()
    lookup_before=undo_texture_target_eligibility(source)
    assert texture_target_eligibility(lookup_before)==source
    for label,value in [('BEFORE',original),('AFTER',source),('LOOKUP_BEFORE',lookup_before)]:
        bodies='\n\n'.join(function(value,name) for name in (
            'ID3D12Resource* D3D12Backend::BindTexture(',
            'void D3D12Backend::FillTexelConstants(',
            'void D3D12Backend::FillTextureConstants('))
        fixture=once(fixture,'/* '+label+' */',bodies)
    return fixture

TARGET_LOOKUP_OLD='''    auto tit = targets_.find(SizedTargetKey(d.data, fi.depth, d.width, d.height) | alt);
    if (tit != targets_.end() && !d.cube && d.depth == 1 && !fi.compressed) {
        return BindTargetTexture(s, stage, d, tit->second, box, stages);
    }'''
TARGET_LOOKUP_NEW='''    // Cubes, volumes and compressed textures cannot use a rendered target.
    if (!d.cube && d.depth == 1 && !fi.compressed) {
        auto tit = targets_.find(SizedTargetKey(d.data, fi.depth, d.width, d.height) | alt);
        if (tit != targets_.end())
            return BindTargetTexture(s, stage, d, tit->second, box, stages);
    }'''

def texture_target_eligibility(source):
    old=function(source,'ID3D12Resource* D3D12Backend::BindTexture(')
    if TARGET_LOOKUP_NEW in old:
        assert old.count(TARGET_LOOKUP_NEW)==1 and TARGET_LOOKUP_OLD not in old
        return source
    # Only move the non-mutating map search. Extension callbacks, guest reads,
    # the separate depth-colour alias search and content checks keep their order.
    return once(source,old,once(old,TARGET_LOOKUP_OLD,TARGET_LOOKUP_NEW))

def undo_texture_target_eligibility(source):
    old=function(source,'ID3D12Resource* D3D12Backend::BindTexture(')
    return once(source,old,once(old,TARGET_LOOKUP_NEW,TARGET_LOOKUP_OLD))

def texture_size_scope(source):
    old=function(source,'ID3D12Resource* D3D12Backend::BindGuestTexture(')
    marker='        // Mip sizes are needed only when checking texture contents.\n'
    declaration='    uint32_t total = TotalSize(d);\n'
    gate='    if (tx.frame != presents_ + 1 || !same || tx.palette != palette.hash) {\n'
    scoped=gate+marker+'    '+declaration
    if marker in old:
        assert old.count(scoped)==1 and old.count('TotalSize(d)')==1
        return source
    assert old.count(declaration)==1 and old.count(gate)==1
    # All consumers already belong to this block; retain the existing frame,
    # layout and palette invalidation checks and the same size calculation.
    new=once(old,declaration,'')
    new=once(new,gate,scoped)
    return once(source,old,new)

def palette_initialization(source):
    declaration='    uint32_t colours[256] = {};'
    lazy='    uint32_t colours[256]; // initialized only for a bound P8 palette'
    gate='    if (d.format != kFmtP8 || !s.palettes[size_t(stage)]) return;'
    clear='    std::memset(palette.colours, 0, sizeof(palette.colours));'
    old=function(source,'void ReadStagePalette(')
    # The unused array is never passed to the decoder without a palette address.
    assert 'palette.va ? palette.colours : nullptr' in source
    if lazy in source:
        assert source.count(lazy)==1 and old.count(gate+'\n'+clear)==1
        return source
    source=once(source,"// a P8 texture's palette: its colours, their address (part of the texture's key) and their hash, all 0 without one",
        "// Bound P8 colours; the address and hash stay zero when no palette is used.")
    source=once(source,declaration,lazy)
    return once(source,old,once(old,gate,gate+'\n'+clear))

def palette_fixture(source):
    start=source.index('struct StagePalette {')
    declaration=source[start:source.index('\n};',start)+3]
    reader=function(source,'void ReadStagePalette(')
    lazy='    uint32_t colours[256]; // initialized only for a bound P8 palette'
    clear='    std::memset(palette.colours, 0, sizeof(palette.colours));\n'
    original=once(declaration,lazy,'    uint32_t colours[256] = {};')+'\n'+once(reader,clear,'')
    fixture=Path(__file__).with_name('test_xbe_palette_init.cpp').read_text()
    fixture=once(fixture,'/* ORIGINAL_PALETTE */',original)
    return once(fixture,'/* PRODUCTION_PALETTE */',declaration+'\n'+reader)

def descriptor_lifetime(source):
    if 'void D3D12Backend::RenewDescriptorHeap(' in source:return source
    source=once(source,'#include "xbe_frame_reuse.h"','#include "xbe_frame_reuse.h"\n#include <new>')
    source=once(source,'        std::vector<ComPtr<ID3D12Resource>> garbage;',
        '        std::vector<ComPtr<ID3D12Resource>> garbage;\n        std::vector<ComPtr<ID3D12DescriptorHeap>> retired_heaps;')
    assert source.count('f.garbage.clear();')==2
    source=source.replace('f.garbage.clear();','f.garbage.clear();\n        f.retired_heaps.clear();')
    source=once(source,'    uint32_t AllocSrvSlots(uint32_t n);',
        '    void RenewDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type);\n    uint32_t AllocSrvSlots(uint32_t n);')
    source=once(source,'uint32_t D3D12Backend::AllocSrvSlots(uint32_t n) {', '''// Retire with this frame's fence, which also covers all earlier queue submissions.
void D3D12Backend::RenewDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) {
    const bool sampler=type==D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    auto& heap=sampler?smp_heap_:srv_heap_;
    ComPtr<ID3D12DescriptorHeap> replacement;
    CreateHeap(type,sampler?kSamplerCount:kSrvCount,true,replacement);
    if(!replacement)throw std::bad_alloc();
    CurrentFrame().retired_heaps.push_back(heap);
    heap=std::move(replacement);
    ID3D12DescriptorHeap* heaps[]={srv_heap_.Get(),smp_heap_.Get()};
    cl_->SetDescriptorHeaps(2,heaps);
}

uint32_t D3D12Backend::AllocSrvSlots(uint32_t n) {''')
    source=once(source,'        LOG_FIRST(4, "d3d12: descriptor ring wrapped within a frame");',
        '        RenewDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);')
    for name in ('uint32_t D3D12Backend::EnsureBlitSamplers(', 'uint32_t D3D12Backend::EnsureStageSamplers('):
        old=function(source,name)
        new=once(old,'samplers_.clear();','RenewDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);\n        samplers_.clear();')
        source=once(source,old,new)
    # Changing either heap invalidates descriptor tables: set both after allocation.
    for name, table, allocation in (
        ('void D3D12Backend::DrawFullScreen(',
         '    cl_->SetGraphicsRootDescriptorTable(2, Gpu(srv_heap_.Get(), srv_slot, srv_inc_));',
         '    uint32_t si = sampler_slot ? *sampler_slot : EnsureBlitSamplers();'),
        ('void D3D12Backend::SetTextureViewsAndSamplers(',
         '    cl_->SetGraphicsRootDescriptorTable(2, Gpu(srv_heap_.Get(), slot, srv_inc_));',
         '    uint32_t samplers = EnsureStageSamplers(s, stages, UnitTexcoords(s, a));')):
        old=function(source,name)
        new=once(old,table+'\n','')
        new=once(new,allocation,allocation+'\n'+table)
        source=once(source,old,new)
    return source

def upload_spill(source):
    if '#include "xbe_upload_spill.h"' in source:return source
    source=once(source,'#include "xbe_frame_reuse.h"','#include "xbe_frame_reuse.h"\n#include "xbe_upload_spill.h"')
    source=once(source,'        uint64_t used = 0;', '''        uint64_t used = 0;
        uc_frame_reuse::UploadSpill spill;
        std::array<ComPtr<ID3D12Resource>,uc_frame_reuse::UploadSpill::page_count> spill_resources;
        std::array<uint8_t*,uc_frame_reuse::UploadSpill::page_count> spill_maps{};''')
    assert source.count('geometry_reuse_.Reset();')==2
    source=source.replace('geometry_reuse_.Reset();','geometry_reuse_.Reset();\n    f.spill.Reset();')
    source=once(source,'    if (CurrentFrame().used > kUploadSize - kUploadReserve) Submit(true);',
        '    if (CurrentFrame().spill.Exhausted()) Submit(true);')
    source=once(source,'    // past the ring every upload is a buffer of its own: seconds lost on a frame that draws a whole level once',
        '    // Reclaim only after bounded spill pages overflow; never reset uploads during a draw.')
    old=function(source,'D3D12Backend::Alloc D3D12Backend::Upload(')
    at=old.index('    // too big for the ring:')
    new=old[:at]+'''    g_perf.overflow++;
    auto range=f.spill.Get(size,align);
    const bool retained=range.page<uc_frame_reuse::UploadSpill::page_count;
    ComPtr<ID3D12Resource> temporary;
    auto& resource=retained?f.spill_resources[range.page]:temporary;
    uint8_t* map=retained?f.spill_maps[range.page]:nullptr;
    if(!resource){
        const uint64_t bytes=retained?uc_frame_reuse::UploadSpill::page_bytes:((size+65535)&~uint64_t(65535));
        D3D12_HEAP_PROPERTIES hp{D3D12_HEAP_TYPE_UPLOAD};auto bd=BufferDesc(bytes);
        ComPtr<ID3D12Resource> replacement;
        if(FAILED(dev_->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&bd,
            D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&replacement))))throw std::bad_alloc();
        D3D12_RANGE read{0,0};
        if(FAILED(replacement->Map(0,&read,reinterpret_cast<void**>(&map))))throw std::bad_alloc();
        resource=std::move(replacement);
        if(retained)f.spill_maps[range.page]=map;
        else f.garbage.push_back(resource);
    }
    return {resource.Get(),range.offset,map+range.offset,resource->GetGPUVirtualAddress()+range.offset};
}'''
    return once(source,old,new)

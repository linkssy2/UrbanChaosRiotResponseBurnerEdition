"""Optional exact-identity world replacements, independent of HD UI."""
from pathlib import Path
from prepare_xbe_perf import once
METHODS=r'''
void D3D12Backend::MatchWorld(Tex& tx,const TextureDesc& d,const FormatInfo& fi,const uint8_t* p,uint32_t bytes) {
    const auto* entry= !ext_&&!tx.signs&&!d.cube&&d.depth==1&&!d.linear&&!fi.depth
        ? uc_world_textures::Match(d.width,d.height,d.format,p,bytes):nullptr;
    if(entry!=tx.world_entry){if(tx.world_res)CurrentFrame().garbage.push_back(tx.world_res);tx.world_res.Reset();tx.world_attempted=false;tx.world_levels=0;}
    tx.world_entry=entry;
}
ID3D12Resource* D3D12Backend::BindWorld(Tex& tx,DrawTextures& stages,int stage) {
    if(!trial_graphics::WorldTextures()||!tx.world_entry)return nullptr;
    if(!tx.world_attempted) {
        tx.world_attempted=true;
        uc_world_textures::Image image;if(!uc_world_textures::Load(*tx.world_entry,image))return nullptr;
        Tex upload;upload.fmt=DXGI_FORMAT_BC3_UNORM;upload.levels=uint32_t(image.mips.size());
        auto desc=Texture2DDesc(upload.fmt,image.width,image.height);desc.MipLevels=UINT16(upload.levels);
        D3D12_HEAP_PROPERTIES heap{D3D12_HEAP_TYPE_DEFAULT};
        if(FAILED(dev_->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&upload.res))))return nullptr;
        uint32_t w=image.width;
        for(uint32_t l=0;l<upload.levels;++l){UploadTextureLevel(upload,0,l,image.mips[l],((w+3)/4)*16);w=std::max(1u,w/2);}
        auto state=D3D12_RESOURCE_STATE_COPY_DEST;Transition(upload.res.Get(),state,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        tx.world_res=std::move(upload.res);tx.world_levels=upload.levels;
    }
    if(!tx.world_res)return nullptr;
    stages.sv[stage].Format=DXGI_FORMAT_BC3_UNORM;stages.sv[stage].Texture2D.MipLevels=tx.world_levels;
    return tx.world_res.Get();
}
'''
def prepare(area):
    out=Path(area)/'src/compat';p=out/'d3d12.cpp';s=p.read_text()
    s=once(s,'#include "xbe_hd_ui.h" // UC_HD_UI_TEXTURES','#include "xbe_hd_ui.h" // UC_HD_UI_TEXTURES\n#include "xbe_world_textures.h"')
    s=once(s,'    bool hd_ui_attempted = false;','''    bool hd_ui_attempted = false;
    const uc_world_textures::Entry* world_entry=nullptr;
    ComPtr<ID3D12Resource> world_res;
    uint32_t world_levels=0;
    bool world_attempted=false;''')
    s=once(s,'    void RetireHdUi(Tex&);','''    void RetireHdUi(Tex&);
    void MatchWorld(Tex&,const TextureDesc&,const FormatInfo&,const uint8_t*,uint32_t);
    ID3D12Resource* BindWorld(Tex&,DrawTextures&,int);''')
    s=once(s,'void D3D12Backend::RetireHdUi(Tex& tx) {','''void D3D12Backend::RetireHdUi(Tex& tx) {
    if(tx.world_res)CurrentFrame().garbage.push_back(tx.world_res);
    tx.world_res.Reset();tx.world_entry=nullptr;tx.world_attempted=false;tx.world_levels=0;''')
    s=once(s,'            MatchHdUi(tx, d, fi, p, total);','''            MatchHdUi(tx, d, fi, p, total);
            MatchWorld(tx, d, fi, p, total);''')
    s=once(s,'    if (auto* replacement = BindHdUi(tx, stages, stage)) return replacement;','''    if (auto* replacement = BindWorld(tx, stages, stage)) return replacement;
    if (auto* replacement = BindHdUi(tx, stages, stage)) return replacement;''')
    s=once(s,'void D3D12Backend::MatchHdUi(',METHODS+'\nvoid D3D12Backend::MatchHdUi(')
    p.write_text(s)
    for name in ['xbe_world_textures.h','test_xbe_world_textures.cpp']:(out/name).write_text(Path(__file__).with_name(name).read_text())

"""Bind prepared, exactly identified UI art without changing guest dimensions.

Run after pc_prompts and the final renderer preparations. The service is loaded
once by frontend font startup; neither matching nor filtering happens per draw.
"""
from pathlib import Path
import json
import shutil
from prepare_xbe_perf import once, function
from prepare_xbe_hd_ui_artwork import prepare as prepare_artwork

MARKER = 'UC_HD_UI_TEXTURES'

METHODS = r'''
// UC_HD_UI_TEXTURES: original and prepared images have independent GPU lifetimes.
void D3D12Backend::RetireHdUi(Tex& tx) {
    if (tx.hd_ui_res) CurrentFrame().garbage.push_back(tx.hd_ui_res);
    tx.hd_ui_res.Reset();
    tx.hd_ui_image = nullptr;
    tx.hd_ui_attempted = false;
}

void D3D12Backend::MatchHdUi(Tex& tx, const TextureDesc& d, const FormatInfo& fi,
                            const uint8_t* p, uint32_t bytes) {
    // Only immutable 2D colour assets in the prepared manifest can match.
    // The service checks the complete authored mip payload; P8 needs a palette.
    const auto* image = !ext_ && !tx.signs && !d.cube && d.depth == 1 &&
        !d.linear && !fi.depth && d.format != 0x0B
        ? uc_hd_ui::Match(d.width, d.height, d.format, p, bytes) : nullptr;
    if (image != tx.hd_ui_image) RetireHdUi(tx);
    tx.hd_ui_image = image;
    if (!tx.hd_ui_res) tx.hd_ui_attempted = false;
}

ID3D12Resource* D3D12Backend::BindHdUi(Tex& tx, DrawTextures& stages, int stage) {
    if (!trial_graphics::HdUi() || !tx.hd_ui_image) return nullptr;
    if (!tx.hd_ui_res && !tx.hd_ui_attempted) {
        tx.hd_ui_attempted = true; // allocation failure falls back, not every draw
        const auto& image = *tx.hd_ui_image;
        if (!image.width || !image.height || image.width > 16384 || image.height > 16384 ||
            uint64_t(image.width) * image.height * 4 != image.bgra.size()) return nullptr;
        Tex upload;
        upload.fmt = DXGI_FORMAT_B8G8R8A8_UNORM;
        upload.levels = 1;
        auto desc = Texture2DDesc(upload.fmt, image.width, image.height);
        D3D12_HEAP_PROPERTIES heap{D3D12_HEAP_TYPE_DEFAULT};
        if (FAILED(dev_->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&upload.res)))) return nullptr;
        UploadTextureLevel(upload, 0, 0, image.bgra, image.width * 4);
        auto state = D3D12_RESOURCE_STATE_COPY_DEST;
        Transition(upload.res.Get(), state, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        tx.hd_ui_res = std::move(upload.res);
    }
    if (!tx.hd_ui_res) return nullptr;
    // Authored UVs, rectangle coordinates and texel constants keep tx.d unchanged.
    stages.sv[stage].Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    stages.sv[stage].Texture2D.MipLevels = 1;
    return tx.hd_ui_res.Get();
}
'''


def renderer_changes():
    return [
        ('#include "xbe_pc_prompts.h"', '#include "xbe_pc_prompts.h"\n#include "xbe_hd_ui.h" // UC_HD_UI_TEXTURES'),
        ('    unsigned pc_prompt_id = 0;\n', '''    unsigned pc_prompt_id = 0;
    const uc_hd_ui::Image* hd_ui_image = nullptr;
    ComPtr<ID3D12Resource> hd_ui_res;
    bool hd_ui_attempted = false;
'''),
        ('    ID3D12Resource* BindPcPrompt(unsigned,unsigned,unsigned,const TextureDesc&,DrawTextures&,int);',
         '''    ID3D12Resource* BindPcPrompt(unsigned,unsigned,unsigned,const TextureDesc&,DrawTextures&,int);
    void RetireHdUi(Tex&);
    void MatchHdUi(Tex&, const TextureDesc&, const FormatInfo&, const uint8_t*, uint32_t);
    ID3D12Resource* BindHdUi(Tex&, DrawTextures&, int);'''),
        ('    tx = Tex{};\n', '    RetireHdUi(tx);\n    tx = Tex{};\n'),
        ('            tx.pc_prompt_id=uc_pc_prompts::Match(d,p,total);',
         '            tx.pc_prompt_id=uc_pc_prompts::Match(d,p,total);\n            MatchHdUi(tx, d, fi, p, total);'),
        ('    if (fi.depth) info.shadow = ShadowDepthKind(d.format);\n    if (ext_) ext_->Sampled(d, nullptr, tx.flags, info);',
         '''    if (auto* replacement = BindHdUi(tx, stages, stage)) return replacement;
    if (fi.depth) info.shadow = ShadowDepthKind(d.format);
    if (ext_) ext_->Sampled(d, nullptr, tx.flags, info);'''),
        ('            CurrentFrame().garbage.push_back(it->second.res);\n            it = textures_.erase(it);',
         '            CurrentFrame().garbage.push_back(it->second.res);\n            RetireHdUi(it->second);\n            it = textures_.erase(it);'),
        ('ID3D12Resource* D3D12Backend::BindPcPrompt(', METHODS+'\nID3D12Resource* D3D12Backend::BindPcPrompt('),
        ('    struct PcPromptSlot { Tex texture;TrialPromptInput input;ULONGLONG updated=0;',
         '    struct PcPromptSlot { Tex texture;TrialPromptInput input;ULONGLONG updated=0;bool hd_ui=false;'),
        ('    bool changed=!slot.texture.res||!uc_pc_prompts::Equal(slot.input,input);',
         '''    const bool hd_ui=trial_graphics::HdUi();
    bool changed=!slot.texture.res||slot.hd_ui!=hd_ui||!uc_pc_prompts::Equal(slot.input,input);'''),
        ('    if(changed&&(id==1||!slot.texture.res||now-slot.updated>=33||input.held!=slot.input.held)){',
         '    if(changed&&(id==1||!slot.texture.res||slot.hd_ui!=hd_ui||now-slot.updated>=33||input.held!=slot.input.held)){'),
        ('        if(!TrialPromptArtwork(id,raw,input,pixels,width,height))return nullptr;',
         '        if(!TrialPromptArtwork(id,raw,input,pixels,width,height,hd_ui))return nullptr;'),
        ('        slot.input=input;slot.updated=now;',
         '        slot.input=input;slot.updated=now;slot.hd_ui=hd_ui;'),
    ]


def renderer_transform(source):
    if MARKER in source:
        baseline = renderer_undo(source)
        assert renderer_transform(baseline) == source
        return source
    for old, new in renderer_changes():
        source = once(source, old, new)
    return source


def renderer_undo(source):
    for old, new in reversed(renderer_changes()):
        source = once(source, new, old)
    return source


PROMPT_HD = r'''
// The prepared strip keeps all 32 authored cells; generated keycaps stay native.
static const uc_hd_ui::Image* TrialHdPromptButtons(bool enabled) {
    const auto* image = enabled ? uc_hd_ui::Buttons() : nullptr;
    return image && image->width == 128 && image->height == 4096 &&
        image->bgra.size() == size_t(128) * 4096 * 4 ? image : nullptr;
}
'''


def art_changes():
    return [
        ('#include "xbe_pc_prompts.h"', '#include "xbe_pc_prompts.h"\n#include "xbe_hd_ui.h" // UC_HD_UI_PROMPTS'),
        ('unsigned*w,unsigned*h){\n *w=id==1?', 'unsigned*w,unsigned*h,const uc_hd_ui::Image* hd_buttons){\n *w=id==1?'),
        (' if(id==1){\n  for(unsigned row=0;row<32;++row)',
         ''' if(id==1&&hd_buttons)memcpy(pixels,hd_buttons->bgra.data(),hd_buttons->bgra.size());
 if(id==1){
  for(unsigned row=0;row<32;++row)'''),
        ('bool TrialPromptArtwork(', PROMPT_HD+'\nbool TrialPromptArtwork('),
        ('std::vector<uint8_t>& pixels,unsigned& width,unsigned& height){',
         'std::vector<uint8_t>& pixels,unsigned& width,unsigned& height,bool hd_ui){'),
        ('    char labels[13][64];std::memcpy(labels,input.labels,sizeof(labels));',
         '''    const auto* hd_buttons=id==1?TrialHdPromptButtons(hd_ui):nullptr;
    char labels[13][64];std::memcpy(labels,input.labels,sizeof(labels));'''),
        ('input.direction,input.held,&width,&height);', 'input.direction,input.held,&width,&height,hd_buttons);'),
        ('            for(unsigned y=0;y<128;++y)for(unsigned x=0;x<128;++x)\n                image[(row*128+y)*128+x]=',
         '''            if(hd_buttons){
                std::memcpy(image+row*128*128,
                    hd_buttons->bgra.data()+size_t(rows[source-1])*128*128*4,128*128*4);
                continue;
            }
            for(unsigned y=0;y<128;++y)for(unsigned x=0;x<128;++x)
                image[(row*128+y)*128+x]='''),
    ]


def art_transform(source):
    if 'UC_HD_UI_PROMPTS' in source:
        baseline = art_undo(source)
        assert art_transform(baseline) == source
        return source
    for old, new in art_changes():
        source = once(source, old, new)
    return source


def art_undo(source):
    for old, new in reversed(art_changes()):
        source = once(source, new, old)
    return source


def renderer_fixture(source):
    signatures = ('bool SameTexture(', 'bool D3D12Backend::CreateTexture(',
                  'void D3D12Backend::RetireHdUi(', 'void D3D12Backend::MatchHdUi(',
                  'ID3D12Resource* D3D12Backend::BindHdUi(',
                  'ID3D12Resource* D3D12Backend::BindGuestTexture(',
                  'ID3D12Resource* D3D12Backend::BindPcPrompt(',
                  'void D3D12Backend::ReleaseOldTextures(')
    fixture = (Path(__file__).parent/'test_xbe_hd_ui_renderer.cpp').read_text()
    tex = source[source.index('struct Tex {'):source.index('\n};', source.index('struct Tex {'))+3]
    fixture = fixture.replace('/* PRODUCTION_TEX */', tex)
    return fixture.replace('/* PRODUCTION_METHODS */', '\n\n'.join(function(source, s) for s in signatures))


def gpu_fixture(source):
    signatures = ('void CopyLevelRows(', 'void D3D12Backend::UploadTextureLevel(',
                  'void D3D12Backend::RetireHdUi(', 'ID3D12Resource* D3D12Backend::BindHdUi(')
    fixture = (Path(__file__).parent/'test_xbe_hd_ui_gpu.cpp').read_text()
    return fixture.replace('/* PRODUCTION_GPU_METHODS */', '\n\n'.join(function(source, s) for s in signatures))


def prepare(area):
    area = Path(area)
    out = area/'src/compat'
    local = Path(__file__).parent
    path = out/'d3d12.cpp'
    source = path.read_text()
    transformed = renderer_transform(source)
    assert renderer_undo(transformed) == (renderer_undo(source) if MARKER in source else source)
    # Replacement cannot bypass guest render targets or alter their constants.
    for signature in ('ID3D12Resource* D3D12Backend::BindTexture(',
                      'void D3D12Backend::FillTextureConstants('):
        assert function(source, signature) == function(transformed, signature)
    path.write_text(transformed)
    art_path = out/'xbe_pc_prompt_art.cpp'
    art_path.write_text(art_transform(art_path.read_text()))
    header = out/'xbe_pc_prompts.h'
    declaration = 'std::vector<uint8_t>& pixels,unsigned& width,unsigned& height);'
    updated = 'std::vector<uint8_t>& pixels,unsigned& width,unsigned& height,bool hd_ui=false);'
    text = header.read_text()
    if updated not in text:
        text = once(text, declaration, updated)
    header.write_text(text)
    for name in ('xbe_hd_ui.h', 'xbe_hd_ui.cpp', 'xbe_hd_ui_filter.h',
                 'xbe_hd_ui_restored.h', 'xbe_hd_ui_reconstruct.h'):
        (out/name).write_text((local/name).read_text())
    prepare_artwork(out/'xbe_hd_ui_artwork.h')
    # Keep the tested runtime beside the exact artwork pinned in its generated header.
    artwork = local.parent/'artwork/hd-ui'
    runtime_artwork = area/'out-msvc/hd-ui'
    runtime_artwork.mkdir(parents=True, exist_ok=True)
    for row in json.loads((artwork/'manifest.json').read_text())['assets']:
        shutil.copy2(artwork/row['png'], runtime_artwork/row['png'])
    (out/'test_hd_ui_renderer.cpp').write_text(renderer_fixture(transformed))
    (out/'test_hd_ui_prompts.cpp').write_text((local/'test_xbe_hd_ui_prompts.cpp').read_text())
    (out/'test_hd_ui_gpu.cpp').write_text(gpu_fixture(transformed))
    (out/'test_hd_ui.cpp').write_text((local/'test_xbe_hd_ui.cpp').read_text())
    (out/'test_hd_ui_restored.cpp').write_text((local/'test_xbe_hd_ui_restored.cpp').read_text())
    (out/'test_hd_ui_reconstruct.cpp').write_text((local/'test_xbe_hd_ui_reconstruct.cpp').read_text())
    # Preserve the existing prompt fixture's original-Off expectations.
    old_test = out/'test_pc_prompts.cpp'
    stub = '\n#include "xbe_hd_ui.h"\nnamespace uc_hd_ui { const Image* Buttons(){return nullptr;} } // UC_HD_UI_OFF_FIXTURE\n'
    text = old_test.read_text()
    if 'UC_HD_UI_OFF_FIXTURE' not in text:
        text = once(text, '#include "xbe_pc_prompts.h"', '#include "xbe_pc_prompts.h"'+stub)
    old_test.write_text(text)
    return {'hd_ui': 'exact original UI payloads, separate retained GPU images',
            'unsupported': 'signed, depth, cube, volume, linear and paletted textures retain originals',
            'mips': 'complete source mip chain identified; prepared HD base exposes one mip',
            'tests': ['test_hd_ui_renderer.cpp', 'test_hd_ui_prompts.cpp', 'test_hd_ui_gpu.cpp']}

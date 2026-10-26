"""Optional exact-identity world replacements, independent of HD UI."""
from pathlib import Path
import hashlib
import struct
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
    area=Path(area)
    # A6220 reads position/UV arrays before unlocking the vertex buffer at A63CC.
    xbe=(area/'assets/Default.xbe').read_bytes()
    base=struct.unpack_from('<I',xbe,0x104)[0]
    count,table=struct.unpack_from('<II',xbe,0x11c)
    sections=[struct.unpack_from('<6I',xbe,table-base+i*56) for i in range(count)]
    section=next(s for s in sections if s[1]<=0xa6220 and 0xa6220+1249<=s[1]+s[4])
    offset=section[3]+0xa6220-section[1]
    assert hashlib.sha256(xbe[offset:offset+1249]).hexdigest()=='cf1c78d64204f977d0f23b6c601b9d44e079c2979791a19fd59630c19c02d55e'
    # The apparent vertex-buffer unlock at A63CF is the retail no-op RET.
    # There is no upload/snapshot before the final skin stream read at A665A.
    section=next(s for s in sections if s[1]<=0x11030<s[1]+s[4])
    assert xbe[section[3]+0x11030-section[1]]==0xc3
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
    for name in ['xbe_world_textures.h','xbe_ledge_uv.h','xbe_ledge_uv_data.h','xbe_arm_mesh.h','xbe_arm_mesh_data.h','test_xbe_world_textures.cpp']:(out/name).write_text(Path(__file__).with_name(name).read_text())
    p=area/'generated/game/x86_recomp.17.cpp';s=p.read_text()
    include='#include "kernel/memory.h"\n#include "xbe_input_memory.h"\n#include "xbe_ledge_uv.h"\n'
    arm_include='#include "xbe_arm_mesh.h"\n'
    s=s.replace(arm_include,'')
    # Remove the previous positions-only hook from incremental generated builds.
    s=s.replace('''\tif (TrialGuestReadable(ctx.esi,0x20)&&TrialGuestReadable(ctx.esp,0x24)&&X86_MEM32(ctx.esi+0x1c)==17) {
\t\tconst uint32_t count=X86_MEM32(ctx.esi+8), positions=X86_MEM32(ctx.esp+0x20);
\t\tif ((count==2387||count==1614)&&TrialGuestReadable(positions,count*10))
\t\t\tuc_arm_mesh::Apply({base+positions,size_t(count)*10});
\t}
''','')
    arm_hook='''\tif (TrialGuestReadable(ctx.esi,0x20)&&TrialGuestReadable(ctx.esp,0x24)&&X86_MEM32(ctx.esi+0x1c)==17) {
\t\tconst uint32_t count=X86_MEM32(ctx.esi+8), positions=X86_MEM32(ctx.esp+0x20), uv=X86_MEM32(ctx.esp+0x10);
\t\tif ((count==2387||count==1614)&&TrialGuestReadable(positions,count*10)&&TrialGuestReadable(uv,count*8))
\t\t\tuc_arm_mesh::Apply({base+positions,size_t(count)*10},{base+uv,size_t(count)*8});
\t}
'''
    s=s.replace(arm_hook,'')
    # Retain the locked position/UV destinations across A6220's later temporary
    # index allocations. Commit only after the companion skin stream is read.
    arm_locals='\tuint32_t trial_arm_positions=0,trial_arm_uv=0,trial_arm_count=0;\n'
    capture='''\tif (TrialGuestReadable(ctx.esi,0x20)&&TrialGuestReadable(ctx.esp,0x24)&&X86_MEM32(ctx.esi+0x1c)==17) {
\t\ttrial_arm_count=X86_MEM32(ctx.esi+8);
\t\tif (trial_arm_count==1614||trial_arm_count==2387||trial_arm_count==3103) {
\t\t\ttrial_arm_positions=X86_MEM32(ctx.esp+0x20);trial_arm_uv=X86_MEM32(ctx.esp+0x10);
\t\t} else trial_arm_count=0;
\t}
'''
    skin_read='\t{ uint32_t t = X86_MEM32((uint32_t)(ctx.edx + 0x4u)); X86_PUSH32(0xA665Du); x86_icall(ctx, base, t); }'
    corrected_read='''\t{
\t\tconst uint32_t trial_arm_skin=TrialGuestReadable(ctx.esp,8)?X86_MEM32(ctx.esp):0;
\t\tuint32_t t = X86_MEM32((uint32_t)(ctx.edx + 0x4u)); X86_PUSH32(0xA665Du); x86_icall(ctx, base, t);
\t\tif (trial_arm_count&&TrialGuestReadable(trial_arm_positions,trial_arm_count*10)&&
\t\t\tTrialGuestReadable(trial_arm_uv,trial_arm_count*8)&&TrialGuestReadable(trial_arm_skin,trial_arm_count*8))
\t\t\tuc_arm_mesh::Apply({base+trial_arm_positions,size_t(trial_arm_count)*10},
\t\t\t\t{base+trial_arm_uv,size_t(trial_arm_count)*8},{base+trial_arm_skin,size_t(trial_arm_count)*8});
\t}
'''.rstrip('\n')
    s=s.replace(arm_locals,'').replace(capture,'')
    local_anchor='\t// 000A6220 mov eax, fs:[0x00000000]'
    # The lift also has an overlapping A64CD tail. Only the complete loader owns
    # captured stream pointers; never inject locals into the independent tail.
    begin=s.index('X86_FUNC(__imp__sub_000A6220) {')
    end=s.index('X86_WEAK_FUNC(sub_000A6220, __imp__sub_000A6220)',begin)
    body=s[begin:end].replace(corrected_read,skin_read)
    assert body.count(local_anchor)==1 and body.count(skin_read)==1
    body=body.replace(local_anchor,arm_locals+local_anchor,1).replace(skin_read,corrected_read,1)
    s=s[:begin]+body+s[end:]
    hook='''	// Repair exact static ledges before the original vertex-buffer unlock.
	if (TrialGuestReadable(ctx.esi,0x20)&&TrialGuestReadable(ctx.esp,0x20)) {
		const uint32_t count=X86_MEM32(ctx.esi+8), flags=X86_MEM32(ctx.esi+0x1c);
		if (count&&count<=1000000&&!(flags&3u)) {
			const uint32_t positions=X86_MEM32(ctx.esp+0x1c), uv=X86_MEM32(ctx.esp+0x10);
			if (TrialGuestReadable(positions,count*16)&&TrialGuestReadable(uv,count*8))
				uc_ledge_uv::Apply({base+positions,size_t(count)*16},{base+uv,size_t(count)*8});
		}
	}
'''
    s=s.replace(include,'').replace(hook,'')
    anchor='\t// 000A63CC mov ecx, [esi+0x20]'
    assert s.count(anchor)==1
    s=s.replace('#include "x86_recomp_shared.h"\n','#include "x86_recomp_shared.h"\n'+include+arm_include,1).replace(anchor,hook+capture+anchor,1)
    p.write_text(s)

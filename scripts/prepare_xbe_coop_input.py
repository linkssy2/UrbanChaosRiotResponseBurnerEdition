"""Keep the retail presence/action readers; admit two local input slots only in co-op."""
from pathlib import Path
import hashlib
import re
import struct

ROOT=Path(__file__).resolve().parents[1]
PINS={
    0x4c3f0:(6,529,'fe3fd4ab16ab596ba6980e27cd3498562ed16fceaf0eca5722f582684184d340','7ca677d139b6d9e474e1c0b5ef38d54168952fcbcdfa8cea34362f1823c8372e'),
    0x4c610:(6,438,'79bb2f32bf8f9c3331a2acd40efa74e692b892f8ba011d581fcb5be630d5cdee','2ecf0e6af3e6ed7083357e912231154ac7c1867b8887c9a27579d62dd644eb46'),
    0x2670f0:(64,109,'33205ff4b707579f055927e06491963510d1bdff3829d3367a4a95fd47b627f5','743ce7bcdf345744f57d6a73b37436a072e4e03fd39828f80a1ac69e6271b962'),
    0x267160:(64,64,'a8aa0f092c3a6eb9f0d031170075c24e84d79deb73290b04f795ac5e280b61f3','4640bc55f6ac78194c60b95ac3bbf017199f8c78e863820eda51e9160356654e'),
    0x5aa00:(8,168,'adde90554e7a7406dc81bbc26dadeab58d84c2c627d3b74875847521c74eef9b','812ba135c397b30d0358bcaa144bc55aaced844d74000aab149fdd5ce19acb93'),
    0x567f0:(7,1239,'1d3c96186a22976bcfbc3b0033734b07ae3a051f3827ef14232f6cdb7cac4ded','bed8e6ad254b926ad01f7b1fab6af1d54baa358722a05b54aaa8bfc859802bdc'),
    0x55bf0:(7,224,'b94a281c6a4f46bcbd0098c5d9cdac188d8e53de3ccb26c0bff33118eea65f09','b8bb2abaf5d36175fd22a304a8176848a92a159b26626c44e61618576e5ea585'),
    0x55cd0:(7,197,'72c3b9e71e47ef97ceeb803c290452b1cf0cbe12942a699e6d7dc85923c49d14','949efca5d9a13c234133ebfac19a2de0240e57d8abc17bf6ee22083615b56ffb'),
}
INCLUDE='#include "xbe_coop_guest_input.h"\n'
HOOK='\ttrial_coop::UpdateGuestInputMapping(base);\n'
ENTRIES={0x5aa00:HOOK,0x4c3f0:'\ttrial_coop::PauseFrontendBegin();\n',
    0x4c610:'\ttrial_coop::PauseConsumeScope coop_pause_consume(ctx,base);\n',
    0x2670f0:'\ttrial_coop::PauseMenuEnter(ctx,base);\n',
    0x267160:'\ttrial_coop::PauseMenuExitScope coop_pause_exit(ctx,base);\n'}
SAMPLE='\ttrial_coop::PauseFrontendSample(ctx,base);\n'
PLAYERS=('0004C41B','0004C45A','0004C46F')
APP_INCLUDE='#include "xbe_coop_pause.h"\n'
HOST_DECL='extern void TrialCoopPauseImGuiInput();\n'
HOST_CALL='    TrialCoopPauseImGuiInput();\n'
APP_GATE='    if (trial_coop::PauseBlocksMessage(msg)) return msg == WM_INPUT ? DefWindowProc(hwnd, msg, wp, lp) : 0;\n'

def clean(source):
    source=source.replace(INCLUDE,'').replace(SAMPLE,'')
    for line in ENTRIES.values():source=source.replace(line,'')
    for address in PLAYERS:
        old=f'// {address} push 0x00\n\t{{ uint32_t v = (uint32_t)(trial_coop::PauseInputPlayer()); X86_PUSH32(v); }}'
        source=source.replace(old,old.replace('trial_coop::PauseInputPlayer()','0x0u'))
    return source

def patch(body,address):
    if address in ENTRIES:body=body.replace('{\n','{\n'+ENTRIES[address],1)
    if address==0x4c3f0:
        for instruction in PLAYERS:
            old=f'// {instruction} push 0x00\n\t{{ uint32_t v = (uint32_t)(0x0u); X86_PUSH32(v); }}'
            assert body.count(old)==1
            body=body.replace(old,old.replace('0x0u','trial_coop::PauseInputPlayer()'))
        assert body.count('\t// 0004C420 fld')==1
        body=body.replace('\t// 0004C420 fld',SAMPLE+'\t// 0004C420 fld')
    return body

def unprepare(area):
    area=Path(area)
    for unit in {pin[0]for pin in PINS.values()}:
        path=area/f'generated/game/x86_recomp.{unit}.cpp'
        if path.exists():
            text=path.read_text();original=clean(text)
            if text!=original:path.write_text(original)
    main=area/'src/compat/main.cpp'
    if main.exists():
        text=main.read_text();original=text.replace(APP_INCLUDE,'').replace(APP_GATE,'').replace(HOST_DECL,'').replace(HOST_CALL,'')
        if text!=original:main.write_text(original)
    renderer=area/'src/compat/d3d12.cpp'
    if renderer.exists():
        text=renderer.read_text();original=text.replace(HOST_DECL,'').replace(HOST_CALL,'')
        if text!=original:renderer.write_text(original)

def prepare_host(area):
    area=Path(area)
    main=area/'src/compat/main.cpp'
    text=main.read_text().replace(APP_INCLUDE,'').replace(APP_GATE,'').replace(HOST_DECL,'').replace(HOST_CALL,'')
    entry='LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {\n'
    assert text.count(entry)==1
    text=APP_INCLUDE+HOST_DECL+text.replace(entry,entry+HOST_CALL+APP_GATE,1)
    assert text.index(HOST_CALL)<text.index(APP_GATE)<text.index('ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp)')
    main.write_text(text)
    renderer=area/'src/compat/d3d12.cpp'
    text=renderer.read_text().replace(HOST_DECL,'').replace(HOST_CALL,'')
    entry='    std::lock_guard lock(app::UiLock());\n    ImGui_ImplDX12_NewFrame();'
    assert text.count(entry)==1
    text=HOST_DECL+text.replace(entry,entry.replace('\n    ImGui_ImplDX12', '\n'+HOST_CALL+'    ImGui_ImplDX12'),1)
    renderer.write_text(text)
    source=(ROOT/'scripts/xbe_trial_menu_input.cpp').read_text()
    start=source.index('void TrialCoopPauseImGuiInput(){');end=source.index('\n}',start)+2
    fixture=(ROOT/'scripts/test_xbe_pause_imgui.cpp').read_text()
    assert fixture.count('// HOST_IMGUI_HELPER')==1
    (area/'src/compat/test_pause_imgui.cpp').write_text(fixture.replace('// HOST_IMGUI_HELPER',source[start:end]))

def prepare(area):
    area=Path(area)
    image=(area/'assets/Default.xbe').read_bytes()
    base=struct.unpack_from('<I',image,0x104)[0]
    count,table=struct.unpack_from('<II',image,0x11c)
    sections=[struct.unpack_from('<6I',image,table-base+i*56)for i in range(count)]
    def raw(address,size):
        section=next(s for s in sections if s[1]<=address and address+size<=s[1]+s[4])
        start=section[3]+address-section[1]
        return image[start:start+size]
    paths={unit:area/f'generated/game/x86_recomp.{unit}.cpp' for unit,_,_,_ in PINS.values()}
    originals={unit:clean(path.read_text())for unit,path in paths.items()}
    bodies={}
    for address,(unit,size,raw_hash,body_hash)in PINS.items():
        assert hashlib.sha256(raw(address,size)).hexdigest()==raw_hash,hex(address)
        body=re.search(rf'X86_FUNC\(__imp__sub_{address:08X}\) \{{.*?^\}}',originals[unit],re.S|re.M)[0]
        assert hashlib.sha256(body.encode()).hexdigest()==body_hash,hex(address)
        bodies[address]=body
    staged=dict(originals)
    for address,body in bodies.items():
        if address in ENTRIES:staged[PINS[address][0]]=staged[PINS[address][0]].replace(body,patch(body,address),1)
    for unit in {PINS[address][0]for address in ENTRIES}:
        staged[unit]=staged[unit].replace('#include "x86_recomp_shared.h"\n','#include "x86_recomp_shared.h"\n'+INCLUDE,1)
    common=area/'src/common';compat=area/'src/compat'
    common.mkdir(parents=True,exist_ok=True);compat.mkdir(parents=True,exist_ok=True)
    for name in ('xbe_coop.h','xbe_coop_input.h','xbe_coop_guest_input.h','xbe_coop_pause.h','xbe_input_memory.h'):
        (common/name).write_text((ROOT/'scripts'/name).read_text())
    leaves=[body.replace('X86_FUNC(__imp__sub_','X86_FUNC(sub_',1) for address,body in bodies.items()if address not in ENTRIES]
    declarations='\n'.join(f'X86_EXTERN_FUNC(sub_{address:08X});'for address in bodies if address not in ENTRIES)
    fixture=(ROOT/'scripts/test_xbe_coop_input.cpp').read_text()
    names={0x5aa00:'Mapping',0x4c3f0:'Frontend',0x4c610:'Consume',0x2670f0:'Enter',0x267160:'Exit'}
    code=declarations+'\n'+'\n'.join(leaves)+'\n'
    for address,name in names.items():
        code+=bodies[address].replace(f'__imp__sub_{address:08X}','Original'+name)+'\n'
        code+=patch(bodies[address],address).replace(f'__imp__sub_{address:08X}','Candidate'+name)+'\n'
    assert fixture.count('// RETAIL_INPUT_FUNCTIONS')==1
    fixture=fixture.replace('// RETAIL_INPUT_FUNCTIONS',code)
    constants=[]
    for address,size in ((0x44c174,4),(0x44bf94,4),(0x44c538,4),(0x44c5f0,4),(0x55d98,64),(0x56cc8,80)):
        data=raw(address,size)
        constants.append('{const uint8_t data[]={'+','.join(str(v)for v in data)+f'}};std::memcpy(g_base+0x{address:x},data,sizeof(data));}}')
    assert fixture.count('// RETAIL_INPUT_CONSTANTS')==1
    (compat/'test_coop_input.cpp').write_text(fixture.replace('// RETAIL_INPUT_CONSTANTS','\n'.join(constants)))
    for unit,text in staged.items():
        if paths[unit].read_text()!=text:paths[unit].write_text(text)

if __name__=='__main__':prepare(ROOT/'build/xbe-full-integration-20261014')

"""Reuse accepted CONTROL prompt artwork at the new texture owner and guest gates.

Only generated private files change. Original source textures are identified by
content, decoded from the user's game, and never copied into the release.
"""
from pathlib import Path
import hashlib
import re
import struct
import zlib
import sys

ROOT=Path(__file__).resolve().parents[1]

def once(s,old,new):
    assert s.count(old)==1,old
    return s.replace(old,new,1)

INPUT=r'''
// A renderer read must never consume or advance the original command gesture.
bool TrialPromptSnapshot(unsigned id,TrialPromptInput& out,unsigned variant){
    std::lock_guard guard(input_lock);out={};
    if(!xbox_KeyboardMode()){
        if(id!=1)return false;
        controller_init();
        out.family=trial_controller::family.load();
        const bool remap=gameplay&&!menu_mode&&InterlockedCompareExchange(&controller_custom,0,0);
        if(!out.family&&!remap)return false;
        bool changed=out.family!=0;out.controller=true;
        for(unsigned action=0;action<12;++action){
            unsigned source=remap?xbox_ControllerBinding(action,0):action+1;
            if(remap&&!source)source=xbox_ControllerBinding(action,1);
            out.bindings[action]=source;
            std::snprintf(out.labels[action],sizeof(out.labels[action]),"%s",trial_controller::Label(source,out.family));
            changed|=source!=action+1;
        }
        return changed;
    }
    unsigned first=id==1?0:12,last=id==1?12:13;
    for(unsigned i=first;i<last;++i){unsigned action=i==12?24:i;
        out.bindings[i]=xbox_KeyboardBinding(action,0);
        if(!out.bindings[i])out.bindings[i]=xbox_KeyboardBinding(action,1);
        xbox_KeyboardActionLabel(action,out.labels[i],sizeof(out.labels[i]));
        if(action==8&&gameplay&&!menu_mode){
            out.bindings[i]=VK_ESCAPE;std::strcpy(out.labels[i],"Esc");
        }
        if(action==1&&menu_mode){
            out.bindings[i]=VK_RBUTTON;std::strcpy(out.labels[i],"Mouse Right");
        }
        if(action==0&&variant==1){
            // Interaction uses the configured secondary A binding (default F).
            unsigned secondary=xbox_KeyboardBinding(action,1);
            if(secondary){out.bindings[i]=secondary;
                std::snprintf(out.labels[i],sizeof(out.labels[i]),"%s",xbox_KeyboardSourceName(secondary));}
        }
    }
    out.held=id==2&&order_held&&captured&&focused()&&!TrialGraphicsOpen();
    if(out.held){out.x=order_x/2*2;out.y=order_y/2*2;out.direction=OrderDirection();}
    return true;
}
'''

METHOD=r'''
ID3D12Resource* D3D12Backend::BindPcPrompt(unsigned id,unsigned variant,const TextureDesc& original,DrawTextures& stages,int stage) {
    TrialPromptInput input;
    if(id!=1||variant>1)variant=0;
    if(!TrialPromptSnapshot(id,input,variant))return nullptr;
    auto& slot=pc_prompts_[input.controller?3:variant?variant+1:id-1];
    const ULONGLONG now=GetTickCount64();
    bool changed=!slot.texture.res||!uc_pc_prompts::Equal(slot.input,input);
    // Same bounded refresh as CONTROL; release/press transitions are immediate.
    if(changed&&(!slot.texture.res||now-slot.updated>=33||input.held!=slot.input.held)){
        std::vector<uint8_t> pixels;unsigned width=0,height=0;
        const auto* raw=static_cast<const uint8_t*>(kernel::HostPtr(original.data));
        if(!TrialPromptArtwork(id,raw,input,pixels,width,height))return nullptr;
        if(!slot.texture.res){
            auto& tx=slot.texture;tx.fmt=DXGI_FORMAT_B8G8R8A8_UNORM;tx.levels=1;
            tx.d.width=width;tx.d.height=height;
            D3D12_HEAP_PROPERTIES heap{D3D12_HEAP_TYPE_DEFAULT};
            auto desc=Texture2DDesc(tx.fmt,width,height);
            if(FAILED(dev_->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,
                D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&tx.res))))return nullptr;
            slot.state=D3D12_RESOURCE_STATE_COPY_DEST;
        }
        Transition(slot.texture.res.Get(),slot.state,D3D12_RESOURCE_STATE_COPY_DEST);
        UploadTextureLevel(slot.texture,0,0,pixels,width*4);
        Transition(slot.texture.res.Get(),slot.state,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        slot.input=input;slot.updated=now;
    }
    stages.sv[stage].Format=DXGI_FORMAT_B8G8R8A8_UNORM;
    stages.sv[stage].Texture2D.MipLevels=1;
    return slot.texture.res.Get();
}
'''

ART_TAIL=r'''
bool TrialPromptArtwork(unsigned id,const uint8_t* source,const TrialPromptInput& input,
                        std::vector<uint8_t>& pixels,unsigned& width,unsigned& height){
    if(id<1||id>2)return false;
    if(id==1&&!pc_hud_supplied){
        if(!source)return false;
        uc_pc_prompts::DecodeButtons(source,pc_hud_buttons_pixels);pc_hud_supplied=true;
    }
    char labels[13][64];std::memcpy(labels,input.labels,sizeof(labels));
    // A physical controller source number is not a keyboard virtual-key code.
    // Unknown button artwork (directions/unbound) gets its actual source name
    // inside the existing graphical badge, never an unrelated mouse picture.
    unsigned badge_bindings[13]{};
    uint32_t* image=pc_hud_pixels(id,input.controller?badge_bindings:input.bindings,labels,input.x,input.y,input.direction,input.held,&width,&height);
    if(!image)return false;
    if(id==1&&input.controller){
        // Inverse of the verified original atlas's pc_hud_actions row order.
        // Copy from the immutable decoded source so swaps cannot alias.
        static constexpr unsigned rows[12]={1,4,3,2,7,9,8,6,11,10,15,16};
        for(unsigned row=0;row<32;++row){
            const int action=pc_hud_actions[row];if(action<0)continue;
            const unsigned source=input.bindings[action];if(!source||source>12)continue;
            if(input.family){
                TrialControllerGlyph(image+row*128*128,source,input.family);
                continue;
            }
            for(unsigned y=0;y<128;++y)for(unsigned x=0;x<128;++x)
                image[(row*128+y)*128+x]=pc_hud_buttons_pixels[(rows[source-1]*32+y/4)*32+x/4];
        }
    }
    pixels.assign(reinterpret_cast<uint8_t*>(image),reinterpret_cast<uint8_t*>(image)+size_t(width)*height*4);
    free(image);return true;
}
'''

def prepare(input_source,graphics_source,area):
    area=Path(area);out=area/'src/compat';local=Path(__file__).parent
    (out/'xbe_pc_prompts.h').write_text((local/'xbe_pc_prompts.h').read_text())
    for name in ('xbe_prompt_context.h','xbe_prompt_context.cpp'):
        (area/'src/common'/name).write_text((local/name).read_text())
    state_path=area/'src/gpu/state.h';state=state_path.read_text()
    field='    unsigned uc_prompt_variant = 0; // original queued text identity\n'
    # StateRanges starts at simple, so metadata before that member is never
    # transmitted to the renderer. Keep this POD in its material-to-end range,
    # alongside fog metadata. Remove the old placement on repeated preparation.
    state=state.replace(field,'')
    state=once(state,'    uint32_t light_enable = 0;                         // bit per light\n',
                     '    uint32_t light_enable = 0;                         // bit per light\n'+field)
    state_path.write_text(state)
    art=(ROOT/'third_party/xboxrecomp/src/nv2a/nv2a_pc_hud_art.h').read_text()
    art=once(art,'#include "../../include/host_controller.h"','')
    art=once(art,'#include "nv2a_pc_hud_assets.h"','''#include <cstdio>
#include <cstdlib>
#include "xbe_pc_prompts.h"
static uint32_t pc_hud_buttons_pixels[32*1024];
static bool pc_hud_supplied=false;
static int pc_hud_load_pixels(){return pc_hud_supplied;}''')
    art=once(art,'uint32_t*copy=malloc((size_t)*w**h*4);','uint32_t*copy=static_cast<uint32_t*>(malloc((size_t)*w**h*4));')
    # Reuse the accepted standalone keycap rasterizer for native menu hints.
    keycap=(ROOT/'native/runtime/hud_keys.c').read_text()
    keycap=keycap[keycap.index('int native_hud_keycap('):]
    (out/'xbe_pc_prompt_art.cpp').write_text(art+ART_TAIL+'\n'+keycap)
    input_source=once(input_source,'#include "core/input.h"','#include "core/input.h"\n#include "xbe_pc_prompts.h"')
    at=input_source.index('void xbox_KeyboardSourceEvent(')
    input_source=input_source[:at]+INPUT+'\n'+input_source[at:]
    old='    if(pad.buttons||std::abs(int(pad.rx))>8689||std::abs(int(pad.ry))>8689)InterlockedExchange(&keyboard_mode,0);'
    input_source=once(input_source,old,'')
    input_source=once(input_source,'    if(port==0)pad.connected=true;',
        '''    if(port==0)pad.connected=true;
    if(port==0&&focused()&&uc_pc_prompts::ControllerActive(pad)&&InterlockedExchange(&keyboard_mode,0))PostMessage(game_window,kCapture,0,0);''')
    input_source=once(input_source,'    if(TrialGraphicsMessage(msg,wp,lp)){', '''    static uc_pc_prompts::PointerActivity pointer_activity;
    auto note_pointer_input=[&]{
        const bool changed=!xbox_KeyboardMode();xbox_KeyboardNoteInput();
        if(changed)PostMessage(game_window,kCapture,0,0);
    };
    if(focused()&&(msg==WM_KEYDOWN||msg==WM_SYSKEYDOWN))note_pointer_input();
    if(!focused()||captured)pointer_activity.Reset();
    else if(msg==WM_MOUSEMOVE){
        if(pointer_activity.Move(short(LOWORD(lp)),short(HIWORD(lp))))note_pointer_input();
    }else if(msg==WM_LBUTTONDOWN||msg==WM_RBUTTONDOWN||msg==WM_MBUTTONDOWN||
             msg==WM_XBUTTONDOWN||msg==WM_MOUSEWHEEL||msg==WM_MOUSEHWHEEL)note_pointer_input();
    if(TrialGraphicsMessage(msg,wp,lp)){''')
    graphics_source=once(graphics_source,'#include "gpu/texture.h"','#include "gpu/texture.h"\n#include "xbe_pc_prompts.h"')
    graphics_source=once(graphics_source,'    uint32_t levels = 1;\n', '    uint32_t levels = 1;\n    unsigned pc_prompt_id = 0;\n')
    graphics_source=once(graphics_source,'    ID3D12Resource* BindTexture(const State& s, int stage, DrawTextures& stages);',
        '    ID3D12Resource* BindTexture(const State& s, int stage, DrawTextures& stages);\n'
        '    ID3D12Resource* BindPcPrompt(unsigned,unsigned,const TextureDesc&,DrawTextures&,int);')
    field='''    struct PcPromptSlot { Tex texture;TrialPromptInput input;ULONGLONG updated=0;
        D3D12_RESOURCE_STATES state=D3D12_RESOURCE_STATE_COPY_DEST; };
    PcPromptSlot pc_prompts_[4];
'''
    graphics_source=once(graphics_source,'    std::unordered_map<uc_xbe_signed::TextureKey, Tex, uc_xbe_signed::TextureKeyHash> textures_;',
        field+'    std::unordered_map<uc_xbe_signed::TextureKey, Tex, uc_xbe_signed::TextureKeyHash> textures_;')
    graphics_source=once(graphics_source,'            tx.sig = sig;\n            UploadTexture(',
        '            tx.sig = sig;\n            tx.pc_prompt_id=uc_pc_prompts::Match(d,p,total);\n            UploadTexture(')
    graphics_source=once(graphics_source,'    FillTextureView(tx, d, stages.sv[stage]);',
        '''    FillTextureView(tx, d, stages.sv[stage]);
    if(tx.pc_prompt_id)
        if(auto* replacement=BindPcPrompt(tx.pc_prompt_id,s.uc_prompt_variant,d,stages,stage))return replacement;''')
    graphics_source=once(graphics_source,'ID3D12Resource* D3D12Backend::BindGuestTexture(',METHOD+'\nID3D12Resource* D3D12Backend::BindGuestTexture(')
    stage_tests(area,art)
    from prepare_xbe_prompt_handoff import prepare as prepare_handoff
    prepare_handoff(ROOT/'build/upstream-review-xboxxbe-20261014',area)
    return input_source,graphics_source

def strip_guest(source):
    source=re.sub(r'\t// uc_pc_prompt_gate: accepted CONTROL visibility boundary\n\tif \([^\n]+\) goto loc_[0-9A-F]+;\n','',source)
    source=re.sub(r'loc_[0-9A-F]+: // uc_pc_prompt_label\n','',source)
    return source.replace('\nextern int xbox_KeyboardMode(); // uc_pc_prompt_decl','')

def transform_guest(source):
    # Duplicated translated interior entries contain the same retail gate; patch
    # each body rather than leaving alternate dispatch entries on the old path.
    source=strip_guest(source);counts=[0,0]
    def body(m):
        text=m[0]
        for i,(start,target,condition) in enumerate((
            ('00075458','000754BC','X86_MEM32(ctx.ebx)>0'),
            ('00158D1B','00158D24','xbox_KeyboardMode()'))):
            marker='\t// '+start+' '
            if marker not in text:continue
            destination='\t// '+target+' '
            assert text.count(marker)==text.count(destination)==1
            if 'loc_'+target+':' not in text:
                text=text.replace(destination,'loc_'+target+': // uc_pc_prompt_label\n'+destination,1)
            text=text.replace(marker,f'\t// uc_pc_prompt_gate: accepted CONTROL visibility boundary\n\tif ({condition}) goto loc_{target};\n'+marker,1)
            counts[i]+=1
        return text
    result=re.sub(r'X86_FUNC\(__imp__sub_[0-9A-F]+\) \{.*?^\}',body,source,flags=re.S|re.M)
    if result!=source:
        result=once(result,'#include "x86_recomp_shared.h"','#include "x86_recomp_shared.h"\nextern int xbox_KeyboardMode(); // uc_pc_prompt_decl')
    return result,counts

def prepare_guest(area):
    area=Path(area)
    data=(area/'assets/Default.xbe').read_bytes()
    base,=struct.unpack_from('<I',data,0x104);n,table=struct.unpack_from('<II',data,0x11c)
    sections=[struct.unpack_from('<6I',data,table-base+i*56) for i in range(n)]
    def read(va,size):
        s=next(s for s in sections if s[1]<=va and va+size<=s[1]+s[4]);at=s[3]+va-s[1]
        return data[at:at+size]
    assert read(0x158D11,0x19).hex()=='8a86d500000084c0740d83bed8000000ff7504b001eb0232c0'
    assert read(0x75458,5).hex()=='6aff6a00e8'
    counts=[0,0]
    for name in ('x86_recomp.11.cpp','x86_recomp.34.cpp'):
        path=area/'generated/game'/name;s=path.read_text()
        changed,n=transform_guest(s);counts=[a+b for a,b in zip(counts,n)]
        assert transform_guest(changed)==(changed,n),'Non-idempotent prompt gate'
        assert strip_guest(changed)==strip_guest(s),'Unrelated generated source changed'
        path.write_text(changed)
    assert counts==[2,4],counts
    prepare_prompt_context(area,read)

CONTEXT_HOOKS={
    1:[('00018CE0','TrialPromptTextScope uc_prompt_text(base,X86_MEM32(ctx.esp+4),X86_MEM32(ctx.esp+16),X86_MEM32(ctx.esp),ctx.edi);')],
    16:[('0009DA4B','TrialPromptQueueRecord(base,ctx.ecx,ctx.esi);'),
        ('0009DB30','TrialPromptQueueReset(ctx.ecx);'),
        ('0009D860','TrialPromptQueueReset(ctx.ecx);'),
        ('0009DB90','TrialPromptDrawScope uc_prompt_draw(base,ctx.ecx);')],
}

def transform_prompt_context(source,chunk):
    declaration='#include "../../src/common/xbe_prompt_context.h" // uc_prompt_context'
    source=re.sub(r'^.*// uc_prompt_context\n','',source,flags=re.M)
    for address,hook in CONTEXT_HOOKS[chunk]:
        line='\t'+hook+' // uc_prompt_context\n'
        # Marker lines were removed above. Removing this line again would
        # erase the earlier9DB30 reset when installing the identical9D860 hook.
        marker='\t// '+address+' '
        assert source.count(marker)==1,address
        source=source.replace(marker,line+marker,1)
    if chunk==16:
        assert source.count('TrialPromptQueueReset(ctx.ecx); // uc_prompt_context')==2
    return once(source,'#include "x86_recomp_shared.h"\n','#include "x86_recomp_shared.h"\n'+declaration+'\n')

def prepare_prompt_context(area,read):
    # Full retail localized draw, accepted quad enqueue, queue reset/destructor,
    # individual immediate consumer and both vtables. Guest ABI stays unchanged.
    for address,size,digest in (
        (0x18ce0,250,'8126a311d1f3da600487db6b319cec1be5d22a4aead4c733b43dc6d9e3d63836'),
        (0x9d9b0,242,'26966356054def9572634c6237ec90e69eaac615c3ec40db4f7a8cfe4801d4c7'),
        (0x9db30,81,'1533908908e9d39e4f61f1448f947463a8fde87574ed43170ff5012675c559bf'),
        (0x9db90,1048,'e0d9502bef18e3e7c3537c354f34c4084215763adb44902ec99fbcd157b8232c'),
        (0x9d860,155,'a10f04a4b7ede9355acc2174ed4a113666f4e3ea83d0f776471fc2f769e49677'),
        (0x9e257,44,'2b8ded20a829a5514926a23c837b1d409968d3d3cf5403b5dd7d7d12cdf594b9'),
        (0x273410,379,'7af30456e1fbba4180561d068fe672b32e9ba7d98d08aab4563a3ff507a34834'),
        (0x273590,147,'afb6022af2d7e27d2cc98e28ca73244a92292bb4b739c036606c93d7ef3fa404'),
        (0x273860,281,'5e6001ceef33b5965ba6b0459efbd1ab10b54fcbf8d3bd60863516f5ee1ded72'),
        (0x115310,11,'53c3ff26fafaf07128084a73da42f2e0b25c29fe98dc71ba41a40a18d4f0a888'),
        (0x1ac8f4,17,'a970a425073123d14a15814b065d1c5fd9e7f4b69ad6bd720a6d06e1e4f3a000'),
        (0x2548d8,190,'ee5240c210616c29b8bd87e3f594cef4af523f02309030eb088c03af5cee380d'),
        (0x26df19,7,'65bb2026aee4851549048beee2f4f05147e8256f406ec4ca99eda18ff5440b92'),
        (0x26ea3f,10,'b006e7c628b0bddf40ac840e5e7e035183450aad3c391b4f7a18c1e90bb1185f'),
        (0x4847e8,4,'26b25d457597a7b0463f9620f666dd10aa2c4373a505967c7c8d70922a2d6ece'),
        (0x485c58,24,'c31b58364ef09a42787e11bc6652fb8f3373847a3362092d78703aef4dd06ae3'),
        (0x456b44,12,'fff9bd6d1a4395a6cc6ea236429ae00aa1718dd73d1ee4e615c53e7e49f038cf'),
        (0x456b50,12,'5b0645101bac845e826f7a9e22a356cb1cc939b336057750e66bd21d8a31996a'),
        (0x456bb8,24,'6456b22916a89bd92a83cc752df1117abfed8025cbfd1120edfd3399c5ba0e3f')):
        assert hashlib.sha256(read(address,size)).hexdigest()==digest,hex(address)
    for chunk in CONTEXT_HOOKS:
        path=area/f'generated/game/x86_recomp.{chunk}.cpp';source=path.read_text()
        result=transform_prompt_context(source,chunk)
        assert transform_prompt_context(result,chunk)==result
        # Exact removal proves only the reviewed host metadata hooks changed.
        def strip(s):return re.sub(r'^.*// uc_prompt_context\n','',s,flags=re.M)
        assert strip(result)==strip(source)
        path.write_text(result)

def stage_tests(area,art):
    local=Path(__file__).parent
    fixture=(local/'test_xbe_pc_prompts.cpp').read_text()
    out=area/'src/compat'
    # Private test inputs only; the production path reads the live user texture.
    raw=(area/'assets/PersistantFilesXBox.lst').read_bytes();at=200;payload=None
    end=196+struct.unpack_from('<I',raw,192)[0]
    for _ in range(struct.unpack_from('<I',raw,196)[0]):
        n=struct.unpack_from('<H',raw,at)[0];at+=2;name=raw[at:at+n].rstrip(b'\0');at+=n
        offset,size,packed,bank=struct.unpack_from('<4I',raw,at);at+=16
        assert at<=end and offset+packed<=len(raw)
        if name==b'font\\font_buttons.tga':
            payload=zlib.decompress(raw[offset:offset+packed]);assert len(payload)==size
    assert payload and struct.unpack_from('<3I',payload)==(2,32,1024)
    sys.path.insert(0,str(ROOT/'launcher/burner'))
    from hud_import import decode,PIXEL_SHA
    buttons=payload[28:28+32768];pixels=decode(buttons)
    assert hashlib.sha256(pixels).hexdigest()==PIXEL_SHA
    (out/'test_pc_prompts_source.bin').write_bytes(buttons)
    (out/'test_pc_prompts_pixels.bin').write_bytes(pixels)
    # The orders pad belongs to the level archive, not PersistantFilesXBox.
    # Read only its directory and one compressed entry, never the whole level.
    orders=None
    with (area/'assets/1-01.ent').open('rb') as file:
        header=file.read(200);count=struct.unpack_from('<I',header,196)[0]
        assert count<100000
        for _ in range(count):
            n,=struct.unpack('<H',file.read(2));name=file.read(n).rstrip(b'\0')
            offset,size,packed,bank=struct.unpack('<4I',file.read(16))
            if name==b'hud\\hud_orders_pad_xbox.tga=nomipmap':
                assert size==4124;file.seek(offset);payload=zlib.decompress(file.read(packed))
                assert len(payload)==size and struct.unpack_from('<3I',payload)==(2,64,64)
                orders=payload[28:];break
    assert orders and hashlib.sha256(orders).hexdigest()=='fc5540f5ea1823c7fb81e94079f8b6119a8380ce25fa45eb851c574bcbe07a8b'
    (out/'test_pc_prompts_orders.bin').write_bytes(orders)
    fixture=fixture.replace('/* INPUT_IMPLEMENTATION */',INPUT)
    keyboard=(ROOT/'third_party/xboxrecomp/src/input/host_keyboard_config.h').read_text()
    begin=keyboard.index('static void keyboard_apply_bindings(')
    fixture=fixture.replace('/* ACTUAL_KEYBOARD_APPLY */',keyboard[begin:keyboard.index('\n}',begin)+2])
    source=strip_guest((area/'generated/game/x86_recomp.34.cpp').read_text())
    candidate,counts=transform_guest(source);assert counts==[0,4]
    def gate(text,name):
        begin=text.index('\t// 00158D11 ');end=text.index('\t// 00158D2A ',begin)
        return 'static void '+name+'(X86Context& ctx,uint8_t* base){\nuint32_t ca=0,cb=0,cf=0,of=0,zf=0,sf=0,pf=0;\n'+text[begin:end]+'return;\n}\n'
    fixture=fixture.replace('/* ORIGINAL_AND_CANDIDATE_GATES */',gate(source,'OriginalGate')+gate(candidate,'CandidateGate'))
    context=(local/'test_xbe_prompt_context.inc').read_text()
    def no_includes(text):return re.sub(r'^#(?:include|pragma)[^\n]*\n','',text,flags=re.M)
    context=context.replace('/* PROMPT_CONTEXT_DECLARATIONS */',no_includes((local/'xbe_prompt_context.h').read_text()))
    context=context.replace('/* PROMPT_CONTEXT_IMPLEMENTATION */',no_includes((local/'xbe_prompt_context.cpp').read_text()))
    bodies=[]
    for chunk,address in ((13,'00086F30'),(16,'0009D7E0'),(16,'0009D810'),
                          (16,'0009D9B0'),(16,'0009DB90'),(1,'00018CE0'),
                          (65,'00273410'),(65,'00273590'),(65,'00273860')):
        text=(area/f'generated/game/x86_recomp.{chunk}.cpp').read_text()
        if chunk in CONTEXT_HOOKS:text=transform_prompt_context(text,chunk)
        begin=text.index('X86_FUNC(__imp__sub_'+address+') {')
        body=text[begin:text.index('\nX86_WEAK_FUNC(sub_'+address,begin)]
        bodies.append(body.replace('__imp__sub_'+address,'sub_'+address))
    selector=(area/'generated/game/x86_recomp.16.cpp').read_text()
    selector=selector[selector.index('\t// 0009E257 '):selector.index('\t// 0009E280 ')]
    bodies.append('X86_FUNC(PromptOriginalSelectRecord) {\nX86_LOAD_FLAGS();\nuint32_t ca=0,cb=0;\n'+selector+
                  '\nreturn;\nloc_0009E298:\nloc_0009E288:\nctx.eax=0;return;\n}')
    for chunk,begin,end,name in ((28,'00115310','0011531B','PromptSimpleCutsceneQuery'),
                               (42,'001AC8F4','001AC905','PromptCinematicQuery'),
                               (62,'002548D8','00254996','PromptOwnedFooter'),
                               (65,'0026DF19','0026DF20','PromptRewardLabel'),
                               (65,'0026EA3F','0026EA49','PromptRewardQuery')):
        source=(area/f'generated/game/x86_recomp.{chunk}.cpp').read_text()
        body=source[source.index('\t// '+begin+' '):source.index('\t// '+end+' ')]
        bodies.append('X86_FUNC('+name+') {\nX86_LOAD_FLAGS();\n'+body+'\n}')
    body='\n'.join(bodies);context=context.replace('/* PROMPT_ORIGINAL_FUNCTIONS */',body)
    image=(area/'assets/Default.xbe').read_bytes();image_base=struct.unpack_from('<I',image,0x104)[0]
    count,table=struct.unpack_from('<II',image,0x11c)
    sections=[struct.unpack_from('<6I',image,table-image_base+i*56) for i in range(count)]
    constants=[]
    for address in sorted(set(int(a,16) for a in re.findall(r'ea = \(uint32_t\)\(0x([0-9A-F]+)u\)',body))):
        section=next(s for s in sections if s[1]<=address and address+4<=s[1]+s[2])
        # New original input witnesses read the zero-filled world global;
        # the fixture explicitly supplies its live object before those calls.
        value=struct.unpack_from('<I',image,section[3]+address-section[1])[0] if address+4<=section[1]+section[4] else 0
        constants.append(f'X86_MEM32(0x{address:x})=0x{value:x};')
    context=context.replace('/* PROMPT_ORIGINAL_CONSTANTS */','\n'.join(constants))
    fixture=fixture.replace('/* PROMPT_CONTEXT_FIXTURE */',context)
    for name in ('source','pixels','orders'):
        fixture=fixture.replace('/* PATH_'+name.upper()+' */','L"'+(out/f'test_pc_prompts_{name}.bin').as_posix()+'"')
    (area/'src/compat/test_pc_prompts.cpp').write_text(fixture)

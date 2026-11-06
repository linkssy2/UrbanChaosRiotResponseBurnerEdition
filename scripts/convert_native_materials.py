"""Offline exact-family conversion. Output contains no Xbox programs or tokens.

Original sources:90AD0 resource layout;8F999 stage binding/draw;0x4EAC00 tint
via8E7C0/9D260; retained vertex-fetch/VP and raw combiner decoding establish
the common typed native formula. A program mismatch is unsupported, not guessed.
"""
from pathlib import Path
import argparse, hashlib, importlib.util, json, math, struct
_spec=importlib.util.spec_from_file_location('archive_reference',Path(__file__).with_name('analyze_frontend.py'))
_archive=importlib.util.module_from_spec(_spec);_spec.loader.exec_module(_archive)
parse_archive,unpack_entry=_archive.parse_archive,_archive.unpack_entry

VERTEX=bytes.fromhex('78200800000000001b00cd00001837080088b020000000001b20cd000038370800489020000000001b40cd000058370800289020000000001b60cd00007837080018b020000000001a404700005835c400e8b020000000001ba44f06ff5336dc18f8a8100000000000060002681000084ce8a0e0000000001a6087006a2800c401e8b0f0')
DECLARATION=bytes.fromhex('00000020000032400100164001000020020040400200002003002240ffffffff')
#61 original words including PSB0. Unspecified words are exactly zero.
_pixel=[0]*61
for _i,_v in {0:0x30425350,1:0xd4d81010,9:0x200f0000,10:0x200c1c80,27:0xc0,
             35:0xc4c80000,46:0xc0,54:0x11101,55:1,58:0xffffffff,59:0xffffffff,60:0x1ff}.items():_pixel[_i]=_v
PIXEL=struct.pack('<61I',*_pixel)
# Two exact combiner stages: r0=T0*C0, then r0=V0*r0 (RGB and alpha).
_constant_pixel=_pixel.copy()
for _i,_v in {1:0xd8d11010,2:0xd4dc1010,28:0xc0,35:0xc8c10000,36:0xc4cc0000,
             47:0xc0,54:0x11102,58:0xfffffff0}.items():_constant_pixel[_i]=_v
CONSTANT_PIXEL=struct.pack('<61I',*_constant_pixel)
# Exact authored no-resource static material: no UV instruction or texture stage.
UNTEXTURED_HEADER=bytes.fromhex('2300000000003f00ffff00010000')
UNTEXTURED_PASS=bytes.fromhex('000000000000000074001800f4000000')
UNTEXTURED_VERTEX=bytes.fromhex('78200700000000001b00cd00001837080088b020000000001b20cd000038370800489020000000001b40cd000058370800289020000000001b60cd00007837080018b020000000001a404700005835c400e8a020000000001ba44f06ff5336dc18f8a810000000001a6087006a2800c401e8b0f0')
UNTEXTURED_DECLARATION=bytes.fromhex('0000002000003240010016400100002002004040ffffffff')
_untextured_pixel=_pixel.copy()
for _i,_v in {1:0xd4d11010,35:0xc4c10000,55:0,58:0xfffffff0}.items():_untextured_pixel[_i]=_v
UNTEXTURED_PIXEL=struct.pack('<61I',*_untextured_pixel)
_alpha_pixel=_constant_pixel.copy();_alpha_pixel[2]=0xd4d81010
ALPHA_PIXEL=struct.pack('<61I',*_alpha_pixel)
EXPOSURE_VERTEX=bytes.fromhex('78200900000000001b00cd00001837080088b020000000001b20cd000038370800489020000000001b40cd00005837080028a020000000001b60cd000078370800189020000000001be44e000050367c0000b02f000000001a404706ff5b35c400e8a810000000001ba04f00006837dc18f8a0200000000000060002681000084ce8a0e0000000001a6087006a2800c401e8b0f0')

# Exact asset programs; evaluated only by isolated verification, never runtime.
SKIN_VERTEX=bytes.fromhex('78201b0000000000000ea00100100078000000200000000055106000021000880000502100000000ff106000fd170088000050510000000000104c00001836880200202f0000000000304c00003836880200302f0000000000504c00005836880200402f00000000550ea001001000780000002000000000ff0280010030fe19000000210000000055108c006c1836880200209f0000000055308c006c383688020030df0000000055508c006d5836880200401f00000000aa0ea0010010007800000020000000001ba44f00005036dc18f8b02000000000ff008c006c1836540200209f00000000ff208c006c383654020030df00000000ff408c006d5836540200401f000000001b00e0000048360800000028000000001b00e0000068360800000024000000001b00e00000883608000000220000000000060002681000084ce8b0e0000000001b00cd00001837040088b020000000001b20cd000038370400489020000000001b40cd000058370400289020000000001b60cd000078370400189020000000001a404700005835c400e8a0200000000000000006ff1300080000b810000000001a6087006a6801c401e8b0f0')
SKIN_VERTEX_26=bytes.fromhex('78201a00000000005510600000103608f80f202100000000000ea0016c103608f80f702000000000ff106000fc173608f80f30910000000000104c006c183608fa0f402f0000000000304c006c183608fa0f502f0000000000504c006c183608fa0f602f00000000550ea0016c103608f80f7020000000001a0620006c10360848e870200000000055108c006d183608fa0f701f0000000055308c006d183608fa0f805f0000000055508c006d183608fa0f909f00000000aa0ea0016c103608f80f702000000000ff008c006d183634fa0fa0df00000000ff208c006e183634fa0fb01f00000000ff408c006e183634fa0f005f000000001b00e0006c483708f80f2028000000001b00e0006c683708f80f2024000000001b00e0006c083608f80f2022000000001b0420006c103608f80f302f000000001b00cd006c18362400887020000000001b20cd006c18362400487020000000001b40cd006c18362400287020000000001b60cd006c18362400187020000000001ba04f006c68360c18f87020000000001b404706ff1b36c400e87810000000001b6087006c2800c401e87030')
SKIN_DECLARATION=bytes.fromhex("00000020000031400100164001000020020040400200002003002240040000200700354008002440ffffffff")
# Exact25-instruction skin schedule: same26-instruction program with its UV
# output instruction removed. Position and vertex-color schedule is unchanged.
SKIN_UNTEXTURED_VERTEX=bytes.fromhex('78201900')+SKIN_VERTEX_26[4:4+7*16]+SKIN_VERTEX_26[4+8*16:]
SKIN_UNTEXTURED_DECLARATION=bytes.fromhex('0000002000003140010016400100002002004040040000200700354008002440ffffffff')
# Muzzle meshes: exact27-instruction skin + raw119 exposure before raw125 tint.
SKIN_EXPOSURE_VERTEX=bytes.fromhex('78201b0000000000000ea00100100078000000200000000055106000021000880000502100000000ff106000fd170088000050510000000000104c00001836880200202f0000000000304c00003836880200302f0000000000504c00005836880200402f00000000550ea001001000780000002000000000ff0280010030fe19000000210000000055108c006c1836880200209f0000000055308c006c383688020030df0000000055508c006d5836880200401f00000000aa0ea0010010007800000020000000001be44e000050367c0000b02f00000000ff008c006c1836540200209f00000000ff208c006c383654020030df00000000ff408c006d5836540200401f000000001b00e0000048360800000028000000001b00e0000068360800000024000000001b00e0000088360800000022000000001ba04f00006837dc18f8b020000000001b00cd00001837040088b020000000001b20cd000038370400489020000000001b40cd000058370400289020000000001b60cd00007837040018b020000000001a404700005835c400e8a020000000001a062006ff13003848e8a810000000001a6087006a2800c401e8b0f0')
_vertex_alpha_pixel=_pixel.copy();_vertex_alpha_pixel[1]=0xd4d11010;_vertex_alpha_pixel[58]=0xfffffff0
VERTEX_ALPHA_PIXEL=struct.pack("<61I",*_vertex_alpha_pixel)
WHITE_ALPHA_SETUP=b"\xc1"+struct.pack("<4f",1,1,1,1)+b"\x86"+bytes(16)+b"\x02"
IFL_SETUP=(b'\xc1'+struct.pack('<4f',1,1,1,1)+b'\xc1'+bytes(16)+b'\x4a\x87'+bytes(16)+b'\x02')
IFL_WHITE_ALPHA_SETUP=WHITE_ALPHA_SETUP[:-1]+IFL_SETUP

HEADER=bytes.fromhex('0100000000000001ffff00010000')
PASS=bytes.fromhex('010000000000000084002000f4000000')
# Exact authored setup templates; they are recognized offline, never executed in
# the native renderer. 8EC20/8F6F0 ->3E7450 disables culling for the draw and
# restores clockwise afterward. 8F204 ->3ED480 sets stage0 V-clamp then V-wrap.
TWO_SIDED_HEADER=bytes.fromhex('4500450000000001ffff00010000')
TWO_SIDED_SETUP=(b'\xc1'+struct.pack('<4I',0,0,0,0)+b'\x82'+struct.pack('<4I',0x93,0x93,0x93,0x93)+
    b'\x02\xc0'+struct.pack('<4I',0x900,0x900,0x900,0x900)+b'\x82'+struct.pack('<4I',0x93,0x93,0x93,0x93))
V_CLAMP_HEADER=bytes.fromhex('4500000000000001ffff00010000')
V_CLAMP_SETUP=(b'\xc1'+struct.pack('<4I',3,3,3,3)+b'\x85'+struct.pack('<4I',1,1,1,1)+
    b'\x02\xc0'+struct.pack('<4I',1,1,1,1)+b'\x85'+struct.pack('<4I',1,1,1,1))
FAMILIES=((HEADER,[b'\x02',b''],0,0),
    (TWO_SIDED_HEADER,[TWO_SIDED_SETUP,TWO_SIDED_SETUP],1,0),
    (V_CLAMP_HEADER,[V_CLAMP_SETUP,b''],0,1))
UV_CLAMP_SETUP=(b'\xc1'+struct.pack('<4I',3,3,3,3)+b'\x85'+bytes(16)+
    b'\xc1'+struct.pack('<4I',3,3,3,3)+b'\x85'+struct.pack('<4I',1,1,1,1)+b'\x02'+
    b'\xc0'+struct.pack('<4I',1,1,1,1)+b'\x85'+bytes(16)+
    b'\xc0'+struct.pack('<4I',1,1,1,1)+b'\x85'+struct.pack('<4I',1,1,1,1))
def _literal(op,value):return bytes([op])+struct.pack('<4I',value,value,value,value)
def _state(literal,value,op,state):return _literal(literal,value)+_literal(op,state)
_no_cull=_state(0xc1,0,0x82,0x93);_restore_cull=_state(0xc0,0x900,0x82,0x93)
_alpha_blend=_state(0xc0,0x303,0x82,0x3f)+_state(0xc0,0x302,0x82,0x3e)+_state(0xc0,1,0x82,0x3b)
SKIN_EXPOSURE_HEADER=bytes.fromhex('ac00450007000001ffffff010000')
SKIN_EXPOSURE_SETUPS=tuple(_no_cull+_state(0xc0,dest,0x82,0x3f)+
    _state(0xc0,0x302,0x82,0x3e)+_state(0xc0,1,0x82,0x3b)+b'\x02'+_restore_cull+b'\x04'
    for dest in (0x303,1))
# NPC assault-rifle flash: the authored87 selector queries image virtual24.
# Actual96090 image vtable4568D4 points that getter to2C2B60 (zero), so the
# frame-selection branch exits without changing its single .tga resource.
SKIN_IMAGE_FRAME_SETUP=_no_cull+IFL_SETUP[:-1]+_alpha_blend+b'\x02'+_restore_cull+b'\x04'
# Same skin/exposure schedule with UV0+raw128 and color output rescheduled.
_skin_scroll=bytearray(SKIN_EXPOSURE_VERTEX)
for _i,_words in ((19,'000000001a0670006810000c48e8b0e0'),(23,'000000001b60cd000078370400189020'),(25,'000000001ba04f06ff6b37dc18f8a810')):
    _skin_scroll[4+16*_i:20+16*_i]=bytes.fromhex(_words)
SKIN_SCROLL_VERTEX=bytes(_skin_scroll)
ALPHA_FAMILIES=(
 (bytes.fromhex('8a00000003000001ffffff010000'),_alpha_blend+b'\x02\x04',0,0,0),
 # Same verified exposure/alpha formula; two-sided glass/fences/fire escapes,
 # with neither foliage's V-clamp nor weed polygon offset.
 (bytes.fromhex('ce00450003000001ffffff010000'),
  _no_cull+_alpha_blend+b'\x02'+_restore_cull+b'\x04',0,0,1),
 (bytes.fromhex('1201450003000001ffffff010000'),
  _no_cull+_state(0xc1,3,0x85,1)+_alpha_blend+b'\x02'+_restore_cull+_state(0xc0,1,0x85,1)+b'\x04',1,0,1),
 (bytes.fromhex('1201450003000001ffffff010600'),
  _no_cull+_state(0xc1,1,0x82,0x95)+_alpha_blend+b'\x02'+_restore_cull+_state(0xc0,0,0x82,0x95)+b'\x04',0,1,1))
ADDITIVE_HEADER=bytes.fromhex('ac00000003000001ffffff010200')
ADDITIVE_SETUP=(_state(0xc1,14,0x82,0x95)+_state(0xc0,1,0x82,0x3f)+
    _state(0xc0,0x302,0x82,0x3e)+_state(0xc0,1,0x82,0x3b)+b'\x02'+_state(0xc0,0,0x82,0x95)+b'\x04')
# Exact ordinary source scripts recovered for darkening overlays, additive
# icicles, and periodic UV translation. Values in float literals are data.
_additive_blend=_state(0xc0,1,0x82,0x3f)+_state(0xc0,0x302,0x82,0x3e)+_state(0xc0,1,0x82,0x3b)
_subtract_begin=_state(0xc0,0x800b,0x82,0x4a)
_subtract_end=_state(0xc0,0x8006,0x82,0x4a)
SUBTRACT_SETUP=_subtract_begin+_additive_blend+b'\x02'+_subtract_end+b'\x04'
DECAL_ALPHA_SETUP=_state(0xc1,15,0x82,0x95)+_alpha_blend+b'\x02'+_state(0xc0,0,0x82,0x95)+b'\x04'
SCROLL_VERTEX=bytes.fromhex('78200900000000001b00cd00001837080088b020000000001b20cd000038370800489020000000001b40cd00005837080028a020000000001b60cd000078370800189020000000001be44e000050367c0000b02f000000001a404706ff5b35c400e8a810000000001a0670006810000c48e8a0e0000000001ba04f00006837dc18f8a020000000001a6087006a2800c401e8b0f0')
# Exact helicopter window/wire, serial decal and three flash layers. Existing
# skin/exposure/UV and blend formulas; rotor UV rotation is deliberately separate.
def _helicopter_scroll(period):
    return (_literal(0xc1,0)+b'\xc1'+struct.pack('<4f',*[period]*4)+_literal(0xc1,0)+b'\x4a'+
        _literal(0xc1,0)+b'\x49'+_literal(0x80,0x20))
HELICOPTER_EFFECTS={
 (bytes.fromhex('0301000007000001ffffff010000'),_helicopter_scroll(.5)+SUBTRACT_SETUP):(4,0,(0.,.5)),
 (bytes.fromhex('ac00000007000001ffffff010000'),SUBTRACT_SETUP):(4,0,(0.,0.)),
 (bytes.fromhex('ac00000007000001ffffff010200'),DECAL_ALPHA_SETUP):(1,15,(0.,0.)),
 (bytes.fromhex('0301000007000001ffffff010200'),_state(0xc1,15,0x82,0x95)+_helicopter_scroll(1.)+_additive_blend+b'\x02'+_state(0xc0,0,0x82,0x95)+b'\x04'):(2,15,(0.,1.)),
}

# Exact helicopter rotor: opcode60 rotates UV about (.5,.5) every .3 seconds.
ROTOR_HEADER=bytes.fromhex('2501450007000001ffffff010000')
ROTOR_SETUP=bytes.fromhex('c10000003f0000003f000000000000803f8023000000230000002300000023000000c1000000000000000000000000000000008293000000930000009300000093000000c19a99993e9a99993e9a99993e9a99993ec1000000000000000000000000000000004a60802000000020000000200000002000000080210000002100000021000000210000008022000000220000002200000022000000c003030000030300000303000003030000823f0000003f0000003f0000003f000000c002030000020300000203000002030000823e0000003e0000003e0000003e000000c001000000010000000100000001000000823b0000003b0000003b0000003b00000002c000090000000900000009000000090000829300000093000000930000009300000004')
ROTOR_VERTEX=bytes.fromhex('78201f0000000000000ea00100100078000000200000000055106000021000880000502100000000ff106000fd170088000050510000000000104c00001836880200202f0000000000304c00003836880200302f0000000000504c00005836880200402f00000000550ea001001000780000002000000000ff0280010030fe19000000210000000055108c006c1836880200209f0000000055308c006c383688020030df0000000055508c006d5836880200401f00000000aa0ea0010010007800000020000000001a66700068140038000050fe00000000ff008c006c1836540200209f00000000ff208c006c383654020030df00000000ff408c006d5836540200401f000000001b00e0000048360800000028000000001b00e0000068360800000024000000001b00e0000088360800000022000000001a40b0000058345400006028000000001b00cd00001837040088b020000000001b20cd000038370400489020000000001b40cd00005837040028a020000000001b60cd00007837040018b020000000001a20b0000038345400006024000000001a00b006ff1b345400006812000000001be44e000050367c0000b02f000000001a404700005835c400e8a020000000001a6070006910003c48e8a090000000001ba04f00006837dc18f8a020000000001a6087006a2800c401e8b0f0')

def scroll_periods(header,setup):
    """Only literal2D UV vectors, opcode4A sawtooth and raw constant128.
    A zero period marks the actual literal-zero axis, never a division by zero.
    Original8E840: fmod(float(phase/period + 0),1), signed remainder.
    """
    effect=HELICOPTER_EFFECTS.get((bytes(header),bytes(setup)))
    if effect:return effect[2] if any(effect[2])else None
    if header not in (bytes.fromhex('e100000003000001ffffff010000'),
        bytes.fromhex('2501450003000001ffffff010000'),bytes.fromhex('3701450003000001ffffff010000'),
        bytes.fromhex('e100000007000001ffffff010000'),bytes.fromhex('0301450007000001ffffff010000'),bytes.fromhex('2501450007000001ffffff010000'),bytes.fromhex('1501450007000001ffffff010000')):return None
    two=header[2:4]!=bytes(2);prefix=setup[34:]if setup[:1]==b'\xc1' and setup[17:34]==b'\x86'+bytes(16)else setup
    if two:
        if not prefix.startswith(_no_cull):return None
        prefix=prefix[len(_no_cull):]
    blend=_alpha_blend if header[4]==7 and _alpha_blend+b'\x02' in prefix else _additive_blend
    tail=_literal(0xc1,0)+b'\x49'+_literal(0x80,0x20)+blend+b'\x02'+(_restore_cull if two else b'')+b'\x04'
    if not prefix.endswith(tail):return None
    expr=prefix[:-len(tail)];values=[]
    while expr:
        if expr.startswith(_literal(0xc1,0)):
            values.append(0.0);expr=expr[17:]
        elif len(expr)>=35 and expr[0]==0xc1 and expr[17:35]==_literal(0xc1,0)+b'\x4a':
            v=struct.unpack('<4f',expr[1:17])
            if not all(math.isfinite(x) and x==v[0] for x in v) or not 1e-6<=abs(v[0])<=1e6:return None
            values.append(v[0]);expr=expr[35:]
        else:return None
    return tuple(values) if len(values)==2 and any(values) else None

REASONS=['supported','setup script','material header/context','pass bindings','vertex program','vertex declaration','pixel program']

def parse_material(data):
    if len(data)<14 or len(data)>2*1024*1024:raise ValueError('material size')
    h=data[:14];at=14;deps=[]
    def take(n):
        nonlocal at
        if n>len(data)-at:raise ValueError('truncated material')
        p=data[at:at+n];at+=n;return p
    for _ in range(h[7]):
        kind,n=struct.unpack('<BH',take(3))
        if not 0<n<256:raise ValueError('unsupported name length')
        name=take(n)
        if b'\0' in name:raise ValueError('unsupported embedded NUL')
        deps.append((kind,name))
    setup=[take(n) for n in struct.unpack_from('<HH',h)]
    passes=[]
    for _ in range(h[11]):
        ph=take(16)
        if ph[0]>4 or any(i>=len(deps) or deps[i][0]==0 for i in ph[1:1+ph[0]]):raise ValueError('invalid pass dependency')
        streams=[take(n) for n in struct.unpack_from('<HHH',ph,8)]
        passes.append((ph,streams))
    if at!=len(data) or any(i!=255 and i>=len(deps) for i in h[8:10]):raise ValueError('material bounds')
    return h,deps,setup,passes

def native_descriptor(data):
    """Return reason, texture, cull, V, U, kind, constant, blend, exposure, bias,
    geometry, alpha mode, color write. Last three are typed enum values0/1.

    Kinds1..3 describe ordinary base material entry8FD80. Kind4 explicitly
    selects source8FF30 textured alpha prepass; its ordinary entry has no draw. Header resource
    slots8/9 and coefficient10 belong to other material entries, not this draw.
    Flags2 does not change8FD80. Exact static flags8 script02 uses RGBA writes.
    Exact skinned flags4/6/7/12 are admitted separately;
    flag8 enables temporary RGBA writes. Other flag combinations stay excluded.
    """
    h,deps,setup,passes=parse_material(data)
    family=next((f for f in FAMILIES if setup==f[1]),None)
    cull=address_v=address_u=blend=exposure=bias=0;kind=1;constant=0xffffffff;scroll=None;static_vertex_alpha=False
    def unsupported(reason):return reason,None,0,0,0,0,0,0,0,0,0,0,0
    flags=struct.unpack_from('<H',h,4)[0]
    if h==ROTOR_HEADER and setup==[ROTOR_SETUP,TWO_SIDED_SETUP]:
        if len(deps)!=1 or not deps[0][0] or not deps[0][1].lower().endswith(b'.tga') or len(passes)!=1:return unsupported(2)
        programs=(ROTOR_VERTEX,SKIN_DECLARATION,PIXEL)
        if passes[0][0]!=PASS[:8]+struct.pack('<HHH',*[len(p)for p in programs])+PASS[14:]:return unsupported(3)
        for i,p in enumerate(programs):
            if passes[0][1][i]!=p:return unsupported(4+i)
        return 0,deps[0][1],1,0,0,1,0xffffffff,1,1,0,1,0,0
    effect=HELICOPTER_EFFECTS.get((bytes(h),bytes(setup[0])))
    if effect and not setup[1]:
        blend,bias,periods=effect
        if len(deps)!=1 or not deps[0][0] or not deps[0][1].lower().endswith(b'.tga') or len(passes)!=1:return unsupported(2)
        programs=(SKIN_SCROLL_VERTEX if any(periods)else SKIN_EXPOSURE_VERTEX,SKIN_DECLARATION,PIXEL)
        expected=PASS[:8]+struct.pack('<HHH',*[len(p)for p in programs])+PASS[14:]
        if passes[0][0]!=expected:return unsupported(3)
        for i,p in enumerate(programs):
            if passes[0][1][i]!=p:return unsupported(4+i)
        return 0,deps[0][1],0,0,0,1,0xffffffff,blend,1,bias,1,0,0
    if h==bytes.fromhex('2300000004003f00ffff00010000'):
        if deps or setup!=[WHITE_ALPHA_SETUP,b''] or len(passes)!=1:return unsupported(2)
        expected=UNTEXTURED_PASS[:8]+struct.pack('<HHH',len(SKIN_UNTEXTURED_VERTEX),len(SKIN_UNTEXTURED_DECLARATION),len(UNTEXTURED_PIXEL))+UNTEXTURED_PASS[14:]
        if passes[0][0]!=expected:return unsupported(3)
        for i,program in enumerate((SKIN_UNTEXTURED_VERTEX,SKIN_UNTEXTURED_DECLARATION,UNTEXTURED_PIXEL)):
            if passes[0][1][i]!=program:return unsupported(4+i)
        return 0,None,0,0,0,3,0xffffffff,0,0,0,1,0,0
    # Exact vehicle surface scripts, same qualified skin/exposure/combiner
    # programs. All state and literal constants are retained in the catalog.
    vehicle_headers=tuple(bytes.fromhex(x) for x in ('8a00000007000001ffffff010000','5601000007000001ffffff010000','3401000007000001ffffff010000','0301450007000001ffffff010000','2501450007000001ffffff010000','e100000007000001ffffff010000','1501450007000001ffffff010000'))
    if h in vehicle_headers and setup[0]!=WHITE_ALPHA_SETUP[:-1]+_alpha_blend+b'\x02\x04':
        script=setup[0];literal=script[:1]==b'\xc1' and script[17:34]==b'\x86'+bytes(16)
        if literal:
            values=struct.unpack('<4f',script[1:17])
            if not all(math.isfinite(v) and 0<=v<=1 for v in values) or values[3]!=1:
                # Existing Molotov all-lane alpha .4 continues below.
                values=None
            if values:
                q=[round(struct.unpack('<f',struct.pack('<f',v*255))[0])for v in values];constant=q[0]<<16|q[1]<<8|q[2]|q[3]<<24;kind=2;script=script[34:]
        clamp=_state(0xc1,3,0x85,0)+_state(0xc1,3,0x85,1)
        restore=_state(0xc0,1,0x85,0)+_state(0xc0,1,0x85,1)
        subtract=clamp+_subtract_begin+_additive_blend+b'\x02'+restore+_subtract_end+b'\x04'
        scroll=scroll_periods(h,setup[0])
        if literal and kind==2 and h==bytes.fromhex('8a00000007000001ffffff010000') and setup[1]==b'' and script==_alpha_blend+b'\x02\x04':blend=1;exposure=1
        elif h in (bytes.fromhex('5601000007000001ffffff010000'),bytes.fromhex('3401000007000001ffffff010000')) and setup[1]==b'' and script==subtract:blend=4;exposure=1;address_u=address_v=1
        elif scroll and setup[1]==(TWO_SIDED_SETUP if h[2:4]!=bytes(2) else b'') and _additive_blend+b'\x02' in script:blend=2;exposure=1;cull=int(bool(h[2:4]!=bytes(2)))
        else:blend=0
        if blend:
            if len(deps)!=1 or not deps[0][0]or not deps[0][1].lower().endswith(b'.tga')or len(passes)!=1:return unsupported(2)
            programs=(SKIN_SCROLL_VERTEX if scroll else SKIN_EXPOSURE_VERTEX,SKIN_DECLARATION,ALPHA_PIXEL if blend==1 else CONSTANT_PIXEL if kind==2 else PIXEL)
            expected=PASS[:8]+struct.pack('<HHH',*[len(p)for p in programs])+PASS[14:]
            if passes[0][0]!=expected:return unsupported(3)
            for i,p in enumerate(programs):
                if passes[0][1][i]!=p:return unsupported(4+i)
            return 0,deps[0][1],cull,address_v,address_u,kind,constant,blend,exposure,0,1,0,0
        kind=1;constant=0xffffffff;scroll=None
    if h in (bytes.fromhex('6800000007000001ffffff010000'),bytes.fromhex('e100000007000001ffffff010000')):
        scrolling=h[0]==0xe1
        if len(deps)!=1 or not deps[0][0] or not deps[0][1].lower().endswith(b'.tga') or setup[1]:return unsupported(2)
        if scrolling:
            if setup[0][:34]!=b'\xc1'+struct.pack('<4f',.4,.4,.4,.4)+b'\x86'+bytes(16) or scroll_periods(h,setup[0])!=(10.,0.):return unsupported(1)
        elif setup[0] not in (_alpha_blend+b'\x02\x04',_additive_blend+b'\x02\x04'):return unsupported(1)
        if len(passes)!=1 or passes[0][0]!=PASS[:8]+struct.pack('<HHH',len(SKIN_EXPOSURE_VERTEX),len(SKIN_DECLARATION),len(PIXEL))+PASS[14:]:return unsupported(3)
        for i,expected in enumerate((SKIN_SCROLL_VERTEX if scrolling else SKIN_EXPOSURE_VERTEX,SKIN_DECLARATION,VERTEX_ALPHA_PIXEL if scrolling else PIXEL)):
            if passes[0][1][i]!=expected:return unsupported(4+i)
        return 0,deps[0][1],0,0,0,1,0x66666666 if scrolling else 0xffffffff,2 if setup[0]==_additive_blend+b'\x02\x04' else 1,1,0,1,int(scrolling),0
    # Separate source8FF30 alpha prepass, never an invented ordinary draw.
    # Ordinary8FD80 has zero passes; its metadata remains explicit as kind4.
    alpha_header=bytes.fromhex('000000002100000100ff00000000')
    skin_alpha_header=alpha_header[:4]+bytes([0x25])+alpha_header[5:]
    skin_hair_header=skin_alpha_header[:2]+bytes([69,0])+skin_alpha_header[4:]
    if h in (alpha_header,alpha_header[:2]+bytes([69,0])+alpha_header[4:],skin_alpha_header,skin_hair_header):
        if len(deps)!=1 or not deps[0][0] or passes or setup[0] or setup[1] not in (b'',TWO_SIDED_SETUP):return unsupported(2)
        if len(setup[1])!=struct.unpack_from('<H',h,2)[0]:return unsupported(1)
        if h in (skin_alpha_header,skin_hair_header) and not deps[0][1].lower().endswith(b'.ifl' if h==skin_hair_header else b'.tga'):return unsupported(3)
        return 0,deps[0][1],int(bool(setup[1])),0,0,4,0xffffffff,1,0,0,int(h in (skin_alpha_header,skin_hair_header)),0,0
    if flags&4:
        if flags not in (4,6,7,12) or h[11:]!=b'\x01\x00\x00' or not deps or any(not k for k,n in deps):return unsupported(2)
        sequence=False
        if flags==7:
            if h==bytes.fromhex('e000450007000001ffffff010000'):
                if setup!=[SKIN_IMAGE_FRAME_SETUP,TWO_SIDED_SETUP]:return unsupported(1)
                blend=exposure=cull=1;alpha=0
            elif h==bytes.fromhex('ce00450007000001ffffff010000'):
                # Police-car glass: existing skin/exposure VP and alpha pixel,
                # literal RGB C0 with alpha1, two-sided SRC_ALPHA/INV_SRC_ALPHA.
                if setup[1]!=TWO_SIDED_SETUP or setup[0][34:]!=SKIN_EXPOSURE_SETUPS[0] or setup[0][:1]!=b'\xc1' or setup[0][17:34]!=b'\x86'+bytes(16):return unsupported(1)
                values=struct.unpack('<4f',setup[0][1:17])
                if not all(math.isfinite(v) and 0<=v<=1 for v in values) or values[3]!=1:return unsupported(1)
                q=[round(struct.unpack('<f',struct.pack('<f',v*255))[0])for v in values]
                constant=q[0]<<16|q[1]<<8|q[2]|q[3]<<24;kind=2;blend=exposure=cull=1;alpha=0
            elif h==bytes.fromhex('8a00000007000001ffffff010000') and setup==[WHITE_ALPHA_SETUP[:-1]+_alpha_blend+b'\x02\x04',b'']:
                blend=exposure=alpha=1
            else:
                if h!=SKIN_EXPOSURE_HEADER:return unsupported(2)
                if setup[1]!=TWO_SIDED_SETUP or setup[0] not in SKIN_EXPOSURE_SETUPS:return unsupported(1)
                blend=SKIN_EXPOSURE_SETUPS.index(setup[0])+1;exposure=cull=1;alpha=0
        elif setup==[b'\x02',b'']:alpha=0
        elif setup==[TWO_SIDED_SETUP,TWO_SIDED_SETUP]:alpha=0;cull=1
        elif setup==[WHITE_ALPHA_SETUP,b'']:alpha=1
        # Brute cleaver handle: original white C0 alpha plus the already
        # qualified two-sided draw/restoration. Exact skin/combiner checks below.
        elif h==bytes.fromhex('670045000c000001ffff00010000') and setup==[WHITE_ALPHA_SETUP[:-1]+TWO_SIDED_SETUP,TWO_SIDED_SETUP]:alpha=cull=1
        elif h==bytes.fromhex('2300000004000001ffff00010000') and len(setup[0])==35 and setup[0][0]==0xc1 and setup[0][17:]==b'\x86'+bytes(16)+b'\x02' and not setup[1]:
            values=struct.unpack('<4f',setup[0][1:17])
            if not all(math.isfinite(v) and 0<=v<=1 for v in values):return unsupported(1)
            q=[round(struct.unpack('<f',struct.pack('<f',v*255))[0]) for v in values]
            constant=q[0]<<16|q[1]<<8|q[2]|q[3]<<24;kind=2;alpha=0
        elif setup==[IFL_SETUP,b'']:alpha=0;sequence=True
        elif setup==[IFL_WHITE_ALPHA_SETUP,b'']:alpha=1;sequence=True
        else:return unsupported(1)
        if len(passes)!=1:return unsupported(3)
        ph,streams=passes[0];vp,decl,pixel=streams
        expected=PASS[:8]+struct.pack('<HHH',len(vp),len(SKIN_DECLARATION),len(PIXEL))+PASS[14:]
        if ph!=expected:return unsupported(3)
        expected_vp=(SKIN_EXPOSURE_VERTEX,) if exposure else (SKIN_VERTEX,) if sequence else (SKIN_VERTEX,SKIN_VERTEX_26)
        if vp not in expected_vp:return unsupported(4)
        if decl!=SKIN_DECLARATION:return unsupported(5)
        if pixel!=(VERTEX_ALPHA_PIXEL if alpha else ALPHA_PIXEL if kind==2 and blend==1 else CONSTANT_PIXEL if kind==2 else PIXEL):return unsupported(6)
        texture=deps[ph[1]][1]
        if not texture.lower().endswith(b'.ifl' if sequence else b'.tga'):return unsupported(3)
        return 0,texture,cull,0,0,kind,constant,blend,exposure,0,1,alpha,int(bool(flags&8))
    if family:cull,address_v=family[2:]
    elif setup==[UV_CLAMP_SETUP,b'']:address_u=address_v=1
    elif h==ADDITIVE_HEADER and setup==[ADDITIVE_SETUP,b'']:blend=2;exposure=1;bias=14
    elif h==ADDITIVE_HEADER and setup==[DECAL_ALPHA_SETUP,b'']:blend=exposure=1;bias=15
    elif h==bytes.fromhex('ac00000003000001ffffff010000') and setup==[SUBTRACT_SETUP,b'']:blend=4;exposure=1
    # Authored bullet/blood decals: identical exposure VP, pixel and alpha setup.
    # Blood's 0x200 flag is retained in the decoded asset. 8FD9E gates its draw
    # through the current profile; descriptor admission does not enable blood.
    elif h in (bytes.fromhex('6800000003000001ffffff010000'),bytes.fromhex('6800000003020001ffffff010000')) and setup==[_alpha_blend+b'\x02\x04',b'']:
        blend=exposure=1
    # Static glass shards: same exposure shader, real two-sided alpha script.
    elif h==bytes.fromhex('ac00450003000001ffffff010000') and setup==[SKIN_EXPOSURE_SETUPS[0],TWO_SIDED_SETUP]:
        cull=blend=exposure=1
    # Literal float4 -> pixel constant0 -> exact draw/restoration suffix.
    elif len(setup[0])>=34 and setup[0][0]==0xc1 and setup[0][17:34]==b'\x86'+bytes(16):
        suffix=setup[0][34:]
        alpha=next((f for f in ALPHA_FAMILIES if h==f[0] and suffix==f[1] and setup[1]==(TWO_SIDED_SETUP if f[4] else b'')),None)
        if alpha:blend=exposure=1;address_v,bias,cull=alpha[2:]
        elif h==bytes.fromhex('ce00000003000001ffffff010000') and setup[1]==b'' and suffix==SUBTRACT_SETUP:blend=4;exposure=1
        elif h==bytes.fromhex('8a00000003000001ffffff010000') and setup[1]==b'' and suffix==_additive_blend+b'\x02\x04':blend=2;exposure=1
        elif (scroll:=scroll_periods(h,setup[0])) and setup[1]==(TWO_SIDED_SETUP if h[2:4]!=bytes(2) else b''):blend=2;exposure=1;cull=int(bool(h[2:4]!=bytes(2)))
        elif [suffix,setup[1]]==[b'\x02',b'']:pass
        elif [suffix,setup[1]]==[TWO_SIDED_SETUP,TWO_SIDED_SETUP]:cull=1
        else:return unsupported(1)
        values=struct.unpack('<4f',setup[0][1:17])
        if not all(math.isfinite(v) and 0<=v<=1 for v in values):return unsupported(1)
        if exposure and values[3]!=1:return unsupported(1)
        # Original3EEE90 uses CVTSS2SI, default nearest-even MXCSR, after MULPS.
        # Keep float32 product rounding before integer conversion.
        q=[round(struct.unpack('<f',struct.pack('<f',v*255))[0]) for v in values]
        constant=q[0]<<16|q[1]<<8|q[2]|q[3]<<24;kind=2
    else:return unsupported(1)
    if not deps and h==UNTEXTURED_HEADER and kind==2 and len(setup[0])==35 and setup[1]==b'':
        if len(passes)!=1 or passes[0][0]!=UNTEXTURED_PASS:return unsupported(3)
        for i,expected in enumerate((UNTEXTURED_VERTEX,UNTEXTURED_DECLARATION,UNTEXTURED_PIXEL)):
            if passes[0][1][i]!=expected:return unsupported(4+i)
        return 0,None,0,0,0,3,constant,0,0,0,0,0,0
    static_rgba=flags==8 and setup==[b'\x02',b'']
    if (not exposure and ((flags not in (0,2) and not static_rgba) or h[6]!=0 or h[11:]!=b'\x01\x00\x00') or
        not deps or any(not k for k,n in deps)):
        return unsupported(2)
    expected_vp=SCROLL_VERTEX if scroll else EXPOSURE_VERTEX if exposure else VERTEX
    expected_pass=PASS[:8]+struct.pack('<H',len(expected_vp))+PASS[10:]
    if len(passes)!=1 or passes[0][0]!=expected_pass:return unsupported(3)
    # Actual fireescape280 uses white C0 only for alpha; RGB has no constant.
    if blend==1 and kind==2 and constant==0xffffffff and passes[0][1][2]==VERTEX_ALPHA_PIXEL:
        kind=1;static_vertex_alpha=True
    for i,expected in enumerate((expected_vp,DECLARATION,VERTEX_ALPHA_PIXEL if static_vertex_alpha else ALPHA_PIXEL if blend==1 and kind==2 else CONSTANT_PIXEL if kind==2 else PIXEL)):
        if passes[0][1][i]!=expected:return unsupported(4+i)
    return 0,deps[passes[0][0][1]][1],cull,address_v,address_u,kind,constant,blend,exposure,bias,0,int(static_vertex_alpha),int(static_rgba)

def classify_native(data):
    return native_descriptor(data)[:4]

def classify(data):
    return classify_native(data)[:2]

def convert(archive_path,output):
    data,entries=parse_archive(archive_path);archive_hash=hashlib.sha256(data).digest();records=[];report=[];version=8
    for e in entries:
        if e['extension'] not in ('.csf','.shader','.skcsf'):continue
        payload=unpack_entry(data,e);reason,texture,cull,address_v,address_u,kind,constant,blend,exposure,bias,geometry,alpha,write=native_descriptor(payload);name=e['name'].encode('latin1')+b'\0'
        tex=(texture or b'')+b'\0';identity=hashlib.sha256(payload).digest()
        if len(name)>1024:raise ValueError('material name too long')
        sequence=bool(texture and texture.lower().endswith(b'.ifl'))
        h,_,setup,_=parse_material(payload);periods=scroll_periods(h,setup[0]) if not reason else None
        periods=periods or (0.,0.)
        rotor=not reason and h==ROTOR_HEADER and setup==[ROTOR_SETUP,TWO_SIDED_SETUP]
        if rotor:version=9
        if sequence and (reason or not geometry):raise ValueError('unqualified texture sequence')
        records.append(struct.pack('<BBHH6BI',kind,reason,len(name),len(tex),cull,address_v|(address_u<<1),blend,exposure,bias,geometry|(alpha<<1)|(write<<2)|(int(sequence)<<3)|(int(rotor)<<4),constant)+identity+struct.pack('<2f',*periods)+name+tex)
        report.append({'name':e['name'],'source_sha256':identity.hex(),'native_template':kind,
            'reason':REASONS[reason],'texture_stage0':texture.decode('latin1') if texture else None,
            'cull_mode':'none' if cull else 'inherit','address_v':'clamp_to_edge' if address_v else 'inherit',
            'address_u':'clamp_to_edge' if address_u else 'inherit','constant_argb':f'{constant:08x}',
            'blend_mode':('parent','src_alpha_inv_src_alpha','src_alpha_one','reserved_parent_multiply','reverse_subtract_src_alpha_one')[blend],'vertex_exposure':bool(exposure),'depth_bias':bias,
            'geometry':('static','skin3')[geometry],'alpha_mode':('texture_vertex','vertex')[alpha],
            'color_write':('inherit','rgba')[write],'texture_selection':'sequence_period1' if sequence else 'direct','uv_periods':periods,'rotor_uv':rotor})
    output.parent.mkdir(parents=True,exist_ok=True)
    output.write_bytes(f'UCMAT00{version}'.encode()+struct.pack('<I',len(records))+archive_hash+b''.join(records))
    manifest={'catalog_version':version,'source_archive':str(archive_path),'source_archive_sha256':archive_hash.hex(),
        'native_catalog_sha256':hashlib.sha256(output.read_bytes()).hexdigest(),
        'supported':sum(r['native_template']!=0 for r in report),'total':len(records),
        'template':'Texture0 * interpolated quantized(BGRA vertex color * object tint); UV0 unchanged',
        'constant_template':'saturate(saturate(Texture0 * unpackARGB(constant_argb)) * interpolated quantized vertex color); includes alpha',
        'untextured_template':'saturate(interpolated quantized(BGRA vertex color * object tint) * unpackARGB(constant_argb)); includes alpha; no image, UV consumption, sampler or resource binding.',
        'alpha_template':'Exact glass/fences/tags/foliage: quantize((vertexColor * raw119) * tint), pixel constant alpha1, SRC_ALPHA/INV_SRC_ALPHA; authored culling, V addressing and polygon offset retained, no alpha-test override. raw119 is caller-owned reciprocal exposure or original white when exposure manager absent. Depth-bias1 emits slope-0.25/units-1; parent owns depth format/composition.',
        'prepass_template':'Kind4 is separate source8FF30 texture*C2 with alpha>=127, RGBwrites and depthwrites. Ordinary8FD80 has zero draws. Caller owns actual ambient C2, exposure and prepass scheduling; native preview uses explicitly supplied PC ambient.',
        'additive_template':'Exact static Window_glow1: same exposure vertex formula, common pixel formula, SRC_ALPHA/ONE, depth-bias14 emits slope-3.5/units-14. Animated IFL and other scripts remain unsupported.',
        'skinned_template':'Exact27/26 programs: existing three-influence palette position, same tint/raw125 vertex color quantization, unchanged UV. Exact muzzle27 additionally applies raw119 exposure before tint with two-sided SRC_ALPHA/ONE or INV_SRC_ALPHA; no depth-write override. Vertex-alpha mode ignores texture alpha; white C0 alpha is exactly1. RGBA write override is source flag8. Exact27 IFL scripts additionally use the separately proved texture_sequence selector with explicit renderer phase and optional per-instance state. No appearance policy or additional lighting passes.',
        'context':'Kinds1..3 ordinary8FD80; kind4 separate source8FF30 texturedalpha prepass with ordinaryzero;  auxiliary lighting/emission resource metadata is not implemented. Blend/alpha/depth and unspecified cull/sampler fields inherited from parent pass; exact per-draw cull/U/V-address overrides explicit. Tint initialized white by original9D260, replaceable by8E7C0.',
        'materials':report}
    output.with_suffix('.json').write_text(json.dumps(manifest,indent=2)+'\n')
    return manifest

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('archive',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
    result=convert(a.archive,a.output);print(f"{result['supported']}/{result['total']} supported: {a.output}")

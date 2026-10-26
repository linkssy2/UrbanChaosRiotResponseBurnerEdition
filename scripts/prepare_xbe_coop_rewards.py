"""Keep personal rewards owned and share authored objectives through native HUD queues."""
from pathlib import Path
import hashlib
import re
import struct

INCLUDE='#include "xbe_coop_rewards.h"\n'
HOOKS={
    4:[('\t// 00038FA9 mov dword ptr [esi+0x9C], 0x00','\tTrialCoopHeadshotCycleFinished(ctx,base);\n')],
    11:[('\t// 00073066 test ecx, ecx','\tTrialCoopHeadshotRecipient(ctx,base);\n'),
        ('\t// 00073066 test ecx, ecx','\tTrialCoopObjectiveRecipient(ctx,base);\n'),
        ('X86_FUNC(__imp__sub_00077720) {\n','\tconst bool trial_coop_objective_silent=TrialCoopObjectiveSilent(ctx,base);\n'),
        ('\t// 0007784B mov ecx, [0x0051C4DC]','\tif(trial_coop_objective_silent)goto loc_0007786F;\n'),
        ('\t// 00077D8F mov eax, [ebp+0x08]','\tif(trial_coop_objective_silent)goto loc_00077DB8;\n'),
        ('\t// 00077700 lea ecx, [esi+0x04]','\tTrialCoopObjectiveRetire(ctx,base);\n')],
    22:[('\t// 000DDCD6 mov [ecx+0xB8], eax','\tTrialCoopHeadshotDamage(ctx,base);\n')],
    37:[('\t// 00172396 mov [esi+0x300], eax','\tTrialCoopRewardHudCreated(ctx,base);\n'),
        ('\t// 0017239C mov edx, [0x0051C4DC]','\tTrialCoopObjectiveFlush(ctx,base);\n')],
}
# Raw original bytes and independently pinned lifted bodies, not fixture copies
# of the helper. Construction/list publication run through these original bodies.
BODIES=(
    (11,'000776B0',98,'9a15d9a6aa0c3092e93dcec52874146cd14e4f9cd5a950c477b3961f378c1fb0','e402c1993781c7e449a5cac6350f82f84b58a88c4c972ec10644131f8f247261'),
    (56,'00228AD0',57,'02bb98be839e6906981e8cdbfa0cdb91fd210a13fc04773c6b76c9d124ed1142','f4d623a7d20b636e63256482eecdbf3cd3d4a78370cf158f84453b3650ecfad8'),
    (22,'000DDCA0',63,'6747108d8d4f39ea73301a7220a2317c68cd95bf4f21558ff39e9a6c4e026186','2e6d85706df168e035a87196cb6001bc92841ccc8ed22f4f7df43df462f7073c'),
    (4,'00036800',178,'1f906ead035e8998bbf053478394208e60437e609029d5d586535476167bbb21','fd49a20dcc6f5dfa9abb16e97d0c3457e762db0b2a4cfaed69986ae61dffe268'),
    (4,'00036760',101,'08fbb87f984ed59db6bfde520dac89aba9ecd640773b4e7f6bb249c8fb9b611c','52db1909aa903af7495a96e541c1a2861f82e41980d38ab0a0570ca82a5a02e0'),
    (11,'00073060',41,'9dbcda37bba9ebadf6a238e3bddec335b78a5be94c96261a39a0a21b2ea38789','087445535e851af02b0452af77100d84a5a0180accb5cd2bd474bd4f3567d44c'),
    (11,'000773D0',127,'cd145e7a2318727ded1dd90aa71107010d853e9d3f3d335aa4d4be4039131c9b','ac1a1809bc801493b45aaa83dd2a3c74721da253863e4d5004bb70df97c7ce60'),
    (11,'00077E60',89,'c8f5b6468c481e801e02d39c643c9f116b4a2652ea4d830dc01e5719d0765a04','9891a3af6088cd86363e22af8c130880c1bdefbdd8c0d180b6e0d0fa85aa74c2'),
    (11,'00073B30',78,'e6ad2a4903a862dd8e34d4bbf949997dc7bdbda497a36f7e578888f215c2f075','5f4ee11604965563f7659d328b2aadc63d788f010fe495dea156408deb812045'),
    (11,'00077610',147,'fe02386e6ba9415720a92788d8625196f215d0c185ab140414b9b2869cb28f68','bf2030adc828b223f4dae9a829c796a78d81d41f0189b3afd1a361a532252dbc'),
    (11,'00072B50',60,'afadb1353b883d95b024a9df1eab40feb131e45afdf63c01826869ce55784691','1b5792ef6c36222bd95b37157dd08c32031ecf6dda7067612b37fa9569fd32f7'),
    (50,'001FB130',26,'13bafaa084785e2c86f83e64e5312117d209eb8cc4676fa3449884295278b5d8','89ff5adfc590f5f925542259dd52f0fb97866fc42fca6c91678d33ba2fe57dde'),
    (50,'001FB150',48,'7525f2186725ea11c138843659b268b5ecee5a48531fb22f1afbe5cea37f859c','91532d2a0187b8e796751da16c5bc26f1395726e263df41578d6e477558175a2'),
    (75,'002C3740',26,'3e6a98d24c28be342f90300b0c38383288e6c0e546777bc11221f475e4b2594e','2c6b68b800e74c4df13f5ddda99837342a72f4f0605e390fa477d7ece24808c7'),
)
PINS=(
    # Ten original objective publications, including exact type/-1/null-value
    # arguments; no personal headshot, stun or evidence publisher is admitted.
    (0xe8ac5,36,'86c24eceb7b12ef6502d42a54aba0851408e4bc187497c3e26114af6be014dda'),
    (0xe8b42,42,'cede6e61575626c58c382c89b17ae92ea2443e5369beb9f0659d2b516724a741'),
    (0xe8c3a,30,'b404fb802dc5d534fdb0053b27bfbfcb51ff15f4e186a2e88ef7b95965124c9a'),
    (0xe8c86,30,'e382e02c48889d03cbfd1ba5ce919bd12960d216872cba3824f8e10e27e66eb7'),
    (0x128f10,232,'f7abb18b17e4985135d04014be4fe4ae1883fb94ee1b6989426800b0b96a7446'),
    (0x1d6650,232,'bf81ffcf04ec761ccd5fc0f083e4ed18d98ff3755cb35262b424277efc8c2711'),
    (0x1e3162,192,'1a73dc5cfef263b9eebb7235cc471907bb04fe4845cae82e8b9c3d723d2fa681'),
    (0x77720,19,'6e6d1464e879f2af540538e7ab13a1b295e22b862344e142df058cd3487d7e24'),
    (0x77840,47,'860d0409ccc3816934143101c0f0fd8990792abf4e56905609ba60aaa2809f7d'),
    (0x77bf8,12,'74c1c6ba05f0ac997cba937681f587a1f0c74c99409c5994ba658e868976f692'),
    (0x77d8f,41,'7921db55199c39209ecb951630adfa7021c54d24fa0cab6ff569a1bd5d889965'),
    (0x776fb,19,'5ea8bcd1447ad0edd4a294c2d3f68a76b5e2ec554c63480516b08da1a21e8af1'),
    (0x172396,18,'7d3e07feac25dceab1a42d1b9e536418008217c2ddd41dc9b2733ccaa6310dea'),
    # The pending-damage consumer calls +6C (371B0/derived wrapper) before
    # +70 (37D10); only the latter can replace native controller+B8.
    (0x44da2c,12,'89f4543cf8b1801969c22594a3236e963a059f8ff67b71d05cd8a6ad526ac6be'),
    (0x38be5,17,'ea7a4bef5d3dae6936369e41184be245a317af90d64d11d0bdd5a90ec5ffc3bf'),
    (0x38f95,17,'93512732a5dfee596f9a69715628895601a0b81db58f24d3d9b3ddf63c97fcd4'),
    (0x38fa6,21,'0475443203eaa19336be0f39593450690126dcef7f404fcce01c135c62797cc1'),
    (0x37578,361,'5e8f3e31040428a62560a9b3765d5eb8de397ba2f708bdfd6e223166ea6ad40d'),
    (0x37228,41,'67037201570635ad7680d88afe2baac892aa78ab8f4008b60d2ad68839e7a335'),
    (0x375c9,20,'938866130cba290cea9b22912b2ce31c5aaec20adaa1e7876fbb84c38af4fe27'),
    (0x70d90,455,'879111defca52dd723604dadc820a063cf5705351be989ed3f38d58867be6671'),
    (0x72fa0,86,'5537f383f41a939a348bbeea5a2763dcf0358030bf4898f18afcac888c34aec4'),
    (0x172380,104,'8dda0ed13040f9fab66e366e01e6e12e07c0ee3e7be3c76b23443de344962b49'),
    (0x37622,16,'ae30ad97674b809dab945e6450b9af5762bd2156ccc9a27d99919ebd3e4b8fe4'),
    (0x3767c,16,'6213bd275564a7e0c0c9d0a3352c8a3d61c20a53d268d384293ee681267280be'),
    (0x376ce,16,'11909e134d8b4a288b62f3602a48aa0e4ccb37f8de816a133c39c90f15224123'),
)

def clean(source,unit):
    source=source.replace(INCLUDE,'')
    if unit==11:source=source.replace('\tTrialCoopShareHeadshot(ctx,base);\n','')
    for _,hook in HOOKS.get(unit,[]):source=source.replace(hook,'')
    return source

def canonical_source(source,unit):
    """Pin/fixture view only: never use this to erase another runtime hook."""
    source=clean(source,unit)
    if unit==11:
        for hook in ('\tTrialCoopEvidencePopup(ctx,base);\n','\tTrialCoopKORecipient(ctx,base);\n'):
            source=source.replace(hook,'')
    if unit==22:source=source.replace('\tTrialCoopKODamage(ctx,base);\n','')
    if unit==4:source=source.replace('\tTrialCoopKOCycleFinished(ctx,base);\n','')
    return source

def unprepare(area):
    for unit in HOOKS:
        path=Path(area)/f'generated/game/x86_recomp.{unit}.cpp'
        if path.exists():
            before=path.read_text();after=clean(before,unit)
            if before!=after:path.write_text(after)

def prepare(area,sdk):
    area=Path(area);local=Path(__file__).parent
    sdk=Path(sdk)
    xbe=(area/'assets/Default.xbe').read_bytes();base=struct.unpack_from('<I',xbe,0x104)[0]
    count,table=struct.unpack_from('<II',xbe,0x11c)
    sections=[struct.unpack_from('<6I',xbe,table-base+i*56)for i in range(count)]
    for va,size,digest in PINS+tuple((int(a,16),size,raw)for _,a,size,raw,_ in BODIES):
        s=next(s for s in sections if s[1]<=va and va+size<=s[1]+s[4]);offset=s[3]+va-s[1]
        assert hashlib.sha256(xbe[offset:offset+size]).hexdigest()==digest,hex(va)
    sources={unit:clean((area/f'generated/game/x86_recomp.{unit}.cpp').read_text(),unit)
             for unit in set(HOOKS)|{u for u,_,_,_,_ in BODIES}}
    test=(local/'test_xbe_coop_rewards.cpp').read_text()
    for unit,address,_,_,digest in BODIES:
        body=re.search(r'X86_FUNC\(__imp__sub_'+address+r'\) \{.*?^\}',canonical_source(sources[unit],unit),re.M|re.S).group()
        assert hashlib.sha256(body.encode()).hexdigest()==digest,address
        marker='// ORIGINAL_'+address;assert test.count(marker)==1,marker
        test=test.replace(marker,body)
        if address=='00073060':
            patched=body.replace('__imp__sub_00073060','CoopPopup',1)
            for anchor,hook in HOOKS[11][:2]:patched=patched.replace(anchor,hook+anchor,1)
            marker='// COOP_POPUP\n';assert test.count(marker)==1
            test=test.replace(marker,patched+'\n')
        if address=='000776B0':
            anchor,hook=HOOKS[11][5]
            patched=body.replace('__imp__sub_000776B0','CoopQueueUpdate',1).replace(anchor,hook+anchor,1)
            marker='// COOP_QUEUE_UPDATE\n';assert test.count(marker)==1
            test=test.replace(marker,patched+'\n')
        if address=='000DDCA0':
            anchor,hook=HOOKS[22][0]
            patched=body.replace('__imp__sub_000DDCA0','CoopDamage',1).replace(anchor,hook+anchor,1)
            marker='// COOP_DAMAGE\n';assert test.count(marker)==1
            test=test.replace(marker,patched+'\n')
    # Execute the linked runtime's real call ABI in the fixture; only guest
    # allocation and unrelated audio/widget endpoints are test boundaries.
    dispatch=(sdk/'runtime/cpu/dispatch.cpp').read_text()
    body=re.search(r'uint32_t CallGuest\(X86Context& ctx, uint32_t fn, const uint32_t\* args, size_t count\) \{.*?^\}',dispatch,re.M|re.S).group()
    assert hashlib.sha256(body.encode()).hexdigest()=='1e3a800968e6deb99ddc92052680c56a77c359aba6d7e0471b343fb0b4e0f16b'
    marker='// ORIGINAL_CALL_GUEST';assert test.count(marker)==1
    test=test.replace(marker,body)
    source=canonical_source(sources[11],11)
    for name,start,end,digest,seam in (
        ('SOUND','00077D8F','00077DB8','f531b2edad7c9e3e39816acce5eb6e4818c1f15ba405a38bf7161f2d3c80632f',4),
        ('SWOOSH','00077840','0007786F','1113fc303e6e14833c5b92eab0012a10f90482bc8a6535ffcfb0190e46663364',3)):
        a=source.index('\t// '+start+' ');z=source.index('loc_'+end+':',a);body=source[a:z]
        assert hashlib.sha256(body.encode()).hexdigest()==digest,start
        for prefix in ('Original','Coop'):
            code=body
            if prefix=='Coop':
                anchor,hook=HOOKS[11][seam];code=code.replace(anchor,hook+anchor,1)
            marker='// '+('ORIGINAL'if prefix=='Original'else'COOP')+'_OBJECTIVE_'+name
            assert test.count(marker)==1,marker
            test=test.replace(marker,'void '+prefix+'Objective'+name.title()+'(X86Context& ctx,uint8_t* base,bool trial_coop_objective_silent) {\n'
                '\tX86_LOAD_FLAGS();\n\tuint32_t ca=0,cb=0;\n'+code+'loc_'+end+':;\n}\n')
    source=canonical_source(sources[4],4);a=source.index('\t// 00037578 ');z=source.index('\t// 000376E1 ',a)
    body=source[a:z]
    assert hashlib.sha256(body.encode()).hexdigest()=='ae6e23e8ded7a14cb1b200511c74674ef07a23485136d67de31abad9906988c2'
    test=test.replace('// ORIGINAL_HEADSHOT_PUBLICATION','void OriginalHeadshotPublication(X86Context& ctx,uint8_t* base) {\n\tX86_LOAD_FLAGS();\n\tuint32_t ca=0,cb=0;\n'+body+
        ';\n\treturn;\nloc_00037802:;loc_000377EA:;loc_000377C3:return;\n}\n')
    a=source.index('\t// 00037228 ');z=source.index('\t// 00037251 ',a);body=source[a:z]
    assert hashlib.sha256(body.encode()).hexdigest()=='ed900392327a4305fc20de7917d56a80fc0080d9871fdac0346e03195dcce89a'
    test=test.replace('// ORIGINAL_HEADSHOT_ELIGIBILITY','void OriginalHeadshotEligibility(X86Context& ctx,uint8_t* base) {\n\tX86_LOAD_FLAGS();\n\tuint32_t ca=0,cb=0;\n'+body+'}\n')
    a=source.index('\t// 00038FA6 ');z=source.index('\t// 00038FBB ',a);body=source[a:z]
    assert hashlib.sha256(body.encode()).hexdigest()=='12e38a3ccb884dc01b5b97250540b6d932b303edde7a5afd5a4df36ebf512af0'
    anchor,hook=HOOKS[4][0];body=body.replace(anchor,hook+anchor,1)
    test=test.replace('// COOP_DAMAGE_END','void CoopDamageEnd(X86Context& ctx,uint8_t* base) {\n'+body+'}\n')
    source=sources[37];body=source[source.index('\t// 00172380 '):source.index('\t// 001723A2 ')]
    assert hashlib.sha256(body.encode()).hexdigest()=='3907952f7b7460bd9845fd10932efe7140da84765500caba9572f0078ef3281c'
    for anchor,hook in HOOKS[37]:body=body.replace(anchor,hook+anchor,1)
    test=test.replace('// ORIGINAL_HUD_CREATION','X86_FUNC(OriginalHudCreation) {\n\tX86_LOAD_FLAGS();\n\tuint32_t ca=0,cb=0;\n'+body+
        '\tctx.esi=X86_MEM32(ctx.esp);ctx.esp+=8;\n}\n')
    for unit,seams in HOOKS.items():
        source=sources[unit]
        for anchor,hook in seams:
            assert source.count(anchor)==(2 if unit==4 else 1)
            source=source.replace(anchor,anchor+hook if anchor.startswith('X86_FUNC')else hook+anchor)
        source=source.replace('#include "x86_recomp_shared.h"\n','#include "x86_recomp_shared.h"\n'+INCLUDE,1)
        (area/f'generated/game/x86_recomp.{unit}.cpp').write_text(source)
    for name in ('xbe_coop_rewards.h','xbe_coop_rewards.cpp'):
        (area/'src/common'/name).write_text((local/name).read_text())
    (area/'src/compat/test_coop_rewards.cpp').write_text(test)

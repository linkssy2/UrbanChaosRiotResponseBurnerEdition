"""Port accepted corpse retention to the current runtime's guest ownership ABI."""
from pathlib import Path
import hashlib
import struct

ENTRIES={
    0x2a5030:(59,'29ed19a97fcf15d36a70264e2d1c14ed2c86dc38c6b723879119754dcb474dc9'),
    0x2a5070:(38,'4836f917369135aa304990c9e275ffd8e6c652920cd3653adc15d5d783b43b5c'),
    0x2a5d50:(74,'a491f508694cfb779bbbd893e36ec5de097d98f84aa3248e8126887ab8d7f68e'),
    0x2a6790:(179,'c67e973f6cd690becc64bf3a386b52d2c971f28f8df11ce238769a6e04fc64ec'),
    0x2a6850:(68,'aa767e75f136c2b2295f96d9a2df35a64f773b57b566f1468e68685a27aa0bd6'),
    0x2a6a70:(146,'9d074fc2253e3928ac6ad3c40408ca2a4e0bbf71868b6d44601f19ce0cbb7504'),
}
OBJECT_ENTRIES={
    0x238430:(14,'3393b576a79c6127c78ff954fd536ef1cf8a245ae9b1eb788fae5721703d7b93'),
    0x238440:(116,'e87da8fd42f7907730765dbe99698ec5c658b603081440e6651061bef1d2f465'),
    0x2384f0:(38,'8ba1e43f36bb3e584810f2e25306d13703c842700e55d315b4369767081346f4'),
}
OBJECT_WITNESS={
    0x2384c0:(11,'2f3f0b515d7f39773eab490d991949899d9978c2dbff8cb500e815a0cf8c69eb'),
    0x2383c0:(103,'28c08d8e4c5dd65c4420abc656d06dfef0c85f2ca209fb8f1b25a902d67ced60'),
    0x2ba100:(102,'881b2e2100245cec722b469752f9115bd6208939924577e1dd28467c6e690527'),
    0x2ba020:(86,'b516787a17986bd9c5ff424bd7119b827d605887befbd14904c488e212dd7ebb'),
    0x2b9f40:(214,'a2ac1a8f31de334d4f056fa3af9bda2e98d41e0a97b35faf864620a1e1db818b'),
    0x3aac90:(23,'f18f012d079fdd827c43bf840b788bd83c56d6716b9f2a50cfa4cdda264e2e63'),
    0x2b9130:(92,'705fe0fa5793f587c5cbe301e6e32572272ddac140334b1e9b54aa8473c89c17'),
    0x2b8c00:(147,'6836aa14d6211a386af3dd900e361df07bd8aae9f50b4f5b3db71bc5af3354d8'),
    0x2b9080:(33,'b0e55a1030862e4e40eff64f5bb514b6e73a908b4d0a7ae5daf1924564920c9c'),
    0x2b90b0:(128,'d74cdefbf1c5168d6534c784df35584d948ba87c26fcb21a990bafbfa3bec22a'),
    0x2b8650:(8,'1e738e4f1e5aaca5fea4f8cdde8dd7d6bcaee6f9c33e787be1393faca721125b'),
    0x2b8b70:(33,'f0073b056b8beb547b29c79b2c51d0e51790cd1549391f2a44924168713746c9'),
    0x2b8ba0:(94,'7f2d607083c4f2b94a0e9bf7350547743830f3792e4d3139e7322dae20122668'),
    0x3aac50:(23,'d5e3cace392c0406b705cf6b1a766e6a3364d76f9d83fbaba3a062cadb3b2be8'),
    0x3aac70:(26,'1abf1905c0e83a23b88c47c46df886cfc2b854b0d6412b2f320c90a28b5c4f8d'),
    0x3aa9f0:(26,'992d9359e3f17599efb44a7cba06a57e10e43e9ada4a5514c96753695ec31b0f'),
    0x3aaa90:(23,'92c87c441dfb897f825e8b8175eacb1ab9254873aedbfa756ef782150d9c851a'),
    0x3aa890:(26,'382de447f9f6ab9f7084c5992a23fc82e9b0103e3ad045f64328f5a23b117abf'),
    0x3a8630:(26,'2a187a08799badb084dc028cb1a84576a740f33b2208d829d4e4ec5246a3171c'),
    0x3a79c0:(23,'e127c6146aa35dde9404fac4fb451a1e520e975539be658a079f571f40239275'),
}
def body(source,address):
    first=f'X86_FUNC(__imp__sub_{address:08X}) {{'
    start=source.index(first)
    return source[start:source.index(f'\nX86_WEAK_FUNC(sub_{address:08X}',start)]
def once(source,old,new):
    if new in source:return source
    assert source.count(old)==1,old
    return source.replace(old,new,1)
def prepare(area):
    area=Path(area);local=Path(__file__).parent
    xbe=(area/'assets/Default.xbe').read_bytes();base=struct.unpack_from('<I',xbe,0x104)[0]
    count,table=struct.unpack_from('<II',xbe,0x11c)
    sections=[struct.unpack_from('<6I',xbe,table-base+i*56)for i in range(count)]
    def image(va,size):
        s=next(s for s in sections if s[1]<=va and va+size<=s[1]+s[4]);at=s[3]+va-s[1]
        return xbe[at:at+size]
    for va,(size,digest) in (ENTRIES|OBJECT_ENTRIES|OBJECT_WITNESS).items():assert hashlib.sha256(image(va,size)).hexdigest()==digest,hex(va)
    name=b'Body::ManualBlobShadow\0'
    assert image(0x48f4f0,len(name))==name
    assert image(0x29e4f0,9)==bytes.fromhex('a110d951008b4010c3')
    for va,data in [(0xe7953,'0f2f053cc14400'),(0x3cbd4,'8b56088b3e528bcbe86ffcffff8bf7eb02'),(0x3cc26,'3bea0f47ea85ed7456')]:
        assert image(va,len(bytes.fromhex(data)))==bytes.fromhex(data),hex(va)
    include='#include "xbe_ragdoll_settings.h"'
    def header(s):return s if include in s else once(s,'#include "x86_recomp_shared.h"','#include "x86_recomp_shared.h"\n'+include)
    path=area/'generated/game/x86_recomp.71.cpp';pool=header(path.read_text())
    for va in ENTRIES:
        old=f'X86_FUNC(__imp__sub_{va:08X}) {{\n'
        pool=once(pool,old,old+f'\tif(TrialRagdollPool(ctx,base,0x{va:x}))return;\n')
    path.write_text(pool)
    path=area/'generated/game/x86_recomp.59.cpp';objects=header(path.read_text())
    # Undo the previous candidate's query override. Explicit particle/render
    # exhaustion checks must retain the original fixed-capacity result.
    objects=objects.replace('\tif(TrialRetainedObjectPool(ctx,base,0x2384c0))return;\n','')
    original_alloc=body(objects,0x238430).replace('\tif(TrialRetainedObjectPool(ctx,base,0x238430))return;\n','')
    for va in OBJECT_ENTRIES:
        old=f'X86_FUNC(__imp__sub_{va:08X}) {{\n'
        objects=once(objects,old,old+f'\tif(TrialRetainedObjectPool(ctx,base,0x{va:x}))return;\n')
    path.write_text(objects)
    path=area/'generated/game/x86_recomp.4.cpp';npc=header(path.read_text())
    npc=once(npc,'\t// 0003CBD4 mov edx, [esi+0x08]',
        '\tif(TrialRagdollKeepUnseen(X86_MEMF32(ctx.esi+0x10)))goto loc_0003CBE5;\n\t// 0003CBD4 mov edx, [esi+0x08]')
    npc=once(npc,'loc_0003CC26:\n',
        'loc_0003CC26:\n\tctx.ebp=TrialRagdollRetireCount(ctx.ebp,ctx.edx);\n')
    path.write_text(npc)
    path=area/'generated/game/x86_recomp.23.cpp';age=header(path.read_text())
    old='\t// 000E7953 comiss xmm0, [0x0044C13C]\n\t{ uint32_t ea = (uint32_t)(0x44C13Cu); float a = ctx.xmm[0].f[0], b = X86_MEMF32(ea);'
    new='\t// 000E7953 comiss xmm0, [0x0044C13C]\n\t{ float a = ctx.xmm[0].f[0], b = TrialRagdollSeconds();'
    age=once(age,old,new);path.write_text(age)
    for name in ('xbe_ragdoll_settings.h','xbe_ragdoll_settings.cpp'):
        (area/'src/common'/name).write_text((local/name).read_text())
    world=(area/'generated/game/x86_recomp.70.cpp').read_text()
    generated=body(world,0x29e4f0).replace('__imp__sub_0029E4F0','sub_0029E4F0',1)+'\n'
    generated+='\n'.join(body(pool,va).replace(f'__imp__sub_{va:08X}',f'sub_{va:08X}',1)for va in ENTRIES)
    generated+='\n'+original_alloc.replace('__imp__sub_00238430','original_00238430',1)
    generated+='\n'+'\n'.join(body(objects,va).replace(f'__imp__sub_{va:08X}',f'sub_{va:08X}',1)for va in [0x2383c0,0x2384c0,*OBJECT_ENTRIES])
    controllers=(area/'generated/game/x86_recomp.73.cpp').read_text()
    generated+='\n'+'\n'.join(body(controllers,va).replace(f'__imp__sub_{va:08X}',f'sub_{va:08X}',1)for va in (0x2ba100,0x2ba020,0x2b9f40,0x2b9130,0x2b8c00,0x2b9080,0x2b90b0,0x2b8650,0x2b8b70,0x2b8ba0))
    retire=npc[npc.index('loc_0003CC26:'):npc.index('\t// 0003CC2F nop')]
    age_test=age[age.index('\t// 000E7953 comiss'):age.index('loc_000E7960:')]
    unseen=npc[npc.index('\t// 0003CBC6 movss'):npc.index('\t// 0003CBD4 mov edx')]
    constants='\n'.join(f'W32(0x{a:x},0x{struct.unpack("<I",image(a,4))[0]:x});'for a in (0x44c13c,0x44e11c,0x44bf94))
    fixture=(local/'test_xbe_ragdoll_settings.cpp').read_text()
    for marker,content in [('// GENERATED_ORIGINAL',generated),('// GENERATED_RETIRE',retire),('// GENERATED_AGE',age_test),('// GENERATED_UNSEEN',unseen),('// GENERATED_CONSTANTS',constants)]:
        assert fixture.count(marker)==1;fixture=fixture.replace(marker,content,1)
    (area/'src/compat/test_ragdoll_settings.cpp').write_text(fixture)

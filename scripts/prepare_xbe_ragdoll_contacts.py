"""Restore the accepted character/ragdoll contact policy in the retail filter."""
from pathlib import Path
import hashlib
import re
import struct

ROOT=Path(__file__).resolve().parents[1]
PINS={
0x1354A0:(31,79,'5326c078554494ae94892b3c6bf6afaa847aa4a07d160d9eabbcf8ed747dd9cf','40cd8bc5567bbd84dee40e60650ffaf49a290e1c335a6481a9c4e9abc6b68f76'),
0x2A8B60:(71,410,'6c9a23b70a92ac9ddca781bfcda07aa971aefe95ae40c0733c509b6a6afe8e44','5c8138194858460133cc0f85dba531ef7b1ccd941bf141518c9966a46d1b33d3'),
0x2A8920:(71,78,'bb7525e95afa45d734cc66ab58c7527f3fe7a4fcb7882d3a0893175d8166b083','6ffc4afe86c0ec369ffb49fa903819325cf0383d0095e1c9e17498aca53b8d38'),
0x2A89B0:(71,176,'90678346d12c9b55642066d9e6ebf7467181b17b422e70ee5e8f0069307fe4aa','9fcd78b81cb3de90f7cc848eb5cf23eb15a2de5fcdaeb834c54fb9151c050665'),
0x2A8970:(71,53,'1bd97ea82d92c2d732cb36d197c693aa50d3d8e0ea7b84dbb220ce4b603a7dc2','8e5d4353f2f09b40860c509fd43ed8b8269095d08b5c95b25b7ed5950d9802bc'),
0x2A8AF0:(71,100,'c073c87f34a24508175f18846b60092372e659bb951d197cdc44e9310437be64','81818f0cb26bf1c7263f7e7b3d3967b71bd9c10d40337ddc209f21504b84b423'),
0x2A8A60:(71,139,'d5472358b9bed42c61eafd38dcf485d91bda51967580e831c1b6e9f75b1b1d67','90ec8982fb8a075a3f1c39c40ff524b0f870abb0740574ec83619df334e5134e'),
0x2A8670:(71,186,'35482d838e0af829c8e2de9936e0db57f21e290adad0fe051a9fb782bb6aa88a','998eece662a6d04fdef3152c7afb8fb084bf8fefeae9c01b9141a6df24a686dc'),
0x2A3090:(71,51,'0503795594f89f9527fd8be859d4ef4288f51525f17e3a58425ca8515f6cc518','bdc1ca644fb4d4ced922af0b5707a246df85bb20502746fee2e97070c2b54ddc'),
0x2A31C0:(71,28,'f41e280fd1f35f3ed32b0b55d7c5baa05912a195f1f476de641e19a92b247c1b','1d5aadffb60f2b0162cfd791bb4c95121fac8b75e95fa0c2b772eb1162926614'),
0x2EDBE0:(81,10,'1b29e5c75b1678cc4c4a0a94119b5ae94b2f2fa3ac98f07ded2d28a6b207ec04','a12c5761193f0bbb538c74b65a8c5b3e1c5fed2ab9b26a596f4bf7ed042737c9'),
0x2FBB00:(83,84,'1ab8ba5d8d641c709322e6a979243143a80c6e56f9ee167726f05535041c7aa8','f12d3e961b781bd7a4d14899a5c05425ba2407ad337ba2471be26b72253bd55b'),
0x2FBB60:(83,84,'1d2b093bf521c297d931f39eddaf48e208be255e9a4b0eebd19e721a489528ed','5a26b0241ded1a1db1e777ddd55b1de43d1984f7d6202af0f5c671c927a1dab5'),
0x319050:(86,105,'cd1da35c157ed57056583c985224f1296640520e2ba76669f1f0ad44503813ec','a4581cf664edb626d77c2935963b92f5089dd066c0ab2692a64f512dcd08aab7'),
0x3190C0:(86,56,'2e00775f90c8bd4d4620cedc5a2281a6a5bc499c7acaefd1883717a2297d18fe','8ad30bfa694b015ef76ab1495dd827d9de7beebfb02ba562ed8ff1eb18a9bc89'),
}
OLD='\t// 002A8A0A jz 0x002A8A39\n\tif ((uint32_t)ca == (uint32_t)cb) { goto loc_002A8A39; }'
NEW='\t// Admit ragdoll bodies to character contacts; retain all other retail filters.'

def prepare(area):
    area=Path(area)
    paths={unit:area/f'generated/game/x86_recomp.{unit}.cpp' for unit,_,_,_ in PINS.values()}
    original={unit:path.read_text().replace(NEW,OLD) for unit,path in paths.items()}
    xbe=(area/'assets/Default.xbe').read_bytes()
    base=struct.unpack_from('<I',xbe,0x104)[0]
    count,table=struct.unpack_from('<II',xbe,0x11c)
    sections=[struct.unpack_from('<6I',xbe,table-base+i*56) for i in range(count)]
    def image(address,size):
        section=next(s for s in sections if s[1]<=address and address+size<=s[1]+s[4])
        offset=section[3]+address-section[1]
        return xbe[offset:offset+size]
    # The ragdoll constructor assigns property 1 = 4 to each authored bone body.
    assert image(0x2A64F9,10)==bytes.fromhex('6a046a0155e81dccffff')
    bodies={}
    for address,(unit,size,raw_hash,body_hash) in PINS.items():
        assert hashlib.sha256(image(address,size)).hexdigest()==raw_hash,hex(address)
        body=re.search(rf'X86_FUNC\(__imp__sub_{address:08X}\) \{{.*?^\}}',original[unit],re.S|re.M)[0]
        assert hashlib.sha256(body.encode()).hexdigest()==body_hash,hex(address)
        bodies[address]=body
    gate=bodies[0x2A89B0]
    assert gate.count(OLD)==1
    bodies[0x2A89B0]=gate.replace(OLD,NEW,1)
    staged=original[71].replace(gate,bodies[0x2A89B0],1)
    declarations='\n'.join(f'X86_EXTERN_FUNC(sub_{a:08X});' for a in bodies)
    extracted='\n\n'.join(body.replace('X86_FUNC(__imp__sub_','X86_FUNC(sub_',1) for body in bodies.values())
    baseline=gate.replace('__imp__sub_002A89B0','original_gate',1)+'\n'+bodies[0x2A8B60].replace('__imp__sub_002A8B60','original_filter',1).replace('sub_002A89B0(ctx','original_gate(ctx')
    fixture=(ROOT/'scripts/test_xbe_ragdoll_contacts.cpp').read_text()
    assert fixture.count('// RETAIL_FUNCTIONS')==1
    target=area/'src/compat/test_ragdoll_contacts.cpp'
    target.parent.mkdir(parents=True,exist_ok=True)
    target.write_text(fixture.replace('// RETAIL_FUNCTIONS',declarations+'\n'+extracted+'\n'+baseline))
    if paths[71].read_text()!=staged:paths[71].write_text(staged)

if __name__=='__main__':prepare(ROOT/'build/xbe-full-integration-20261014')

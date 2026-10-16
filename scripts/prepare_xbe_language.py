"""Select the disc's original language at its qualified startup cache boundary."""
from pathlib import Path
import hashlib
import re
import struct


def once(source,old,new):
    assert source.count(old)==1,old
    return source.replace(old,new,1)


def between(source,begin,end):
    start=source.index(begin)
    return source[start:source.index(end,start)]


def body(source,address):
    return between(source,f'X86_FUNC(__imp__sub_{address:08X}) {{',
                   f'\nX86_WEAK_FUNC(sub_{address:08X}')


def prepare(sdk,area):
    """After prepare_io_apc and graphics settings; root owns build and init wiring."""
    sdk,area=Path(sdk),Path(area);local=Path(__file__).parent
    data=(area/'assets/Default.xbe').read_bytes()
    base=struct.unpack_from('<I',data,0x104)[0];n,table=struct.unpack_from('<II',data,0x11c)
    sections=[struct.unpack_from('<6I',data,table-base+i*56) for i in range(n)]
    def read(va,size):
        section=next(s for s in sections if s[1]<=va and va+size<=s[1]+s[4])
        at=section[3]+va-section[1];return data[at:at+size]
    for va,size,digest in [
        (0x16c00,104,'d4ab1e78f02aad4c09d962b8e8518c73ce55d86b3ca04c86ca2f11e912922aa2'),
        (0x16ea1,7,'5a4805a858cc4dc9efce72d76c182d520ed9cfadad83850e52454f9be33b062c'),
        (0x16eea,5,'da648da7c4a945d5edd6f29cfbb97d740cbbb18d0102621afae994c23209cf67'),
        (0x800e7,45,'80de606dc64cf67e249dc6cece933d7adbb97a6e379d728a8e1d49d51efb8c9d'),
        (0x7fa5b,41,'6e8a564437da2f6c737bd122bf2c9a466dffab75e08ffb16a091d89773e9b093'),
        (0x17790,350,'2d0e919b85202ff63ff3f0a754d2783b37995cf0f0a0439bc432327e06df6959'),
        (0x5cc6e,25,'0d5ca7260c220c297326964fc9d6dd1f4e4a60621870f53100ae2129ed935c9d'),
        (0x8da07,50,'40a0a5ece2e355980bd64ec1d7b5c1f9b92b2b9d7cc721841281842037eba7eb'),
        (0x875ff,23,'03bc262e42a0fc3898abc18fff8cc2eb65cc3aad31275b7f858796549540a628')]:
        assert hashlib.sha256(read(va,size)).hexdigest()==digest,hex(va)
    for name in ('xbe_language.h','xbe_language.cpp'):
        (area/'src/common'/name).write_text((local/name).read_text())
    path=area/'generated/game/x86_recomp.0.cpp';source=path.read_text()
    source=re.sub(r'^.*// uc_language_startup\n','',source,flags=re.M)
    original=body(source,0x16c00)
    source=once(source,'#include "x86_recomp_shared.h"\n',
                '#include "x86_recomp_shared.h"\n#include "../../src/common/xbe_language.h" // uc_language_startup\n')
    source=once(source,'\t// 00016C00 ',
                '\tTrialLanguageStartup(ctx,base); // uc_language_startup\n\t// 00016C00 ')
    path.write_text(source)
    # Keep both supported EEPROM query forms consistent with the frozen choice.
    kernel_path=area/'src/compat/xboxkrnl.cpp';kernel=kernel_path.read_text()
    include='#include "../common/xbe_language.h" // uc_language_kernel\n'
    if include not in kernel:kernel=include+kernel
    for old,new in [
        ('case 7: v = 1; break;                      // XC_LANGUAGE english',
         'case 7: v = trial_language::XboxLanguage(); break; // XC_LANGUAGE'),
        ('W32(value + 0xE8, 1);                     // language',
         'W32(value + 0xE8, trial_language::XboxLanguage()); // language')]:
        if new not in kernel:kernel=once(kernel,old,new)
        assert kernel.count(new)==1
    kernel_path.write_text(kernel)
    # Existing production Save is atomic and throws on failure; do not regress
    # language rollback to the upstream unchecked/truncating ofstream helper.
    settings=(area/'src/compat/settings.cpp').read_text()
    assert 'MOVEFILE_REPLACE_EXISTING' in settings and 'f.exceptions(' in settings
    sdk_source=(area/'generated/game/x86_recomp.12.cpp').read_text()
    fragments=[0x800e7,0x800e8,0x800ea,0x800eb,0x800ec,0x800ee,
               0x800f0,0x800f3,0x800f4,0x800f8,0x800fa,0x800ff]
    extracted='\n'.join(f'X86_EXTERN_FUNC(sub_{va:08X});' for va in fragments)
    extracted+='\n'+body(sdk_source,0x7fa5b).replace('__imp__sub_','sub_')
    extracted+='\n'+'\n'.join(body(sdk_source,va).replace('__imp__sub_','sub_') for va in fragments)
    extracted+='\n'+original.replace('__imp__sub_00016C00','OriginalLanguage')
    extracted+='\n'+body(source,0x16c00).replace('__imp__sub_00016C00','SelectedLanguage')
    extracted+='\n'+body(source,0x17790).replace('__imp__sub_00017790','OriginalBank')
    movie=(area/'generated/game/x86_recomp.13.cpp').read_text()
    extracted+='\nX86_FUNC(OriginalMovieMask) {\nuint32_t ca=0,cb=0; (void)ca; (void)cb;\nX86_LOAD_FLAGS();\n'+between(movie,'\t// 000875FF ','\t// 00087616 ')+'}\n'
    kernel_body=between(kernel,'KERNEL_EXPORT(24, ExQueryNonVolatileSetting, 5) {','\nKERNEL_EXPORT(1, AvGetSavedDataAddress')
    kernel_body=kernel_body.replace('KERNEL_EXPORT(24, ExQueryNonVolatileSetting, 5)',
                                    'static uint32_t QuerySettings(const uint32_t* a)')
    test=(local/'test_xbe_language.cpp').read_text()
    for marker,replacement in [('/* PRODUCTION_LANGUAGE */',(local/'xbe_language.cpp').read_text()),
                               ('/* PRODUCTION_QUERY */',kernel_body),('/* ORIGINAL_FUNCTIONS */',extracted)]:
        test=once(test,marker,replacement)
    (area/'src/compat/test_language.cpp').write_text(test)

"""Extend full-generation entity handles beyond the original inline slots."""
from pathlib import Path
import hashlib, json, re

ROOT = Path(__file__).resolve().parents[1]
INCLUDE = '#include "xbe_entity_registry.h"\n'
HOOKS = [
    ('\t// 00207C97 or eax, 0xFFFFFFFF\n', '''\t// A full inline table must not publish an unregistered entity.
\tctx.eax = trial_entity_registry::AllocateExtra(ctx, base, ctx.ecx, X86_MEM32(ctx.esp + 8));
\tctx.edx = X86_MEM32(ctx.ecx + 0x2000);
\tctx.esi = X86_MEM32(ctx.esp); ctx.esp += 12; return;
''', 1),
    ('\t// 00207C9E mov esi, [esp+0x08]\n', '\tif (trial_entity_registry::HasExtra(ctx.ecx, ctx.eax)) goto loc_00207C8E;\n', 1),
    ('\t// 00207D92 xor eax, eax\n', '\tif (uint32_t object = trial_entity_registry::FindExtra(ctx.ecx, ctx.edx)) { ctx.eax = object; ctx.esp += 8; return; }\n', 1),
    ('\t// 00207DC0 cmp [eax+ecx*8], edx\n', '\tif (uint32_t object = trial_entity_registry::FindExtra(ctx.eax, ctx.edx)) { ctx.eax = object; goto uc_registry_00207DC9; }\n', 1),
    ('\t// 00207DC9 test eax, eax\n', 'uc_registry_00207DC9:\n', 1),
    ('\t// 00207F18 cmp [edx+eax*8], edi\n', '\tif (uint32_t object = trial_entity_registry::FindExtra(ctx.edx, ctx.edi)) { ctx.esi = object; goto uc_registry_00207F21; }\n', 1),
    ('\t// 00207F21 test esi, esi\n', 'uc_registry_00207F21:\n', 1),
    ('\t// 00208055 cmp [eax+esi*8+0x3C], ebx\n', '\tif (uint32_t object = trial_entity_registry::FindExtra(ctx.eax + 0x3c, ctx.ebx)) { ctx.edi = object; goto uc_registry_0020805F; }\n', 1),
    ('\t// 0020805F test edi, edi\n', 'uc_registry_0020805F:\n', 1),
    ('\t// 0020806F mov ecx, [eax+esi*8+0x3C]\n', '\tif (trial_entity_registry::HasExtra(ctx.eax + 0x3c, ctx.ebx)) { trial_entity_registry::Release(base, ctx.eax + 0x3c, ctx.ebx); goto loc_00208081; }\n', 1),
    ('\t// 002081A9 cmp [eax+edx*8+0x3C], ecx\n', '\tif (uint32_t object = trial_entity_registry::FindExtra(ctx.eax + 0x3c, ctx.ecx)) { ctx.eax = object; goto loc_002081B7; }\n', 1),
    ('\t// 00208EED cmp [eax+edx*8], ecx\n', '\tif (trial_entity_registry::HasExtra(ctx.eax, ctx.ecx)) trial_entity_registry::Release(base, ctx.eax, ctx.ecx);\n', 3),
    ('\t// 00208F6D pop edi\n', '\ttrial_entity_registry::Reset(ctx.ebx + 0x3c);\n', 3),
    ('\t// 002096B0 mov [eax+0x2000], ebx\n', '\ttrial_entity_registry::Reset(ctx.eax); // 002096B0\n', 1),
    ('\t// 0020986F mov dword ptr [esi+0x2000], 0x00\n', '\ttrial_entity_registry::Reset(ctx.esi); // 0020986F\n', 2),
    ('\t// 002099FF mov [ecx+0x2000], ebx\n', '\ttrial_entity_registry::Reset(ctx.ecx); // 002099FF\n', 1),
    ('\t// 00209E84 mov [esi+0x203C], ebx\n', '\ttrial_entity_registry::Reset(ctx.esi + 0x3c); // 00209E84\n', 2),
]


def body(source, address):
    return re.search(rf'X86_FUNC\(__imp__sub_{address:08X}\) \{{.*?^\}}', source, re.M | re.S)[0]


def clean(source):
    for _, hook, count in HOOKS:
        assert source.count(hook) in (0, count), hook
        source = source.replace(hook, '')
    return source.replace(INCLUDE, '')


def unprepare(area):
    path = Path(area) / 'generated/game/x86_recomp.52.cpp'
    if path.exists():
        old = path.read_text(); restored = clean(old)
        if old != restored: path.write_text(restored)


def prepare(area):
    area = Path(area)
    path = area / 'generated/game/x86_recomp.52.cpp'
    original = clean(path.read_text())
    pins = json.loads((ROOT / 'scripts/xbe_entity_registry_pins.json').read_text())
    for address, digest in pins.items():
        assert hashlib.sha256(body(original, int(address, 16)).encode()).hexdigest() == digest, address
    patched = original
    for anchor, hook, count in HOOKS:
        assert original.count(anchor) == count, anchor
        patched = patched.replace(anchor, hook + anchor)
    fixture = (ROOT / 'scripts/test_xbe_entity_registry.cpp').read_text()
    bodies = []
    for address in (0x207c70, 0x207d70):
        bodies.append(body(original, address).replace(f'__imp__sub_{address:08X}', f'sub_{address:08X}', 1))
        bodies.append(body(patched, address).replace(f'__imp__sub_{address:08X}', f'Fixed_{address:08X}', 1))
    assert fixture.count('// RETAIL_FUNCTIONS') == 1
    (area / 'src/compat/test_entity_registry.cpp').write_text(fixture.replace('// RETAIL_FUNCTIONS', '\n'.join(bodies)))
    for folder in ('src/compat', 'src/common'):
        (area / folder / 'xbe_entity_registry.h').write_bytes((ROOT / 'scripts/xbe_entity_registry.h').read_bytes())
    path.write_text(INCLUDE + patched)

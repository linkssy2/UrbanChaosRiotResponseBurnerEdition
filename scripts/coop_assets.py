"""Derive a resource-only co-op LST from the user's original multiplayer archive."""
from pathlib import Path
import argparse
import collections
import hashlib
import json
import re
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from analyze_frontend import parse_archive, unpack_entry

ANIMATION_PATH = 'assets_characteranimation\\characterspecific\\cca_multiplayer\\'
ROOTS = tuple('assets_characters\\cha_multiplayer\\cha_multiplayer_burners\\cha_multiplayer_' + name + '_skin.amf'
             for name in ('avenger', 'banger', 'magnum', 'prophet')) + (
    'assets_characters\\cha_multiplayer\\cha_multiplayer_tzero\\cha_multiplayer_tzero_skin.amf',
    'assets_playerarms\\arm_multiplayer_burner_arms_skin.amf',
)
# Original weapon tables 4FA090..4FA48F name distinct third-person meshes.
WEAPON_ROOTS = (
    'assets_characters\\cha_multiplayer\\cha_multiplayer_tzero\\cha_multiplayer_tzero_shield.amf',
    'assets_characters\\cha_cops\\cha_cop_tazer_skin.amf',
    'assets_characters\\cha_multiplayer\\cha_multiplayer_tzero\\cha_multiplayer_tzero_handgun_skin.amf',
    'assets_characters\\cha_patriotassault\\cha_patriotassault_assaultriffle_skin.amf',
    'assets_characters\\cha_burnerbanger\\cha_burnerbanger_shotgun_skin.amf',
    'assets_characters\\cha_saintavenger\\cha_saintavenger_machinepistol_skin.amf',
    'assets_characters\\cha_patriotmagnum\\cha_patriotmagnum_magnum_skin.amf',
    'assets_characters\\cha_burnerbrute\\cha_burnerbrute_cleaver_skin.amf',
    'assets_characters\\cha_saintprophet\\cha_saintprophet_saw_skin.amf',
    'assets_characters\\cha_patriotgrenade\\cha_patriotgrenade_grenadelauncher_skin.amf',
    'assets_characters\\cha_multiplayer\\cha_multiplayer_molotov_skin.amf',
    'assets_characters\\cha_multiplayer\\cha_multiplayer_nailbomb_skin.amf',
    'assets_characters\\cha_multiplayer\\cha_multiplayer_smokegrenade_skin.amf',
    'assets_characters\\cha_multiplayer\\cha_multiplayer_riotgrenade_skin.amf',
    'assets_characters\\cha_multiplayer\\cha_multiplayer_cop_shotgun_skin.amf',
    'assets_characters\\cha_multiplayer\\cha_multiplayer_cop_assaultrifle_skin.amf',
)
ROOTS += WEAPON_ROOTS
WEAPON_TEXTURES = {
    'characters\\cha_burnerblazer\\cha_burnerblazer_molotov.tga',
    'characters\\cha_burnerblazer\\cha_burnerblazer_molotov2.tga',
    'characters\\cha_multiplayer\\cha_multiplayer_tzero\\cha_mp_tzero_handgun.tga',
    'characters\\cha_multiplayer\\cha_multiplayer_tzero\\cha_mp_tzero_rifle.tga',
    'characters\\cha_multiplayer\\cha_multiplayer_tzero\\cha_mp_tzero_shield.tga',
    'characters\\cha_multiplayer\\cha_multiplayer_tzero\\cha_mp_tzero_shotgun.tga',
    'characters\\cha_patriotmagnum\\cha_patriotmagnum_shield.tga',
    'characters\\cha_saintprophet\\cha_saintprophet_blade.tga',
    'playerweaponsandequipment\\eqp_chainsaw\\eqp_chainsaw_main_new.tga',
    'playerweaponsandequipment\\eqp_cleaver\\eqp_cleaver.ifl',
    'playerweaponsandequipment\\eqp_cleaver\\eqp_cleaver.tga',
    'playerweaponsandequipment\\eqp_cleaver\\eqp_cleaver02.tga',
    'playerweaponsandequipment\\eqp_cleaver\\eqp_cleaver03.tga',
    'playerweaponsandequipment\\eqp_cleaver\\eqp_cleaver_handle.tga',
    'playerweaponsandequipment\\eqp_csgasgrenade\\eqp_csgasgrenade.tga',
    'playerweaponsandequipment\\eqp_csgasgrenade\\eqp_csgasgrenade_clip.tga',
    'playerweaponsandequipment\\eqp_grenadelauncher\\eqp_grenadelauncher.tga',
    'playerweaponsandequipment\\eqp_machinepistol\\eqp_machinepistol_new.tga',
    'playerweaponsandequipment\\eqp_magnum44\\eqp_magnum44_main.tga',
    'playerweaponsandequipment\\eqp_nailbomb\\eqp_nailbomb_dynamite01.tga',
    'playerweaponsandequipment\\eqp_nailbomb\\eqp_nailbomb_dynamite02.tga',
    'playerweaponsandequipment\\eqp_nailbomb\\eqp_nailbomb_fuse.tga',
    'playerweaponsandequipment\\eqp_nailbomb\\eqp_nailbomb_nailpoints.tga',
    'playerweaponsandequipment\\eqp_nailbomb\\eqp_nailbomb_nails.tga',
    'playerweaponsandequipment\\eqp_riotgrenade\\eqp_riotgrenade.tga',
    'playerweaponsandequipment\\eqp_riotgrenade\\eqp_riotgrenade_clip.tga',
    'playerweaponsandequipment\\eqp_riotgrenade\\eqp_riotgrenade_name.tga',
    'playerweaponsandequipment\\eqp_shotgun\\eqp_shotgun_new.tga',
    'playerweaponsandequipment\\eqp_taser\\eqp_taser3_low.tga',
    'playerweaponsandequipment\\eqp_tavor\\eqp_tavor_main.tga',
}
ANIMATIONS = tuple(ANIMATION_PATH + 'cca_multiplayer_anim_' + name + '.kfa' for name in (
    'shield_idle', 'shield_attack', 'shield_gren_throw_back', 'shield_gren_throw_loop', 'shield_gren_throw_forwards',
    'taser_idle', 'taser_attack_start', 'taser_attack_loop', 'taser_attack_end',
    'handgun_idle', 'handgun_fire', 'handgun_reload', 'handgun_idle_lowered',
    'enemyassaultrifle_idle', 'enemyassaultrifle_fire', 'enemyassaultrifle_reload', 'enemyassaultrifle_idle_lowered',
    'sawnoff_shotgun_idle', 'sawnoff_shotgun_fire', 'sawnoff_shotgun_reload', 'sawnoff_shotgun_idle_lowered',
    'machinepistols_idle', 'machinepistols_fire', 'machinepistols_reload', 'machinepistols_lowered',
    'magnum_idle', 'magnum_fire', 'magnum_reload', 'magnum_idle_lowered',
    'cleaver_idle', 'cleaver_swipe', 'cleaver_throw_back', 'cleaver_throw_hold_loop', 'cleaver_throw_forwards',
    'chainsaw_idle', 'chainsaw_melee', 'chainsaw_attack_start', 'chainsaw_attack_loop', 'chainsaw_attack_end',
    'grenadelauncher_idle', 'grenadelauncher_fire', 'grenadelauncher_reload', 'grenadelauncher_idle_lowered',
    'molotovnailsmokestun_idle', 'melee_molotov', 'grenadethrowhighstart', 'grenade_loop_hold', 'grenadethrowhighend',
    'grenadethrowlowstart', 'grenade_low_loop_hold', 'grenadethrowlowend',
    'copshotgun_idle', 'copshotgun_fire', 'copshotgun_reload', 'copshotgun_idle_lowered',
    'copassaultrifle_idle', 'copassaultrifle_fire', 'copassaultrifle_reload', 'copassaultrifle_idle_lowered',
    'stand', 'run', 'crouchforward', 'backwalk', 'crouch', 'crouchback', 'holstered_run',
    'deathpose_hand_feet', 'tazer_stun_loop', 'backrun', 'walk2',
))
TEMPLATES = {ANIMATION_PATH + 'cca_multiplayer_thirdperson_template.sttxt',
             'assets_playerweaponsandequipment\\eqp_reference_template.sttxt'}
SHARED_TEXTURES = {'characters\\cha_patriotbanger\\cha_patriotbanger_mask_suspect_si.tga'}
# ThirdPersonPlayer requests these directly when spawning, dying and drawing its
# shadow. They are not referenced by the skin, so body-only closure omits them.
LIFECYCLE_RESOURCES = ('multiplayer\\spawn01.simpleparticlesystem',
                       'multiplayer\\death_distortion.simpleparticlesystem',
                       'fx\\fx_shadows\\fx_shadows_blob02.tga')
LIFECYCLE_TEXTURES = {'particles\\gasexplosion01.tga', 'particles\\shapes\\ring.tga'}
# The original 40-01 archive repeats this texture. Retail lookup has both a
# sequential fast path (8C834) and a last-name map (8D8AB); preserve both original
# leaf records in order instead of deciding which the runtime will resolve.
DUPLICATE_TEXTURES = {'particles\\shapes\\ring.tga'}
REFERENCE = re.compile(rb'(?:\$n:\\)?(?:assets_|characters\\|playerarms\\|playerweaponsandequipment\\|particles\\)[a-z0-9_\\.$ -]+?\.(?:skcsf|tga|ifl|kfa|sttxt)')


def admitted(name):
    """Only reviewed body/arm/weapon families, materials and lifecycle effects are eligible."""
    if (name in ANIMATIONS or name in TEMPLATES or name in SHARED_TEXTURES or
            name in LIFECYCLE_RESOURCES or name in LIFECYCLE_TEXTURES or name in WEAPON_TEXTURES):
        return True
    if any(name == root[:-4] + ext for root in ROOTS for ext in ('.amf', '.mdlhk', '.entsupp')):
        return True
    if any(name.startswith('$n:\\' + root + '$') and name.endswith('.skcsf') for root in ROOTS):
        return True
    return (name.startswith(('characters\\cha_multiplayer\\cha_multiplayer_burners\\',
                             'characters\\cha_multiplayer\\cha_multiplayer_tzero\\', 'playerarms\\'))
            and name.endswith(('.tga', '.ifl')))


def build(source, output, *, cancel=None):
    source, output = Path(source), Path(output)
    check = cancel or (lambda: None)
    check()
    if source.resolve() == output.resolve():
        raise ValueError('The source game archive must remain unchanged')
    raw, entries = parse_archive(source)
    if struct.unpack_from('<I', raw)[0] != 0x1c:
        raise ValueError('Unsupported original archive version')
    by_name = {}
    all_names = {entry['name'].lower() for entry in entries}
    for entry in entries:
        name = entry['name'].lower()
        if entry['bank'] == 0 and admitted(name):
            if name in by_name and name not in DUPLICATE_TEXTURES:
                raise ValueError('Duplicate common character resource: ' + name)
            by_name[name] = entry
    selected = set(ROOTS) | set(ANIMATIONS) | TEMPLATES | set(LIFECYCLE_RESOURCES)
    selected.update(root[:-4] + ext for root in ROOTS for ext in ('.mdlhk', '.entsupp'))
    missing = selected - by_name.keys()
    if missing:
        raise ValueError('Missing original character resource: ' + sorted(missing)[0])
    payloads, references = {}, {}
    while selected - payloads.keys():
        check()
        name = sorted(selected - payloads.keys())[0]
        entry = by_name[name]
        if entry['packed_size'] == 0:
            raise ValueError('External character payload is unsupported: ' + name)
        payload = unpack_entry(raw, entry)
        payloads[name] = payload
        # These original binary records embed full material/template/texture names.
        # Keep exact original resource identities, including the $n: material prefix.
        lower = payload.lower()
        dependencies = {other for other in all_names if other != name and other.encode('latin1') in lower}
        dependencies.update(match.decode('latin1') for match in REFERENCE.findall(lower))
        dependencies.discard(name)
        forbidden = dependencies - by_name.keys()
        if forbidden:
            raise ValueError('Unreviewed character dependency: ' + sorted(forbidden)[0])
        references[name] = sorted(dependencies)
        selected.update(dependencies)
    ordered = [entry for entry in entries if entry['bank'] == 0 and entry['name'].lower() in selected]
    if len(ordered) > 0xffff:
        raise ValueError('Character resource count exceeds the retail directory capacity')
    names = [entry['name'].encode('latin1') + b'\0' for entry in ordered]
    if any(len(name) >= 0x400 for name in names):
        raise ValueError('Character resource name exceeds the retail reader limit')
    directory_size = 4 + sum(2 + len(name) + 16 for name in names)
    # 8D950/8D670: version, admitted capacity, fixed format flag, local stream ID.
    # No entity/audio banks or external SAF storage belong to this resource-only LST.
    result = bytearray(0xc8)
    struct.pack_into('<I', result, 0, 0x1c)
    struct.pack_into('<HHH', result, 0x12, len(ordered), 1, 0)
    struct.pack_into('<II', result, 0xc0, directory_size, len(ordered))
    position = 0xc4 + directory_size
    compressed = []
    for entry, name in zip(ordered, names):
        check()
        start, size = entry['data_offset'], entry['packed_size']
        if size == 0:
            raise ValueError('External character payload is unsupported: ' + entry['name'])
        packed = raw[start:start + size]
        result.extend(struct.pack('<H', len(name)) + name)
        result.extend(struct.pack('<IIII', position, entry['unpacked_size'], size, 0))
        compressed.append(packed)
        position += size
    for packed in compressed:
        result.extend(packed)
    check()
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(result)
    checked, checked_entries = parse_archive(output)
    if len(checked_entries) != len(ordered):
        raise ValueError('Written character directory is incomplete')
    for old, new, packed in zip(ordered, checked_entries, compressed):
        assert old['name'] == new['name'] and new['bank'] == 0
        assert checked[new['data_offset']:new['data_offset'] + new['packed_size']] == packed
        assert unpack_entry(checked, new) == unpack_entry(raw, old)
    return {'source_sha256': hashlib.sha256(raw).hexdigest(), 'sha256': hashlib.sha256(result).hexdigest(),
            'size': len(result), 'entries': len(ordered), 'types': dict(collections.Counter(e['extension'] for e in ordered)),
            'names': [e['name'] for e in ordered], 'dependencies': references,
            'scope': 'Original character resources and reviewed spawn/death/shadow effects only; no world, UI, text-bank or audio payloads. Runtime preload not yet verified.'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    report = build(args.source, args.output)
    args.output.with_suffix('.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({key: report[key] for key in ('entries', 'size', 'sha256', 'types')}))

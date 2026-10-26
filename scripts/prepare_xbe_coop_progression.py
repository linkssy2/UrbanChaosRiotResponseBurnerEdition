"""Gate proved campaign PLAYER enter events on both local actors' original overlap."""
from pathlib import Path
import hashlib
import json
import re
import struct
import coop_progression_assets
import prepare_xbe_coop_shields

INCLUDE='#include "xbe_coop_progression.h"\n'
HOOK='\tTrialCoopProgressionEnter(ctx,base);\n'
ANCHORS=('\t// 0021169B test al, al','\t// 002117BC test al, al')
PINS=(
    (0x211610,524,'088494dd33c410bb7c2c75c2827d5897b6a33341a617afae173b10a6df9be38b'),
    # Original command-width dispatch, its two jump tables and appended custom
    # trigger slots underpin the build-time census and source-index identity.
    (0x216fa0,1403,'b7d392ddf1a91a387a2e9d68b85738794640084734c17baaebd9b806c25a249b'),
    (0x21751c,68,'aa1c1010ae2b29ca846e0ecb243f3de00afccfdc244527da68753d16496f5336'),
    (0x217560,57,'9c061d6fbc53ba57ef464d009ffb77df5ede8cc40a149182b43fe19d2841e002'),
    (0x213720,217,'94f38ce0a7eb6bd6974d6b67b1d747bb20383064b7e224a54a5975e2e68d5ce0'),
)
BODY_DIGEST='8ad2f7e12bc864aecc936e5dc9c5755f230a4efad30b6b34542102f4ddd8d1b5'

def clean(source):
    return source.replace(INCLUDE,'').replace(HOOK,'')

def unprepare(area):
    path=Path(area)/'generated/game/x86_recomp.53.cpp'
    if path.exists():
        before=path.read_text();after=clean(before)
        if before!=after:path.write_text(after)

def gate_key(gate):
    return (gate['provider_slot_count'],gate['source_index'],gate['volume_ordinal'],gate['enter_group'])

def legacy_census(area):
    """Private regression witness for the former creation/unlock policy."""
    current=coop_progression_assets.ENSURE_PROGRESS
    operations=coop_progression_assets.DOOR_PROGRESS_OPERATIONS
    try:
        coop_progression_assets.ENSURE_PROGRESS=current|{'DoorProp'}
        coop_progression_assets.DOOR_PROGRESS_OPERATIONS=operations|{23}
        return coop_progression_assets.build(area)
    finally:
        coop_progression_assets.ENSURE_PROGRESS=current
        coop_progression_assets.DOOR_PROGRESS_OPERATIONS=operations

def samples(area,result,legacy):
    """Private fixture embeds original inputs; runtime header contains hashes only."""
    out=[];entries=[];sample_index=0
    for level in result['report']['levels']:
        reader,triggers=coop_progression_assets.read_level(area/'assets'/level['file'])
        selected_keys={gate_key(g)for g in level['gates']}
        old=next(v for v in legacy['report']['levels']if v['file']==level['file'])
        chosen=[(g,True)for g in level['gates']]
        # Every removed population/unlock root exercises the original update as an
        # unselected event, including sequential crossing and legacy replays.
        chosen += [(g,False)for g in old['gates']if gate_key(g)not in selected_keys]
        # Ordinary shield pickup remains a control with exact authored inputs.
        if level['file']=='12-01.ent':
            garage=next(g for g,selected in chosen if g['volume_ordinal']==29 and not selected)
            assert garage['enter_group']==1
            t=triggers[29]
            assert [name for name,_ in t['scripts']]==['create walkway action']
            commands=coop_progression_assets.decode(t['scripts'][0][1])
            # These original command offsets prove that the deferred event is
            # the population script for the reported missing garage objects.
            for offset,target,kind in ((8,528,'DoorProp'),(72,1517,'AnimatingProp'),(80,208,'BreakableProp')):
                assert dict(offset=offset,opcode=22,argument=target)in commands
                assert reader.entities[target]['kind']==kind
            ordinal=30;t=triggers[ordinal]
            chosen.append((dict(provider_slot_count=level['provider_slot_count'],
                source_index=len(reader.entities)+sum(bool(t['flags']&32)for t in triggers[:ordinal]),
                volume_ordinal=ordinal,enter_group=1),False))
        for gate,selected in chosen:
            t=triggers[gate['volume_ordinal']];assert t['shape']==3 and len(t['boxes'])%24==0
            name=f'sample{sample_index}';sample_index+=1
            def raw(label,data):
                out.append('static constexpr uint8_t '+name+label+'[]={'+','.join(str(v)for v in data)+'};')
            for i,(_label,code)in enumerate(t['scripts']):raw(f'code{i}',code)
            raw('boxes',t['boxes']);raw('rows',t['properties'])
            out.append('static constexpr const uint8_t* '+name+'codes[]={'+','.join(name+f'code{i}'for i in range(len(t['scripts'])))+'};')
            out.append('static constexpr uint32_t '+name+'sizes[]={'+','.join(str(len(code))for _,code in t['scripts'])+'};')
            control='GaragePopulation'if level['file']=='12-01.ent'and gate['volume_ordinal']==29 else (
                'ShieldPickup'if level['file']=='12-01.ent'and gate['volume_ordinal']==30 else'None')
            entries.append('{%du,%du,%du,%du,%du,%s,%s,%s,%s,%du,%s,Control::%s},'%(
                gate['provider_slot_count'],gate['source_index'],gate['volume_ordinal'],gate['enter_group'],
                len(t['scripts']),name+'codes',name+'sizes',name+'rows',name+'boxes',len(t['boxes'])//24,
                'true'if selected else'false',control))
    out.append('static constexpr Sample samples[]={\n'+ '\n'.join(entries)+'\n};')
    return '\n'.join(out)

def prepare(area):
    area=Path(area);local=Path(__file__).parent
    xbe=(area/'assets/Default.xbe').read_bytes();base=struct.unpack_from('<I',xbe,0x104)[0]
    count,table=struct.unpack_from('<II',xbe,0x11c)
    sections=[struct.unpack_from('<6I',xbe,table-base+i*56)for i in range(count)]
    for va,size,digest in PINS+prepare_xbe_coop_shields.PINS:
        s=next(s for s in sections if s[1]<=va and va+size<=s[1]+s[4]);offset=s[3]+va-s[1]
        assert hashlib.sha256(xbe[offset:offset+size]).hexdigest()==digest,hex(va)
    source=clean((area/'generated/game/x86_recomp.53.cpp').read_text())
    body=re.search(r'X86_FUNC\(__imp__sub_00211610\) \{.*?^\}',source,re.M|re.S).group()
    assert hashlib.sha256(body.encode()).hexdigest()==BODY_DIGEST
    test=(local/'test_xbe_coop_progression.cpp').read_text()
    test=test.replace('// ORIGINAL_00211610',body)
    patched=body.replace('__imp__sub_00211610','CoopEnterUpdate',1)
    for anchor in ANCHORS:
        assert source.count(anchor)==1
        source=source.replace(anchor,HOOK+anchor,1)
        patched=patched.replace(anchor,HOOK+anchor,1)
    test=test.replace('// PATCHED_00211610',patched)
    test=test.replace('// LEGACY_00211610',patched.replace('CoopEnterUpdate','LegacyCoopEnterUpdate',1)
        .replace('TrialCoopProgressionEnter(ctx,base);','legacy::TrialCoopProgressionEnter(ctx,base);'))
    for unit,address,digest in prepare_xbe_coop_shields.BODIES:
        text=(area/f'generated/game/x86_recomp.{unit}.cpp').read_text()
        text=prepare_xbe_coop_shields.clean(text,prepare_xbe_coop_shields.HOOKS.get(unit,[]))
        original=re.search(r'X86_FUNC\(__imp__sub_'+address+r'\) \{.*?^\}',text,re.M|re.S).group()
        assert hashlib.sha256(original.encode()).hexdigest()==digest,address
        marker='// ORIGINAL_'+address;assert test.count(marker)==1,marker
        test=test.replace(marker,original)
    result=coop_progression_assets.build(area)
    legacy=legacy_census(area)
    test=test.replace('// LEGACY_CENSUS',legacy['header'].replace('#pragma once\n','')
        .replace('namespace coop_progression_assets','namespace legacy_progression_assets'))
    test=test.replace('// AUTHORED_SAMPLES',samples(area,result,legacy))
    assert all(marker not in test for marker in ('// ORIGINAL_','// PATCHED_','// LEGACY_','// AUTHORED_'))
    source=source.replace('#include "x86_recomp_shared.h"\n','#include "x86_recomp_shared.h"\n'+INCLUDE,1)
    (area/'generated/game/x86_recomp.53.cpp').write_text(source)
    for name in ('xbe_coop_progression.h','xbe_coop_progression.cpp'):
        (area/'src/common'/name).write_text((local/name).read_text())
    (area/'src/common/xbe_coop_progression_assets.h').write_text(result['header'])
    (area/'src/compat/test_coop_progression.cpp').write_text(test)
    (area/'coop-progression-census.json').write_text(json.dumps(result['report'],indent=2)+'\n')

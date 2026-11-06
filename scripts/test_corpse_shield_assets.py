"""Original-disc recipe, native AMF cursor and articulated skin regression checks."""
from pathlib import Path
import argparse
import itertools
import json
import math
import random
import re
import struct
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parent))
from analyze_frontend import parse_archive, unpack_entry
from corpse_shield_assets import (ROOTS, NATIVE, PICKUP, PICKUP_SOURCE, SHIELD, THIRD, BODY, IDLE, JOINTS,
                                  build_archive, digest, read_model, matrix,
                                  inverse, transform, body_pose, IDENTITY)
from coop_injection_assets import decode_clip, model_rig, pose_frame, _multiply, _rotation
from native_scene_checks.inspect_scene import Reader
from native_level_checks.inspect_level import load as load_level

ARCHIVE_SHA256 = '9dae777c28c028510ed823268b46b5b6ce6174d67d175b1db8e7a955b352cbe6'


def load(path):
    raw, entries = parse_archive(path)
    return {e['name'].lower(): unpack_entry(raw, e) for e in entries if e['bank'] == 0 and e['packed_size']}


def composed(model, local):
    result = []
    for i, pose in enumerate(local):
        parent = struct.unpack_from('<H', model['metadata'][i], 22)[0]
        result.append(_multiply(pose, result[parent]) if i else pose)
    return result


def skinned(model, world):
    rows = [_multiply(matrix(meta, 96), pose) for meta, pose in zip(model['metadata'], world)]
    result = []
    for i, (bones, weight) in enumerate(zip(model['resolved'], model['weights'])):
        p = [max(-1., v/32767.)*model['scale'] for v in struct.unpack_from('<3h', model['data'], model['position_at']+i*10)]
        out = [0.,0.,0.]
        for bone, w in zip(bones or (0,0,0), (*weight[3:], 255-sum(weight[3:]))):
            if w:
                point = transform(p, rows[bone])
                for axis in range(3):
                    out[axis] += point[axis]*w/255.
        result.append(out)
    return result


def native_bounds(model, world):
    # B8FE0 / ACD00: inverse bind * current bone transforms each model-space
    # box. The first transformed box replaces the loader's group bounds.
    points = []
    for at in model['bounds']:
        bone = struct.unpack_from('<H', model['data'], at+48)[0]
        box = struct.unpack_from('<6f', model['data'], at)
        palette = _multiply(matrix(model['metadata'][bone], 96), world[bone])
        points.extend(transform(p, palette) for p in itertools.product(
            *((box[a], box[a+3]) for a in range(3))))
    return ([min(p[a] for p in points) for a in range(3)],
            [max(p[a] for p in points) for a in range(3)])


def check_native_bounds(model, local):
    world = composed(model, local)
    minimum, maximum = native_bounds(model, world)
    actual = skinned(model, world)
    assert all(minimum[a]-.001 <= p[a] <= maximum[a]+.001
               for p in actual for a in range(3)), 'native animated cull bounds exclude body'


def check_influence_bounds(model):
    # If every nonzero influence's model-space vertex is inside that bone's
    # box, their transformed union contains any finite blended physical pose.
    boxes = {}
    for at in model['bounds']:
        values = struct.unpack_from('<12f', model['data'], at)
        bone = struct.unpack_from('<H', model['data'], at+48)[0]
        assert all(abs(values[a+6]-(values[a]+values[a+3])*.5) < 1e-4 and
                   abs(values[a+9]-(values[a+3]-values[a])*.5) < 1e-4 for a in range(3))
        boxes.setdefault(bone, []).append(values)
    for i, (bones, weight) in enumerate(zip(model['resolved'], model['weights'])):
        p = [max(-1., v/32767.)*model['scale'] for v in struct.unpack_from(
            '<3h', model['data'], model['position_at']+i*10)]
        for bone, w in zip(bones or (0, 0, 0), (*weight[3:], 255-sum(weight[3:]))):
            if w:
                assert any(all(box[a] <= p[a] <= box[a+3] for a in range(3))
                           for box in boxes.get(bone, [])), ('unbounded skin influence', i, bone)


def run(game, output):
    game = Path(game)
    originals = load(game/'40-01.ent') | load(game/'shield.lst') | load(game/'12-01.ent')
    hashes = {name: digest((game/name).read_bytes()) for name in ('shield.lst', '40-01.ent', '12-01.ent')}
    report = build_archive(game, output)
    assert report['sha256'] == ARCHIVE_SHA256
    assert report == build_archive(game, output)
    resources = load(output)
    # Adding table bodies must not change the accepted held ragdoll geometry.
    for name, expected_hash in zip((*ROOTS, NATIVE), (
            'e35fac091b5d10657de7849de31927cf8a0714081d650930e9097a65ecf01c91',
            '703126b2176c7b74a948f8b8a6d72ce1109ff1fe53d9f2ad367fd893c18f1802',
            '7f38665e6991809fabb487a227c50fafdf41ec1337440da489789da6791dddfa')):
        assert digest(resources[name]) == expected_hash
    original_body = read_model(originals[BODY])
    native = read_model(resources[NATIVE])
    assert native['bones'] == 73 and native['vertices'] == original_body['vertices']
    assert resources[NATIVE][:76] == originals[BODY][:76]
    assert resources[NATIVE][76+73*64:] == originals[BODY][76+73*64:]
    expected = body_pose(original_body)
    actual = composed(native, native['locals'])
    assert max(abs(a-b) for x,y in zip(actual,expected) for r,s in zip(x,y) for a,b in zip(r,s)) < .001
    for suffix in ('.mdlhk', '.entsupp'):
        assert resources[NATIVE[:-4]+suffix] == originals[BODY[:-4]+suffix]
    pickup = read_model(resources[PICKUP])
    original_pickup = read_model(originals[PICKUP_SOURCE])
    reader = Reader(resources[PICKUP]);reader.model()
    assert reader.pos == len(resources[PICKUP]) and len(reader.nodes) == 1
    assert pickup['bones'] == original_pickup['bones'] == 121 and not pickup['template']
    assert resources[PICKUP][76:76+121*224] == originals[PICKUP_SOURCE][76:76+121*224]
    assert pickup['materials'] == original_body['materials'] and pickup['weights'] == original_body['weights']
    assert resources[PICKUP][pickup['position_at']+pickup['vertices']*10:pickup['bounds'][0]] == originals[BODY][original_body['position_at']+original_body['vertices']*10:original_body['bounds'][0]]
    assert {b for row in pickup['resolved'] if row for b in row if b is not None} == {0}
    for suffix in ('.mdlhk', '.entsupp'):
        assert resources[PICKUP[:-4]+suffix] == originals[PICKUP_SOURCE[:-4]+suffix]
    check_influence_bounds(pickup)
    check_native_bounds(pickup, pickup['locals'])
    seated = body_pose(original_body, seated=True)
    for a,b in ((10,11),(11,12),(24,25),(25,26),(37,38),(38,39),(41,42),(42,43)):
        assert abs(math.dist(seated[a][3][:3],seated[b][3][:3])-math.dist(expected[a][3][:3],expected[b][3][:3])) < .001
    for hip,knee,foot in ((37,38,39),(41,42,43)):
        assert abs(seated[knee][3][1]-seated[hip][3][1]) < 4
        assert seated[hip][3][2]-seated[knee][3][2] > 50
        assert seated[knee][3][1]-seated[foot][3][1] > 46
    visible = skinned(pickup, composed(pickup,pickup['locals']))
    static_bounds = struct.unpack_from('<6f',resources[PICKUP],pickup['group_at']+4)
    assert all(static_bounds[a] <= p[a] <= static_bounds[a+3] for p in visible for a in range(3))
    level = load_level(game/'12-01.ent');level.level()
    seat_vertices = [i for i,(bones,w) in enumerate(zip(original_body['resolved'],original_body['weights']))
                     if bones and sum(z for b,z in zip(bones,(*w[3:],255-sum(w[3:]))) if b in (2,37,41)) >= 128]
    original_bottom = struct.unpack_from('<f',originals[PICKUP_SOURCE],original_pickup['group_at']+8)[0]
    for entity in (585,578):
        prop = level.entities[entity]
        assert prop['kind'] == 'BasicProp' and prop['asset'].lower() == PICKUP_SOURCE
        placement = [list(prop['matrix'][i:i+4]) for i in range(0,16,4)]
        anchor = transform([0.,original_bottom,0.],placement)
        assert abs(min(transform(visible[i],placement)[1] for i in seat_vertices)-anchor[1]) < .01
    report.setdefault('checks',[]).append(dict(model=PICKUP,bones=121,vertices=pickup['vertices'],
                                             collision='original pickup unchanged',animation='none',bounds=static_bounds))
    template = native['template'].lower()
    assert resources[template] == originals[template]
    clips = re.findall(rb'assets_[a-z0-9_\\]+\.kfa', originals[template].lower())
    assert len(clips) == 11
    for clip in clips:
        name = clip.decode('latin1')
        assert resources[name] == originals[name]
    shield_rig = model_rig(originals[SHIELD])
    _, shield_local = pose_frame(decode_clip(originals[IDLE]), shield_rig, 0)
    for kind, name in enumerate(ROOTS):
        data, start, count = resources[name], (79 if not kind else 3), (121 if not kind else 3+len(JOINTS))
        # Independent cursor follows original A6220; derived output must end on
        # the native template-name frontier, not merely satisfy our own reader.
        reader = Reader(data)
        reader.model()
        assert reader.pos == len(data) and len(reader.nodes) == 1
        model = read_model(data)
        assert model['bones'] == count and model['vertices'] == original_body['vertices']
        assert model['materials'] == original_body['materials']
        assert model['data'][model['position_at']+model['vertices']*10:model['draw_at']] == originals[BODY][original_body['position_at']+original_body['vertices']*10:original_body['draw_at']]
        assert model['data'][model['draw_at']:model['bounds'][0]] == originals[BODY][original_body['draw_at']:original_body['bounds'][0]]
        assert model['weights'] == original_body['weights']
        assert all(start <= bone < start+len(JOINTS) for row in model['resolved'] if row for bone in row if bone is not None)
        local = [[row[:] for row in m] for m in model['locals']]
        if not kind:
            for bone in range(count):
                if not start <= bone < start+len(JOINTS):
                    assert local[bone] == read_model(originals[SHIELD])['locals'][bone]
                    local[bone] = [row[:] for row in shield_local[bone]]
        world = composed(model, local)
        # Presentation joints must retain the native physics body's size;
        # changing its attachment position must not shrink the visible limbs.
        for left, right in itertools.combinations(range(len(JOINTS)), 2):
            rendered = math.dist(world[start+left][3][:3], world[start+right][3][:3])
            physical = math.dist(expected[JOINTS[left]][3][:3], expected[JOINTS[right]][3][:3])
            assert abs(rendered-physical) < .003, ('native body scale mismatch', kind, left, right)
        if not kind:
            # The held torso covers the original shield centre, rather than
            # sitting beside/below it. Check the composed native pose, not a
            # duplicate of the asset-placement calculation.
            anchor, chest = world[78][3], world[start+JOINTS.index(6)][3]
            assert abs(chest[0]-anchor[0]) < .002
            assert abs(chest[1]-anchor[1]-14.) < .002
        actual = skinned(model, world)
        baked = [[max(-1., v/32767.)*model['scale'] for v in struct.unpack_from('<3h', data, model['position_at']+i*10)] for i in range(model['vertices'])]
        maximum = max(abs(a-b) for p,q in zip(actual,baked) for a,b in zip(p,q))
        assert maximum < .002, ('rest skin changed geometry', kind, maximum)
        check_influence_bounds(model)
        check_native_bounds(model, local)
        # A nontrivial head bend moves the mask/head, leaving pelvis and feet
        # untouched: compact palette remapping must preserve articulated parts.
        angle = math.radians(20)
        rotation = [row+[0.] for row in _rotation([math.sin(angle/2),0.,0.,math.cos(angle/2)])]+[[0.,0.,0.,1.]]
        head = start+JOINTS.index(8)
        rotated = _multiply(rotation, local[head])
        rotated[3] = local[head][3]
        local[head] = rotated
        animated = skinned(model, composed(model, local))
        deltas = [math.dist(a,b) for a,b in zip(animated,actual)]
        head_vertices = [i for i,bones in enumerate(model['resolved']) if bones and head in bones]
        assert max(deltas[i] for i in head_vertices) > 2
        assert all(deltas[i] < 1e-8 for i,bones in enumerate(model['resolved']) if bones and head not in bones)
        bounds = struct.unpack_from('<12f', data, model['group_at']+4)
        assert all(abs(bounds[6+a]-(bounds[a]+bounds[3+a])*.5)<1e-4 and
                   abs(bounds[9+a]-(bounds[3+a]-bounds[a])*.5)<1e-4 for a in range(3))
        # Reconstruct the actual dynamic native box, not just the static group
        # box that the first native per-bone bound replaces before culling.
        for pitch_sign, roll_sign in ((1,1), (1,-1), (-1,1), (-1,-1)):
            moving = [[row[:] for row in m] for m in (shield_local if not kind else model['locals'])]
            for joint in range(len(JOINTS)):
                limit = (.05 if joint == 0 else .10 if joint == 1 else .45)
                sx,cx = math.sin(limit*pitch_sign),math.cos(limit*pitch_sign)
                sy,cy = math.sin(limit*roll_sign),math.cos(limit*roll_sign)
                turn = [[cy,0.,-sy,0.],[sx*sy,cx,sx*cy,0.],[cx*sy,-sx,cx*cy,0.],[0.,0.,0.,1.]]
                moving[start+joint] = _multiply(turn,model['locals'][start+joint])
                moving[start+joint][3] = model['locals'][start+joint][3]
            swung = skinned(model,composed(model,moving))
            assert all(bounds[a] <= p[a] <= bounds[3+a] for p in swung for a in range(3)), ('articulated cull bounds',kind,pitch_sign,roll_sign)
            check_native_bounds(model, moving)
        random_ = random.Random(0xB8FE0)
        for _ in range(8):
            moving = [[row[:] for row in m] for m in local]
            for joint in range(len(JOINTS)):
                axis = [random_.uniform(-1., 1.) for _ in range(3)]
                length = math.sqrt(sum(v*v for v in axis))
                angle = random_.uniform(-1.2, 1.2)
                q = [v/length*math.sin(angle/2) for v in axis]+[math.cos(angle/2)]
                turn = [row+[0.] for row in _rotation(q)]+[[0.,0.,0.,1.]]
                moving[start+joint] = _multiply(turn, model['locals'][start+joint])
            check_native_bounds(model, moving)
        if not kind:
            for clip_name, data_ in originals.items():
                if 'eqp_shield_anim_' not in clip_name or not clip_name.endswith('.kfa'):
                    continue
                clip = decode_clip(data_)
                for frame in sorted({0, clip['frames']//2, clip['frames']-1}):
                    _, moving = pose_frame(clip, shield_rig, frame)
                    for joint in range(len(JOINTS)):
                        moving[start+joint] = model['locals'][start+joint]
                    check_native_bounds(model, moving)
        for suffix in ('.mdlhk', '.entsupp'):
            source = (SHIELD,THIRD)[kind][:-4]+suffix
            if source in originals:
                assert resources[name[:-4]+suffix] == originals[source]
        report.setdefault('checks', []).append(dict(model=name, bones=count, vertices=model['vertices'], rest_error=maximum, head_motion=max(deltas)))
    # Cancellation and incompatible source identities must not truncate an
    # installed archive; setup can safely offer a retry after an interrupted import.
    previous = output.read_bytes()
    def cancel():
        raise RuntimeError('cancel test')
    try:
        build_archive(game, output, cancel=cancel)
        raise AssertionError('cancellation ignored')
    except RuntimeError as error:
        assert str(error) == 'cancel test'
    assert output.read_bytes() == previous
    assert all(digest((game/name).read_bytes()) == value for name,value in hashes.items())
    # Independently check rigid inverse using a rotated and translated anchor.
    transform_ = [[0.,1.,0.,0.],[-1.,0.,0.,0.],[0.,0.,1.,0.],[7.,9.,11.,1.]]
    assert _multiply(transform_,inverse(transform_)) == IDENTITY
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game', type=Path, required=True)
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='corpse-shield-assets-') as temporary:
        result = run(args.game, Path(temporary)/'corpseshield.lst')
    if args.report:
        args.report.parent.mkdir(parents=True,exist_ok=True)
        args.report.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({'ok':True,'sha256':result['sha256'],'size':result['size'],'checks':result['checks']}))

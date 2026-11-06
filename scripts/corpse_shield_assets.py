"""Derive an articulated Burner shield from the owner's original disc resources.

The native shield animation still drives its supporting hand. The presentation
aliases retain the Burner mesh/materials; a separate full skeleton provides the
native ragdoll's initial pose without constructing a second AI character.
"""
from pathlib import Path
import hashlib
import math
import re
import struct
import tempfile
import zlib

from analyze_frontend import parse_archive, unpack_entry
from coop_injection_assets import decode_clip, model_rig, pose_frame, _aim_x, _multiply, _rotation

RECIPE_REVISION = 4
BODY = r'assets_characters\cha_multiplayer\cha_multiplayer_burners\cha_multiplayer_avenger_skin.amf'
SHIELD = r'assets_playerweaponsandequipment\eqp_shield\eqp_shield_skin.amf'
THIRD = r'assets_characters\cha_multiplayer\cha_multiplayer_tzero\cha_multiplayer_tzero_shield.amf'
IDLE = r'assets_playerweaponsandequipment\eqp_shield\eqp_shield_anim_idle.kfa'
ROOTS = (r'assets_cheats\corpse_shield\corpse_shield_fp.amf',
         r'assets_cheats\corpse_shield\corpse_shield_tp.amf')
NATIVE = r'assets_cheats\corpse_shield\corpse_shield_native.amf'
PICKUP = r'assets_cheats\corpse_shield\corpse_shield_pickup.amf'
PICKUP_SOURCE = r'assets_playerweaponsandequipment\eqp_shield\eqp_shield.amf'
SOURCE_HASHES = {
    PICKUP_SOURCE: '0b1c6e1fb43cfc799ac1713c2bab31f69cd0a75611294d10f2b94bcb451605d8',
    PICKUP_SOURCE[:-4]+'.mdlhk': '04b64fd559fa47c36c8f513a072c44baf63235257b3753a81f8c465a4707146f',
    PICKUP_SOURCE[:-4]+'.entsupp': '4cbbd8ca5215b8d161aec181a74b694f4e24b001d5b081dc0030ed797a8973e0',
    BODY: 'b630b7b194e356f68dd9624a95487b5c52c878f003f0fbf15965f573d788e813',
    BODY[:-4]+'.mdlhk': 'be77b40cf8c8228736cf5718c15fedc8b59810ad3bc06c328f9696f80fde0235',
    BODY[:-4]+'.entsupp': '4cbbd8ca5215b8d161aec181a74b694f4e24b001d5b081dc0030ed797a8973e0',
    SHIELD: '4178a96d5529429688f82629ebdc932222f36cf9eb3c8b6b096913630ec3fde2',
    THIRD: 'e507ba61d76bd9a2cb509e51aac747ecdbc569dd1da8ac892a39095e7497f4d3',
    IDLE: '6bea54401bf7de3afbc46bda7ed4bd02b1381d3453fc5b069d6804572396dc27',
    SHIELD[:-4]+'.mdlhk': '2a5014cf73e34fbadf5af512310c718d8ec558cac84bffedee0de722521a249f',
    THIRD[:-4]+'.mdlhk': '7d980fce0a14bbc7f7346bd6a9e4ad9862734254d3b7287c7f91f09f80cf557f',
    THIRD[:-4]+'.entsupp': '4cbbd8ca5215b8d161aec181a74b694f4e24b001d5b081dc0030ed797a8973e0',
}
JOINTS = (2, 3, 5, 6, 8, 10, 11, 12, 24, 25, 26, 37, 38, 41, 42)
IDENTITY = [[float(r == c) for c in range(4)] for r in range(4)]


def digest(data):
    return hashlib.sha256(data).hexdigest()


def matrix(data, at):
    return [list(struct.unpack_from('<4f', data, at + r*16)) for r in range(4)]


def flatten(m):
    return struct.pack('<16f', *(v for row in m for v in row))


def inverse(m):
    # All authored and derived joints are rigid transforms, not scaled bones.
    out = [row[:] for row in IDENTITY]
    for r in range(3):
        for c in range(3):
            out[r][c] = m[c][r]
    for r in range(3):
        out[3][r] = -sum(m[3][k]*out[k][r] for k in range(3))
    return out


def transform(p, m, direction=False):
    return [sum(p[r]*m[r][c] for r in range(3)) + (0 if direction else m[3][c]) for c in range(3)]


def read_model(data):
    """Bounded original A6220 AMF layout, preserving UVs and draw topology."""
    if data[:4] != b'AMF\n' or len(data) < 76:
        raise ValueError('Invalid corpse shield source model')
    materials, groups, bones = struct.unpack_from('<3I', data, 4)
    if groups != 1 or not 1 <= bones <= 121 or not 1 <= materials <= 64:
        raise ValueError('Unsupported corpse shield source skeleton')
    at = 76
    locals_ = [matrix(data, at+i*64) for i in range(bones)]
    at += bones*64
    metadata = [data[at+i*160:at+(i+1)*160] for i in range(bones)]
    at += bones*160
    names = []
    for _ in range(materials):
        length = struct.unpack_from('<H', data, at)[0]
        at += 2
        names.append(data[at:at+length].decode('latin1'))
        at += length
    group_at = at
    if struct.unpack_from('<I', data, at)[0] != 1:
        raise ValueError('Unsupported corpse shield source group')
    at += 56
    node_at = at
    children, vertices, draws, indices, extra, arrays, flags = struct.unpack_from('<7I', data, at+4)
    if children or flags != 17 or not 0 < vertices < 20000 or not 0 < draws < 1000:
        raise ValueError('Unsupported corpse shield source streams')
    at += 112
    position_at = at
    at += vertices*30
    draw_at = at
    draw_records = [struct.unpack_from('<IHHHHHB', data, at+i*20) for i in range(draws)]
    at += (draws+arrays)*20
    indices_at = at
    index_values = struct.unpack_from('<'+'H'*indices, data, at)
    at += indices*2
    bounds = []
    for _ in range(extra):
        bounds.append(at)
        at += 56+struct.unpack_from('<H', data, at+50)[0]*4
    palette_at = at
    updates = []
    for _ in range(draws):
        count = struct.unpack_from('<I', data, at)[0]
        if count > 192:
            raise ValueError('Invalid corpse shield skin palette')
        at += 8
        updates.append([(at+i*4, *struct.unpack_from('<HbB', data, at+i*4)) for i in range(count)])
        at += count*4
    skin_at = at
    weights = [struct.unpack_from('<hhhBB', data, at+i*8) for i in range(vertices)]
    at += vertices*8
    if struct.unpack_from('<I', data, at)[0]:
        raise ValueError('Unsupported corpse shield extra skin stream')
    at += 4
    template_at = at
    length = struct.unpack_from('<H', data, at)[0]
    at += 2
    template = data[at:at+length].decode('latin1')
    if at+length != len(data):
        raise ValueError('Invalid corpse shield model length')
    palette, resolved = {}, [None]*vertices
    for draw, changes in zip(draw_records, updates):
        for _, bone, row, _ in changes:
            if bone >= bones:
                raise ValueError('Invalid source palette bone')
            palette[row] = bone
        first, _, _, _, _, primitives, topology = draw
        if topology not in (4, 5):
            raise ValueError('Unsupported corpse shield topology')
        count = primitives*3 if topology == 4 else primitives+2
        for vertex in index_values[first:first+count]:
            if vertex >= vertices:
                raise ValueError('Invalid corpse shield vertex')
            row = tuple(palette.get(p) for p in weights[vertex][:3])
            w = (*weights[vertex][3:], 255-sum(weights[vertex][3:]))
            if any(v and b is None for b, v in zip(row, w)) or (resolved[vertex] is not None and any(v and a != b for a, b, v in zip(row, resolved[vertex], w))):
                raise ValueError('Ambiguous corpse shield palette')
            resolved[vertex] = row
    return dict(data=data, bones=bones, locals=locals_, metadata=metadata, materials=names,
                scale=struct.unpack_from('<f', data, 68)[0], group_at=group_at,
                node_at=node_at, position_at=position_at, draw_at=draw_at,
                indices_at=indices_at, bounds=bounds, updates=updates,
                palette_at=palette_at, skin_at=skin_at, weights=weights,
                resolved=resolved, vertices=vertices, template_at=template_at,
                template=template)


def body_pose(body, seated=False):
    """Slumped head and hanging elbows, keeping native segment lengths."""
    world = []
    for i, (local, meta) in enumerate(zip(body['locals'], body['metadata'])):
        parent = struct.unpack_from('<H', meta, 22)[0]
        if i and parent >= i:
            raise ValueError('Invalid Burner parent ordering')
        pose = _multiply(local, world[parent]) if i else local
        direction = {10: [.10, -1., .18], 11: [-.12, -1., -.12],
                     24: [-.12, -1., .12], 25: [.08, -1., -.18],
                     37: [.06, -1., .04], 38: [.02, -.96, .28],
                     41: [-.07, -1., .03], 42: [-.02, -.98, .18]}.get(i)
        if seated:
            direction = {10: [.08, -.90, -.25], 11: [-.08, -.75, -.66],
                         24: [-.08, -.90, -.30], 25: [.08, -.80, -.60],
                         37: [.12, -.05, -1.], 38: [.02, -1., -.03],
                         41: [-.12, -.05, -1.], 42: [-.02, -1., -.02]}.get(i, direction)
        if direction:
            rotation = _aim_x([row[:3] for row in pose[:3]], direction)
            pose = [row+[0.] for row in rotation]+[pose[3]]
        if i in (6, 8, 39, 43):
            # Bow the torso/chin and let the feet hang instead of keeping the
            # living idle's planted ankles and upward-looking head.
            angle = math.radians({6: -8, 8: -28, 39: -30, 43: -25}[i])
            turn = [row+[0.] for row in _rotation([math.sin(angle/2), 0., 0., math.cos(angle/2)])]+[[0.,0.,0.,1.]]
            rotation = _multiply(pose, turn)
            if i == 8:
                lean = math.radians(12)
                side = [row+[0.] for row in _rotation([0., 0., math.sin(lean/2), math.cos(lean/2)])]+[[0.,0.,0.,1.]]
                rotation = _multiply(rotation, side)
            rotation[3] = pose[3]
            pose = rotation
        world.append(pose)
    return world


def normal_unpack(value):
    out = []
    for shift, width in ((0, 11), (11, 11), (22, 10)):
        n = (value >> shift) & ((1 << width)-1)
        if n & (1 << (width-1)):
            n -= 1 << width
        out.append(max(-1., n / ((1 << (width-1))-1)))
    return out


def normal_pack(vector):
    length = math.sqrt(sum(v*v for v in vector))
    if length < 1e-8:
        return 0
    result = 0
    for v, (shift, width) in zip(vector, ((0, 11), (11, 11), (22, 10))):
        result |= (round(max(-1., min(1., v/length))*((1 << (width-1))-1)) & ((1 << width)-1)) << shift
    return result


def derive_model(body_data, shield_data, idle, third=False):
    body, shield = read_model(body_data), read_model(shield_data)
    pose = body_pose(body)
    if third:
        anchor, start, count = 2, 3, 3+len(JOINTS)
        anchor_world = _multiply(shield['locals'][2], shield['locals'][0])
        # The third-person shield attachment already positions this assembly.
        support = [-17., 2., -8.]
    else:
        anchor, start, count = 78, 79, 121
        anchor_world = pose_frame(decode_clip(idle), model_rig(shield_data), 0)[0][anchor]
        # Match the original shield's horizontal centre and raise the supported
        # torso slightly. The old underarm offset put it below/right of cover.
        support = [anchor_world[3][0], anchor_world[3][1]+14., anchor_world[3][2]-9.]
    anchor_inverse = inverse(anchor_world)
    # Keep native body dimensions, with the torso supported by the shield hand.
    placement = [row[:] for row in IDENTITY]
    placement[3] = [support[0], support[1]-pose[6][3][1], support[2], 1.]
    target = _multiply(placement, anchor_inverse)
    vertices, normals = [], []
    for i, (weights, bones) in enumerate(zip(body['weights'], body['resolved'])):
        xyz = [max(-1., v/32767.)*body['scale'] for v in struct.unpack_from('<3h', body_data, body['position_at']+i*10)]
        n = normal_unpack(struct.unpack_from('<I', body_data, body['position_at']+i*10+6)[0])
        p, normal = [0.,0.,0.], [0.,0.,0.]
        for bone, weight in zip(bones or (0,0,0), (*weights[3:], 255-sum(weights[3:]))):
            if not weight:
                continue
            skin = _multiply(matrix(body['metadata'][bone], 96), pose[bone])
            pp, nn = transform(xyz, skin), transform(n, skin, True)
            for axis in range(3):
                p[axis] += pp[axis]*weight/255.
                normal[axis] += nn[axis]*weight/255.
        vertices.append(transform(p, target))
        normals.append(transform(normal, target, True))
    # Compact joints have identity orientations in the held assembly. Their
    # inverse binds exactly cancel the authored resting pose before motion.
    compact = {bone: start+i for i, bone in enumerate(JOINTS)}
    remap = {}
    for bone in range(body['bones']):
        ancestor = bone
        while ancestor not in compact and ancestor:
            ancestor = struct.unpack_from('<H', body['metadata'][ancestor], 22)[0]
        remap[bone] = compact.get(ancestor, start)
    local = [[row[:] for row in m] for m in shield['locals']]
    metadata = [bytearray(m) for m in shield['metadata']]
    if third:
        local.extend([[row[:] for row in IDENTITY] for _ in JOINTS])
        metadata.extend(bytearray(160) for _ in JOINTS)
    absolute = {}
    for bone in JOINTS:
        current = compact[bone]
        ancestor = struct.unpack_from('<H', body['metadata'][bone], 22)[0]
        while ancestor not in compact and ancestor:
            ancestor = struct.unpack_from('<H', body['metadata'][ancestor], 22)[0]
        parent = compact.get(ancestor, anchor)
        absolute[current] = transform(pose[bone][3][:3], target)
        parent_position = absolute[parent] if parent in absolute else [0.,0.,0.]
        local[current] = [row[:] for row in IDENTITY]
        local[current][3] = [absolute[current][i]-parent_position[i] for i in range(3)]+[1.]
        meta = bytearray(160)
        name = ('CorpseJoint%02d' % (current-start)).encode()
        meta[4:4+len(name)] = name
        struct.pack_into('<HHH', meta, 22, parent, 0, 0)
        struct.pack_into('<4f', meta, 80, *local[current][3][:3], 0.)
        # Rest model world includes the original shield's authoring root.
        bind_world = [row[:] for row in IDENTITY]
        bind_world[3] = absolute[current]+[1.]
        meta[96:160] = flatten(inverse(_multiply(bind_world, anchor_world)))
        metadata[current] = meta
    # Rebuild links as well as parent order for both native traversal paths.
    for meta in metadata:
        struct.pack_into('<HH', meta, 24, 0, 0)
    children = {}
    for i, meta in enumerate(metadata[1:], 1):
        parent = struct.unpack_from('<H', meta, 22)[0]
        children.setdefault(parent, []).append(i)
    for parent, group in children.items():
        struct.pack_into('<H', metadata[parent], 24, group[0])
        for a, b in zip(group, group[1:]):
            struct.pack_into('<H', metadata[a], 26, b)
    header = bytearray(body_data[:76])
    struct.pack_into('<I', header, 12, count)
    tag = b'CorpseShieldTP' if third else b'CorpseShieldFP'
    header[16:56] = tag.ljust(40, b'\0')
    header[73] = 1
    # Positions are stored in native model coordinates (before anchor transform).
    # Bake the rest anchor into geometry so the standard inverse-bind palette
    # preserves them, then the shield animation moves the whole held assembly.
    vertices = [transform(p, anchor_world) for p in vertices]
    normals = [transform(n, anchor_world, True) for n in normals]
    scale = max(abs(v) for p in vertices for v in p)*1.01
    struct.pack_into('<f', header, 68, scale)
    tail = bytearray(body_data[body['group_at']:body['template_at']])
    origin = body['group_at']
    # B65B5 copies this group AABB to the model's broad-phase culling bounds.
    # The held body is translated away from the source character's T-pose;
    # include conservative room for the runtime's bounded dangling joints.
    minimum = [min(p[a] for p in vertices)-64 for a in range(3)]
    maximum = [max(p[a] for p in vertices)+64 for a in range(3)]
    center = [(a+b)*.5 for a,b in zip(minimum, maximum)]
    extent = [(b-a)*.5 for a,b in zip(minimum, maximum)]
    struct.pack_into('<12f', tail, 4, *minimum, *maximum, *center, *extent)
    for i, (p, n) in enumerate(zip(vertices, normals)):
        struct.pack_into('<3hI', tail, body['position_at']-origin+i*10,
                         *(round(v/scale*32767) for v in p), normal_pack(n))
    for changes in body['updates']:
        for at, bone, _, _ in changes:
            struct.pack_into('<H', tail, at-origin, remap[bone])
    # B8FE0 transforms these model-coordinate boxes by inverse bind * composed
    # bone, then replaces the group box with their union. Applying inverse bind
    # here as well makes the native culler apply it twice and lose the held body.
    # Each remapped joint needs only the vertices with a nonzero influence.
    influenced = {}
    for p, bones, weights in zip(vertices, body['resolved'], body['weights']):
        for bone, weight in zip(bones or (0, 0, 0), (*weights[3:], 255-sum(weights[3:]))):
            if weight:
                influenced.setdefault(remap[bone], []).append(p)
    bounded = set()
    for at in body['bounds']:
        bone = struct.unpack_from('<H', body_data, at+48)[0]
        mapped = remap[bone]
        points = influenced.get(mapped, [transform(absolute[mapped], anchor_world)])
        # Cover position quantization, rather than adding arbitrary swing padding:
        # the native culler transforms every box with that joint's current pose.
        padding = scale/32767. + .001
        minimum = [min(p[a] for p in points)-padding for a in range(3)]
        maximum = [max(p[a] for p in points)+padding for a in range(3)]
        center = [(a+b)*.5 for a, b in zip(minimum, maximum)]
        extent = [(b-a)*.5 for a, b in zip(minimum, maximum)]
        struct.pack_into('<12f', tail, at-origin, *minimum, *maximum, *center, *extent)
        struct.pack_into('<H', tail, at-origin+48, mapped)
        bounded.add(mapped)
    if influenced.keys()-bounded:
        raise ValueError('Derived corpse shield has an unbounded skin influence')
    material_names = b''.join(struct.pack('<H', len(n.encode('latin1')))+n.encode('latin1') for n in body['materials'])
    template = (shield['template'] if not third else '').encode('latin1')
    result = bytes(header)+b''.join(flatten(m) for m in local)+b''.join(metadata)+material_names+tail+struct.pack('<H', len(template))+template
    checked = read_model(result)
    if checked['bones'] != count or checked['materials'] != body['materials'] or checked['vertices'] != body['vertices']:
        raise ValueError('Derived corpse shield model validation failed')
    return result


def derive_native_model(body_data):
    """Keep the native skeleton and physics metadata, starting in the held pose."""
    body = read_model(body_data)
    world = body_pose(body)
    local = []
    for i, pose in enumerate(world):
        parent = struct.unpack_from('<H', body['metadata'][i], 22)[0]
        local.append(_multiply(pose, inverse(world[parent])) if i else pose)
    return body_data[:76]+b''.join(flatten(m) for m in local)+body_data[76+body['bones']*64:]


def derive_pickup_model(body_data, shield_data):
    """Bake a seated body while retaining the pickup's authored collision rig."""
    body, shield = read_model(body_data), read_model(shield_data)
    pose = body_pose(body, seated=True)
    # Both original table props lean by 15 degrees. Undo that lean only in
    # the visible mesh, keeping their authored collision and trigger transforms.
    angle = math.radians(-15)
    upright = [row+[0.] for row in _rotation([math.sin(angle/2), 0., 0., math.cos(angle/2)])]+[[0.,0.,0.,1.]]
    bottom = struct.unpack_from('<f', shield_data, shield['group_at']+8)[0]
    root_inverse = inverse(shield['locals'][0])
    vertices, normals, seat_points = [], [], []
    for i, (weights, bones) in enumerate(zip(body['weights'], body['resolved'])):
        xyz = [max(-1., v/32767.)*body['scale'] for v in struct.unpack_from('<3h', body_data, body['position_at']+i*10)]
        n = normal_unpack(struct.unpack_from('<I', body_data, body['position_at']+i*10+6)[0])
        p, normal = [0.,0.,0.], [0.,0.,0.]
        for bone, weight in zip(bones or (0,0,0), (*weights[3:], 255-sum(weights[3:]))):
            if not weight:
                continue
            skin = _multiply(matrix(body['metadata'][bone], 96), pose[bone])
            pp, nn = transform(xyz, skin), transform(n, skin, True)
            for axis in range(3):
                p[axis] += pp[axis]*weight/255.
                normal[axis] += nn[axis]*weight/255.
        vertices.append(p)
        normals.append(normal)
        if sum(w for bone,w in zip(bones or (0,0,0),(*weights[3:],255-sum(weights[3:]))) if bone in (2,37,41)) >= 128:
            seat_points.append(p)
    if not seat_points:
        raise ValueError('Static pickup has no pelvis or thigh support vertices')
    seat = min(p[1] for p in seat_points)
    visible = []
    for i,p in enumerate(vertices):
        p = transform([p[0]-pose[2][3][0],p[1]-seat,p[2]-pose[2][3][2]],upright)
        p[1] += bottom
        visible.append(p)
        vertices[i] = transform(p,root_inverse)
        normals[i] = transform(transform(normals[i],upright,True),root_inverse,True)
    header = bytearray(body_data[:76])
    struct.pack_into('<I', header, 12, shield['bones'])
    header[16:56] = b'CorpseShieldPickup'.ljust(40, b'\0')
    header[73] = 1
    scale = max(abs(v) for p in vertices for v in p)*1.01
    struct.pack_into('<f', header, 68, scale)
    tail = bytearray(body_data[body['group_at']:body['template_at']])
    origin = body['group_at']
    padding = scale/32767.+.001
    def bounds(points):
        low = [min(p[a] for p in points)-padding for a in range(3)]
        high = [max(p[a] for p in points)+padding for a in range(3)]
        return (*low, *high, *((a+b)*.5 for a,b in zip(low,high)), *((b-a)*.5 for a,b in zip(low,high)))
    struct.pack_into('<12f', tail, 4, *bounds(visible))
    for i, (p,n) in enumerate(zip(vertices,normals)):
        struct.pack_into('<3hI', tail, body['position_at']-origin+i*10,
                         *(round(v/scale*32767) for v in p), normal_pack(n))
    for changes in body['updates']:
        for at, _, _, _ in changes:
            struct.pack_into('<H', tail, at-origin, 0)
    for at in body['bounds']:
        struct.pack_into('<12f', tail, at-origin, *bounds(vertices))
        struct.pack_into('<H', tail, at-origin+48, 0)
    materials = b''.join(struct.pack('<H', len(n.encode('latin1')))+n.encode('latin1') for n in body['materials'])
    skeleton = shield_data[76:76+shield['bones']*(64+160)]
    result = bytes(header)+skeleton+materials+tail+b'\0\0'
    checked = read_model(result)
    if checked['bones'] != shield['bones'] or checked['template'] or checked['materials'] != body['materials']:
        raise ValueError('Derived static pickup validation failed')
    return result


def build_archive(game_dir, output, *, cancel=None):
    check = cancel or (lambda: None)
    output, game_dir = Path(output), Path(game_dir)
    resources, archives = {}, {}
    for filename in ('shield.lst', '40-01.ent', '12-01.ent'):
        check()
        path = game_dir/filename
        if path.resolve() == output.resolve():
            raise ValueError('The source disc archive must remain unchanged')
        raw, entries = parse_archive(path)
        archives[filename] = digest(raw)
        for entry in entries:
            if entry['bank'] == 0 and entry['packed_size']:
                resources.setdefault(entry['name'].lower(), (raw, entry))
    def resource(name):
        name = name.lower()
        if name not in resources:
            raise ValueError('Missing original corpse shield resource: '+name)
        raw, entry = resources[name]
        data = unpack_entry(raw, entry)
        if name in SOURCE_HASHES and digest(data) != SOURCE_HASHES[name]:
            raise ValueError('Unsupported original corpse shield source identity: '+name)
        return data
    body, shield, third, idle = (resource(n) for n in (BODY, SHIELD, THIRD, IDLE))
    payloads = {ROOTS[0]: derive_model(body, shield, idle), ROOTS[1]: derive_model(body, third, idle, True),
                NATIVE: derive_native_model(body), PICKUP: derive_pickup_model(body, resource(PICKUP_SOURCE))}
    # Use native opaque Burner material/texture resources and the existing shield
    # hooks. The full 73-bone alias supplies the native ragdoll skeleton
    # and physics description; it is not an AI character or a scene archive.
    for alias, original in zip((*ROOTS, NATIVE, PICKUP), (SHIELD, THIRD, BODY, PICKUP_SOURCE)):
        for suffix in ('.mdlhk', '.entsupp'):
            source = original[:-4]+suffix
            if source in resources:
                payloads[alias[:-4]+suffix] = resource(source)
    pending = {n.lower() for n in read_model(body)['materials']} | {
        read_model(shield)['template'].lower(), read_model(body)['template'].lower()}
    # SKTP's leading reference-skin AMF is an exporter identity, absent from the
    # disc; its eleven actual animation dependencies are the KFA names below it.
    reference = re.compile(rb'(?:\$n:\\)?(?:assets_|characters\\|playerweaponsandequipment\\)[a-z0-9_\\.$ -]+?\.(?:skcsf|tga|ifl|sttxt|kfa|mdlhk|entsupp)')
    while pending-payloads.keys():
        check()
        name = sorted(pending-payloads.keys())[0]
        data = resource(name)
        payloads[name] = data
        pending.update(m.decode('latin1') for m in reference.findall(data.lower()))
    # Preserve original dependency order; aliases come after their materials,
    # textures and template so an eager resource resolver sees complete inputs.
    names = [name for name in resources if name in payloads]
    names += sorted(set(payloads)-set(names)-set(ROOTS)-{NATIVE, PICKUP})
    names += [*ROOTS, NATIVE, PICKUP]
    directory_size = 4+sum(2+len(n.encode('latin1'))+1+16 for n in names)
    result = bytearray(0xc8)
    struct.pack_into('<I', result, 0, 0x1c)
    struct.pack_into('<HHH', result, 0x12, len(names), 1, 0)
    struct.pack_into('<II', result, 0xc0, directory_size, len(names))
    at, compressed = 0xc4+directory_size, []
    for name in names:
        data = payloads[name]
        encoded, packed = name.encode('latin1')+b'\0', zlib.compress(data, 9)
        result += struct.pack('<H', len(encoded))+encoded+struct.pack('<4I', at, len(data), len(packed), 0)
        compressed.append(packed)
        at += len(packed)
    result += b''.join(compressed)
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(prefix=output.name+'.', suffix='.tmp', dir=output.parent, delete=False) as stream:
            temporary = Path(stream.name)
            stream.write(result)
        raw, entries = parse_archive(temporary)
        if len(entries) != len(names) or any(unpack_entry(raw, e) != payloads[e['name']] for e in entries):
            raise ValueError('Corpse shield archive roundtrip failed')
        check()
        temporary.replace(output)
        temporary = None
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)
    return dict(recipe_revision=RECIPE_REVISION, sha256=digest(result), size=len(result),
                entries=len(names), names=names, source_archives=archives,
                models={n: digest(payloads[n]) for n in (*ROOTS, NATIVE, PICKUP)})

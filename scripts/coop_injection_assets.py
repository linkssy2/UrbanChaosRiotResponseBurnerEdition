"""Derive a partner-facing hypospray animation from the user's original assets.

This recipe changes three joint rotations, not the selected arms, their lengths,
the hand's grip, the injector, or its original contact event. Only the derived
KFA is written under a new resource name. No original archive is modified.
"""
from pathlib import Path
import hashlib
import math
import struct
import tempfile
import zlib

from analyze_frontend import parse_archive, unpack_entry

RECIPE_REVISION = 1
SOURCE = r'assets_playerweaponsandequipment\eqp_hypospray\eqp_hypospray_anim_heal_self.kfa'
MODEL = r'assets_playerweaponsandequipment\eqp_hypospray\eqp_hypospray_skin.amf'
TEMPLATE = r'assets_playerweaponsandequipment\eqp_reference_template.sttxt'
ALIAS = r'assets_coop\medical\eqp_hypospray_anim_heal_partner.kfa'
SOURCE_SHA256 = '9ec69291b0359967e701a82e80b24bd82b8c34f94ccd78306e17ff6a221bfbf4'
MODEL_SHA256 = 'a16f918c3a0d2d471c96629722a6737e526a00aabc9ed0d220a8861986ed81bf'
TEMPLATE_SHA256 = 'fff4471dc832997077942691f5e309d0249b298ed98ec88e578c109f7643d9ca'
CHANGED_BONES = (32, 33, 34)
CONTACT_PHASE = struct.unpack('<f', bytes.fromhex('c98ddc3e'))[0]
ARMS = (
    ('armsbare.lst', r'assets_playerarms\arm_barearm_skin.amf',
     '7402fcff3a161c02f03cefecd39a408a1468f2c580e753f36c5cb7e419fffa15'),
    ('armssleeved.lst', r'assets_playerarms\arm_sleevearms_skin.amf',
     'a7f1717d42d011173c8ed37d7c73ca289c0901967f98fdafbcc51f8352759c43'),
    ('40-01.ent', r'assets_playerarms\arm_multiplayer_burner_arms_skin.amf',
     '1ed7d6ea06a7bdc934a0c7fe171bbdf17cedde50fb76915106394fac38e5a404'),
)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def _checked(data, expected, label):
    if digest(data) != expected:
        raise ValueError('Unsupported original ' + label + ' identity')
    return data


def _resource(raw, entries, name):
    matches = [entry for entry in entries if entry['bank'] == 0 and entry['name'].lower() == name]
    if len(matches) != 1:
        raise ValueError('Missing or ambiguous original resource: ' + name)
    return unpack_entry(raw, matches[0])


def _f32(value):
    return struct.unpack('<f', struct.pack('<f', value))[0]


_QUAT_SCALE = struct.unpack('<f', bytes.fromhex('f3043538'))[0]
_QUAT_BIAS = struct.unpack('<f', bytes.fromhex('f304353f'))[0]


def _unpack_quaternion(packed):
    bits = int.from_bytes(packed, 'little')
    omitted, bits = bits & 3, bits >> 2
    result = [0.0] * 4
    for index in range(4):
        if index != omitted:
            result[index] = _f32((bits & 32767) * _QUAT_SCALE - _QUAT_BIAS)
            bits >>= 15
    result[omitted] = _f32(math.sqrt(max(0.0, 1.0 - sum(v * v for v in result))))
    return result


def _pack_quaternion(quaternion):
    q = _unit(quaternion)
    omitted = max(range(4), key=lambda index: abs(q[index]))
    if q[omitted] < 0:
        q = [-value for value in q]
    result, shift = omitted, 2
    for index in range(4):
        if index != omitted:
            value = round((q[index] + _QUAT_BIAS) / _QUAT_SCALE)
            if not 0 <= value <= 32767:
                raise ValueError('Partner injection quaternion outside packed range')
            result |= value << shift
            shift += 15
    return result.to_bytes(6, 'little')


def decode_clip(data):
    """Bounded KFNM 0/2/1-or-9 cursor, following original 1F35F0/1F3854."""
    if len(data) < 50 or data[:4] != b'KFNM':
        raise ValueError('Invalid hypospray clip')
    control, _, _, _, encoding, flags, count, frames, root, size = struct.unpack_from('<I3fIHIIiI', data, 4)
    if control != 0 or encoding != 2 or flags not in (1, 9) or not 1 <= count <= 121 or not 2 <= frames <= 1000:
        raise ValueError('Unsupported hypospray clip layout')
    at = 42 + count * 4
    mask_size = frames * ((count + 7) // 8) if flags & 8 else 0
    if at + mask_size + size + frames * 4 + 8 > len(data):
        raise ValueError('Truncated hypospray clip')
    tracks = list(struct.unpack_from('<' + 'i' * count, data, 42))
    ids = [~track if track < 0 else track for track in tracks]
    if len(set(ids)) != count or min(ids) < 0 or max(ids) >= 121:
        raise ValueError('Invalid hypospray bone tracks')
    masks = data[at:at + mask_size]
    at += mask_size
    blob = data[at:at + size]
    times = data[at + size:at + size + frames * 4]
    phases = struct.unpack('<' + 'f' * frames, times)
    if phases[0] != 0 or phases[-1] != 1 or any(not a < b for a, b in zip(phases, phases[1:])):
        raise ValueError('Invalid hypospray key times')
    position = 0
    translations, codes = [], []
    for frame in range(frames):
        row = []
        for track in range(count):
            if not frame or tracks[track] >= 0:
                row.append(blob[position:position + 12])
                position += 12
            else:
                row.append(translations[0][track])
        translations.append(row)
    translation_bytes = blob[:position]
    for frame in range(frames):
        row = []
        for track in range(count):
            skip = flags & 8 and masks[frame * ((count + 7) // 8) + track // 8] & (1 << (track % 8))
            if skip:
                if not frame:
                    raise ValueError('First hypospray key cannot be omitted')
                row.append(codes[-1][track])
            else:
                row.append(blob[position:position + 6])
                position += 6
        codes.append(row)
    if position != size or any(len(code) != 6 for row in codes for code in row):
        raise ValueError('Invalid hypospray key payload')
    return dict(header=data[:42], tracks=tracks, ids=ids, frames=frames, root=root,
                codes=codes, translations=translations, translation_bytes=translation_bytes,
                times=times, phases=phases, tail=data[at + size + frames * 4:])


def model_rig(data):
    if len(data) < 76 or data[:4] != b'AMF\n' or struct.unpack_from('<I', data, 12)[0] != 121:
        raise ValueError('Unsupported first-person model skeleton')
    metadata = 76 + 121 * 64
    if len(data) < metadata + 121 * 160:
        raise ValueError('Truncated first-person model skeleton')
    return [dict(parent=struct.unpack_from('<H', data, metadata + index * 160 + 22)[0],
                 name=data[metadata + index * 160 + 4:metadata + index * 160 + 22].split(b'\0')[0],
                 offset=struct.unpack_from('<4f', data, metadata + index * 160 + 80),
                 local=[list(struct.unpack_from('<4f', data, 76 + index * 64 + row * 16)) for row in range(4)])
            for index in range(121)]


def _dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def _unit(vector):
    length = math.sqrt(_dot(vector, vector))
    if not math.isfinite(length) or length < 1e-8:
        raise ValueError('Degenerate partner injection pose')
    return [value / length for value in vector]


def _cross(a, b):
    return [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]]


def _multiply(a, b):
    return [[sum(a[row][k] * b[k][column] for k in range(len(b))) for column in range(len(b[0]))]
            for row in range(len(a))]


def _transpose(matrix):
    return list(map(list, zip(*matrix)))


def _rotation(q):
    x, y, z, w = q
    return [[1 - 2 * (y*y + z*z), 2*(x*y + z*w), 2*(x*z - y*w)],
            [2*(x*y - z*w), 1 - 2*(x*x + z*z), 2*(y*z + x*w)],
            [2*(x*z + y*w), 2*(y*z - x*w), 1 - 2*(x*x + y*y)]]


def _quaternion(matrix):
    # The game uses row-vector rotation matrices; transpose the familiar
    # column-vector conversion, then choose its largest stable diagonal.
    m = _transpose(matrix)
    trace = sum(m[index][index] for index in range(3))
    if trace > 0:
        scale = math.sqrt(trace + 1) * 2
        return _unit([(m[2][1]-m[1][2])/scale, (m[0][2]-m[2][0])/scale,
                      (m[1][0]-m[0][1])/scale, scale/4])
    index = max(range(3), key=lambda i: m[i][i])
    j, k = (index + 1) % 3, (index + 2) % 3
    scale = math.sqrt(1 + m[index][index] - m[j][j] - m[k][k]) * 2
    q = [0.0] * 4
    q[index], q[j], q[k], q[3] = scale/4, (m[index][j]+m[j][index])/scale, (m[index][k]+m[k][index])/scale, (m[k][j]-m[j][k])/scale
    return _unit(q)


def _slerp(a, b, fraction):
    a, b = _unit(a), _unit(b)
    dot = _dot(a, b)
    if dot < 0:
        b, dot = [-value for value in b], -dot
    if dot > .9995:
        return _unit([(1-fraction)*x + fraction*y for x, y in zip(a, b)])
    angle = math.acos(max(-1, min(1, dot)))
    return [(math.sin((1-fraction)*angle)*x + math.sin(fraction*angle)*y)/math.sin(angle) for x, y in zip(a, b)]


def _aim_x(reference, direction):
    """Rotate a bone's X axis onto its child while preserving its roll."""
    source, target = _unit(reference[0]), _unit(direction)
    axis = _cross(source, target)
    cosine = max(-1, min(1, _dot(source, target)))
    if cosine < -.9999:
        axis, angle = _unit(reference[1]), math.pi
    elif cosine > .9999999:
        return reference
    else:
        axis, angle = _unit(axis), math.acos(cosine)
    q = [value * math.sin(angle/2) for value in axis] + [math.cos(angle/2)]
    return _multiply(reference, _rotation(q))


def pose_frame(clip, rig, frame, overrides=None):
    """FK follows AMF parent indices and original offset/length translation rule."""
    by_bone = {bone: index for index, bone in enumerate(clip['ids'])}
    world, locals_ = [], []
    for bone, item in enumerate(rig):
        local = [list(row) for row in item['local']]
        if item['name'] and bone in by_bone:
            index = by_bone[bone]
            q = (overrides or {}).get(bone, _unpack_quaternion(clip['codes'][frame][index]))
            rotation = _rotation(q)
            translation = struct.unpack('<3f', clip['translations'][frame][index])
            scale = math.sqrt(_dot(item['offset'], item['offset']))
            local = [row + [0.0] for row in rotation] + [[item['offset'][axis] + scale*translation[axis] for axis in range(3)] + [1.0]]
        if not bone:
            local = [[1.,0.,0.,0.], [0.,0.,1.,0.], [0.,1.,0.,0.], [0.,0.,0.,1.]]
        parent = item['parent']
        if bone and not 0 <= parent < bone:
            raise ValueError('Invalid first-person parent order')
        locals_.append(local)
        world.append(_multiply(local, world[parent]) if bone else local)
    return world, locals_


def _smooth(value):
    value = max(0., min(1., value))
    return value*value*(3-2*value)


def _partner_rotations(clip, rig, frame):
    phase = clip['phases'][frame]
    blend = min(_smooth(phase/.30), _smooth((1-phase)/.40))
    if blend == 0:
        return {}
    # Reach, press at the existing Heal event, briefly hold, then withdraw.
    press = min(_smooth((phase-.24)/(CONTACT_PHASE-.24)), _smooth((.72-phase)/.17))
    world, local = pose_frame(clip, rig, frame)
    neutral_world, neutral_local = pose_frame(clip, rig, 0)
    shoulder = world[32][3][:3]
    wrist = [shoulder[0]+12, shoulder[1]-12, shoulder[2]-70-10*press]
    delta = [b-a for a, b in zip(shoulder, wrist)]
    distance, direction = math.sqrt(_dot(delta, delta)), _unit(delta)
    upper = math.sqrt(_dot(local[33][3][:3], local[33][3][:3]))
    lower = math.sqrt(_dot(local[34][3][:3], local[34][3][:3]))
    if not abs(upper-lower) < distance < upper+lower:
        raise ValueError('Partner injection reach exceeds the original arm lengths')
    along = (upper*upper - lower*lower + distance*distance)/(2*distance)
    height = math.sqrt(max(0., upper*upper-along*along))
    pole = [-1., -.35, 0.]
    pole = _unit([pole[i]-direction[i]*_dot(pole, direction) for i in range(3)])
    elbow = [shoulder[i]+direction[i]*along+pole[i]*height for i in range(3)]
    upper_rotation = _aim_x([row[:3] for row in neutral_world[32][:3]], [elbow[i]-shoulder[i] for i in range(3)])
    lower_reference = _multiply([row[:3] for row in neutral_local[33][:3]], upper_rotation)
    lower_rotation = _aim_x(lower_reference, [wrist[i]-elbow[i] for i in range(3)])
    # Scene Root swaps Y/Z (reflection). Injector -Z is its narrow nozzle;
    # target it forwards and slightly down, with the button facing the player.
    z = _unit([0., .35, 1.])
    injector_rotation = [[1.,0.,0.], _cross([1.,0.,0.], z), z]
    injector_to_hand = _multiply([row[:3] for row in local[80][:3]], [row[:3] for row in local[75][:3]])
    hand_rotation = _multiply(_transpose(injector_to_hand), injector_rotation)
    authored = {32: _multiply(upper_rotation, _transpose([row[:3] for row in world[31][:3]])),
                33: _multiply(lower_rotation, _transpose(upper_rotation)),
                34: _multiply(hand_rotation, _transpose(lower_rotation))}
    indices = {bone: index for index, bone in enumerate(clip['ids'])}
    # Blend from/to the original lowered endpoint poses, not through the
    # self-injection wrist roll which is precisely what this action replaces.
    endpoint = 0 if phase < .5 else clip['frames']-1
    return {bone: _slerp(_unpack_quaternion(clip['codes'][endpoint][indices[bone]]), _quaternion(rotation), blend)
            for bone, rotation in authored.items()}


def derive_clip(source, model, template):
    _checked(source, SOURCE_SHA256, 'hypospray animation')
    _checked(model, MODEL_SHA256, 'hypospray model')
    _checked(template, TEMPLATE_SHA256, 'equipment template')
    clip, rig = decode_clip(source), model_rig(model)
    codes = []
    for frame in range(clip['frames']):
        overrides = _partner_rotations(clip, rig, frame)
        for bone, original in zip(clip['ids'], clip['codes'][frame]):
            codes.append(_pack_quaternion(overrides[bone]) if bone in overrides else original)
    # Explicit rotations remove omitted-key cache dependence for the new clip.
    blob = clip['translation_bytes'] + b''.join(codes)
    header = bytearray(clip['header'])
    struct.pack_into('<H', header, 24, 1)
    struct.pack_into('<I', header, 38, len(blob))
    result = bytes(header) + struct.pack('<'+'i'*len(clip['tracks']), *clip['tracks']) + blob + clip['times'] + clip['tail']
    decoded = decode_clip(result)
    if decoded['tail'] != clip['tail'] or decoded['times'] != clip['times']:
        raise ValueError('Partner injection lost original timing or events')
    return result


def build_archive(game_dir, output, *, cancel=None):
    """Write one lazy native KFA alias; safe to preload before campaign load."""
    check = cancel or (lambda: None)
    check()
    source_path, output = Path(game_dir)/'1-01.ent', Path(output)
    if source_path.resolve() == output.resolve():
        raise ValueError('The source game archive must remain unchanged')
    raw, entries = parse_archive(source_path)
    source = _resource(raw, entries, SOURCE)
    model = _resource(raw, entries, MODEL)
    template = _resource(raw, entries, TEMPLATE)
    derived = derive_clip(source, model, template)
    check()
    name = ALIAS.encode('latin1') + b'\0'
    directory_size = 4 + 2 + len(name) + 16
    packed = zlib.compress(derived, 9)
    result = bytearray(0xc8)
    struct.pack_into('<I', result, 0, 0x1c)
    struct.pack_into('<HHH', result, 0x12, 1, 1, 0)
    struct.pack_into('<II', result, 0xc0, directory_size, 1)
    result += struct.pack('<H', len(name)) + name + struct.pack('<4I', 0xc4+directory_size, len(derived), len(packed), 0) + packed
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(prefix=output.name+'.', suffix='.tmp', dir=output.parent, delete=False) as stream:
            temporary = Path(stream.name)
            stream.write(result)
        checked, records = parse_archive(temporary)
        if len(records) != 1 or records[0]['name'] != ALIAS or records[0]['bank'] != 0 or unpack_entry(checked, records[0]) != derived:
            raise ValueError('Partner injection archive roundtrip failed')
        check()
        temporary.replace(output)
        temporary = None
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)
    return {'size': len(result), 'sha256': digest(result), 'entries': 1, 'names': [ALIAS],
            'recipe_revision': RECIPE_REVISION,
            'source': {'archive': '1-01.ent', 'archive_sha256': digest(raw),
                       'clip_sha256': SOURCE_SHA256, 'model_sha256': MODEL_SHA256,
                       'template_sha256': TEMPLATE_SHA256, 'derived_sha256': digest(derived)},
            'scope': 'Partner-facing native hypospray clip; selected arms, grip, equipment and contact timing retained.'}

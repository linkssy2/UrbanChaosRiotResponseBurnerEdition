"""Read-only structural oracle following original 943E0/A84C0/A6A50/A6220."""
from pathlib import Path
import collections
import json
import struct
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from analyze_frontend import parse_archive, unpack_entry

class Reader:
    def __init__(self, data):
        self.data, self.pos = data, 0
        self.models, self.nodes, self.draws = [], [], []
    def take(self, n):
        assert 0 <= n <= len(self.data)-self.pos, (hex(self.pos), n, len(self.data))
        p = self.pos; self.pos += n
        return self.data[p:self.pos]
    def number(self, fmt='<I'):
        return struct.unpack(fmt, self.take(struct.calcsize(fmt)))[0]
    def name(self):
        return self.take(self.number('<H')).decode('latin1')
    def node(self, material_count, reference=None, depth=0):
        assert depth < 64
        offset = self.pos
        h = struct.unpack('<28I', self.take(112))
        _, children, vertices, draws, indices, extra, arrays, flags, *_ = h
        assert max(children, vertices, draws, indices, extra, arrays) < 1000000
        info = dict(offset=offset, vertices=vertices, draws=draws, indices=indices, flags=flags, depth=depth, extra=extra, arrays=arrays)
        self.nodes.append(info)
        if not vertices: return info
        streams=[]
        if not flags & 2:
            for stride in [10 if flags&1 else 16, 0 if flags&16 else 8, 0 if flags&8 else 4, 8, 0 if flags&32 else 8]:
                streams.append(self.pos if stride else None); self.take(vertices*stride)
            info['streams']=streams
        else:
            assert reference is not None
            info['streams']=reference['streams']
            assert vertices == reference['vertices']
        descriptors = self.take(draws*20)
        self.take(arrays*20)
        index_values = struct.unpack('<'+'H'*indices, self.take(indices*2))
        for i in range(draws):
            d = descriptors[i*20:(i+1)*20]
            first, mat, alternate, start, nverts, nprim, topology = struct.unpack_from('<IHHHHHB',d)
            assert mat < material_count and topology in (4,5), (hex(offset), d.hex())
            nindices = nprim*3 if topology==4 else nprim+2
            assert first+nindices <= indices and (not nindices or max(index_values[first:first+nindices]) < vertices)
            self.draws.append(dict(node=offset, first=first, material=mat, alternate=alternate, start=start, vertices=nverts, primitives=nprim, topology=topology, flags=list(d[15:17])))
        for _ in range(extra):
            e=self.take(56); self.take(struct.unpack_from('<H',e,50)[0]*4)
        if flags&1:
            for _ in range(draws):
                e=self.take(8); self.take(struct.unpack_from('<I',e)[0]*4)
            if not flags&2: self.take(vertices*8)
        info['terminal']=self.number()
        for _ in range(children): self.node(material_count, reference, depth+1)
        return info
    def model(self):
        offset=self.pos
        assert self.take(4)==b'AMF\n', hex(offset)
        h=struct.unpack('<18I',self.take(72))
        materials, groups, bones=h[:3]
        assert max(materials,groups,bones)<100000
        matrices=self.take(bones*64)
        self.take(bones*160)
        names=[self.name() for _ in range(materials)]
        self.models.append(dict(offset=offset, materials=materials, groups=groups, bones=bones,
                               matrices=[list(struct.unpack_from('<16f',matrices,i*64)) for i in range(bones)], names=names))
        for _ in range(groups):
            g=self.take(56); count=struct.unpack_from('<I',g)[0]
            assert count < 100000
            first=None
            for i in range(count):
                node=self.node(materials, first)
                if i==0: first=node
        self.models[-1]['aux']=self.name()
    def scene(self):
        h=self.take(464)
        assert h[:4]==h[-4:]==b'GEO\n'
        lights=struct.unpack_from('<I',h,32)[0]
        self.take(lights*176)
        root=self.number(); sectors=self.number()
        for _ in range(sectors):
            for _ in range(self.number()): self.model()
        mesh_end=self.pos
        # AA040 -> B70C0: optional room bounds/index lists/planes and portals.
        rooms=portals=0
        if self.number():
            rooms=self.number()
            for _ in range(rooms):
                self.take(24); self.take(self.number()*4); self.take(self.number()*16)
            portals=self.number()
            for _ in range(portals):
                self.take(48); self.take(self.number()*4)
        assert self.pos == len(self.data), (self.pos, len(self.data))
        return dict(size=len(self.data), consumed=self.pos, geometry_end=mesh_end, rooms=rooms, portals=portals, lights=lights, root=root, sectors=sectors,
                    models=len(self.models), nodes=len(self.nodes), vertices=sum(n['vertices'] for n in self.nodes), draws=len(self.draws),
                    flags=dict(collections.Counter(n['flags'] for n in self.nodes)), alternate=dict(collections.Counter(d['alternate'] for d in self.draws)),
                    topology=dict(collections.Counter(d['topology'] for d in self.draws)),
                    terminals=dict(collections.Counter(n.get('terminal') for n in self.nodes)),
                    group_counts=dict(collections.Counter(m['groups'] for m in self.models)), bone_counts=dict(collections.Counter(m['bones'] for m in self.models)),
                    unique_matrices=[list(m) for m in {tuple(x) for m in self.models for x in m['matrices']}],
                    aux_names=sorted({m['aux'] for m in self.models}))

def load(path):
    data, entries=parse_archive(path)
    entry=next(e for e in entries if e['name'].lower()==path.name.lower())
    return Reader(unpack_entry(data,entry))

if __name__=='__main__':
    for name in sys.argv[1:]:
        r=load(Path(name)); print(name, json.dumps(r.scene(), indent=2))

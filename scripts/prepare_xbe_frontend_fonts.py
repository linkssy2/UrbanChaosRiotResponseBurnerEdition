"""Stage native original-font loading and a private pixel/metric contract fixture."""
from pathlib import Path
import struct
import zlib
import hashlib

ROOT=Path(__file__).resolve().parents[1]

def entry(archive,name):
    raw=archive.read_bytes();at=200
    for _ in range(struct.unpack_from('<I',raw,196)[0]):
        n,=struct.unpack_from('<H',raw,at);at+=2;found=raw[at:at+n].rstrip(b'\0');at+=n
        offset,size,packed,bank=struct.unpack_from('<4I',raw,at);at+=16
        if found==name:
            result=zlib.decompress(raw[offset:offset+packed]);assert len(result)==size
            return result
    raise ValueError(name)

def decode(raw,width,height):
    # Independent Python reference, same accepted BC3 rules as hud_import.py.
    assert len(raw)==width*height
    out=bytearray(width*height*4)
    for by in range(height//4):
        for bx in range(width//4):
            b=raw[(by*(width//4)+bx)*16:][:16];a0,a1=b[:2]
            alpha=[a0,a1]+([((7-i)*a0+i*a1)//7 for i in range(1,7)] if a0>a1 else [((5-i)*a0+i*a1)//5 for i in range(1,5)]+[0,255])
            bits=int.from_bytes(b[2:8],'little');colors=[]
            for c in struct.unpack_from('<2H',b,8):
                r=c>>11;g=(c>>5)&63;bl=c&31
                colors.append(((r<<3)|(r>>2),(g<<2)|(g>>4),(bl<<3)|(bl>>2)))
            colors += [tuple((2*x+y)//3 for x,y in zip(colors[0],colors[1])),tuple((x+2*y)//3 for x,y in zip(colors[0],colors[1]))]
            indices=int.from_bytes(b[12:16],'little')
            for i in range(16):
                at=((by*4+i//4)*width+bx*4+i%4)*4
                out[at:at+4]=bytes((*colors[(indices>>(2*i))&3],alpha[(bits>>(3*i))&7]))
    return out

def prepare(area):
    area=Path(area);out=area/'src/compat';local=Path(__file__).parent
    for name in ('xbe_frontend_fonts.h','xbe_frontend_fonts.cpp','xbe_frontend_text.h'):
        (out/name).write_text((local/name).read_text())
    fixture=(local/'test_xbe_frontend_fonts.cpp').read_text()
    fixture=fixture.replace('/* GAME_PATH */','L"'+(area/'assets').as_posix()+'"')
    for name in ('medium','large'):
        metrics=entry(area/'assets/dialoguefonts.lst',f'fonts\\{name}.abc'.encode())
        texture=entry(area/'assets/PersistantFilesXBox.lst',f'fonts\\{name}%4b%.tga'.encode())
        selector,width,height,one,levels,flags,size=struct.unpack_from('<7I',texture)
        assert selector==2 and one==1 and not flags&0xff00 and width in (256,512) and height==width
        nominal,=struct.unpack_from('<f',metrics,4);line,baseline=struct.unpack_from('<2f',metrics,16)
        maximum,=struct.unpack_from('<H',metrics,28);start=34+(maximum+1)*2
        expected=bytearray(struct.pack('<3f2I',nominal,line,baseline,width,height))
        for c in range(32,255):
            index,=struct.unpack_from('<H',metrics,30+c*2)
            u0,v0,u1,v1,bearing,glyph_width,tail,zero=struct.unpack_from('<4f4h',metrics,start+index*24)
            expected+=struct.pack('<7f',bearing+tail,bearing,glyph_width,u0,v0,u1,v1)
        expected+=decode(texture[28:28+width*height],width,height)
        path=out/f'test_font_{name}.bin';path.write_bytes(expected)
        fixture=fixture.replace('/* '+name.upper()+'_PATH */','L"'+path.as_posix()+'"')
    (out/'test_frontend_fonts.cpp').write_text(fixture)
    return {'fonts':['medium','large'],'source':'user archive bytes only','test':'original glyph advances/bearings/UVs, full BC3 base atlas, ImGui draw vertices, startup fallback'}

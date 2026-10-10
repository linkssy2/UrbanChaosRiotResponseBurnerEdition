"""Derive the unchanged HUD backdrop from the user's verified disc texture."""
import hashlib,struct,zlib
PIXEL_SHA='531b3beb0a9a15f29ad9653eeee2250f5d203d70e445c77ec3e20504627f03d4'
def decode(raw):
    if len(raw)!=32768:raise ValueError('Invalid HUD texture size')
    out=bytearray(32*1024*4)
    for by in range(256):
        for bx in range(8):
            b=raw[(by*8+bx)*16:][:16];a0,a1=b[:2]
            alpha=[a0,a1]+([((7-i)*a0+i*a1)//7 for i in range(1,7)] if a0>a1 else [((5-i)*a0+i*a1)//5 for i in range(1,5)]+[0,255])
            bits=int.from_bytes(b[2:8],'little');colors=[]
            for c in struct.unpack_from('<2H',b,8):
                r=(c>>11)&31;g=(c>>5)&63;bl=c&31
                colors.append(((bl<<3)|(bl>>2),(g<<2)|(g>>4),(r<<3)|(r>>2)))
            colors += [tuple((2*x+y)//3 for x,y in zip(colors[0],colors[1])),tuple((x+2*y)//3 for x,y in zip(colors[0],colors[1]))]
            indices=int.from_bytes(b[12:16],'little')
            for i in range(16):
                at=((by*4+i//4)*32+bx*4+i%4)*4
                out[at:at+4]=bytes((*colors[(indices>>(2*i))&3],alpha[(bits>>(3*i))&7]))
    return bytes(out)
def prepare(game,output,manifest,archive_entries):
    if output.is_file() and hashlib.sha256(output.read_bytes()).hexdigest()==PIXEL_SHA:return
    name='PersistantFilesXBox.lst';raw=(game/name).read_bytes()
    if hashlib.sha256(raw).hexdigest()!=manifest['files'][name]['sha256']:raise ValueError('Unsupported HUD archive')
    entries=archive_entries(raw)
    row=next((r for r in entries if r[0]==b'font\\font_buttons.tga'),None)
    if row is None:raise ValueError('Missing HUD texture')
    _,offset,size,packed=row
    if size>1024*1024:raise ValueError('Invalid HUD payload size')
    decoder=zlib.decompressobj();payload=decoder.decompress(raw[offset:offset+packed],size+1)
    if len(payload)!=size or not decoder.eof or decoder.unused_data or decoder.unconsumed_tail:raise ValueError('Invalid HUD payload')
    if struct.unpack_from('<3I',payload)!=(2,32,1024):raise ValueError('Unsupported HUD format')
    pixels=decode(payload[28:28+32768])
    if hashlib.sha256(pixels).hexdigest()!=PIXEL_SHA:raise ValueError('HUD conversion verification failed')
    temporary=output.with_suffix('.tmp');temporary.write_bytes(pixels);temporary.replace(output)

"""Synthetic text-bank checks; no game data required."""
import struct
from convert_zarf_spanish import blocks, strings, merge


def bank(value, audio=b'voice'):
    key=bytearray(56);key[:5]=b'hello';key[49]=1;key[50]=1
    variant=struct.pack('<III',0,0,len(audio))
    text=value+b'\0'
    return bytes(key)+variant+text+audio, len(text)

original_block,n=bank(b'Hello')
original=struct.pack('<HH5I',4,32,56,12,n,5,1)+original_block*4
spanish_block,m=bank(b'Hola, mundo',b'xxxxx')
# The beta's stale language count must not copy its voice payload.
spanish=struct.pack('<HH5I',5,32,56,12,m,5,1)+spanish_block
result,report=merge(original,spanish)
h,bs=blocks(result)
assert strings(h,bs[0])[b'hello'][0][1]==b'Hola, mundo'
assert all(strings(h,b)[b'hello'][0][1]==b'Hello' for b in bs[1:])
assert all(b[-5:]==b'voice' for b in bs)
assert report['changed_strings']==1
bad=bytearray(spanish);struct.pack_into('<I',bad,24+56,0xffffffff)
try: merge(original,bad)
except ValueError: pass
else: raise AssertionError('invalid text offset accepted')
bad=bytearray(spanish);struct.pack_into('<I',bad,24+52,12)
try: merge(original,bad)
except ValueError: pass
else: raise AssertionError('invalid variant range accepted')
print('Spanish merge: translated text, all other languages, original voices, invalid offsets passed')

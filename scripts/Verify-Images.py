"""Validate RP2040 UF2, source binary, manifest hashes and ARM ELF before flashing."""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from bin_to_uf2 import verify
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('image',type=Path)
args=parser.parse_args()
base=args.image.with_suffix('')
binary=base.with_suffix('.bin').read_bytes()
uf2=base.with_suffix('.uf2').read_bytes()
elf=base.with_suffix('.elf').read_bytes()
metadata=json.loads(base.with_suffix('.manifest.json').read_text())
verify(uf2,binary)
if hashlib.sha256(uf2).hexdigest()!=metadata['uf2_sha256']: raise ValueError('UF2 SHA256 mismatch')
if hashlib.sha256(binary).hexdigest()!=metadata['binary_sha256']: raise ValueError('Binary SHA256 mismatch')
if elf[:7]!=b'\x7fELF\x01\x01\x01' or struct.unpack_from('<H',elf,18)[0]!=40:
    raise ValueError('Not ELF32 little-endian ARM')
entry=struct.unpack_from('<I',elf,24)[0]
if not 0x10000000<=entry<0x10200000: raise ValueError('Unexpected ELF entry point')
print(json.dumps({'image':str(args.image),'rp2040_uf2_verified':True,
                  'binary_roundtrip_verified':True,'arm_elf_verified':True,
                  'entry':hex(entry),'sha256':metadata['uf2_sha256']}))

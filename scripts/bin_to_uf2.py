"""RP2040-only UF2 packaging and round-trip verification of SDK-linked binaries."""
import argparse
import hashlib
import json
import struct
from pathlib import Path

MAGIC0, MAGIC1, END = 0x0A324655, 0x9E5D5157, 0x0AB16F30
FAMILY, BASE = 0xE48BFF56, 0x10000000

def package(binary: bytes) -> bytes:
    if not 264 <= len(binary) <= 2 * 1024 * 1024:
        raise ValueError('Image must fit original Pico 2 MiB flash and contain vectors')
    stack, reset = struct.unpack_from('<II', binary, 256)
    if not 0x20000000 < stack <= 0x20042000:
        raise ValueError(f'Unexpected RP2040 SRAM stack vector: {stack:#x}')
    if not reset & 1 or not BASE <= (reset & ~1) < BASE + len(binary):
        raise ValueError(f'Unexpected Thumb reset vector: {reset:#x}')
    count = (len(binary) + 255) // 256
    blocks = []
    for index in range(count):
        chunk = binary[index*256:(index+1)*256].ljust(256,b'\0')
        header = struct.pack('<8I',MAGIC0,MAGIC1,0x2000,BASE+index*256,256,index,count,FAMILY)
        blocks.append(header + chunk + bytes(220) + struct.pack('<I',END))
    return b''.join(blocks)

def verify(uf2: bytes, binary: bytes):
    count = (len(binary)+255)//256
    if len(uf2) != count*512:
        raise ValueError('UF2 byte count mismatch')
    restored = bytearray()
    for index in range(count):
        block = uf2[index*512:(index+1)*512]
        expected = (MAGIC0,MAGIC1,0x2000,BASE+index*256,256,index,count,FAMILY)
        if struct.unpack_from('<8I',block) != expected or struct.unpack_from('<I',block,508)[0] != END:
            raise ValueError(f'Invalid UF2 block {index}')
        restored.extend(block[32:288])
    if restored[:len(binary)] != binary or any(restored[len(binary):]):
        raise ValueError('UF2 round-trip failed')

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binary',type=Path)
    parser.add_argument('uf2',type=Path)
    args=parser.parse_args()
    binary=args.binary.read_bytes(); result=package(binary); verify(result,binary)
    args.uf2.write_bytes(result)
    metadata={'binary':str(args.binary),'uf2':str(args.uf2),'binary_bytes':len(binary),
              'uf2_bytes':len(result),'family':'RP2040','family_id':hex(FAMILY),
              'binary_sha256':hashlib.sha256(binary).hexdigest(),
              'uf2_sha256':hashlib.sha256(result).hexdigest(),'round_trip_verified':True}
    args.uf2.with_suffix('.manifest.json').write_text(json.dumps(metadata,indent=2))
    print(json.dumps(metadata))

if __name__=='__main__': main()

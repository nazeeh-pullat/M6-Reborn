"""Offline tests for rejecting malformed or wrong-target flash artifacts."""
import struct
import unittest
from bin_to_uf2 import package, verify

def image():
    result=bytearray(700)
    struct.pack_into('<II',result,256,0x20040000,0x10000101)
    return bytes(result)

class Tests(unittest.TestCase):
    def test_last_partial_block_roundtrip(self):
        binary=image(); verify(package(binary),binary)
    def test_wrong_target_vector_rejected(self):
        binary=bytearray(image()); struct.pack_into('<I',binary,256,0x08000000)
        with self.assertRaises(ValueError): package(binary)
    def test_wrong_family_rejected(self):
        binary=image(); uf2=bytearray(package(binary)); struct.pack_into('<I',uf2,28,0)
        with self.assertRaises(ValueError): verify(uf2,binary)
    def test_wrong_address_rejected(self):
        binary=image(); uf2=bytearray(package(binary)); struct.pack_into('<I',uf2,12,0x20000000)
        with self.assertRaises(ValueError): verify(uf2,binary)
    def test_corruption_and_truncation_rejected(self):
        binary=image(); uf2=bytearray(package(binary)); uf2[40]^=1
        with self.assertRaises(ValueError): verify(uf2,binary)
        with self.assertRaises(ValueError): verify(package(binary)[:-1],binary)

if __name__=='__main__': unittest.main(verbosity=2)

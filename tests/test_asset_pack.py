#!/usr/bin/env python3
"""Host roundtrip/corruption tests for the actual DOS resource decoder."""
import importlib.util
from pathlib import Path
import random
import struct
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location('pack_assets', ROOT/'tools/pack_assets.py')
packer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(packer)

HARNESS = r'''
#include "ASSETS.H"
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char **argv) {
    if (argc != 3) return 1;
    AssetStream stream;
    if (!stream.open(argv[1])) return 2;
    if (stream.planes() != 2) return 3;
    FILE *output = fopen("DECODED.RAW", "wb");
    unsigned char bytes[2048];
    unsigned long remaining = 76800UL;
    unsigned int requested = (unsigned int)atoi(argv[2]);
    while (remaining) {
        unsigned int length = remaining < requested ? (unsigned int)remaining : requested;
        if (!stream.read(bytes, length)) { fclose(output); return 4; }
        if (fwrite(bytes, 1, length, output) != length) return 5;
        remaining -= length;
    }
    fclose(output);
    if (stream.read(bytes, 1)) return 6;
    stream.close();
    stream.close();
    return 0;
}
'''


class AssetTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory(prefix='kakuro-assets-')
        cls.directory = Path(cls.temp.name)
        harness = cls.directory/'loader.cpp'
        harness.write_text(HARNESS)
        cls.executable = cls.directory/'loader'
        subprocess.run(['g++','-O2','-Wall','-Wextra','-Dstricmp=strcasecmp',
                        '-I',str(ROOT/'src'),str(ROOT/'src/ASSETS.CPP'),
                        str(harness),'-o',str(cls.executable)], check=True)
        cls.pack = (ROOT/'assets/KAKURO.DAT').read_bytes()

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def run_loader(self, data, name='START.VGA', chunk=2048):
        (self.directory/'KAKURO.DAT').write_bytes(data)
        return subprocess.run([str(self.executable),name,str(chunk)],cwd=self.directory).returncode

    def test_packet_roundtrips(self):
        randomizer = random.Random(8088)
        samples = [b'',b'X',b'X'*2,b'X'*129,b'X'*10000,bytes(range(256))*11]
        samples += [bytes(randomizer.randrange(256) for _ in range(n))
                    for n in (127,128,129,2047,2048,2049,76800)]
        for sample in samples:
            self.assertEqual(packer.decode(packer.encode(sample)),sample)
        for truncated in (b'\x80',b'\x01\xFF'):
            with self.assertRaises(ValueError):
                packer.decode(truncated)

    def test_actual_images_arbitrary_chunk_boundaries(self):
        for name, folder in [('START','splash'),('BOARD','background'),('MENU','menu')]:
            source = (ROOT/f'assets/{folder}/{name}.VGA').read_bytes()[:76800]
            for chunk in (1,79,80,127,128,129,2047,2048):
                with self.subTest(name=name,chunk=chunk):
                    self.assertEqual(self.run_loader(self.pack,name+'.VGA',chunk),0)
                    self.assertEqual((self.directory/'DECODED.RAW').read_bytes(),source)

    def test_missing_and_custom_entries(self):
        self.assertEqual(self.run_loader(self.pack,'MISSING.VGA'),2)
        self.assertEqual(self.run_loader(self.pack,'custom/START.VGA'),2)
        self.assertEqual(self.run_loader(self.pack,'start.vga'),0)

    def test_directory_corruption(self):
        for location,value in ((0,0),(8,0),(8,255),(9+24,4),(9+25,64)):
            data = bytearray(self.pack)
            data[location] = value
            self.assertEqual(self.run_loader(data),2)
        for field,value in ((12,0),(12,0xffffffff),(16,0),(16,0xffffffff),(20,1)):
            data = bytearray(self.pack)
            struct.pack_into('<I',data,9+field,value)
            self.assertEqual(self.run_loader(data),2)
        self.assertEqual(self.run_loader(self.pack[:20]),2)
        self.assertEqual(self.run_loader(self.pack[:-1],'MENU.VGA'),2)

    def test_packet_corruption(self):
        # A valid directory with malformed payload must fail while decoding.
        offset = 9 + packer.ENTRY.size
        palette = bytes([0]*12)
        for payload in (b'\x80',b'\x7F\x01',b'\xFF\x00'*600+b'\x00\x00'):
            data = packer.MAGIC+b'\x01'+packer.ENTRY.pack(
                b'START.VGA',offset,len(payload),76800,2,palette)+payload
            self.assertEqual(self.run_loader(data),4)


if __name__ == '__main__':
    unittest.main()

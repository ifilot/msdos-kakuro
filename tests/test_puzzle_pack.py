#!/usr/bin/env python3
"""Compare the actual archive loader to source boards and reject corrupt records."""
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest
ROOT=Path(__file__).resolve().parent.parent
HARNESS=r'''
#include "PUZZLE.H"
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv) {
    if(argc<3)return 1;
    Puzzle packed;
    if(!packed.loadPacked((unsigned int)atoi(argv[2]),argv[1]))return 2;
    if(argc==3)return 0;
    Puzzle source;
    if(!source.load(argv[3]))return 3;
    if(packed.rows()!=source.rows() || packed.columns()!=source.columns()
        || packed.difficulty()!=source.difficulty() || strcmp(packed.source(),source.source()))return 4;
    for(int r=0;r<source.rows();++r)for(int c=0;c<source.columns();++c)
        if(memcmp(packed.cell(r,c),source.cell(r,c),sizeof(Puzzle::Cell)))return 5;
    return 0;
}
'''
class ArchiveTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp=tempfile.TemporaryDirectory(prefix='kakuro-puzzles-')
        cls.directory=Path(cls.temp.name);cls.binary=cls.directory/'loader'
        (cls.directory/'main.cpp').write_text(HARNESS)
        subprocess.run(['g++','-O2','-Wall','-Wextra','-Dstricmp=strcasecmp','-Dstrnicmp=strncasecmp',
                        '-I',str(ROOT/'src'),str(ROOT/'src/PUZZLE.CPP'),str(cls.directory/'main.cpp'),
                        '-o',str(cls.binary)],check=True)
        cls.data=(ROOT/'assets/PUZZLES.DAT').read_bytes()
    @classmethod
    def tearDownClass(cls):cls.temp.cleanup()
    def run_loader(self,data,id=1,source=None):
        pack=self.directory/'PUZZLES.DAT';pack.write_bytes(data)
        command=[str(self.binary),str(pack),str(id)]
        if source:command.append(str(source))
        return subprocess.run(command).returncode
    def test_every_field_of_every_puzzle(self):
        for id in range(1,97):
            name=f'{id:03d}'
            with self.subTest(id=id):
                self.assertEqual(self.run_loader(self.data,id,ROOT/'assets/puzzles'/f'{name}.puz'),0)
    def test_missing_ids(self):
        for id in (0,97,98,99,100,101):self.assertEqual(self.run_loader(self.data,id),2)
    def test_invalid_index_and_bounds(self):
        for location,value in ((0,0),(8,0),(10+9+6,2)):
            data=bytearray(self.data);data[location]=value
            self.assertEqual(self.run_loader(data),2)
        for field,value in ((0,1),(0,65535),(2,0),(2,241),(4,0),(4,241)):
            data=bytearray(self.data);struct.pack_into('<H',data,19+field,value)
            self.assertEqual(self.run_loader(data),2)
        self.assertEqual(self.run_loader(self.data[:19]),2)
        self.assertEqual(self.run_loader(self.data[:920]),2)
    def fixture(self,raw,codec=0,payload=None):
        payload=raw if payload is None else payload
        directory=bytearray(909)
        struct.pack_into('<HHHBH',directory,9,919,len(payload),len(raw),codec,sum(raw)&65535)
        return b'KAKPUZ1\0'+struct.pack('<H',101)+directory+payload
    def test_checksum_and_cell_validation(self):
        raw=bytes((1,1,0,0,1))
        self.assertEqual(self.run_loader(self.fixture(raw)),0)
        for bad in (bytes((0,1,0,0,1)),bytes((16,1,0,0,1)),bytes((1,1,10,0,1)),
                    bytes((1,1,0,96,1)),bytes((1,1,0,0,19)),bytes((1,1,0,0,33))):
            self.assertEqual(self.run_loader(self.fixture(bad)),2)
        data=bytearray(self.fixture(raw));data[-1]^=1
        self.assertEqual(self.run_loader(data),2)
    def test_compression_boundaries(self):
        raw=bytes((1,1,0,0,1))
        self.assertEqual(self.run_loader(self.fixture(raw,1,bytes((4,))+raw)),0)
        for payload in (b'\x80',b'\x7f\x00',b'\xff\x00',bytes((4,))+raw+b'\0\0'):
            self.assertEqual(self.run_loader(self.fixture(raw,1,payload)),2)
if __name__=='__main__':unittest.main()

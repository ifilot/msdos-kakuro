#!/usr/bin/env python3
"""Replay imported streams against fake ports and an independent event model."""
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent.parent
ASSETS = ROOT / 'assets/sound'


def decode(data, fm):
    events, boundaries = {}, {}
    pos, tick = 16, 0
    loop = struct.unpack_from('<I', data, 8)[0]
    while pos < len(data):
        boundaries[pos] = tick
        a = data[pos]
        if (fm and a == 0) or (not fm and a == 253):
            wait = data[pos+1]
            pos += 2
            if not wait:
                return events, tick, boundaries.get(loop)
            tick += wait
        elif fm:
            events.setdefault(tick, []).append(tuple(data[pos:pos+2]))
            pos += 2
        else:
            count = 2 if a & 240 in (192, 208) else 3
            events.setdefault(tick, []).append(tuple(data[pos:pos+count]))
            pos += count
    raise ValueError('Missing stream end')


def frame(decoded, tick):
    events, duration, loop = decoded
    if tick >= duration:
        if loop is None:
            return []
        tick = loop + (tick-duration) % (duration-loop)
    return events.get(tick, [])


def channel(reg):
    for c, slot in enumerate((0, 1, 2, 8, 9, 10, 16, 17, 18)):
        if reg in (160+c, 176+c, 192+c):
            return c
        if any(reg in (base+slot, base+slot+3) for base in (32, 64, 96, 128, 224)):
            return c
    return None


def model(music, effect, at, ticks, fm):
    m, s = decode(music, fm), decode(effect, fm)
    mask = struct.unpack_from('<H', effect, 12)[0]
    owned, active = set(), False
    hw, shadow = [0]*256, [0]*256
    for t in range(ticks):
        out = bytearray()
        if t == at:
            active = True
            owned = {c for c in range(16) if mask >> c & 1}
            if fm:
                for c in owned:
                    hw[176+c] = shadow[176+c] & ~32
        for event in frame(m, t):
            if fm:
                reg, val = event
                shadow[reg] = val
                if channel(reg) not in owned:
                    hw[reg] = val
            else:
                out.extend(event)
        if active:
            if t-at < s[1]:
                for event in frame(s, t-at):
                    if fm:
                        hw[event[0]] = event[1]
                    else:
                        out.extend(event)
            else:
                if fm:
                    for c in owned:
                        hw[176+c] = 0
                    for reg in range(32, 246):
                        if channel(reg) in owned:
                            hw[reg] = shadow[reg] & ~32 if 176 <= reg <= 184 else shadow[reg]
                else:
                    for c in sorted(owned):
                        out.extend((176|c, 123, 0, 224|c, 0, 64))
                owned, active = set(), False
        yield bytes(hw[32:246]).hex() if fm else out.hex()


class SoundTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.directory = Path(cls.temp.name)
        cls.host = cls.directory / 'sound-host'
        subprocess.run(['g++', '-std=c++98', '-Wall', '-Wextra', '-Werror',
                        '-DSNDDRV_HOST', '-o', str(cls.host),
                        str(ROOT / 'tests/host/sound_main.cpp'),
                        str(ROOT / 'src/SNDDRV.CPP')], check=True)

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def test_pack_matches_imported_exports(self):
        data = (ROOT / 'assets/SOUND.DAT').read_bytes()
        self.assertEqual(data[:8], b'KSN1\x06\x00\x00\x00')
        for i, (folder, ext) in enumerate([('fm', 'OPL'), ('midi', 'MDS')]):
            for j, name in enumerate(('MENU', 'GAME', 'SFX')):
                offset, size, checksum = struct.unpack_from('<IHH', data, 8+(i*3+j)*8)
                exported = (ASSETS / folder / (name+'.'+ext)).read_bytes()
                self.assertEqual(data[offset:offset+size], exported)
                self.assertEqual(checksum, sum(exported) & 65535)

    def test_playback_loops_and_effect_channel_restoration(self):
        for dev, folder, ext in [('adlib', 'fm', 'OPL'), ('sb', 'fm', 'OPL'), ('mpu', 'midi', 'MDS')]:
            bank = (ASSETS / folder / ('SFX.'+ext)).read_bytes()
            # Discover complete stream boundaries independently of sfx.h.
            offsets = [n for n in range(len(bank)) if bank[n:n+4] in (b'OPLR', b'MIDR')]
            offsets.append(len(bank))
            for song in ('MENU', 'GAME'):
                music_path = ASSETS / folder / (song+'.'+ext)
                music = music_path.read_bytes()
                duration = decode(music, folder=='fm')[1]
                for effect_index, at in [(0, 0), (4, duration-5), (17, 100)]:
                    with self.subTest(device=dev, song=song, effect=effect_index):
                        effect = bank[offsets[effect_index]:offsets[effect_index+1]]
                        effect_path = self.directory / 'effect'
                        effect_path.write_bytes(effect)
                        ticks = duration+400
                        result = subprocess.run([str(self.host), dev, str(music_path), str(effect_path),
                                                 str(at), str(ticks)], check=True, text=True, capture_output=True)
                        actual = [line.split(' ', 1)[1] for line in result.stdout.splitlines()]
                        self.assertEqual(actual, list(model(music, effect, at, ticks, folder=='fm')))

    def test_validator_accepts_all_streams_and_rejects_malformed_input(self):
        for folder, ext, dev in [('fm', 'OPL', 1), ('midi', 'MDS', 3)]:
            bank = (ASSETS / folder / ('SFX.'+ext)).read_bytes()
            offsets = [n for n in range(len(bank)) if bank[n:n+4] in (b'OPLR', b'MIDR')] + [len(bank)]
            streams = [(ASSETS / folder / (name+'.'+ext)).read_bytes() for name in ('MENU', 'GAME')]
            streams += [bank[a:b] for a,b in zip(offsets, offsets[1:])]
            path = self.directory / 'validate'
            def valid(data, device=dev):
                path.write_bytes(data)
                return subprocess.run([str(self.host), 'validate', str(device), str(path)]).returncode == 0
            for stream in streams:
                self.assertTrue(valid(stream))
                self.assertFalse(valid(stream[:-1]))
                self.assertFalse(valid(stream[:15]))
                self.assertFalse(valid(stream, 3 if dev==1 else 1))
            for field, value in [(4,2), (5,2), (6,59), (8,0), (10,1), (14,1)]:
                bad = bytearray(streams[0]); bad[field] = value
                if field == 8: bad[8:12] = bytes(4)
                self.assertFalse(valid(bad), f'Invalid header byte {field}')


if __name__ == '__main__':
    unittest.main()

#!/usr/bin/env python3
"""Build the DOS image resource pack; stdlib only, lossless byte RLE."""
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parent.parent
MAGIC = b'KAKPAK1\0'
ENTRY = struct.Struct('<12sIIIB12s')


def encode(data):
    """Packets: 0..127 literal length+1; 128..255 repeat length-127."""
    result = bytearray()
    pos = 0
    while pos < len(data):
        run = 1
        while pos + run < len(data) and run < 128 and data[pos+run] == data[pos]:
            run += 1
        if run >= 3:
            result.extend((127 + run, data[pos]))
            pos += run
            continue
        start = pos
        pos += run
        while pos < len(data) and pos - start < 128:
            run = 1
            while pos + run < len(data) and run < 3 and data[pos+run] == data[pos]:
                run += 1
            if run >= 3:
                break
            pos += min(run, 128 - (pos-start))
        result.append(pos - start - 1)
        result.extend(data[start:pos])
    return bytes(result)


def decode(data):
    result = bytearray()
    pos = 0
    while pos < len(data):
        control = data[pos]
        pos += 1
        length = control - 127 if control & 128 else control + 1
        if control & 128:
            if pos == len(data):
                raise ValueError('Truncated repeat packet')
            result.extend([data[pos]] * length)
            pos += 1
        else:
            if pos + length > len(data):
                raise ValueError('Truncated literal packet')
            result.extend(data[pos:pos+length])
            pos += length
    return bytes(result)


def main():
    colors = bytes((50,50,37,33,25,25,16,12,12,8,8,8))
    items = [
        ('START.VGA', ROOT/'assets/splash/START.VGA', (ROOT/'assets/splash/START.PAL').read_bytes()),
        ('BOARD.VGA', ROOT/'assets/background/BOARD.VGA', colors),
        ('MENU.VGA', ROOT/'assets/menu/MENU.VGA', colors),
    ]
    directory = bytearray()
    payload = bytearray()
    offset = len(MAGIC) + 1 + len(items)*ENTRY.size
    source_size = 12
    for name, path, palette in items:
        original = path.read_bytes()
        source_size += len(original)
        if len(original) not in (76800,153600) or any(original[76800:]):
            raise ValueError(f'{name}: expected four-color 640x480 planar image')
        if len(palette) != 12 or max(palette) > 63:
            raise ValueError(f'{name}: invalid six-bit DAC palette')
        raw = original[:76800]
        packed = encode(raw)
        if decode(packed) != raw:
            raise ValueError(f'{name}: compression roundtrip failed')
        directory.extend(ENTRY.pack(name.encode(),offset,len(packed),len(raw),2,palette))
        payload.extend(packed)
        offset += len(packed)
        print(f'{name}: {len(original):,} -> {len(packed):,} bytes')
    output = MAGIC + bytes([len(items)]) + directory + payload
    (ROOT/'assets/KAKURO.DAT').write_bytes(output)
    print(f'KAKURO.DAT: {len(output):,} bytes; {100*(1-len(output)/source_size):.1f}% smaller than loose images/palette')


if __name__ == '__main__':
    main()

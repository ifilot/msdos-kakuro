#!/usr/bin/env python3
"""Convert the CX16 packed 2bpp courtyard into two VGA bit planes and DAC RGB."""
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ASSETS = ROOT / 'assets/splash'


def main():
    packed = (ASSETS / 'courtyard.bin').read_bytes()
    palette = (ASSETS / 'courtyard.pal').read_bytes()
    if len(packed) != 76800 or len(palette) != 8:
        raise ValueError('Expected a 640x480 2bpp bitmap and four CX16 colors')
    planes = [bytearray(38400), bytearray(38400)]
    for offset, byte in enumerate(packed):
        for pixel in range(4):
            color = (byte >> (6 - pixel * 2)) & 3
            x = offset * 4 + pixel
            for plane in range(2):
                if color & (1 << plane):
                    planes[plane][x // 8] |= 0x80 >> (x & 7)
    rgb = bytearray()
    for i in range(4):
        gb, r = palette[i * 2:i * 2 + 2]
        if r > 15:
            raise ValueError('Invalid CX16 palette component')
        rgb.extend(component * 63 // 15 for component in (r, gb >> 4, gb & 15))
    (ASSETS / 'START.VGA').write_bytes(planes[0] + planes[1])
    (ASSETS / 'START.PAL').write_bytes(rgb)


if __name__ == '__main__':
    main()

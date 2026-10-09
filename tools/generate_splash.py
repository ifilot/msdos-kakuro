#!/usr/bin/env python3
"""Convert the CX16 courtyard pixels into VGA planes with a richer DAC palette."""
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
    # Keep the source pixels; use a richer warm palette for VGA/CRT displays.
    rgb = bytes((57,53,36, 35,21,19, 14,9,8, 5,5,6))
    (ASSETS / 'START.VGA').write_bytes(planes[0] + planes[1])
    (ASSETS / 'START.PAL').write_bytes(rgb)


if __name__ == '__main__':
    main()

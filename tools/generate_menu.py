#!/usr/bin/env python3
"""Prepare the CX16 journal artwork for native VGA; regeneration needs Pillow."""
from pathlib import Path
from PIL import Image

ASSETS = Path(__file__).resolve().parent.parent / 'assets/menu'


def main():
    palette = Image.new('P', (1, 1))
    palette.putpalette([204,204,153,136,102,102,68,51,51,34,34,34] + [0,0,0]*252)
    with Image.open(ASSETS / 'journal-source.png') as source:
        image = source.convert('RGB').resize((640,480), Image.Resampling.LANCZOS)
    image = image.quantize(palette=palette, dither=Image.Dither.NONE)
    image = image.point(lambda index: min(index, 3))
    image.save(ASSETS / 'journal.png')
    planes = [bytearray(38400) for _ in range(4)]
    for pixel, color in enumerate(image.tobytes()):
        for plane in range(4):
            if color & (1 << plane):
                planes[plane][pixel // 8] |= 0x80 >> (pixel & 7)
    (ASSETS / 'MENU.VGA').write_bytes(b''.join(planes))


if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""Draw a pixel-exact Kakuro icon and encode Quickmenu's EGA ICC format.

The editable drawing uses integer coordinates only; no quantization or
antialiasing. Format reference: https://github.com/ifilot/quickmenu-icon-creator
"""
from pathlib import Path
import struct
import zlib

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / 'assets/quickmenu'
EGA = [(0,0,0),(0,0,170),(0,170,0),(0,170,170),
       (170,0,0),(170,0,170),(170,85,0),(170,170,170),
       (85,85,85),(85,85,255),(85,255,85),(85,255,255),
       (255,85,85),(255,85,255),(255,255,85),(255,255,255)]


def draw():
    pixels = [[None]*32 for _ in range(32)]

    def rect(x,y,w,h,color):
        for row in range(y,y+h):
            for col in range(x,x+w):
                pixels[row][col] = color

    # Raised wooden puzzle tile with a transparent margin and hard shadow.
    rect(3,3,27,27,0)
    rect(1,1,28,28,0)
    rect(2,2,26,26,6)
    rect(2,2,26,1,14)
    rect(2,2,1,26,14)
    rect(4,4,22,22,0)
    # A 3x3 board: diagonal clue cells and cream answer cells.
    for row in range(3):
        for col in range(3):
            x,y = 5+col*7,5+row*7
            clue = row==0 or col==0
            rect(x,y,6,6,8 if clue else 15)
            if clue and (row or col):
                for i in range(6):
                    rect(x+i,y+i,1,1,7)
                # Small clue marks, kept apart from the diagonal.
                rect(x+4,y,2,1,14)
                rect(x,y+4,1,2,14)
    # Legible tiny digits in the answer cells.
    digits = {'1':['010','110','010','010','111'],
              '3':['111','001','111','001','111']}
    for row,col,digit in [(1,1,'1'),(1,2,'3'),(2,1,'3'),(2,2,'1')]:
        x,y = 13+(col-1)*7,12+(row-1)*7
        for dy,line in enumerate(digits[digit]):
            for dx,value in enumerate(line):
                if value=='1': rect(x+dx,y+dy,1,1,0)
    return pixels


def bitmap(pixels, mask=False):
    data = bytearray(struct.pack('<HH',31,31))
    for row in pixels:
        values = [(15 if value is not None else 0) if mask
                  else (value or 0) for value in row]
        for plane in range(4):
            for start in range(0,32,8):
                data.append(sum(((values[start+i]>>plane)&1)<<(7-i)
                                for i in range(8)))
    return data


def png(pixels):
    def chunk(kind,data):
        return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data))
    raw = bytearray()
    for row in pixels:
        raw.append(0)
        for value in row:
            raw.extend((*EGA[value],255) if value is not None else (0,0,0,0))
    return (b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',32,32,8,6,0,0,0))
            +chunk(b'IDAT',zlib.compress(raw))+chunk(b'IEND',b''))


def main():
    OUT.mkdir(parents=True,exist_ok=True)
    pixels = draw()
    (OUT/'KAKURO.ICC').write_bytes(bitmap(pixels)+bitmap(pixels,True))
    (OUT/'KAKURO.PNG').write_bytes(png(pixels))
    svg = ['<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 32 32" width="320" height="320" shape-rendering="crispEdges">']
    for y,row in enumerate(pixels):
        for x,value in enumerate(row):
            if value is not None:
                color = '#%02x%02x%02x' % EGA[value]
                svg.append(f'<rect x="{x}" y="{y}" width="1" height="1" fill="{color}"/>')
    (OUT/'KAKURO.SVG').write_text('\n'.join(svg+['</svg>'])+'\n')
    print('Generated Quickmenu icon: 32x32, EGA, 1032-byte ICC with mask')


if __name__ == '__main__':
    main()

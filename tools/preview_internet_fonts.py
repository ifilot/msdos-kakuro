#!/usr/bin/env python3
"""Render real BDF bitmap glyphs for choosing a larger game font (Pillow)."""
from pathlib import Path
import json
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent.parent
ASSETS = ROOT / 'assets/fonts/internet'
FONTS = [
    ('Times Roman', 'timR18.bdf'),
    ('New Century Schoolbook', 'ncenR18.bdf'),
    ('Courier', 'courR18.bdf'),
    ('Helvetica', 'helvR18.bdf'),
    ('Lucida Sans', 'luRS18.bdf'),
    ('Lucida Bright', 'lubR18.bdf'),
    ('Spleen', 'spleen-12x24.bdf'),
    ('ProFont', 'profont22.bdf'),
    ('UW ttyp0', 't0-22-uni.bdf'),
    ('Terminus', 'ter-u24n.bdf'),
]
PAPER, CELL, HINT = '#cccc99', '#ded6aa', '#ad927d'
INK, BROWN, ROSE, GOLD = '#222222', '#443333', '#886666', '#ffd271'


def read_bdf(path):
    glyphs = {}
    current = None
    bitmap = False
    for line in path.read_text(errors='replace').splitlines():
        fields = line.split()
        if not fields:
            continue
        if fields[0] == 'STARTCHAR':
            current = {'rows': []}
        elif fields[0] == 'ENDCHAR':
            if current['code'] >= 0:
                w, h, _, _ = current['box']
                assert len(current['rows']) == h
                image = Image.new('1', (max(w, 1), max(h, 1)))
                for y, row in enumerate(current['rows']):
                    value = int(row, 16)
                    bits = len(row) * 4
                    for x in range(w):
                        if value & (1 << (bits - 1 - x)):
                            image.putpixel((x, y), 1)
                current['image'] = image
                glyphs[current['code']] = current
            current = None
            bitmap = False
        elif current is not None:
            if fields[0] == 'ENCODING':
                current['code'] = int(fields[1])
            elif fields[0] == 'DWIDTH':
                current['advance'] = int(fields[1])
            elif fields[0] == 'BBX':
                current['box'] = tuple(map(int, fields[1:]))
            elif fields[0] == 'BITMAP':
                bitmap = True
            elif bitmap:
                current['rows'].append(line.strip())
    for c in '0123456789WELDON! ':
        assert ord(c) in glyphs, (path, c)
    return glyphs


def text_mask(font, text):
    image = Image.new('1', (sum(font[ord(c)]['advance'] for c in text) + 64, 96))
    x, baseline = 24, 56
    for c in text:
        g = font[ord(c)]
        w, h, dx, dy = g['box']
        image.paste(g['image'], (x + dx, baseline - dy - h))
        x += g['advance']
    box = image.getbbox()
    assert box
    return image.crop(box)


def centered(image, mask, box, color):
    x, y, w, h = box
    assert mask.width <= w and mask.height <= h
    image.paste(color, (x + (w - mask.width) // 2,
                        y + (h - mask.height) // 2), mask)


def main():
    label = ImageFont.truetype('DejaVuSans.ttf', 13)
    small = ImageFont.truetype('DejaVuSans.ttf', 11)
    fonts = [read_bdf(ASSETS / 'source' / filename) for _, filename in FONTS]
    metrics = []
    for page in range(2):
        image = Image.new('RGB', (640, 622), PAPER)
        draw = ImageDraw.Draw(image)
        draw.text((40, 14), f'BITMAP FONT OPTIONS {page * 5 + 1} - {page * 5 + 5}',
                  font=label, fill=BROWN)
        draw.text((40, 34), 'Native bitmap pixels - digits in 56px cells, then a heading',
                  font=small, fill=BROWN)
        for row in range(5):
            i = page * 5 + row
            top = 56 + row * 110
            font = fonts[i]
            name, filename = FONTS[i]
            heights = [text_mask(font, c).height for c in '0123456789']
            draw.text((40, top), f'{i + 1}. {name}', font=label, fill=BROWN)
            draw.text((440, top + 2), f'digit ink: {max(heights)}px high', font=small, fill=BROWN)
            for n, c in enumerate('1234567890'):
                x, y = 40 + n * 56, top + 20
                draw.rectangle((x, y, x + 56, y + 56), fill=CELL if n % 2 == 0 else HINT,
                               outline=ROSE)
                centered(image, text_mask(font, c), (x + 1, y + 1, 55, 55), INK)
            draw.rectangle((40, top + 81, 600, top + 107), fill=BROWN)
            centered(image, text_mask(font, 'WELL DONE!'), (40, top + 81, 560, 26), GOLD)
            metrics.append({'number': i + 1, 'name': name, 'file': filename,
                            'digit_height': max(heights)})
        image.save(ASSETS / f'comparison-{page + 1}.png')
        image.resize((1280, 1244), Image.Resampling.NEAREST).save(ASSETS / f'comparison-{page + 1}-2x.png')
    (ASSETS / 'fonts.json').write_text(json.dumps(metrics, indent=2) + '\n')


if __name__ == '__main__':
    main()

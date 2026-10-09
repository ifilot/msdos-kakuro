#!/usr/bin/env python3
"""Convert the chosen ProFont BDF into a Borland-compatible ASCII bitmap table."""
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def main():
    source = ROOT / 'assets/fonts/internet/source/profont22.bdf'
    glyphs = {}
    current = None
    bitmap = False
    for line in source.read_text(errors='replace').splitlines():
        fields = line.split()
        if not fields:
            continue
        if fields[0] == 'STARTCHAR':
            current = {'rows': []}
        elif fields[0] == 'ENDCHAR':
            code = current['code']
            if 32 <= code <= 126:
                assert current['advance'] == 12
                w, h, dx, dy = current['box']
                assert len(current['rows']) == h
                rows = [0] * 22
                for y, row in enumerate(current['rows']):
                    bits = len(row) * 4
                    value = int(row, 16)
                    for x in range(w):
                        if value & (1 << (bits - 1 - x)):
                            px, py = dx + x, 18 - dy - h + y
                            assert 0 <= px < 12 and 0 <= py < 22
                            rows[py] |= 1 << (11 - px)
                glyphs[code] = rows
            current = None
            bitmap = False
        elif current is not None:
            if fields[0] == 'ENCODING': current['code'] = int(fields[1])
            elif fields[0] == 'DWIDTH': current['advance'] = int(fields[1])
            elif fields[0] == 'BBX': current['box'] = tuple(map(int, fields[1:]))
            elif fields[0] == 'BITMAP': bitmap = True
            elif bitmap: current['rows'].append(line.strip())
    assert len(glyphs) == 95
    lines = ['// Generated from ProFont22. MIT license: assets/fonts/PROFONT-LICENSE.txt',
             'static const unsigned short profontRows[95][22] = {']
    for code in range(32, 127):
        lines.append('    {' + ','.join('0x%03X' % row for row in glyphs[code]) + '},')
    lines.append('};')
    lines.append('static const unsigned char profontBounds[95][4] = {')
    for code in range(32, 127):
        pixels = [(x, y) for y, row in enumerate(glyphs[code])
                  for x in range(12) if row & (1 << (11 - x))]
        bounds = (min(x for x, y in pixels), max(x for x, y in pixels),
                  min(y for x, y in pixels), max(y for x, y in pixels)) if pixels else (12, 0, 22, 0)
        lines.append('    {' + ','.join(map(str, bounds)) + '},')
    lines.append('};\n')
    (ROOT / 'src/PROFDAT.H').write_text('\n'.join(lines))


if __name__ == '__main__':
    main()

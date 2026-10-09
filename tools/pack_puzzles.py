#!/usr/bin/env python3
"""Pack source boards into indexed, five-bit cells with optional byte RLE."""
from pathlib import Path
import struct
from generate_catalog import metadata
from pack_assets import encode, decode

ROOT = Path(__file__).resolve().parent.parent
MAGIC = b'KAKPUZ1\0'
ENTRY = struct.Struct('<HHHBH')
SLOTS = 101  # slot 0 reserved, numbered puzzles=1..100; zero offsets are empty slots.


def record(path):
    info = metadata(path)
    if not info:
        return None
    source = ''
    cells = []
    for line in path.read_text().splitlines():
        text = line.strip()
        if not text:
            continue
        if text.startswith('#') and (len(text) == 1 or not text[1].isdigit()):
            if not cells and not source:
                source = text[1:].strip()[:95]
            continue
        cells.extend(int(token[1:]) + 9 if token.startswith('#') else int(token)
                     for token in text.split())
    source_bytes = source.encode('ascii')
    if any(b < 32 or b > 126 for b in source_bytes):
        raise ValueError(f'{path}: non-printable source metadata')
    output = bytearray((*info, len(source_bytes)))
    output.extend(source_bytes)
    bits = available = 0
    for cell in cells:
        bits |= cell << available
        available += 5
        while available >= 8:
            output.append(bits & 255)
            bits >>= 8
            available -= 8
    if available:
        output.append(bits)
    return bytes(output)


def main():
    directory = bytearray(SLOTS * ENTRY.size)
    payload = bytearray()
    offset = len(MAGIC) + 2 + len(directory)
    count = source_size = 0
    for identifier in range(1, SLOTS):
        name = f'{identifier:03d}'
        path = ROOT / 'assets/puzzles' / (name + '.puz')
        if not path.exists():
            continue
        raw = record(path)
        if raw is None:
            continue
        compressed = encode(raw)
        codec = int(len(compressed) < len(raw))
        packed = compressed if codec else raw
        assert len(raw) <= 240 and decode(compressed) == raw
        directory[identifier*ENTRY.size:(identifier+1)*ENTRY.size] = ENTRY.pack(
            offset, len(packed), len(raw), codec, sum(raw) & 65535)
        payload.extend(packed)
        offset += len(packed)
        source_size += path.stat().st_size
        count += 1
    if offset > 65535:
        raise ValueError('Puzzle archive exceeds 16-bit offsets')
    output = MAGIC + struct.pack('<H', SLOTS) + directory + payload
    (ROOT/'assets/PUZZLES.DAT').write_bytes(output)
    print(f'PUZZLES.DAT: {count} puzzles, {len(output):,} bytes '
          f'({100*(1-len(output)/source_size):.1f}% smaller than source files)')


if __name__ == '__main__':
    main()

# DOS image resource pack

`KAKURO.DAT` holds the start screen, journal menu, and puzzle background.
Build it with `python3 tools/pack_assets.py`; regeneration uses Python's
standard library and the existing planar image files.

The pack is 132,299 bytes, compared with 384,012 bytes for the original
three loose VGA images and splash palette (65.5% fewer disk bytes).
Each image uses two 38,400-byte VGA planes; the unused two high planes
are cleared together by VGA. Byte RLE compresses flat artwork areas.
Literal packets copy directly, and repeat packets expand with `memset`.
The decoder buffers 2 KB of compressed input, streams to the VGA loader,
and allocates no entire image or DOS memory block above 64 KB.

Keeping resources outside the executable avoids increasing conventional
memory occupied by the DOS executable, keeps image offsets independent
of Borland's 16-bit code/data segments, and permits one shared disk file
instead of many files. Loading still performs the VGA writes required
for a full screen. Compression reduces disk transfers; it does not promise
a particular frame rate or load time on hardware that has not been timed.

The old loose-file loaders remain available for explicit custom paths and
development tests. Source PNGs, `.VGA` images, palettes, Python generators,
and font source/license files need not accompany a release. The runtime
directory needs `KAKURO.EXE`, `KAKURO.DAT`, and `PUZZLES.DAT`;
fonts and catalog metadata are compiled into the executable.

## Binary format

All integers are little endian. The header is eight bytes `KAKPAK1\0`,
then a one-byte entry count (currently three). Each 37-byte entry has:

| Field | Bytes |
| --- | --- |
| Null-terminated DOS image filename | 12 |
| Absolute payload offset | 4 |
| Compressed byte count | 4 |
| Expanded byte count (76,800) | 4 |
| Number of planes (2) | 1 |
| Four RGB DAC triples, components 0–63 | 12 |

Payloads are sequential byte RLE streams over both planes. A packet's
control byte 0–127 precedes `control+1` literal bytes. Control 128–255
precedes one byte repeated `control-127` times (1–128). The generator
uses repeats only when at least three bytes match. Packets may cross
plane, disk buffer, and output chunk boundaries. The decoder rejects
invalid metadata, truncated packets, output overrun, and trailing packet
data. Host tests compile and exercise the actual C++ decoder with all
three images and arbitrary output chunk sizes:

`python3 tests/test_asset_pack.py`

Help and About reuse the journal image. Their text is wrapped into
`src/DOCDATA.H` at build time, so reading pages need no additional shipped
images or text files. Puzzle data has its own [indexed archive](puzzles/ARCHIVE.md).

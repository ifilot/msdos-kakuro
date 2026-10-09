# PUZZLES.DAT

`python3 tools/pack_puzzles.py` generates the compressed archive from the
original `.puz` development sources. The application and release ship only
`PUZZLES.DAT`, never a directory of individual puzzle files. The current
archive contains 96 numbered puzzles in 5,756 bytes, 57.0% smaller
than the 13,399-byte numbered source collection before filesystem overhead.

Each board retains dimensions, difficulty, source metadata, answer digits,
and given-cell flags. Five-bit cell codes encode blocked cells (0), hidden
answers (1–9), and given answers (10–18, digit = code minus 9). Clue sums,
cell types, initial values, and locks are reconstructed by the same routines
as the text loader; tests compare every field of every cell against the sources.
Mutable entries and played/solved status remain session-only.

Records optionally use the image pack's byte RLE when it makes them smaller.
Loading seeks directly to the selected index entry and reads at most 240 bytes
of payload. Two 240-byte buffers suffice for input and decoded data. Structural
validation and a per-record checksum reject corrupt boards; failed loads
preserve the previous board.

## Format

All integers are little endian. The eight-byte magic `KAKPUZ1\0` is followed
by a two-byte slot count, currently 101. Slot 0 is reserved and empty; slots 1–100 are numbered
IDs. Each nine-byte directory entry stores:

| Field | Bytes |
| --- | ---: |
| Absolute payload offset (0 means absent) | 2 |
| Stored length | 2 |
| Decoded length | 2 |
| Codec: 0 raw compact record, 1 byte RLE | 1 |
| Sum of decoded bytes modulo 65,536 | 2 |

Decoded records start with one byte each for rows, columns, difficulty, and
source-string length, followed by the ASCII source string and five-bit cell
codes in row-major order, least-significant bits first. Unused final bits are
zero. Offsets keep the archive within 64 KB. Empty source placeholders 097–100
have absent entries and are excluded from the menu.

`Puzzle::loadPacked(id)` loads an archive entry. Familiar identifiers such as
`001.PUZ` and `PUZZLES\001.PUZ` also select archive entries.
`XXX.PUZ` does not select a shipped puzzle.
Explicit custom paths such as `CUSTOM\BOARD.PUZ` still use the text loader for
development. Rebuild after editing source boards to refresh both the compiled
menu catalog and the archive.

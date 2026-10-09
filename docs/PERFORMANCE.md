# Performance on the 8086/8088 target

The first optimization pass preserves the artwork and layout. The rendered
journal and gameplay screenshots are pixel-for-pixel identical to the previous
versions. Three agent reviews covered VGA writes, menu/catalog loading, and
asset storage; their changes were integrated and checked together.

## Drawing and input

Mode 12h fills use VGA set/reset to update all four planes in one CPU write.
Partial bytes read the VGA latches and use bit masks to preserve neighbors;
interiors use the Borland runtime's repeated memory operations. Horizontal and
vertical grid lines take this path rather than plotting individual pixels.

Monochrome glyph rows write masked VGA bytes. ProFont ink bounds are generated
at build time rather than scanning every glyph pixel to center each label.
BIOS text, clue digits, and precomputed blossom silhouettes use the same row
renderer. This retains transparent gaps, clipping, and native/2x scaling.

Moving the gameplay cursor restores the old border and draws the new border.
It leaves cell backgrounds, clue diagonals, and numbers in place. Entering an
unchanged number does no drawing or completion check. Full journal pages redraw
only their cards and details, retaining the book background in VGA memory.
Transitions involving the sparse last page reload its background to restore
exposed artwork.

## Startup and disk reads

The menu's compiled catalog is 96 records of four bytes (384 bytes). It replaces
101 file opens, puzzle parsing, and clue derivation at startup. A puzzle is read
only when selected for play. Main also avoids loading the default puzzle before
the splash and then loading it again on Enter. Missing or corrupt archive entries are reported when selected;
regenerate/rebuild after changing puzzle metadata.

The resource pack contains two active planes for each four-color image and
lossless byte RLE. It is **132,299 bytes**, versus **384,012 bytes** for the
original loose images and palette: **65.5% fewer disk bytes**. Decoder input and
VGA output each use a 2 KB buffer. No full-screen RAM allocation is required.
The upper two VGA planes are cleared together. Full-screen loads still write
76,800 image bytes and clear 38,400 address locations for the upper planes.

Ship `KAKURO.EXE`, `KAKURO.DAT`, and `PUZZLES.DAT`, with the
instructions and font license. Fonts and catalog metadata are compiled in;
large artwork stays outside the EXE so DOS need not load all of it into the
program's conventional memory. `make release` builds `build/KAKURO.ZIP` and an
unpacked `build/release` directory. Source PNGs, loose VGA files, and individual
puzzle files are omitted. The puzzle archive is 5,756 bytes; cells use five-bit
codes plus optional byte RLE, with a direct index and per-board checksum.
Selecting a puzzle reads a single small record instead of opening a separate
file. Help uses the existing book image; its text is compiled and scrolling updates
only the reading panel, without rereading artwork. About draws a small modal
over the current journal with generated version and build metadata.
See [the resource format](../assets/RESOURCE-PACK.md).

## Measurement and validation

`make test-perf` writes workload timings in BIOS ticks (approximately 55 ms) to
`build/perf-test/PERFORM.TXT`. Tests use DOSBox-X 2025.02.01, normal core,
`cputype=8086`, fixed 3,000 emulator cycles, and VGA mode 12h. The compiler itself
runs in a separate 386 emulator session because it requires protected-mode
support. Program output targets 8086/8088, with `-ml -G -1-` and no source debug
information. Broad Borland `-O1`/`-O2` presets produced incorrect splash test
pixel dumps, so they are avoided in this build.

The baseline used the saved pre-optimization source and the original `-ml -v`
build options. Both workloads run with the same emulator CPU/cycle settings.
These comparisons measure the combined rendering/loading/build changes.
Emulated filesystem reads are host-backed: these figures do not model floppy
seek times or transfer rates. The CPU model is experimental and does not model
the physical 8088's 8-bit bus, prefetch queue, and VGA wait states accurately.
Physical 4.77 MHz performance remains to be measured on hardware or a suitable
machine emulator. In particular, full scene changes still need disk decoding
and VGA transfers; steady gameplay and full menu page changes avoid image I/O.

Functional checks execute with the 8086 instruction set at fixed 30,000 emulator
cycles: VGA byte comparisons and edge masks; glyph transparency, clipping, and
scaling; all splash pixels and palette values; catalog metadata against all 96
puzzle files; fast menu page pixels against full repaint; gameplay input and
completion; and packed image bytes, chunk boundaries, and malformed streams.
Host tests also exercise the actual C++ resource decoder across eight chunk
sizes. The legacy Mode X font preview remains covered.

Measured on 2026-10-09, before adding Help/About and the puzzle archive.
These figures document the initial optimization pass; the current benchmark
also includes the revised journal and packed puzzle loader. One BIOS tick is about 55 ms. Lower is better.

| Workload | Before (ticks) | After (ticks) | Approximate improvement |
| --- | ---: | ---: | ---: |
| Build menu catalog 3 times | 47 | 1 | 47.0× |
| Draw complete menu 3 times | 336 | 36 | 9.3× |
| Load puzzle 3 times | 0 | 0 | Below timer resolution |
| Draw complete board 3 times | 246 | 21 | 11.7× |
| Move selector 400 times | 990 | 18 | 55.0× |
| Draw ProFont digit 1,000 times | 360 | 22 | 16.4× |

The catalog result is near the timer resolution; its structural improvement
is zero startup puzzle-file opens. Cursor and digit workloads are deliberately
long enough to reduce rounding error. Each nonzero result has roughly one tick
of timing uncertainty. These are relative emulator results, not physical
4.77 MHz load times.

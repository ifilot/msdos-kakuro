# MSDOS Kakuro

Repository: [ifilot/msdos-kakuro](https://github.com/ifilot/msdos-kakuro).

A planned Kakuro game for DOS, written in Borland C++ with a custom VGA
graphics engine targeting 640×480 and 16 colors (standard VGA mode 12h).

The application opens with the same [courtyard start screen](assets/splash/README.md)
as the CX16 game, displayed at 640×480 using four custom VGA colors. Press
Enter to open the [puzzle journal](assets/menu/README.md), or Escape to exit.
The journal uses a compiled puzzle index and shows 24 puzzle cards per page
with dimensions, difficulty blossoms,
and played/solved markers. Gameplay retains the
same 640×480 mode and installs a warm palette with at most 16 colors.

The application loads and displays a Kakuro puzzle on a 640×480 VGA screen.
Blocked cells are dark, clue cells show across/down sums separated by a
diagonal, and revealed hints have a rose-brown background. Ordinary answer cells
start empty. An amber border marks the selected cell, initially the first
editable answer cell.
Cell values use the selected **ProFont22 bitmap font**, centered by their
visible pixels at native resolution. The modal uses ProFont too, with a larger
heading. Clue sums retain the compact 3×5 digits at 2× scale. A hanging wooden
sign identifies the puzzle on a parchment inset. Filled cherry blossoms show
the difficulty, with outlined blossoms for the remaining levels; unknown
difficulty is labelled "UNRATED". The courtyard extends to the bottom without a footer.
The [generated courtyard background](assets/background/README.md) and warm
parchment/brown palette match the start screen's style.

From WSL/Linux, with the `kakuro-build` image already built:

```sh
make run
make run PUZZLE=071
make fonts
```

This builds `build/app/KAKURO.EXE` and opens a visible DOSBox-X window. It uses
a locally installed DOSBox-X/DOSBox when available, otherwise Docker with
the WSLg/X11 display. Puzzle 001 is the default; `PUZZLE` selects another
three-digit puzzle ID (or `xxx` for the extra source puzzle) to preselect in the journal.

In the journal, use arrows to select, Page Up/Page Down (or `[`/`]`) to
change pages, Home/End to select the first/last card on a page, and Enter
to play. F1 opens Help and F2 opens About. **F3 opens sound settings**, in
the journal or during a puzzle. Escape closes the application. Empty source placeholders are skipped at build time. Rebuild after editing
puzzle metadata so the journal index and compressed archive stay in sync.

- Arrow keys move the selector one cell, stopping at the board edges.
- `1`–`9` enter or replace a number in an editable answer cell.
- Backspace, Delete, or `0` clear that cell.
- Escape returns to the puzzle journal.

The selector can visit every cell; clues, blocked cells, and revealed hints
remain locked. Entries are kept in memory for this session; saving is not
implemented yet. Returning to the journal and reopening the active puzzle
preserves its entries; selecting a different puzzle starts a fresh board.
Played and solved markers last for the current session. After each entry, the game checks that every answer is filled
and every displayed clue has the correct sum without repeated digits. Any
valid solution is accepted, including alternatives to the stored source answer.

A solved board opens a gold-bordered congratulations modal over the dimmed
puzzle. Enter dismisses it to view the completed board; Escape returns to the journal. The
completed board stays locked until another puzzle is loaded.

`make build` compiles without opening a window. `make test-puzzle` checks
loading, puzzle rendering, game controls, completion rules, and the modal;
`make test-video` checks VGA drawing routines.
`make test-menu` checks the catalog, page navigation, card selection rendering,
status markers, and returning to the active board.
`make test-start` checks the courtyard pixels, high-resolution mode, DAC
colors, and transition to gameplay while retaining mode 12h.
`make fonts` opens [five bitmap digit samples](assets/fonts/README.md) for
choosing the cell font. `make test-fonts` checks their placement in VGA memory.
Those five are the original prototype samples; the selected ProFont is option
8 in the [ten downloaded font comparisons](assets/fonts/internet/README.md).

[`Puzzle`](src/PUZZLE.H) loads a selected board from `PUZZLES.DAT`, retains source solutions and
metadata, derives clue sums, and initializes cell values, reveal flags, and
locks. [`Game`](src/GAME.H) connects this board to [`VgaScreen`](src/VGA.H),
which provides pixels, clipped rectangles, squares, lines, grids, BIOS font
characters/text, compact clue digits, and palette updates. It restores text
mode when destroyed.
[`StartScreen`](src/START.H) loads the planar courtyard image and handles
Enter/Escape before the journal. [`MenuScreen`](src/MENU.H) catalogs loadable
puzzles, draws the journal, and handles game selection.
[`DocumentScreen`](src/DOCVIEW.H) displays scrollable journal-style Help and
a compact About modal with version, author, compiler, build time, and commit
ID. Help text is prewrapped into the executable; build metadata is generated
automatically. `VERSION` controls the release version, and `BUILD_COMMIT`
can supply an ID for source exports without Git metadata.

The copied [puzzle files](assets/puzzles/README.md) retain their CX16 format.
[The puzzle archive](assets/puzzles/ARCHIVE.md) packs all board cells, given
numbers and source metadata into 5,822 bytes. Clues and initial board flags
are reconstructed identically when a board is loaded. Custom text puzzle paths
remain available for development.
The 96 numbered puzzles and the extra puzzle are populated; IDs 097–100 are
empty source placeholders and cannot be opened.

Performance work targets the 8086/8088 instruction set. VGA fills update all
four planes together, text uses masked bitmap rows, and selector movement
changes only cell borders. Full journal pages change without reloading the
artwork. See [performance measurements and limits](docs/PERFORMANCE.md).

Artwork ships in a lossless [resource pack](assets/RESOURCE-PACK.md),
`KAKURO.DAT`, with buffered streaming and no full-screen RAM allocation.
`make release` produces `build/KAKURO.ZIP`: the executable, resource pack,
`PUZZLES.DAT`, instructions, and font licensing. Individual puzzle files are
kept as development sources and are excluded from the application and release. The original PNGs
and loose VGA assets stay in the development repository.

[Sound](src/SOUND.H) plays the imported menu/game music and effects through
AdLib, Sound Blaster FM or MPU-401 General MIDI. Automatic detection prefers
SB FM, then AdLib, then MIDI; missing hardware falls back to silence. F3 lets
you select the device and ports, toggle music/effects and save SOUND.CFG.
`make run` now enables host audio, including WSLg/PulseAudio with Docker.
The release also includes one `SOUND.DAT` pack; playback performs no disk I/O.
Sound Blaster currently uses its FM chip. MIDI needs a General MIDI synth,
provided by FluidSynth in the Docker runner when its system soundfont is
available. See [audio assets](assets/sound/README.md) and
[sound implementation and configuration](docs/SOUND.md).
`make test-sound` checks host stream playback and DOS hardware/timer integration.

`make test-perf` writes BIOS tick measurements to `build/perf-test/PERFORM.TXT`.
`make test-archive` compares all 97 archived boards with their source files and
checks corrupt archives. `make test-documents` checks Help/About navigation
and journal return.
`make test-assets` checks packed image decoding and malformed input on both host
and DOS. Tests execute on DOSBox-X's 8086 CPU; the Borland compiler runs in
a separate emulator process with a newer CPU.

See [build environment instructions](buildenv/README.md).

```sh
docker build -t kakuro-build buildenv
docker run --rm --user "$(id -u):$(id -g)" \
  --volume "$PWD:/workspace" kakuro-build make verify-toolchain smoke
```

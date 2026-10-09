# DOS C++ build environment

The build follows [Nightwatch's container approach](https://github.com/ifilot/nightwatch/tree/master/buildenv):
Debian Trixie, packaged DOSBox/DOSBox-X, and a bundled Borland DOS toolchain.
The image uses Borland C++ 3.1 rather than Turbo C 2.0. See
[toolchain provenance](BORLAND-NOTICE.md) for source media and checksums.

## Container build

Run from the repository root in WSL/Linux:

```sh
docker build -t kakuro-build buildenv
docker run --rm --user "$(id -u):$(id -g)" \
  --volume "$PWD:/workspace" kakuro-build make verify-toolchain smoke
```

Docker Desktop must be running with Linux containers and WSL integration
enabled for this distro (Settings → Resources → WSL Integration).
The image build downloads Debian packages; subsequent smoke builds need no
network or compiler downloads. Neither a separate DOS installation nor a GUI
is required for headless compilation.

If package downloads stall because Docker cannot resolve `deb.debian.org`,
check Docker Desktop's DNS configuration. During validation, the configured
servers `1.1.1.1` and `8.8.8.8` timed out inside containers, while WSL DNS and
connections to the mirror's IP address worked. This temporary build override
uses WSL's current mirror address without changing Docker settings:

```sh
mirror_address=$(getent ahostsv4 deb.debian.org | awk 'NR==1 {print $1}')
test -n "$mirror_address" && docker build \
  --add-host "deb.debian.org:$mirror_address" -t kakuro-build buildenv
```

The built image can run the smoke test with `docker run --network none`.

## Native build

With Python 3, GNU Make, and DOSBox-X or DOSBox installed:

```sh
make verify-toolchain smoke
```

`DOSBOX` selects the emulator executable. `DOS_TOOLCHAIN` selects the directory
containing `BC/`; its default is this repository's `buildenv/`.
The container defaults to DOSBox-X. Native builds prefer DOSBox-X and fall back
to DOSBox. For example:

```sh
DOSBOX=/usr/bin/dosbox make smoke
```

The runner mounts the toolchain as C: and a fresh temporary build directory as
D:. It invokes `BCC.EXE` with explicit include/library paths, the large memory
model (`-ml`), default 8086 code generation, and debugging information (`-v`).
It checks DOS error levels, a completion marker, and the program's output;
the emulator's host exit code alone cannot establish compiler success.
A stalled emulator is stopped after 60 seconds.

Outputs are `build/smoke/SMOKE.EXE`, `COMPILE.TXT`, `RESULT.TXT`, and
`EMULATOR.TXT`. The executable verifies C++ class construction and methods,
16-bit integers, far pointers, and runtime file I/O. `make build` currently
builds the Kakuro game in `build/app/KAKURO.EXE` using `tools/dos.py`.

## Build and run the game

With the container image built, run these commands directly from WSL/Linux:

```sh
make build
make run
make run PUZZLE=071
make test-puzzle
make test-video
make test-start
make test-menu
make test-assets
make test-archive
make test-documents
make test-perf
make release
```

These commands use a native DOSBox-X/DOSBox installation if one is available,
otherwise they invoke the `kakuro-build` image (`DOS_IMAGE` overrides its name).
`make run` rebuilds the application and opens a visible emulator window. In Docker,
it forwards `DISPLAY` and mounts the X11 socket, supporting WSLg. A configured
`XAUTHORITY` file is forwarded when present. No `xhost` access changes are needed.
Compilation and tests are headless. The 640×480 courtyard screen waits for
Enter to open the puzzle journal; Escape exits. In the journal, arrows select
a card, Page Up/Page Down change pages, and Enter starts the selected puzzle. F1 opens Help, F2 opens About.
Reading pages support arrows, Page Up/Down, Home/End and Escape to return.
In the game, arrow keys move the
selector and `1`–`9` enter numbers. Backspace, Delete, or `0` clear an editable
cell. Escape returns to the journal; Escape there restores text mode and closes
DOSBox. Clues and hints are locked. Reopening the current puzzle keeps its
entries; selecting another puzzle loads a fresh board. The `make fonts` preview exits on any key.
DOSBox-X's quit warning is disabled in the generated configuration, so the
window's close button also exits immediately without a confirmation prompt.

The start screen, journal, and gameplay use 640×480 VGA mode 12h. The start screen uses
four DAC colors; gameplay installs its own palette within 16 entries. The
background is native 640×480. Board graphics and clue digits use 2× drawing
scale; selected ProFont cell digits and modal text use native pixels, with a
2× modal heading. The game automatically sizes and centers the board.
It preselects `PUZZLES\001.PUZ` by default; `PUZZLE` preselects another puzzle.
The build packs all playable puzzle definitions into `PUZZLES.DAT`. Individual
`.puz` files remain development sources and are not copied into the application.
The build copies `KAKURO.DAT` beside the executable. It contains compressed
start, journal, and board images plus their palettes. On a DOS machine, copy
`KAKURO.EXE`, `KAKURO.DAT`, and `PUZZLES.DAT`; `make release` bundles
these into `build/KAKURO.ZIP` and excludes empty puzzle placeholders.
Run from that directory. You can also
preselect a packed puzzle with its familiar identifier, for example
`KAKURO.EXE PUZZLES\050.PUZ`.

`make test-puzzle` loads all 96 archived puzzles and checks invalid-file
handling, clue sums, initial hints/locks, and puzzle rendering without exposing
unrevealed answers. It also checks selector movement, board boundaries, number
entry/replacement, clearing, and protection of locked cells. Logs and a rendered board's pixel/palette dumps are in
`build/puzzle-test/`.
Completion tests cover missing/wrong answers, repeated digits, valid alternative
solutions, modal input capture, dismissal, and reset on loading. `MODAL.RAW`
and `MODAL.PAL` contain the congratulations screen and its six-bit VGA palette.
`make test-start` verifies all 307,200 courtyard pixels against the CX16 source,
the four-color DAC and attribute mapping, retention of mode 12h during gameplay,
game rendering after the transition, and missing/invalid asset handling.
Its logs and 640×480 pixel/palette dumps are in `build/start-test/`.
`make test-video` reads actual emulated VGA planes to verify all 76,800 pixels
of the original palette test, plus clipping, plane boundaries, palette changes,
clearing, text restoration, lines, grids, squares, compact digits, and scaled
BIOS-font glyphs. Its logs and pixel/palette dumps are in `build/video-test/`.
It also verifies all four background planes and invalid background file handling.

The optional legacy 256-color font preview uses Mode X; its timing follows the
[320×240 setup documented by Michael Abrash](https://www.phatcode.net/res/224/files/html/ch47/47-02.html).
The character renderer obtains the BIOS ROM 8×8 fonts through the
[font-information interrupt documented in RBIL](https://fd.lod.bz/rbil/interrup/video/101130.html).

## Interactive IDE

Launch a desktop DOSBox-X session, then use these DOS commands, replacing the
host path with the absolute path to this checkout's `buildenv`:

```text
mount c "/absolute/path/to/msdos-kakuro/buildenv"
c:
path C:\BC\BIN
bc
```

In the IDE, set Options → Directories to `C:\BC\INCLUDE` and `C:\BC\LIB`,
and choose a writable project output directory. `TD.EXE` can debug executables
built with `-v`. Interactive IDE/debugger use has not yet been validated.

## Reproduce toolchain extraction

Install 7-Zip (`7z`), mtools (`mcopy`), Info-ZIP (`unzip`), and Python 3.
Download the exact archive linked in the provenance notice, then:

```sh
python3 buildenv/import-borland.py /path/to/archive.7z --destination /tmp/borland31/BC
DOS_TOOLCHAIN=/tmp/borland31 make smoke
```

The importer validates the archive SHA-256 and requires a destination that
does not already exist. It directly extracts the DOS payload rather than
running the interactive Borland installer. Archive member CRCs are verified
by the extraction tools.

## Scope and verification

The bundled compiler, linker, and resulting DOS executable were verified with
DOSBox 0.74-3 on Ubuntu 24.04 in WSL and with DOSBox-X
`2025.02.01+dfsg-3` in the Debian Trixie Docker image. The container checksums
and compile/run smoke test passed as the host user's UID/GID with networking
disabled. The image build used the temporary DNS override described above.

Game development will use period-compatible C++ and a custom VGA engine.
640×480 at 16 colors uses four bit planes of 38,400 bytes each. Each pixel
combines one bit from each plane to select a color. The large memory
model supports far code/data pointers, but individual ordinary objects still
have the 64 KiB segment constraint. Rendering directly into VGA planes avoids
requiring a single 153,600-byte conventional-memory framebuffer.

The build uses the large memory model, speed code generation (`-G`), and
explicitly disables 80186/286 instructions (`-1-`). It omits source debugging
information from release executables. Borland's broad `-O1`/`-O2` presets
miscompile the splash pixel-reading test in this toolchain, so these presets
are avoided. Performance improvements come from the drawing and loading paths.
Compilation runs as a 386 because the compiler itself needs protected-mode
support; tests and the game run separately with `cputype=8086`.
`DOS_CPU` and `DOS_CYCLES` override game/test emulator settings; for example,
`DOS_CYCLES=3000 make run` gives a slower preview. DOSBox cycle settings do
not model a particular physical clock frequency or VGA wait states.
See [performance notes](../docs/PERFORMANCE.md) for measurements and limits.

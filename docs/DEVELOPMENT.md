# Developer guide

For gameplay, requirements and controls, see the [README](../README.md).
The game is written in Borland C++ 3.1 for 16-bit real-mode DOS, targeting the
8086/8088 instruction set and VGA mode 12h (640×480, 16 colors).

## Build environment

From the repository root on WSL/Linux, install Python 3 and GNU Make, then
build the Docker image:

```sh
docker build -t kakuro-build buildenv
docker run --rm --user "$(id -u):$(id -g)" \
  --volume "$PWD:/workspace" kakuro-build make verify-toolchain smoke
```

Docker Desktop needs Linux containers and WSL integration when using WSL.
Alternatively, use a native DOSBox-X or DOSBox installation with the bundled
toolchain. See [build environment instructions](../buildenv/README.md) for
setup, compiler provenance, configuration and troubleshooting.

```sh
make build
make run
make run PUZZLE=071
make release
```

`make build` produces `build/app/KAKURO.EXE` without opening a window.
`make run` rebuilds and opens a visible DOSBox-X/DOSBox window. The runner uses
a native emulator when available, otherwise the `kakuro-build` Docker image
with WSLg/X11 display forwarding. Host audio is enabled, including
WSLg/PulseAudio in Docker. `PUZZLE` preselects a three-digit puzzle ID
in the journal; the default is `001`.

`make release` produces `build/KAKURO.ZIP` with the executable,
`KAKURO.DAT`, `PUZZLES.DAT`, `SOUND.DAT`, instructions and font notices.
Individual puzzle files, original PNGs, loose VGA assets and the compiler
remain development sources and are excluded from the distribution.

## Architecture and rendering

- [`Puzzle`](../src/PUZZLE.H) loads a board from `PUZZLES.DAT`, retains source
  answers and metadata, derives clue sums, and initializes values, reveal flags
  and locks. Explicit custom text puzzle paths are supported for development.
- [`Game`](../src/GAME.H) connects the board to
  [`VgaScreen`](../src/VGA.H) and handles navigation, entry and completion.
  Completion checks every displayed run's sum and digit uniqueness rather than
  requiring the stored source answer.
- `VgaScreen` provides pixels, clipped rectangles, squares, lines, grids, BIOS
  text, compact clue digits and palette updates. It restores text mode when
  destroyed.
- [`StartScreen`](../src/START.H) loads the planar courtyard image and handles
  Enter/Escape before the journal. [`MenuScreen`](../src/MENU.H) uses a compiled
  puzzle catalog, displays 24 cards per page and tracks session status.
- [`DocumentScreen`](../src/DOCVIEW.H) displays scrollable Help and a compact
  About modal. Help text is prewrapped into the executable; build metadata is
  generated automatically.
- [`Sound`](../src/SOUND.H) handles music, effects and device selection.
  Automatic detection prefers Sound Blaster FM, then AdLib, then MIDI; absent
  hardware falls back to silence. Playback performs no disk I/O.

The start screen uses four custom VGA colors. Gameplay retains mode 12h and
installs the warm 16-color palette. Cell numbers use ProFont22 centered by their
visible pixels; clue sums use compact 3×5 digits at 2× scale. See the
[start artwork](../assets/splash/README.md),
[board background](../assets/background/README.md) and
[journal artwork](../assets/menu/README.md) notes.

VGA fills update all four planes together, text uses masked bitmap rows, and
selector movement changes only cell borders. Journal pages change without
reloading the artwork. See [performance measurements and limits](PERFORMANCE.md).

## Assets and puzzles

The copied [puzzle sources](../assets/puzzles/README.md) retain their CX16
format. The distribution contains 96 numbered puzzles. The extra `xxx.puz` source
board and empty placeholders 097–100 are excluded from the journal and archive.
Rebuild after editing boards or metadata to refresh both the catalog and archive.

```sh
python3 tools/pack_puzzles.py
```

The [puzzle archive format](../assets/puzzles/ARCHIVE.md) packs board cells,
given numbers and metadata into five-bit records with optional byte RLE.
Clues and initial flags are reconstructed when loading a board.
Graphics ship in a lossless [resource pack](../assets/RESOURCE-PACK.md),
`KAKURO.DAT`, using buffered streaming without a full-screen RAM allocation.
See [audio assets](../assets/sound/README.md) and
[sound implementation and configuration](SOUND.md) for `SOUND.DAT`, ports,
`SOUND.CFG` and MIDI synthesis. The Docker runner supplies FluidSynth when its
system soundfont is available.

`make fonts` opens the [five prototype digit samples](../assets/fonts/README.md).
The selected ProFont is option 8 in the
[ten downloaded font comparisons](../assets/fonts/internet/README.md).

## Tests

Tests execute on DOSBox-X's 8086 CPU. The Borland compiler runs in a separate
emulator process with a newer CPU. Host checks also validate the asset formats.

| Command | Coverage |
| --- | --- |
| `make verify-toolchain smoke` | Bundled compiler checksums and C++ toolchain smoke test. |
| `make test-video` | VGA drawing routines. |
| `make test-puzzle` | Loading, rendering, controls, completion rules and modal. |
| `make test-start` | Courtyard pixels, video mode, DAC and transition. |
| `make test-menu` | Catalog, pagination, selection, status and active-board return. |
| `make test-fonts` | Prototype font placement in VGA memory. |
| `make test-assets` | Image pack decoding and malformed input on host and DOS. |
| `make test-archive` | All 96 archived boards against their sources and corrupt input. |
| `make test-documents` | Help/About navigation and return to the journal. |
| `make test-sound` | Host audio streams and DOS hardware/timer integration. |
| `make test-perf` | BIOS tick measurements in `build/perf-test/PERFORM.TXT`. |

## Browser edition

```sh
make site
python3 -m http.server 8000 --directory build/site
```

Open <http://localhost:8000>; serve through HTTP instead of opening `index.html`
directly. `make site` first builds the DOS distribution, then generates
`build/site/` for a static host. The browser runs the same executable through
WebAssembly DOSBox, with keyboard/touch controls and optional FM sound.
MPU-401 synthesis is not included in the browser.

Python's standard library handles the site build; npm is only needed for the
browser tests. The first build downloads js-dos `emulators` 8.5.2 and matching
source archives, verifies pinned SHA-256 checksums and caches them in
`build/site-cache/`. Subsequent site builds can work offline once the DOS build
environment is available. All runtime resources, emulator licensing and
corresponding source archives are self-hosted. The site includes the DOS ZIP
for download.

To run browser checks after `make site`:

```sh
npm install --prefix build/browser-tools --no-audit --no-fund playwright@1.64.0
build/browser-tools/node_modules/.bin/playwright install chromium
NODE_PATH="$PWD/build/browser-tools/node_modules" node tests/check_site.cjs
```

## CI, releases and GitHub Pages

[GitHub Actions](../.github/workflows/build.yml) builds the Docker environment,
checks the toolchain, runs host/DOS tests and packages `KAKURO.ZIP` on pushes
and pull requests. The distribution and diagnostic logs are downloadable
artifacts. Pushing a tag publishes a GitHub release with the tested ZIP.

Before tagging, update `VERSION`, the version badge in the README, and add a matching entry to
[CHANGELOG.md](../CHANGELOG.md); release notes are extracted from that entry.
For example, push `v0.1.0` for source version `0.1.0`. Build metadata includes
the version, compiler, author, timestamp and commit ID. `BUILD_COMMIT` can supply
an ID for source exports without Git metadata.

The `pages` job runs **only on pushes to `master`**, after the DOS tests pass.
It reuses the tested archive, builds the website, checks gameplay in Chromium
and deploys with the official Pages actions. Other branches, pull requests,
tags and manual workflow runs do not build or deploy the site.

Enable **Settings → Pages → Build and deployment → Source: GitHub Actions**
in the repository. After a successful deployment, the site is available at
<https://ifilot.github.io/msdos-kakuro/>. Releases and Pages use GitHub's
built-in token; Pages also uses OIDC. No deployment or release secret is needed.

## Licensing

The project's own code, website, graphics and audio are `GPL-3.0-only`.
[LICENSE](../LICENSE) contains the full GPLv3 text;
[NOTICE.md](../NOTICE.md) identifies the scope, editable asset sources and
third-party exceptions. Preserve upstream font and tool notices. The packager
ships `LICENSE.TXT` and `NOTICE.TXT` with both DOS downloads and the website.
Distribute the corresponding source for the release, including editable graphics
and audio sources, in accordance with GPLv3.

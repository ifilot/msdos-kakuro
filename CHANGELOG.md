# Changelog

Changes are grouped by the source version in `VERSION`. GitHub releases are
published when a tag is pushed; release notes use the matching version below.

## [Unreleased]

### Changed

- Simplified the README around installation, rules and controls; moved build,
  test and implementation details into a dedicated developer guide.

- Reworked the website as a project introduction, with detailed Kakuro rules, a
  worked sum example, controls, technical notes and links to the implementation.

### Added

- README badges for GitHub Actions, version and GPLv3; explicit project and
  asset licensing, third-party notices and editable audio sources.
- GPLv3 license text and attribution notices in DOS and website distributions.

- Self-hosted WebAssembly/DOSBox browser game with courtyard styling, FM sound,
  fullscreen, touch controls and a downloadable DOS edition via `make site`.
- GitHub Pages builds and browser smoke tests exclusively on pushes to `master`.

## [0.1.0]

### Added

- DOS Kakuro game built with Borland C++ 3.1, targeting 8086/8088 and VGA.
- A 640x480, 16-color courtyard start screen, puzzle journal and game board.
- 97 puzzles with clue cells, fixed hints, difficulty indicators and session
  status marks, shipped in the compressed `PUZZLES.DAT` archive.
- Keyboard navigation, digit entry and erasing, solution checking and a
  congratulations modal accepting any valid solution.
- ProFont cell numbers, compact clue digits and cherry blossom difficulty icons.
- Help pages and an About modal with version, author, compiler, build time and
  Git commit metadata.
- AdLib, Sound Blaster FM and MPU-401 General MIDI music and effects, automatic
  detection, an F3 settings panel and persistent audio preferences.
- Packed graphics and audio assets in `KAKURO.DAT` and `SOUND.DAT`.
- Buffered asset loading, planar VGA drawing and partial redraws for navigation.
- Docker build environment, headless DOSBox tests and a packaged DOS distribution.
- GitHub Actions builds and tests for pushes and pull requests, downloadable
  build diagnostics and automatic tagged releases with `KAKURO.ZIP`.

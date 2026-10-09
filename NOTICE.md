# Licensing and attribution

Copyright (C) 2026 Ivo Filot.

MSDOS Kakuro's original source code, build tools, website, documentation,
graphics and sound assets are distributed under the **GNU General Public
License, version 3 only** (`GPL-3.0-only`). See [LICENSE](LICENSE) for the full
terms. This licensing statement grants rights held by the project author;
it does not replace the terms or ownership of third-party materials.

The game is distributed without any warranty. You may redistribute and modify
it under the terms of GPLv3, including its corresponding-source requirements.

## Graphics and audio

The project's courtyard and journal artwork, UI graphics, original bitmap
characters, music and effects are covered by GPLv3 to the extent copyright
applies. The courtyard and journal backgrounds were generated with OpenAI's
image generator. Artwork and audio imported from Ivo Filot's CX16 projects are
included under the author's GPLv3 grant for this project.

The preferred editable forms of the assets are retained in the repository:

- Graphics: original PNGs in `assets/splash/`, `assets/background/` and
  `assets/menu/`, together with their conversion scripts in `tools/`.
- Audio: composition, effect and instrument YAML files in `assets/sound/source/`.
  The exported OPL/MIDI streams are in `assets/sound/fm/` and `assets/sound/midi/`.
  See `assets/sound/README.md` for generation and packing instructions.
- Puzzles: text boards in `assets/puzzles/`; their original source annotations
  are preserved. This grant covers the project's contributions and does not
  assert ownership of third-party puzzle material.

Generated `KAKURO.DAT`, `SOUND.DAT` and other derived assets retain the terms
of their source material. A compiled or compressed form is not a replacement
for the preferred editable sources when distributing under GPLv3.

## Third-party materials

These materials retain their own notices and are not relicensed by this grant:

- **ProFont**, used for cell numbers and larger text: MIT license. Keep
  `assets/fonts/PROFONT-LICENSE.txt` with redistributions. The DOS archive
  includes this notice as `PROFONT.TXT`.
- **Other downloaded font samples** in `assets/fonts/internet/`: their original
  font-specific terms apply. See that directory's README and accompanying
  license/source notes. They are development references, not fonts used by
  the released game.
- **js-dos / DOSBox browser emulator**: GNU GPL version 2, with upstream notices
  retained. The generated website includes the emulator license and matching
  source archives under `emulator/`. The DOS game and browser emulator are
  separate programs.
- **Borland C++ tools, headers and libraries** under `buildenv/BC/`: proprietary
  third-party materials. See `buildenv/BORLAND-NOTICE.md`. The project license
  does not grant rights to those materials. The compiler is not included in
  the game or website distribution.

DOS distributions include this notice as `NOTICE.TXT` and the full GPLv3 text
as `LICENSE.TXT`. The corresponding project sources are available at
https://github.com/ifilot/msdos-kakuro.

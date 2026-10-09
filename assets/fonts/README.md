# Bitmap digit samples

Five original digit designs for Kakuro cells, with glyphs for 0–9:
These are the original prototypes; Bold was the earlier choice. The game now
uses **ProFont22**, option 8 in the [downloaded comparisons](internet/README.md).
Its ASCII glyphs are embedded through `tools/generate_profont.py` and
`src/PROFDAT.H`; no runtime font library is required. The complete MIT license
is retained in [PROFONT-LICENSE.txt](PROFONT-LICENSE.txt).

| Choice | Style | Bitmap size | Character |
|---|---|---|---|
| 1 | Clean | 5×9 | Narrow, light, simple strokes |
| 2 | Round | 6×9 | Wider curves and softer corners |
| 3 | Slab | 7×9 | Small serifs and a traditional numeral shape |
| 4 | LCD | 7×11 | Seven-segment display style |
| 5 | Bold | 7×9 | Heavier strokes for strong contrast |

Run `make fonts` to see all five in the DOS application. Each row shows
digits 1–9 in identical 22-pixel cells, alternating the hint and answer-cell
backgrounds. Press any key or close the window to exit.

`samples.png` is a four-times enlargement of the actual emulated VGA pixel
and palette dumps produced by `make test-fonts`. Scaling uses nearest-neighbor
sampling so every bitmap pixel remains visible.

`digits.json` contains the editable designs (`#` is ink, `.` is background).
`tools/generate_fonts.py` converts them to `src/DIGITDAT.H`, which is rebuilt
automatically when the design data changes. These are original prototypes,
not downloads of commercial font families.

`DigitFonts::drawDigit` uses `VgaScreen::drawCenteredBitmap` to center each
digit's visible ink inside the cell, excluding its grid border. Leading and
trailing font padding does not affect placement. On odd/even pixel boundaries,
the unavoidable rounding is at most half a pixel. The original compact clue
digits use their existing drawing routine.

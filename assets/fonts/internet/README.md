# Ten larger bitmap font options

These are real downloaded bitmap glyphs, rendered from BDF files for comparison
on October 9, 2026. They are candidates for puzzle entries and larger headings.
ProFont (option 8) is now selected for cell values and modal text. The game's
existing small header and clue fonts are retained. The source comparison
sheets remain available for reference.

Open [options 1–5](comparison-1.png) and [options 6–10](comparison-2.png).
The corresponding `-2x.png` files provide nearest-neighbor enlarged previews.
Digits are centered by their visible ink in 56×56 cells, matching the first
puzzle's cell size. Headings use the original advance widths and baseline
offsets. Candidate glyphs are monochrome and unscaled in the native sheets;
small font-name labels use a separate presentation font.

| Number | Font | Bitmap file | Maximum digit ink height |
| --- | --- | --- | --- |
| 1 | Times Roman | timR18.bdf | 17 px |
| 2 | New Century Schoolbook | ncenR18.bdf | 18 px |
| 3 | Courier | courR18.bdf | 15 px |
| 4 | Helvetica | helvR18.bdf | 18 px |
| 5 | Lucida Sans | luRS18.bdf | 18 px |
| 6 | Lucida Bright | lubR18.bdf | 18 px |
| 7 | Spleen | spleen-12x24.bdf | 15 px |
| 8 | ProFont | profont22.bdf | 14 px |
| 9 | UW ttyp0 | t0-22-uni.bdf | 13 px |
| 10 | Terminus | ter-u24n.bdf | 15 px |

Options 1–9 come from the [U8g2 BDF collection](https://github.com/olikraus/u8g2/tree/master/tools/font/bdf).
`revision.txt` pins the downloaded repository revision. Raw font URLs have the
form `https://raw.githubusercontent.com/olikraus/u8g2/REVISION/tools/font/bdf/FILENAME`.
The files retain their original copyright properties. Font-specific source and
license notes are saved alongside the comparison, as is the collection's
`U8G2-LICENSE.txt`; the font licenses are distinct from the library's license.

Option 10 comes from the [Terminus author's site](https://terminus-font.sourceforge.net/),
release 4.49.1, with its complete `TERMINUS-OFL.txt`. The source archive was
downloaded from
`https://downloads.sourceforge.net/project/terminus-font/terminus-font-4.49/terminus-font-4.49.1.tar.gz`
and verified against its published SHA-256:
`d961c1b781627bf417f9b340693d64fc219e0113ad3a3af1a3424c7aa373ef79`.
Only the chosen BDF and license were extracted; no downloaded code was executed.

Regenerate the sheets with `python3 tools/preview_internet_fonts.py` (Pillow).
The script checks required digit/heading glyphs, bitmap row counts, and that
every sample fits its box. `fonts.json` records the numbered choices and metrics;
`SHA256SUMS` records the downloaded fonts and provenance files.

These glyphs can be converted into compact C++ bitmap tables for our existing
renderer, so choosing one does not require a font library at DOS runtime.

# Puzzle courtyard background

Generated with the built-in imagegen tool on October 9, 2026, using
`assets/splash/courtyard.png` as the style and palette reference. The complete
generation prompt is in [PROMPT.txt](PROMPT.txt).

- `courtyard-source.png`: original generated artwork.
- `courtyard.png`: native 640×480 four-color game asset.
- `BOARD.VGA`: four 38,400-byte VGA planes, using palette indices 0–3.

The scene keeps its center quiet and puts the cherry blossoms, bamboo, shoji,
and veranda detail along the edges. The four colors match the start screen:
parchment, dusty rose, warm brown, and charcoal. Board cells remain opaque for
legibility, with lighter parchment answers, rose-brown hints, and a gold selector.
A wooden sign hangs from the veranda roof, with a parchment puzzle label,
warm grain details, brass pegs, and native-size lettering. Five-petal cherry
blossom icons indicate difficulty. Filled blossoms show the source rating;
outlined blossoms fill the remaining positions in the five-level display
(higher source ratings extend it). Unknown ratings show "UNRATED".
The courtyard remains visible along the bottom; keyboard controls are documented
in the main README.

Regenerate the DOS asset with `python3 tools/generate_background.py` (Pillow
required). Generated assets are committed, so regular builds need no imaging
dependency. `make build` copies `BOARD.VGA` beside the executable.

The VGA loader streams 80-byte rows directly into each plane without allocating
a 153,600-byte buffer. If the asset cannot be loaded, the game uses a plain warm
background. The congratulations modal dims both the board and scenery through
the DAC; dismissal reloads the scene and restores the full palette.

`make test-video` checks every background byte in emulated VGA memory and
missing/truncated file handling. Puzzle tests check background rendering and
restoration after modal dismissal. `SHA256SUMS` records the asset contents.

## License

This project's artwork is licensed under **GPL-3.0-only**, to the extent
copyright applies. Source PNGs and conversion scripts are retained for editing.
See [LICENSE](../../LICENSE) and [licensing and attribution](../../NOTICE.md).

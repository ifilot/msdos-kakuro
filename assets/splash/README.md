# Courtyard start screen

Copied from `/mnt/c/PROGRAMMING/CX16/cx16-kakuro/assets/splash/comparisons`
on October 9, 2026, at the author's request. `courtyard.png` is the selected
`courtyard-640x480-4colors.png`; `courtyard.bin` and `courtyard.pal` are the
matching packed 2bpp bitmap and CX16 palette. Artwork, title, and the embedded
"PRESS ENTER" prompt are preserved.

`tools/generate_splash.py` uses only the Python standard library to convert
these source bytes into `START.VGA` (two 38,400-byte VGA bit planes) and
`START.PAL` (four RGB triples with six-bit DAC components). RGB components are
scaled from the CX16's four-bit range to VGA's six-bit range. `SHA256SUMS`
records both the copied sources and generated files.

The start screen uses BIOS mode 12h, 640×480 with 16 available colors; only
four are used. The attribute palette maps color indices directly to DAC
entries, and the upper two video planes are cleared. Rows stream from disk
using an 80-byte buffer, avoiding a conventional-memory bitmap above 64 KiB.
The display stays blank during loading.

Enter proceeds to the selected puzzle, Escape exits, and other keys wait.
Gameplay retains mode 12h and sets its 16-color palette without changing
resolution. Backgrounds load at native 640×480; board drawing uses 2× scale.

The register setup follows the FreeVGA descriptions of
[attribute-controller palettes](https://www.cs.jhu.edu/~huang/cs318/fall17/project/specs/freevga/vga/attrreg.htm)
and [graphics-controller write mode 0](https://www.osdever.net/FreeVGA/vga/graphreg.htm).

`make test-start` compares every emulated pixel against the original packed
CX16 image, checks the DAC and attribute palette, verifies the retained game mode and text-mode restoration
and text-mode restoration, and rejects missing/truncated/invalid assets.
The VGA pixel and palette dumps are in `build/start-test/`.

## License

This project's artwork is licensed under **GPL-3.0-only**, to the extent
copyright applies. Source PNGs and conversion scripts are retained for editing.
See [LICENSE](../../LICENSE) and [licensing and attribution](../../NOTICE.md).

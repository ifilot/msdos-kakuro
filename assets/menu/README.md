# Puzzle journal

The game-selection screen reuses the blank open-journal artwork from the
CX16 Kakuro application, with its 24-card layout, six columns and four rows.
The title, cards, controls, status marks, and blossom difficulty icons are
rendered by the DOS application at 640×480 using the game's warm VGA palette.
Puzzle numbers use ProFont; small labels use the BIOS bitmap font.

`journal-source.png` was copied from
`cx16-kakuro/assets/menu/concepts/03-puzzle-journal-source.png`.
Its SHA256 is `cc402611e07881dc5ad658d48eec134667262c6c0a332b614f39f7be44f72dc6`.
`journal.png` is the four-color preview; `MENU.VGA` stores four 38,400-byte VGA
planes. The upper two planes are initially empty and used by the UI's colors.

To regenerate the committed artwork (requires Pillow):

```sh
python3 tools/generate_menu.py
```

Regular builds use the committed VGA bitmap and do not require Pillow.
The build generates a 384-byte compiled catalog for 96 numbered puzzles, excluding the extra
XXX board and empty placeholders.
The menu reads this catalog without opening puzzle files. Rebuild after
editing puzzle metadata; a missing archive entry is reported when selected.
Played/solved markers are kept for the current session. Only the current
puzzle's board stays in memory; switching puzzles loads a fresh board.

F1 (or H) opens Help; F2 (or A) opens About. Both return to the
current journal page and selection. Their footer buttons share the existing
wood-and-paper palette.

## License

This project's artwork is licensed under **GPL-3.0-only**, to the extent
copyright applies. Source PNGs and conversion scripts are retained for editing.
See [LICENSE](../../LICENSE) and [licensing and attribution](../../NOTICE.md).

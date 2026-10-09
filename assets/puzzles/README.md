# CX16 Kakuro puzzles

Copied on October 9, 2026 from
`/mnt/c/PROGRAMMING/CX16/cx16-kakuro/assets/puzzles` in Ivo Filot's CX16 Kakuro
application, at the author's request. All `.puz` files except the corrected
`031.puz` remain byte-for-byte copies; original source/difficulty comments
are retained. `SHA256SUMS` records their current contents.

`001.puz` through `096.puz` contain the 96 numbered puzzles. `xxx.puz` is an
additional 9×9 development source with unknown metadata, excluded from the
shipped catalog and archive. `097.puz` through `100.puz`
are empty placeholders in the original application. Populated boards range
from 6×6 to 10×10.

The files are plain text. A leading comment records source/difficulty, such
as `# B5/P1/2*`. Rows contain whitespace-separated cells:

- `0`: a non-answer cell. It becomes a clue cell when at least two consecutive
  answer cells follow to the right or below; otherwise it stays blocked.
- `1` through `9`: an unrevealed solution digit. Its initial visible value is zero.
- `#1` through `#9`: a revealed hint digit, initially visible and locked.

The loader derives across/down sums from consecutive solution digits and
keeps the solution separate from the current visible board values. Non-answer
cells are locked. Source metadata is retained and difficulty is extracted from
the final `/N*` field when present.

The `Puzzle` class accepts rectangular boards up to 15×15, checks dimensions
and tokens, and preserves its current board if loading another file fails.
The source corpus fits the game's 640×480 view with readable clue digits.
`Puzzle::setValue` accepts values 1–9 (or zero to clear) only in unlocked answer
cells. Entering numbers leaves the source solution, clue sums, and hints intact.
`Puzzle::isSolved` checks filled answers, sums, and distinct digits in every
displayed across/down clue. Single-cell runs have no clue in this format;
each answer must belong to at least one displayed run.

`031.puz` was corrected on October 9, 2026: row 3, column 7 changed from `1`
to `3` (counting from 1). The original down run was `1,1,2`, whose sum of 4
cannot be achieved with three distinct positive digits. The corrected run is
`3,1,2`; its derived down clue changes from 4 to 6, and the across clue for
that cell changes from 8 to 10. Layout, revealed hints, and all other source
digits remain unchanged. Valid solutions exist, including the corrected
stored answer; the puzzle permits alternatives, which the game accepts.
All 96 numbered stored answers now pass the completion check.

The original `031.puz` SHA-256 was
`f2c870e7fbd2f4e9f5b1f07b4ab2738489444a249755bc6f6d604c6501137ed7`.

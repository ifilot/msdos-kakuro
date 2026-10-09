#include <dos.h>
#include <stdio.h>
#include <string.h>
#include "GAME.H"

unsigned _stklen = 16384;
static int errors = 0;
static FILE *checks = 0;

static void check(int condition, const char *message)
{
    if (!condition) {
        ++errors;
        printf("FAIL: %s\n", message);
        if (checks) fprintf(checks, "FAIL: %s\n", message);
    }
}

static unsigned char readNativePixel(int x, int y)
{
    volatile unsigned char far *video = (volatile unsigned char far *)MK_FP(0xA000, 0);
    unsigned char value = 0;
    for (int plane = 0; plane < 4; ++plane) {
        outport(0x3CE, (plane << 8) | 4);
        if (video[(unsigned int)y * 80U + x / 8] & (0x80 >> (x & 7)))
            value |= 1 << plane;
    }
    return value;
}

static unsigned char readPixel(int x, int y)
{
    return readNativePixel(x * 2, y * 2);
}

int main()
{
    checks = fopen("CHECKS.TXT", "w");
    if (!checks) return 2;
    int x, y;
    unsigned int i;
    Puzzle puzzle;
    check(!puzzle.isSolved(), "Unloaded puzzle is not solved");
    check(puzzle.load("PUZZLES\\001.PUZ"), "Load first puzzle");
    check(puzzle.rows() == 6 && puzzle.columns() == 6, "First dimensions");
    check(puzzle.difficulty() == 2, "Difficulty metadata");
    check(!strcmp(puzzle.source(), "B5/P1/2*"), "Source metadata");
    check(puzzle.cell(0, 1)->down == 10, "First down clue");
    check(puzzle.cell(0, 2)->down == 28, "Long down clue");
    check(puzzle.cell(1, 0)->across == 18, "Across clue");
    check(puzzle.cell(3, 3)->down == 15, "Internal down clue");
    check(puzzle.cell(4, 1)->across == 25, "Bottom across clue");
    check(puzzle.cell(1, 1)->value == 2 && puzzle.cell(1, 1)->revealed
          && puzzle.cell(1, 1)->locked, "Revealed hint state");
    check(puzzle.cell(1, 2)->solution == 6 && puzzle.cell(1, 2)->value == 0
          && !puzzle.cell(1, 2)->locked, "Hidden answer state");
    check(puzzle.cell(0, 0)->type == Puzzle::Blocked && puzzle.cell(0, 0)->locked,
          "Blocked state");
    check(!puzzle.cell(-1, 0) && !puzzle.cell(0, 6), "Safe cell bounds");
    for (int digit = 1; digit <= 9; ++digit) {
        check(puzzle.setValue(1, 2, digit), "Editable cell accepts 1..9");
        check(puzzle.cell(1, 2)->value == digit, "Entry updates board state");
    }
    check(puzzle.setValue(1, 2, 0) && puzzle.cell(1, 2)->value == 0, "Clear entry");
    check(!puzzle.setValue(1, 2, -1) && !puzzle.setValue(1, 2, 10), "Reject invalid values");
    check(!puzzle.setValue(-1, 2, 1) && !puzzle.setValue(1, 6, 1), "Reject invalid positions");
    check(!puzzle.setValue(1, 1, 5) && !puzzle.setValue(1, 1, 0), "Protect revealed hints");
    check(!puzzle.setValue(0, 0, 5) && !puzzle.setValue(1, 0, 5), "Protect blocks and clues");
    check(puzzle.cell(1, 2)->solution == 6 && puzzle.cell(1, 0)->across == 18,
          "Entries preserve source solutions and clue sums");
    check(!puzzle.isSolved(), "Empty answer cells prevent completion");
    check(puzzle.load("FIXTURES\\ALTERN.PUZ"), "Load alternate-solution fixture");
    puzzle.setValue(1, 1, 2); puzzle.setValue(1, 2, 2);
    puzzle.setValue(2, 1, 2); puzzle.setValue(2, 2, 2);
    check(!puzzle.isSolved(), "Correct sums with repeated digits are not solved");
    puzzle.setValue(1, 1, 1); puzzle.setValue(1, 2, 3);
    puzzle.setValue(2, 1, 1); puzzle.setValue(2, 2, 3);
    check(!puzzle.isSolved(), "Correct across runs do not bypass invalid down runs");
    puzzle.setValue(1, 1, 1); puzzle.setValue(1, 2, 2);
    puzzle.setValue(2, 1, 2); puzzle.setValue(2, 2, 1);
    check(!puzzle.isSolved(), "Distinct digits with wrong sums are not solved");
    puzzle.setValue(1, 1, 3); puzzle.setValue(1, 2, 1);
    puzzle.setValue(2, 1, 1); puzzle.setValue(2, 2, 3);
    check(puzzle.isSolved(), "Valid alternative solution is accepted");
    puzzle.setValue(2, 2, 0);
    check(!puzzle.isSolved(), "Clearing a solved board makes it incomplete");
    check(puzzle.load("PUZZLES\\001.PUZ"), "Restore first puzzle");

    const char *bad[] = {"FIXTURES\\RAGGED.PUZ", "FIXTURES\\TOKEN.PUZ",
        "FIXTURES\\HINT0.PUZ", "FIXTURES\\LONG.PUZ", "FIXTURES\\BIG.PUZ",
        "PUZZLES\\097.PUZ", "PUZZLES\\098.PUZ", "PUZZLES\\099.PUZ",
        "PUZZLES\\100.PUZ", "MISSING.PUZ"};
    for (i = 0; i < sizeof(bad) / sizeof(bad[0]); ++i) {
        check(!puzzle.load(bad[i]), "Reject invalid/missing/empty file");
        check(puzzle.rows() == 6 && puzzle.cell(1, 1)->value == 2,
              "Failed load preserves previous board");
        check(puzzle.error()[0] != 0, "Load error explains failure");
    }
    check(puzzle.load("FIXTURES\\FIRST.PUZ"), "Hint at start of line");
    check(puzzle.cell(0, 0)->revealed && puzzle.cell(0, 0)->value == 1,
          "Leading hint is not treated as a comment");
    check(puzzle.load("FIXTURES\\SMALL.PUZ"), "Small board and blank lines");
    check(puzzle.cell(0, 1)->down == 4 && puzzle.cell(0, 2)->down == 6
          && puzzle.cell(1, 0)->across == 3 && puzzle.cell(2, 0)->across == 7,
          "Small-board clues at edges");
    check(puzzle.load("FIXTURES\\SINGLE.PUZ"), "Isolated cell fixture");
    check(puzzle.difficulty() == 0, "Unknown metadata");
    check(puzzle.cell(0, 2)->type == Puzzle::Blocked,
          "Clue runs do not wrap between rows or include a single cell");
    puzzle.setValue(0, 1, 1); puzzle.setValue(1, 0, 2); puzzle.setValue(1, 2, 3);
    check(!puzzle.isSolved(), "Answers without any clue cannot form a solved board");

    for (int id = 1; id <= 96; ++id) {
        char filename[24];
        sprintf(filename, "PUZZLES\\%03d.PUZ", id);
        if (!puzzle.load(filename)) { check(0, filename); continue; }
        check(puzzle.rows() >= 6 && puzzle.rows() <= 10
              && puzzle.rows() == puzzle.columns(), "Corpus dimensions");
        for (int row = 0; row < puzzle.rows(); ++row) {
            for (int col = 0; col < puzzle.columns(); ++col) {
                const Puzzle::Cell *cell = puzzle.cell(row, col);
                check(cell->value == (cell->revealed ? cell->solution : 0),
                      "Only initial hints have visible values");
                check(cell->locked == (cell->revealed || !cell->solution),
                      "Initial locks");
                check(cell->solution <= 9, "Solution range");
            }
        }
        for (int r = 0; r < puzzle.rows(); ++r)
            for (int c = 0; c < puzzle.columns(); ++c)
                if (!puzzle.cell(r, c)->locked)
                    puzzle.setValue(r, c, puzzle.cell(r, c)->solution);
        check(puzzle.isSolved(), filename);
        if (id == 31) {
            check(puzzle.cell(2, 6)->solution == 3, "Puzzle 031 corrected answer");
            check(puzzle.cell(2, 5)->across == 10 && puzzle.cell(1, 6)->down == 6,
                  "Puzzle 031 corrected across and down clues");
        }
    }

    VgaScreen screen;
    Game game(screen);
    check(game.loadPuzzle("PUZZLES\\001.PUZ"), "Game loads puzzle");
    screen.initialize();
    game.draw();
    int backgroundPixel = readPixel(28, 56);
    check(backgroundPixel <= 3, "Courtyard background rendered");
    check(game.selectedRow() == 1 && game.selectedColumn() == 2,
          "Selector starts on first editable cell");
    check(readPixel(131, 80) == 6, "Selector border rendered");
    check(readPixel(76, 38) == 3, "Blocked cell rendered");
    check(readPixel(110, 40) == 2, "Clue cell rendered");
    check(readPixel(103, 50) == 1, "Grid rendered");
    check(readPixel(104, 66) == 5, "Hint cell colored");
    check(readPixel(95, 67) == 0, "Across clue digits rendered above diagonal");
    check(readPixel(106, 58) == 0, "Down clue digits rendered below diagonal");
    // Entire ordinary cell interior must be blank: no solution leakage.
    for (y = 66; y < 93; ++y)
        for (x = 132; x < 159; ++x)
            check(readPixel(x, y) == 4, "Hidden solution is not drawn");
    int ink = 0;
    int minX = 640, maxX = -1, minY = 480, maxY = -1;
    for (y = 132; y < 186; ++y)
        for (x = 208; x < 262; ++x)
            if (readNativePixel(x, y) == 3) {
                ++ink;
                if (x < minX) minX = x;
                if (x > maxX) maxX = x;
                if (y < minY) minY = y;
                if (y > maxY) maxY = y;
            }
    check(ink > 0, "ProFont renders the hint digit");
    check(minX + maxX >= 468 && minX + maxX <= 469
          && minY + maxY >= 316 && minY + maxY <= 317,
          "Hint digit ink is centered in its cell");
    check(maxY - minY + 1 == 14, "ProFont digit retains native 14-pixel ink height");

    FILE *pixels = fopen("SCREEN.RAW", "wb");
    if (!pixels) return 2;
    unsigned char scanline[640];
    for (y = 0; y < 480; ++y) {
        for (x = 0; x < 640; ++x) scanline[x] = readNativePixel(x, y);
        if (fwrite(scanline, 1, 640, pixels) != 640) return 2;
    }
    if (fclose(pixels)) return 2;
    FILE *palette = fopen("PALETTE.RAW", "wb");
    if (!palette) return 2;
    outportb(0x3C7, 0);
    for (i = 0; i < 768; ++i)
        if (fputc(inportb(0x3C9), palette) == EOF) return 2;
    if (fclose(palette)) return 2;

    check(game.handleKey(Game::Up), "Arrow keeps game running");
    check(game.selectedRow() == 0 && game.selectedColumn() == 2, "Move up to clue");
    check(readPixel(131, 80) == 1 && readPixel(131, 50) == 6,
          "Movement restores old border and draws new selector");
    game.handleKey('5');
    check(game.puzzle().cell(0, 2)->value == 0, "Cannot enter into a clue");
    for (i = 0; i < 10; ++i) { game.handleKey(Game::Up); game.handleKey(Game::Left); }
    check(game.selectedRow() == 0 && game.selectedColumn() == 0, "Top/left boundaries");
    game.handleKey('5');
    check(game.puzzle().cell(0, 0)->value == 0, "Cannot enter into a block");
    game.handleKey(Game::Down);
    game.handleKey(Game::Right);
    game.handleKey('5'); game.handleKey(8); game.handleKey(Game::Delete); game.handleKey('0');
    check(game.puzzle().cell(1, 1)->value == 2, "Keyboard cannot alter or erase a hint");
    game.handleKey(Game::Right);
    game.handleKey('7');
    check(game.puzzle().cell(1, 2)->value == 7, "Keyboard enters a digit");
    game.handleKey('9');
    check(game.puzzle().cell(1, 2)->value == 9, "Keyboard replaces existing digit");
    ink = 0;
    for (y = 132; y < 186; ++y)
        for (x = 264; x < 318; ++x)
            if (readNativePixel(x, y) == 3) ++ink;
    check(ink > 0, "Entered ProFont digit drawn immediately");
    game.handleKey('x');
    check(game.puzzle().cell(1, 2)->value == 9, "Other keys do not exit or edit");
    int eraseKeys[] = {8, Game::Delete, '0'};
    for (i = 0; i < 3; ++i) {
        game.handleKey('9'); game.handleKey(eraseKeys[i]);
        check(game.puzzle().cell(1, 2)->value == 0, "Keyboard clears entry");
        for (y = 132; y < 186; ++y)
            for (x = 264; x < 318; ++x)
                check(readNativePixel(x, y) == 4, "Clearing erases previous glyph");
    }
    check(!game.loadPuzzle("MISSING.PUZ") && game.selectedRow() == 1
          && game.selectedColumn() == 2, "Failed load preserves selection");
    for (i = 0; i < 10; ++i) { game.handleKey(Game::Down); game.handleKey(Game::Right); }
    check(game.selectedRow() == 5 && game.selectedColumn() == 5, "Bottom/right boundaries");
    check(!game.handleKey(27), "Escape exits");
    check(game.loadPuzzle("PUZZLES\\001.PUZ") && game.selectedRow() == 1
          && game.selectedColumn() == 2 && game.puzzle().cell(1, 2)->value == 0,
          "Reload resets selection and entries");
    game.draw();
    for (int r = 0; r < game.puzzle().rows(); ++r)
        for (int c = 0; c < game.puzzle().columns(); ++c) {
            const Puzzle::Cell *entry = game.puzzle().cell(r, c);
            if (entry->locked) continue;
            check(!game.isCompleted(), "Completion waits for the final answer");
            while (game.selectedRow() < r) game.handleKey(Game::Down);
            while (game.selectedRow() > r) game.handleKey(Game::Up);
            while (game.selectedColumn() < c) game.handleKey(Game::Right);
            while (game.selectedColumn() > c) game.handleKey(Game::Left);
            game.handleKey('0' + entry->solution);
        }
    check(game.puzzle().isSolved() && game.isCompleted()
          && game.completionModalVisible(), "Final entry opens congratulations modal");
    check(readPixel(28, 56) == 11 && readPixel(30, 58) == 10
          && readPixel(295, 191) == 14, "Modal border, panel and shadow rendered");
    int lastRow = game.selectedRow(), lastCol = game.selectedColumn();
    int lastValue = game.puzzle().cell(lastRow, lastCol)->value;
    game.handleKey(Game::Left); game.handleKey('0'); game.handleKey(8);
    check(game.selectedRow() == lastRow && game.selectedColumn() == lastCol
          && game.puzzle().cell(lastRow, lastCol)->value == lastValue
          && game.completionModalVisible(), "Modal captures navigation and editing");
    check(!game.handleKey(27), "Escape can exit from congratulations modal");
    pixels = fopen("MODAL.RAW", "wb");
    if (!pixels) return 2;
    for (y = 0; y < 480; ++y) {
        for (x = 0; x < 640; ++x) scanline[x] = readNativePixel(x, y);
        if (fwrite(scanline, 1, 640, pixels) != 640) return 2;
    }
    if (fclose(pixels)) return 2;
    palette = fopen("MODAL.PAL", "wb");
    if (!palette) return 2;
    outportb(0x3C7, 0);
    for (i = 0; i < 768; ++i)
        if (fputc(inportb(0x3C9), palette) == EOF) return 2;
    if (fclose(palette)) return 2;
    game.draw();
    check(readPixel(28, 56) == 11, "Redraw preserves modal");
    game.handleKey(13);
    check(game.isCompleted() && !game.completionModalVisible()
          && readPixel(28, 56) == backgroundPixel, "Enter dismisses modal and restores board");
    outportb(0x3C7, 4);
    check(inportb(0x3C9) == 55, "Dismissal restores undimmed board palette");
    game.handleKey('0'); game.handleKey(Game::Left);
    check(game.puzzle().isSolved() && game.selectedColumn() == lastCol,
          "Completed board stays solved after dismissal");
    check(!game.handleKey(27), "Escape exits from completed board");
    check(game.loadPuzzle("PUZZLES\\001.PUZ") && !game.isCompleted(),
          "Loading another puzzle resets completion state");
    check(game.loadPuzzle("FIXTURES\\SOLVED.PUZ") && game.completionModalVisible(),
          "Already solved all-hint board opens modal on load");
    game.draw();
    check(readPixel(28, 56) == 11, "Already solved board displays modal");
    check(!game.loadPuzzle("MISSING.PUZ") && game.completionModalVisible(),
          "Failed load preserves completion state");
    screen.restoreTextMode();
    if (fclose(checks)) return 2;
    checks = 0;

    FILE *result = fopen("RESULT.TXT", "w");
    if (!result) return 2;
    fprintf(result, errors ? "Puzzle tests failed\n" : "Puzzle tests passed\n");
    if (fclose(result)) return 2;
    return errors ? 1 : 0;
}

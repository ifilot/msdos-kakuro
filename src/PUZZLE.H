#ifndef KAKURO_PUZZLE_H
#define KAKURO_PUZZLE_H

class Puzzle {
public:
    enum { MaxDimension = 15, MaxCells = MaxDimension * MaxDimension };
    enum CellType { Blocked, Clue, Answer };
    struct Cell {
        unsigned char type;
        unsigned char solution; // Source answer, never shown for ordinary cells.
        unsigned char value;    // Current entry: 0 (empty), a hint, or a user digit.
        unsigned char across;
        unsigned char down;
        unsigned char revealed;
        unsigned char locked;
    };

    Puzzle();
    // Failure preserves the previous board; error() explains the failure.
    int load(const char *filename);
    int loadPacked(unsigned int id, const char *archive = "PUZZLES.DAT");
    int rows() const { return rowCount; }
    int columns() const { return columnCount; }
    int difficulty() const { return stars; } // 0 means unspecified.
    const char *source() const { return sourceText; }
    const char *error() const { return errorText; }
    const Cell *cell(int row, int column) const;
    // Values are 0 (clear) or 1..9; clues, blocks, and hints cannot be edited.
    int setValue(int row, int column, int value);
    // All answers filled; each across/down run has distinct digits and its clue sum.
    int isSolved() const;

private:
    Cell cells[MaxCells];
    int rowCount, columnCount, stars;
    char sourceText[96];
    char errorText[96];
    int read(const char *filename);
    int readPacked(unsigned int id, const char *archive);
    int fail(const char *message);
    void deriveClues();
};

#endif

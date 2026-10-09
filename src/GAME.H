#ifndef KAKURO_GAME_H
#define KAKURO_GAME_H

#include "PUZZLE.H"
#include "VGA.H"
#include "SOUND.H"

class Game {
public:
    Game(VgaScreen &screen);
    void setSound(Sound *audio) { sound = audio; }
    int loadPuzzle(const char *filename);
    void draw();
    enum Key { Up = 0x148, Down = 0x150, Left = 0x14B,
               Right = 0x14D, Delete = 0x153 };
    // ASCII keys or 0x100 | DOS extended scan code. Returns 0 for Escape.
    int handleKey(int key);
    int selectedRow() const { return cursorRow; }
    int selectedColumn() const { return cursorColumn; }
    const Puzzle &puzzle() const { return board; }
    int isCompleted() const { return state != Playing; }
    int completionModalVisible() const { return state == Congratulations; }

private:
    VgaScreen &screen;
    Sound *sound;
    Puzzle board;
    char puzzleName[13];
    int cursorRow, cursorColumn;
    int boardX, boardY, cellSize;
    enum State { Playing, Congratulations, Completed };
    State state;
    void drawCell(int x, int y, int size, const Puzzle::Cell &cell);
    void paintCell(int row, int column);
    void drawSelector();
    void drawCompletionModal();
    void drawHeader();
};

#endif

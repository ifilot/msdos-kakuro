#include <stdio.h>
#include <string.h>
#include "GAME.H"
#include "DIGITS.H"
#include "PROFONT.H"
#include "BLOSSOM.H"

enum { Background = 3, BlockColor = 3, ClueColor = 2, GridColor = 1,
       EmptyColor = 4, HintColor = 5, InkColor = 3, TextColor = 0,
       SelectorColor = 6, PanelColor = 10, GoldColor = 11,
       ModalTextColor = 12, ButtonColor = 13, ShadowColor = 14,
       WoodColor = 7, PetalColor = 8 };

struct ThemeColor { unsigned char index, red, green, blue; };
static const ThemeColor theme[] = {
    {0,50,50,37}, {1,33,25,25}, {2,16,12,12}, {3,8,8,8},
    {4,55,53,42}, {5,43,36,31}, {6,63,52,28},
    {7,23,17,14}, {8,53,40,35}
};

static void applyTheme(VgaScreen &screen, int dimmed)
{
    for (unsigned int i = 0; i < sizeof(theme) / sizeof(theme[0]); ++i) {
        const ThemeColor &c = theme[i];
        int divisor = dimmed ? 2 : 1;
        screen.setPaletteColor(c.index, c.red / divisor, c.green / divisor, c.blue / divisor);
    }
}

Game::Game(VgaScreen &display) : screen(display), sound(0), cursorRow(-1), cursorColumn(-1),
                                boardX(0), boardY(0), cellSize(0), state(Playing)
{
    puzzleName[0] = 0;
}

int Game::loadPuzzle(const char *filename)
{
    if (!board.load(filename)) return 0;
    state = board.isSolved() ? Congratulations : Playing;
    cursorRow = cursorColumn = 0;
    cellSize = 0;
    int found = 0;
    for (int row = 0; row < board.rows() && !found; ++row)
        for (int col = 0; col < board.columns(); ++col)
            if (board.cell(row, col)->type == Puzzle::Answer
                && !board.cell(row, col)->locked) {
                cursorRow = row;
                cursorColumn = col;
                found = 1;
                break;
            }
    const char *name = filename;
    for (const char *cursor = filename; *cursor; ++cursor)
        if (*cursor == '\\' || *cursor == '/') name = cursor + 1;
    strncpy(puzzleName, name, sizeof(puzzleName) - 1);
    puzzleName[sizeof(puzzleName) - 1] = 0;
    return 1;
}

int Game::handleKey(int key)
{
    if (key == 27) { if(sound)sound->playEffect(Sound::Back); return 0; }
    if (state == Congratulations) {
        if (key == 13) {
            state = Completed;
            if(sound)sound->playEffect(Sound::Select);
            if (cellSize > 0) draw();
        }
        return 1;
    }
    if (state == Completed) return 1;
    if (cursorRow < 0 || cursorColumn < 0) return 1;
    int oldRow = cursorRow, oldColumn = cursorColumn;
    switch (key) {
        case Up: if (cursorRow > 0) --cursorRow; break;
        case Down: if (cursorRow + 1 < board.rows()) ++cursorRow; break;
        case Left: if (cursorColumn > 0) --cursorColumn; break;
        case Right: if (cursorColumn + 1 < board.columns()) ++cursorColumn; break;
        default: {
            int value = -1;
            if (key >= '1' && key <= '9') value = key - '0';
            if (key == 8 || key == Delete || key == '0') value = 0;
            const Puzzle::Cell *cell = board.cell(cursorRow, cursorColumn);
            if (value >= 0 && cell && cell->value != value
                && board.setValue(cursorRow, cursorColumn, value)) {
                paintCell(cursorRow, cursorColumn);
                if(sound)sound->playEffect(value ? (Sound::Effect)(Sound::Place1+value-1) : Sound::Back);
                if (board.isSolved()) {
                    state = Congratulations;
                    if(sound)sound->playEffect(Sound::Solved);
                    if (cellSize > 0) drawCompletionModal();
                }
            } else if(value>=0 && cell && (cell->type!=Puzzle::Answer || cell->locked)) {
                if(sound)sound->playEffect(Sound::Wrong);
            }
            return 1;
        }
    }
    if (oldRow != cursorRow || oldColumn != cursorColumn) {
        if(sound)sound->playEffect(Sound::Click);
        // The cursor is only a border: keep both interiors and glyphs intact.
        if (cellSize > 0) {
            screen.drawGrid(boardX + oldColumn * cellSize,
                            boardY + oldRow * cellSize, 1, 1, cellSize, GridColor);
            drawSelector();
        }
    }
    return 1;
}

void Game::drawSelector()
{
    if (state != Playing) return;
    if (cellSize <= 0 || cursorRow < 0 || cursorColumn < 0) return;
    int x = boardX + cursorColumn * cellSize;
    int y = boardY + cursorRow * cellSize;
    screen.drawGrid(x, y, 1, 1, cellSize, SelectorColor);
}

void Game::paintCell(int row, int column)
{
    if (cellSize <= 0) return;
    const Puzzle::Cell *cell = board.cell(row, column);
    if (!cell) return;
    int x = boardX + column * cellSize;
    int y = boardY + row * cellSize;
    drawCell(x, y, cellSize, *cell);
    screen.drawGrid(x, y, 1, 1, cellSize, GridColor);
    if (row == cursorRow && column == cursorColumn) drawSelector();
}

void Game::drawCell(int x, int y, int size, const Puzzle::Cell &cell)
{
    unsigned char color = BlockColor;
    if (cell.type == Puzzle::Clue) color = ClueColor;
    if (cell.type == Puzzle::Answer) color = cell.revealed ? HintColor : EmptyColor;
    screen.colorSquare(x + 1, y + 1, size - 1, color);
    if (cell.type == Puzzle::Clue) {
        screen.drawLine(x + 1, y + 1, x + size - 1, y + size - 1, GridColor);
        if (cell.across) {
            int digits = cell.across >= 100 ? 3 : (cell.across >= 10 ? 2 : 1);
            screen.drawNumber(x + size - 2 - (digits * 4 - 1), y + 2,
                              cell.across, TextColor);
        }
        if (cell.down) screen.drawNumber(x + 2, y + size - 7, cell.down, TextColor);
    } else if (cell.type == Puzzle::Answer && cell.value) {
        char digit[2] = {(char)('0' + cell.value), 0};
        screen.setDrawingScale(1);
        ProFont::drawCentered(screen, x * 2 + 2, y * 2 + 2,
                              size * 2 - 2, size * 2 - 2, digit, InkColor);
        screen.setDrawingScale(2);
    }
}

void Game::draw()
{
    if (!board.rows() || !board.columns()) return;
    screen.setDrawingScale(2);
    applyTheme(screen, 0);
    if (!screen.loadHighBitmap("BOARD.VGA")) screen.clear(Background);
    int size = (screen.drawingWidth() - 32) / board.columns();
    int heightSize = (screen.drawingHeight() - 44) / board.rows();
    if (size > heightSize) size = heightSize;
    if (size > 28) size = 28;
    int left = (screen.drawingWidth() - board.columns() * size - 1) / 2;
    int top = 24 + (196 - board.rows() * size - 1) / 2;
    boardX = left;
    boardY = top;
    cellSize = size;
    screen.fillRect(left - 3, top - 3, board.columns() * size + 7,
                    board.rows() * size + 7, ClueColor);

    drawHeader();
    for (int row = 0; row < board.rows(); ++row)
        for (int col = 0; col < board.columns(); ++col)
            drawCell(left + col * size, top + row * size, size, *board.cell(row, col));
    screen.drawGrid(left, top, board.columns(), board.rows(), size, GridColor);
    drawSelector();
    if (state == Congratulations) drawCompletionModal();
}

void Game::drawHeader()
{
    screen.setDrawingScale(1);
    int slots = board.difficulty() > 5 ? board.difficulty() : 5;
    int width = 190 + slots * 16;
    int left = (VgaScreen::Width - width) / 2;
    // A suspended wood frame, with warm grain, brass pegs and a paper label.
    screen.fillRect(left + 16, 0, 2, 9, GridColor);
    screen.fillRect(left + width - 18, 0, 2, 9, GridColor);
    screen.fillRect(left + 2, 9, width, 34, BlockColor);
    screen.fillRect(left, 7, width, 34, BlockColor);
    screen.fillRect(left + 1, 8, width - 2, 32, ClueColor);
    screen.drawLine(left + 7, 9, left + width - 8, 9, GridColor);
    screen.drawLine(left + 8, 38, left + width - 9, 38, WoodColor);
    screen.drawLine(left + 6, 11, left + width - 7, 11, WoodColor);
    screen.fillRect(left + 8, 13, 156, 23, GridColor);
    screen.fillRect(left + 9, 14, 154, 21, TextColor);
    screen.drawLine(left + 10, 34, left + 161, 34, HintColor);
    screen.putPixel(left + 4, 23, SelectorColor);
    screen.putPixel(left + width - 5, 23, SelectorColor);

    char label[13];
    strcpy(label, puzzleName);
    char *extension = strrchr(label, '.');
    if (extension) *extension = 0;
    screen.drawText(left + 18, 21, "KAKURO", InkColor);
    screen.drawLine(left + 76, 18, left + 76, 31, GridColor);
    screen.drawText(left + 85, 21, "No.", GridColor);
    screen.drawText(left + 117, 21, label, InkColor);
    screen.drawText(left + 176, 12,
                    board.difficulty() ? "DIFFICULTY" : "UNRATED", TextColor);
    if (board.difficulty())
        for (int flower = 0; flower < slots; ++flower)
            Blossom::draw(screen, left + 177 + flower * 16, 23,
                        flower < board.difficulty());
    else screen.drawText(left + 208, 25, "--", GridColor);
    if (state != Playing) {
        // A small completion seal on the parchment label.
        screen.drawLine(left + 147, 17, left + 150, 20, GridColor);
        screen.drawLine(left + 150, 20, left + 155, 16, GridColor);
    }
    screen.setDrawingScale(2);
}

void Game::drawCompletionModal()
{
    // Dim the board through its palette, preserving every underlying pixel.
    applyTheme(screen, 1);
    screen.setPaletteColor(PanelColor, 16, 12, 12);
    screen.setPaletteColor(GoldColor, 63, 52, 28);
    screen.setPaletteColor(ModalTextColor, 55, 53, 42);
    screen.setPaletteColor(ButtonColor, 33, 25, 25);
    screen.setPaletteColor(ShadowColor, 4, 4, 4);
    screen.fillRect(32, 60, 264, 132, ShadowColor);
    screen.fillRect(28, 56, 264, 132, GoldColor);
    screen.fillRect(30, 58, 260, 128, PanelColor);
    screen.drawChar(48, 78, '*', GoldColor);
    screen.drawChar(264, 78, '*', GoldColor);
    screen.fillRect(72, 145, 176, 22, ButtonColor);
    screen.setDrawingScale(1);
    ProFont::drawCentered(screen, 128, 142, 384, 40, "WELL DONE!", GoldColor, 2);
    ProFont::drawCentered(screen, 64, 200, 512, 28, "Congratulations!", ModalTextColor);
    ProFont::drawCentered(screen, 64, 240, 512, 28, "You solved this Kakuro.", ModalTextColor);
    ProFont::drawCentered(screen, 144, 290, 352, 44, "Enter: view board", ModalTextColor);
    ProFont::drawCentered(screen, 64, 344, 512, 24, "Esc: menu", ModalTextColor);
    screen.setDrawingScale(2);
}

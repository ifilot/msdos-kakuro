#include <stdio.h>
#include <string.h>
#include "MENU.H"
#include "SOUNDMNU.H"
#include "GAME.H"
#include "CATALOG.H"
#include "PROFONT.H"
#include "BLOSSOM.H"

enum { Paper = 0, Rose = 1, Brown = 2, Ink = 3, Hint = 5, Gold = 6 };

MenuScreen::MenuScreen(VgaScreen &display)
    : screen(display), sound(0), puzzleCount(0), selected(0), visible(0)
{
    errorText[0] = 0;
}

static const char *basename(const char *name)
{
    const char *result = name;
    for (; *name; ++name) if (*name == '/' || *name == '\\') result = name + 1;
    return result;
}

static void puzzleNumber(char *text, unsigned int id)
{
    text[0] = '0' + id / 100;
    text[1] = '0' + (id / 10) % 10;
    text[2] = '0' + id % 10;
    text[3] = 0;
}

static char *dimension(char *text, unsigned char value)
{
    // Puzzle dimensions are at most fifteen, so no general printf/division.
    if (value >= 10) { *text++ = '1'; value -= 10; }
    *text++ = '0' + value;
    return text;
}

int MenuScreen::loadCatalog(const char *preferred)
{
    puzzleCount = selected = visible = 0;
    errorText[0] = 0;
    // Metadata lives in the executable: no directory search, 101 file opens,
    // puzzle parsing, or clue derivation before displaying the journal.
    for (unsigned int index = 0; index < puzzleCatalogCount && puzzleCount < MaxPuzzles; ++index) {
        const CatalogRecord &record = puzzleCatalog[index];
        unsigned int id = record.id;
        char filename[20];
        strcpy(filename, "PUZZLES\\000.PUZ");
        char number[4];
        puzzleNumber(number, id);
        memcpy(filename + 8, number, 3);
        Entry &item = entries[puzzleCount];
        strcpy(item.filename, filename);
        item.rows = record.rows; item.columns = record.columns;
        item.difficulty = record.difficulty; item.status = Ready;
        item.id = id;
        if (preferred && !stricmp(basename(preferred), basename(filename))) selected = puzzleCount;
        ++puzzleCount;
    }
    return puzzleCount != 0;
}

const MenuScreen::Entry *MenuScreen::entry(int index) const
{
    return index >= 0 && index < puzzleCount ? &entries[index] : 0;
}

const char *MenuScreen::selectedFile() const
{
    return puzzleCount ? entries[selected].filename : "";
}

void MenuScreen::markPlayed(int index, int solved)
{
    if (index < 0 || index >= puzzleCount) return;
    if (solved) entries[index].status = Solved;
    else if (entries[index].status != Solved) entries[index].status = Played;
}

void MenuScreen::showError(const char *message)
{
    strncpy(errorText, message, sizeof(errorText) - 1);
    errorText[sizeof(errorText) - 1] = 0;
    if (visible) drawDetails();
}

int MenuScreen::handleKey(int key)
{
    if(SoundMenu::toggleKey(sound,key)) {
        if(visible)SoundMenu::drawButtons(screen,sound,24);
        return Wait;
    }
    if (key == 27) return Exit;
    if (key == 0x13B || key == 'h' || key == 'H') return Help;
    if (key == 0x13C || key == 'a' || key == 'A') return About;
    if (key == 0x13F) return Audio;
    if (!puzzleCount) return Wait;
    if (key == 13) return Play;
    int old = selected, slot = selected % PageSize, page = pageNumber();
    switch (key) {
        case Game::Left: if (slot % Columns) --selected; break;
        case Game::Right:
            if (slot % Columns < Columns - 1 && selected + 1 < puzzleCount) ++selected;
            break;
        case Game::Up: if (slot >= Columns) selected -= Columns; break;
        case Game::Down:
            if (slot + Columns < PageSize && selected + Columns < puzzleCount) selected += Columns;
            break;
        case 0x149: case '[':
            if (page) selected -= PageSize;
            break;
        case 0x151: case ']':
            if (page + 1 < pageCount()) {
                selected += PageSize;
                if (selected >= puzzleCount) selected = puzzleCount - 1;
            }
            break;
        case 0x147: selected = page * PageSize; break; // Home
        case 0x14F: // End
            selected = (page + 1) * PageSize - 1;
            if (selected >= puzzleCount) selected = puzzleCount - 1;
            break;
    }
    if (selected != old) {
        errorText[0] = 0;
        if (visible) {
            if (old / PageSize != pageNumber()) {
                // Every card covers its previous page counterpart. Keep the
                // journal artwork in VGA memory rather than reading it again.
                // Sparse pages require restoring the exposed paper artwork.
                if ((old / PageSize + 1) * PageSize <= puzzleCount
                    && (pageNumber() + 1) * PageSize <= puzzleCount) {
                    for (int index = pageNumber() * PageSize;
                         index < (pageNumber() + 1) * PageSize; ++index) drawCard(index);
                    drawDetails();
                    drawPageNumber();
                } else draw();
            }
            else { drawCard(old); drawCard(selected); drawDetails(); }
        }
    }
    return Wait;
}

static void panel(VgaScreen &screen, int x, int y, int width, int height,
                  unsigned char face, unsigned char border)
{
    screen.fillRect(x + 2, y + 2, width, height, Ink);
    screen.fillRect(x, y, width, height, border);
    screen.fillRect(x + 1, y + 1, width - 2, height - 2, face);
}

void MenuScreen::drawCard(int index)
{
    const Entry *item = entry(index);
    if (!item || index / PageSize != pageNumber()) return;
    int slot = index % PageSize, column = slot % Columns;
    int x = 100 + column * 72 + (column >= 3 ? 24 : 0);
    int y = 112 + slot / Columns * 56;
    int active = index == selected;
    unsigned char face = active ? Brown : Paper, text = active ? Paper : Brown;
    panel(screen, x, y, 56, 48, face, active ? Gold : Brown);
    screen.drawLine(x + 5, y + 3, x + 50, y + 3, active ? Rose : Hint);
    char number[4], shape[8];
    puzzleNumber(number, item->id);
    ProFont::drawCentered(screen, x + 4, y + 7, 48, 23, number, text);
    char *end = dimension(shape, item->rows);
    *end++ = 'x';
    *dimension(end, item->columns) = 0;
    screen.drawText(x + (56 - strlen(shape) * 8) / 2, y + 35, shape, text);
    if (item->status != Ready) {
        screen.fillRect(x + 45, y + 4, 7, 7, text);
        screen.fillRect(x + 46, y + 5, 5, 5, face);
        if (item->status == Solved) {
            screen.drawLine(x + 46, y + 7, x + 48, y + 9, text);
            screen.drawLine(x + 48, y + 9, x + 51, y + 6, text);
        }
    }
}

void MenuScreen::drawDetails()
{
    if (!puzzleCount) return;
    panel(screen, 96, 344, 448, 24, Paper, Brown);
    if (errorText[0]) screen.drawText(104, 352, errorText, Brown);
    else {
        const Entry &item = entries[selected];
        char number[4], details[48];
        puzzleNumber(number, item.id);
        const char *status = item.status == Solved ? "SOLVED" : item.status == Played ? "PLAYED" : "READY";
        sprintf(details, "PUZZLE %s  %dx%d  %s", number, item.rows, item.columns, status);
        screen.drawText(108, 352, details, Brown);
        if (item.difficulty)
            for (int flower = 0; flower < item.difficulty && flower < 5; ++flower)
                Blossom::draw(screen, 436 + flower * 17, 349, 1);
        else screen.drawText(436, 352, "UNRATED", Rose);
    }
}

void MenuScreen::drawPageNumber()
{
    panel(screen, 242, 416, 156, 32, Paper, Brown);
    char page[20];
    sprintf(page, "< PAGE %d/%d >", pageNumber() + 1, pageCount());
    screen.drawText(320 - strlen(page) * 4, 428, page, Brown);
}

void MenuScreen::draw()
{
    screen.setDrawingScale(1);
    static const unsigned char colors[9][3] = {
        {57,53,36},{35,21,19},{14,9,8},{5,5,6},{59,56,43},
        {44,31,24},{63,48,16},{24,14,9},{58,35,33}
    };
    for (int i = 0; i < 9; ++i) screen.setPaletteColor(i, colors[i][0], colors[i][1], colors[i][2]);
    if (!screen.loadHighBitmap("MENU.VGA")) screen.clear(Paper);
    panel(screen, 176, 24, 288, 56, Paper, Brown);
    ProFont::drawCentered(screen, 184, 28, 272, 30, "KAKURO", Brown, 2);
    screen.drawText(260, 64, "SELECT A PUZZLE", Brown);
    visible = 1;
    for (int index = pageNumber() * PageSize; index < puzzleCount
         && index < (pageNumber() + 1) * PageSize; ++index) drawCard(index);
    panel(screen, 152, 376, 336, 20, Paper, Brown);
    screen.drawText(160, 382, "Arrows: select   PgUp/PgDn: page", Brown);
    SoundMenu::drawButtons(screen,sound,24);
    panel(screen, 32, 416, 104, 32, Brown, Rose);
    screen.drawText(44, 428, "PLAY Enter", Paper);
    panel(screen, 148, 416, 88, 32, Paper, Brown);
    screen.drawText(164, 428, "HELP F1", Brown);
    panel(screen, 404, 416, 88, 32, Paper, Brown);
    screen.drawText(416, 428, "ABOUT F2", Brown);
    panel(screen, 504, 416, 104, 32, Paper, Brown);
    screen.drawText(524, 428, "QUIT Esc", Brown);
    if (puzzleCount) drawDetails();
    else screen.drawText(224, 200, "NO PUZZLES AVAILABLE", Brown);
    drawPageNumber();
}

int MenuScreen::handleMouse(int x,int y,int button)
{
    if(button==2)return handleKey(27);
    if(button!=1)return Wait;
    int key=SoundMenu::buttonKey(x,y,24);
    if(key)return handleKey(key);
    if(y>=416 && y<448) {
        if(x>=32 && x<136)return handleKey(13);
        if(x>=148 && x<236)return handleKey(0x13B);
        if(x>=242 && x<398)return handleKey(x<320 ? 0x149 : 0x151);
        if(x>=404 && x<492)return handleKey(0x13C);
        if(x>=504 && x<608)return handleKey(27);
    }
    for(int slot=0;slot<PageSize;++slot) {
        int index=pageNumber()*PageSize+slot;
        if(index>=puzzleCount)break;
        int column=slot%Columns;
        int left=100+column*72+(column>=3 ? 24 : 0),top=112+slot/Columns*56;
        if(x>=left && x<left+56 && y>=top && y<top+48) {
            int old=selected;selected=index;errorText[0]=0;
            if(visible) { drawCard(old);drawCard(selected);drawDetails(); }
            return Play;
        }
    }
    return Wait;
}

HoverRect MenuScreen::hoverAt(int x,int y) const
{
    if(sound && SoundMenu::buttonKey(x,y,24))return SoundMenu::buttonHover(x,y,24);
    if(y>=416 && y<448) {
        if(puzzleCount && x>=32 && x<136)return HoverRect(32,416,104,32);
        if(x>=148 && x<236)return HoverRect(148,416,88,32);
        if(x>=242 && x<320 && pageNumber()>0)return HoverRect(242,416,78,32);
        if(x>=320 && x<398 && pageNumber()+1<pageCount())return HoverRect(320,416,78,32);
        if(x>=404 && x<492)return HoverRect(404,416,88,32);
        if(x>=504 && x<608)return HoverRect(504,416,104,32);
    }
    for(int slot=0;slot<PageSize;++slot) {
        if(pageNumber()*PageSize+slot>=puzzleCount)break;
        int column=slot%Columns;
        int left=100+column*72+(column>=3 ? 24 : 0),top=112+slot/Columns*56;
        if(x>=left && x<left+56 && y>=top && y<top+48)return HoverRect(left,top,56,48);
    }
    return HoverRect();
}

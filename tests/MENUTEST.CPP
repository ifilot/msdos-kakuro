#include <dos.h>
#include <stdio.h>
#include <string.h>
#include "MENU.H"
#include "GAME.H"
unsigned _stklen = 16384;
static int errors;
static FILE *checks;
static void check(int condition, const char *message)
{
    if (!condition) { ++errors; fprintf(checks, "FAIL: %s\n", message); }
}
static unsigned char pixel(int x, int y)
{
    volatile unsigned char far *memory = (volatile unsigned char far *)MK_FP(0xA000, 0);
    unsigned char value = 0;
    for (int plane = 0; plane < 4; ++plane) {
        outport(0x3CE, (plane << 8) | 4);
        if (memory[(unsigned int)y * 80U + x / 8] & (0x80 >> (x & 7))) value |= 1 << plane;
    }
    return value;
}
int main()
{
    checks = fopen("CHECKS.TXT", "w");
    if (!checks) return 2;
    VgaScreen screen;
    MenuScreen menu(screen);
    check(menu.handleKey(13) == MenuScreen::Wait, "Empty catalog cannot launch");
    check(menu.handleKey(27) == MenuScreen::Exit, "Escape exits empty menu");
    check(menu.loadCatalog("puzzles\\071.puz"), "Load catalog");
    check(menu.count() == 96 && menu.pageCount() == 4, "Only numbered playable puzzles");
    check(menu.selectedIndex() == 70 && menu.pageNumber() == 2, "Preferred puzzle selected");
    check(menu.entry(70)->id == 71 && menu.entry(95)->id == 96, "Numbered puzzle IDs");
    check(!menu.entry(-1) && !menu.entry(96), "Bounds checked");
    // Only tests open the whole corpus: production uses its compact index.
    Puzzle catalogPuzzle;
    for (int record = 0; record < menu.count(); ++record) {
        const MenuScreen::Entry *item = menu.entry(record);
        check(catalogPuzzle.load(item->filename), "Catalog points to a valid puzzle");
        check(item->rows == catalogPuzzle.rows()
              && item->columns == catalogPuzzle.columns()
              && item->difficulty == catalogPuzzle.difficulty(),
              "Generated metadata matches runtime puzzle parser");
    }
    menu.handleKey(0x147);
    check(menu.selectedIndex() == 48, "Home selects page start");
    menu.handleKey(Game::Left); menu.handleKey(Game::Up);
    check(menu.selectedIndex() == 48, "Arrows stop at page edges");
    menu.handleKey(Game::Down); menu.handleKey(Game::Right);
    check(menu.selectedIndex() == 55, "Arrow grid movement");
    menu.handleKey(0x14F);
    check(menu.selectedIndex() == 71, "End selects final card");
    menu.handleKey(']'); menu.handleKey(0x151);
    check(menu.selectedIndex() == 95, "Last page clamps to populated card");
    menu.handleKey(Game::Right); menu.handleKey(Game::Down); menu.handleKey(0x151);
    check(menu.selectedIndex() == 95, "No selection of blank cards");
    menu.handleKey('[');
    check(menu.selectedIndex() == 71, "Previous page preserves slot");
    menu.loadCatalog("001.PUZ");
    screen.initialize(); menu.draw();
    check(pixel(100,112) == 6 && pixel(172,112) == 2, "Selected gold border and normal brown border");
    menu.handleKey(Game::Right);
    check(pixel(100,112) == 2 && pixel(172,112) == 6, "Selection redraw restores old card");
    menu.handleKey(Game::Left);
    menu.markPlayed(0,0); menu.draw();
    menu.handleKey(0x151);
    check(menu.selectedIndex() == 24 && pixel(100,112) == 6
          && pixel(172,112) == 2, "Fast page transition paints selected and normal cards");
    // Compare every card pixel to a complete repaint. This catches stale
    // digits, size labels and badge pixels when reusing the journal artwork.
    static unsigned char cardPixels[56 * 48];
    int cardRow, cardCol;
    for (cardRow = 0; cardRow < 48; ++cardRow)
        for (cardCol = 0; cardCol < 56; ++cardCol)
            cardPixels[cardRow * 56 + cardCol] = pixel(100 + cardCol,112 + cardRow);
    menu.draw();
    int equal = 1;
    for (cardRow = 0; cardRow < 48; ++cardRow)
        for (cardCol = 0; cardCol < 56; ++cardCol)
            if (cardPixels[cardRow * 56 + cardCol] != pixel(100 + cardCol,112 + cardRow)) equal = 0;
    check(equal, "Fast page pixels match full journal repaint");
    menu.handleKey(0x149);
    check(menu.selectedIndex() == 0 && pixel(100,112) == 6,
          "Fast previous page restores original selection");
    check(menu.handleKey(13) == MenuScreen::Play, "Enter launches selected puzzle");
    Game game(screen);
    check(game.loadPuzzle(menu.selectedFile()), "Load selection into game");
    game.draw();
    int row = game.selectedRow(), column = game.selectedColumn();
    game.handleKey('7');
    check(game.puzzle().cell(row,column)->value == 7, "Enter a value");
    check(!game.handleKey(27), "Game Escape returns to caller");
    menu.markPlayed(0,0); menu.draw(); game.draw();
    check(game.puzzle().cell(row,column)->value == 7, "Menu round trip preserves active board");
    check(menu.entry(0)->status == MenuScreen::Played, "Played status tracked");
    menu.markPlayed(1,1); menu.markPlayed(1,0);
    check(menu.entry(1)->status == MenuScreen::Solved, "Solved status retained");
    menu.showError("Cannot load this puzzle"); menu.handleKey(Game::Right);
    menu.handleKey(Game::Left); menu.draw();
    FILE *file = fopen("SCREEN.RAW", "wb");
    if (!file) return 2;
    unsigned char scanline[640];
    for (int y=0; y<480; ++y) {
        for (int x=0; x<640; ++x) scanline[x]=pixel(x,y);
        if (fwrite(scanline,1,640,file)!=640) return 2;
    }
    if (fclose(file)) return 2;
    file=fopen("PALETTE.RAW","wb");
    if (!file) return 2;
    outportb(0x3C7,0);
    for (int i=0; i<768; ++i) if (fputc(inportb(0x3C9),file)==EOF) return 2;
    if (fclose(file) || fclose(checks)) return 2;
    file=fopen("RESULT.TXT","w");
    if (!file) return 2;
    fprintf(file,errors ? "Menu tests failed\n" : "Menu tests passed\n");
    if (fclose(file)) return 2;
    return errors ? 1 : 0;
}

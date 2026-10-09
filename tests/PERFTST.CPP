// BIOS ticks provide a reproducible comparison, not a physical 8088 clock model.
#include <dos.h>
#include <stdio.h>
#include "MENU.H"
#include "GAME.H"
#include "PROFONT.H"
unsigned _stklen = 16384;
static unsigned long ticks()
{
    union REGS r;
    r.h.ah = 0;
    int86(0x1A, &r, &r);
    return ((unsigned long)r.x.cx << 16) | r.x.dx;
}
static void record(FILE *f, const char *label, unsigned long start)
{
    unsigned long end = ticks();
    fprintf(f, "%s=%lu\n", label, end >= start ? end-start : end+0x1800B0UL-start);
}
int main()
{
    FILE *file = fopen("PERFORM.TXT","w");
    if (!file) return 2;
    VgaScreen screen;
    MenuScreen menu(screen);
    unsigned long start = ticks();
    for (int i=0; i<3; ++i) if (!menu.loadCatalog("001.PUZ")) return 2;
    record(file,"catalog_3",start);
    screen.initialize();
    start=ticks();
    for (i=0; i<3; ++i) menu.draw();
    record(file,"menu_draw_3",start);
    Game game(screen);
    start=ticks();
    for (i=0; i<3; ++i) if (!game.loadPuzzle("PUZZLES\\001.PUZ")) return 2;
    record(file,"puzzle_load_3",start);
    start=ticks();
    for (i=0; i<3; ++i) game.draw();
    record(file,"game_draw_3",start);
    start=ticks();
    for (i=0; i<400; ++i) game.handleKey((i&1) ? Game::Left : Game::Right);
    record(file,"cursor_moves_400",start);
    screen.setDrawingScale(1);
    start=ticks();
    for (i=0; i<1000; ++i)
        ProFont::drawCentered(screen,100,100,32,32,"8",3);
    record(file,"digits_1000",start);
    if (fclose(file)) return 2;
    file=fopen("RESULT.TXT","w");
    if (!file) return 2;
    fputs("Performance tests passed\n",file);
    return fclose(file) ? 2 : 0;
}

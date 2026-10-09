#include <dos.h>
#include <stdio.h>
#include <string.h>
#include "MOUSE.H"
#include "HOVER.H"
#include "MENU.H"
#include "GAME.H"
#include "SOUNDMNU.H"
#include "DOCVIEW.H"
unsigned _stklen=16384;
static FILE *checks;
static int errors, shows, hides;
static unsigned mockButtons, mockX=300, mockY=200, rangeX, rangeY;
static void check(int ok,const char *message)
{ if(!ok){++errors;fprintf(checks,"FAIL: %s\n",message);} }

// Borland's interrupt parameters refer to the saved register frame.
void interrupt mockMouse(unsigned bp,unsigned di,unsigned si,unsigned ds,
    unsigned es,unsigned dx,unsigned cx,unsigned bx,unsigned ax,
    unsigned ip,unsigned cs,unsigned flags)
{
    switch(ax) {
        case 0:ax=0xFFFF;bx=2;break;
        case 1:++shows;break;
        case 2:++hides;break;
        case 3:bx=mockButtons;cx=mockX;dx=mockY;break;
        case 7:rangeX=dx;break;
        case 8:rangeY=dx;break;
    }
}

static int pixel(int x,int y)
{
    volatile unsigned char far *v=(volatile unsigned char far *)MK_FP(0xA000,0);
    int value=0;
    for(int plane=0;plane<4;++plane) {
        outport(0x3CE,(plane<<8)|4);
        if(v[(unsigned)y*80+x/8]&(0x80>>(x&7)))value|=1<<plane;
    }
    return value;
}

int main()
{
    checks=fopen("CHECKS.TXT","w");if(!checks)return 2;
    VgaScreen screen;screen.initialize();
    void interrupt (*original)(...)=getvect(0x33);
    Mouse mouse;
    setvect(0x33,0);
    check(!mouse.initialize(),"Missing mouse driver safely leaves keyboard mode");
    int x=-1,y=-1;
    mouse.show();mouse.hide();
    check(!mouse.poll(x,y) && x==-1 && y==-1,"Absent driver needs no polling calls");
    setvect(0x33,(void interrupt (*)(...))mockMouse);
    check(mouse.initialize() && rangeX==639 && rangeY==479,"Mouse uses native VGA bounds");
    mouse.show();mouse.show();mouse.hide();mouse.hide();
    check(shows==1 && hides==1,"Cursor show/hide calls stay balanced");
    check(mouse.poll(x,y)==Mouse::Motion,"Initial position generates a hover event");
    check(!mouse.poll(x,y),"Stationary pointer generates no redraw events");
    mockX=310;
    check(mouse.poll(x,y)==Mouse::Motion && x==310,"Movement generates a hover event without clicking");
    mockX=300;
    mockButtons=1;
    check(mouse.poll(x,y)==1 && x==300 && y==200,"Left click returns pixel coordinates");
    check(!mouse.poll(x,y),"Held click does not repeat across screen transitions");
    mockButtons=0;mouse.poll(x,y);mockButtons=2;
    check(mouse.poll(x,y)==2,"Right click recognized after release");
    mockButtons=0;mouse.poll(x,y);mockButtons=1;
    check(mouse.poll(x,y)==1,"Next left click recognized after release");
    mouse.hide();
    setvect(0x33,original);
    // Reinitialize against the emulator's driver for cursor save/restore checks.
    check(mouse.initialize(),"DOSBox mouse driver detected");
    screen.clear(0);screen.fillRect(280,180,40,40,6);
    union REGS r;r.x.ax=4;r.x.cx=300;r.x.dx=200;int86(0x33,&r,&r);
    mouse.show();mouse.hide();
    check(pixel(300,200)==6,"Driver cursor restores pixels when hidden before redraw");
    // Cursor remains hidden for all drawing below.

    Hover hover(screen);
    // Exercise all palette bits across unaligned horizontal and vertical edges.
    for(int sampleRow=0;sampleRow<15;++sampleRow)
        for(int sampleColumn=0;sampleColumn<20;++sampleColumn)
            screen.fillRect(10+sampleColumn*4,10+sampleRow*4,4,4,
                            (sampleColumn+sampleRow*3)&15);
    hover.update(HoverRect(11,11,75,45));hover.clear();
    for(int sample=0;sample<73;++sample) {
        check(pixel(12+sample,12)==(((2+sample)/4)&15),
              "Hover restores all colors across unaligned top edge");
        check(pixel(12+sample,54)==((((2+sample)/4)+33)&15),
              "Hover restores all colors across unaligned bottom edge");
    }
    for(sample=0;sample<41;++sample) {
        check(pixel(12,13+sample)==(((3+sample)/4*3)&15),
              "Hover restores patterned left edge");
        check(pixel(84,13+sample)==((18+(3+sample)/4*3)&15),
              "Hover restores patterned right edge");
    }
    Sound sound; // Off by default: clicks change preferences without ISA hardware.
    MenuScreen menu(screen);menu.setSound(&sound);menu.loadCatalog();menu.draw();
    check(SoundMenu::buttonKey(518,32,24)==0x13D &&
          SoundMenu::buttonKey(558,32,24)==0x13E &&
          SoundMenu::buttonKey(598,32,24)==0x13F,"Speaker, note and cog hit boxes");
    check(!SoundMenu::buttonKey(544,32,24),"Space between audio buttons is inert");
    int selectedBefore=menu.selectedIndex();
    int originalCard=pixel(173,113),outsideCard=pixel(170,113);
    hover.update(menu.hoverAt(180,120));
    check(pixel(173,113)==3 && pixel(170,113)==outsideCard,
          "Card hover darkens its background without changing neighbors");
    check(menu.selectedIndex()==selectedBefore,"Hover does not select or launch a puzzle");
    hover.update(menu.hoverAt(180,120));
    hover.clear();
    check(pixel(173,113)==originalCard,"Repeated hover and leave restore original card pixels");
    hover.update(menu.hoverAt(110,120));
    check(pixel(101,113)==3,"Selected card also gets a darker hover face");hover.clear();
    check(pixel(101,113)==2,"Selected card restores its normal brown face");
    hover.update(menu.hoverAt(518,32));
    check(pixel(509,25)==3,"Audio button hover highlighted");
    hover.update(menu.hoverAt(544,32));
    check(pixel(509,25)==0,"Leaving audio button restores its face");
    check(!menu.hoverAt(250,430).width,"Unavailable previous-page control is not highlighted");
    menu.handleMouse(518,32,1);check(!sound.effectsEnabled(),"Speaker click toggles effects");
    menu.handleMouse(558,32,1);check(!sound.musicEnabled(),"Note click toggles music");
    check(menu.handleMouse(598,32,1)==MenuScreen::Audio,"Cog click opens hardware panel");
    check(menu.handleMouse(180,120,1)==MenuScreen::Play && menu.selectedIndex()==1,
          "Clicking a card launches the matching puzzle");
    check(menu.handleMouse(360,430,1)==MenuScreen::Wait && menu.pageNumber()==1,
          "Page control switches journal page");
    check(menu.handleMouse(160,430,1)==MenuScreen::Help &&
          menu.handleMouse(420,430,1)==MenuScreen::About,"Help and About buttons clickable");
    check(menu.handleMouse(510,430,1)==MenuScreen::Exit,"Quit button clickable");
    check(menu.handleMouse(20,20,1)==MenuScreen::Wait,"Background clicks are ignored");

    Game game(screen);game.setSound(&sound);game.setMouseAvailable(1);
    check(game.loadPuzzle("PUZZLES\\001.PUZ"),"Load clickable puzzle");game.draw();
    game.handleKey('7');
    static unsigned char cellBefore[56*56];
    unsigned char cellRow[56];
    for(int glyphRow=0;glyphRow<56;++glyphRow)
        screen.readPixels(262,130+glyphRow,cellBefore+glyphRow*56,56);
    hover.update(game.hoverAt(290,158));
    int glyphPixels=0;
    for(glyphRow=2;glyphRow<54;++glyphRow)
        for(int glyphColumn=2;glyphColumn<54;++glyphColumn) {
            unsigned char color=cellBefore[glyphRow*56+glyphColumn];
            if(color==3) {
                ++glyphPixels;
                check(pixel(262+glyphColumn,130+glyphRow)==0,
                      "Hovered digit keeps its shape in readable light ink");
            } else if(color==4)
                check(pixel(262+glyphColumn,130+glyphRow)==3,
                      "Hovered cell interior is dark behind the digit");
        }
    check(glyphPixels>0 && game.puzzle().cell(1,2)->value==7,
          "Hover leaves entered digit and puzzle data intact");
    // Dump the dark hover state for visual review.
    FILE *snapshot=fopen("SCREEN.RAW","wb");if(!snapshot)return 2;
    unsigned char snapshotRow[640];
    for(int snapshotY=0;snapshotY<480;++snapshotY) {
        screen.readPixels(0,snapshotY,snapshotRow,640);
        if(fwrite(snapshotRow,1,640,snapshot)!=640)return 2;
    }
    if(fclose(snapshot))return 2;
    snapshot=fopen("PALETTE.RAW","wb");if(!snapshot)return 2;
    outportb(0x3C7,0);
    for(int paletteByte=0;paletteByte<768;++paletteByte)fputc(inportb(0x3C9),snapshot);
    if(fclose(snapshot))return 2;
    hover.clear();
    for(glyphRow=0;glyphRow<56;++glyphRow) {
        screen.readPixels(262,130+glyphRow,cellRow,56);
        check(!memcmp(cellRow,cellBefore+glyphRow*56,56),
              "Leaving cell hover restores every digit and background pixel");
    }
    int originalCell=pixel(319,131),originalDigit=pixel(155,155);
    hover.update(game.hoverAt(346,158));
    check(pixel(330,140)==3 && pixel(155,155)==originalDigit,
          "Native pixel cell hover darkens its face without touching other cells");
    check(game.selectedColumn()==2,"Cell hover preserves keyboard selection");
    hover.clear();
    check(pixel(319,131)==originalCell && screen.drawingScaleFactor()==2,
          "Cell hover restores pixels and game drawing scale");
    check(!game.hoverAt(150,74).width,"Blocked cell has no hover highlight");
    hover.update(game.hoverAt(156,456));
    check(pixel(149,449)==3,"Digit button hover highlighted");hover.clear();
    check(pixel(149,449)==0,"Digit button highlight restores paper face");
    game.handleMouse(346,158,1);
    check(game.selectedRow()==1 && game.selectedColumn()==3,"Native pixel click selects answer cell");
    game.handleMouse(156,456,1);
    check(game.puzzle().cell(1,3)->value==1,"Digit button writes selected cell");
    game.handleMouse(430,456,1);
    check(game.puzzle().cell(1,3)->value==0,"Clear button erases selected cell");
    game.handleMouse(150,74,1);
    check(game.selectedColumn()==3,"Blocked cell click does not move selector");
    check(game.handleMouse(598,16,1)==Game::AudioSettings,"Puzzle cog opens hardware panel");
    check(!game.handleMouse(30,456,1),"Puzzle Back button returns to journal");
    check(!game.handleMouse(20,20,2),"Right click acts as Escape");

    check(game.loadPuzzle("FIXTURES\\SOLVED.PUZ"),"Load completed puzzle for mouse modal");
    game.draw();
    hover.update(game.hoverAt(200,310));
    check(pixel(145,291)==14,"Completion button has a dark background");hover.clear();
    check(!game.handleMouse(200,310,1) && game.isCompleted(),"Completion primary button returns to journal");
    game.loadPuzzle("FIXTURES\\SOLVED.PUZ");game.draw();
    check(game.handleMouse(200,356,1) && !game.completionModalVisible(),
          "Completion secondary button shows solved board");
    SoundMenu panel(screen,sound);panel.draw();
    int rowPixel=pixel(137,155);
    hover.update(panel.hoverAt(180,160));
    check(pixel(137,155)==14,"Hardware device row hover highlighted");hover.clear();
    check(pixel(137,155)==rowPixel && panel.selectedIndex()==0,
          "Hardware hover preserves selection and row pixels");
    panel.handleMouse(180,160,1);check(panel.selectedIndex()==1,"Mouse selects Off device row");
    panel.handleMouse(180,390,1);check(sound.selectedDevice()==Sound::Off,"Apply control sets device");
    check(!panel.handleMouse(360,390,1),"Hardware panel Close control works");
    DocumentScreen document(screen);document.show(DocumentScreen::Help);
    hover.update(document.hoverAt(540,440));
    check(pixel(513,425)==3 && document.topLine()==0,"Help hover does not scroll");hover.clear();
    document.handleMouse(540,440,1);check(document.topLine()==1,"Help Down control scrolls");
    document.handleMouse(576,399,1);check(document.topLine()==document.lineCount()-18,
          "Help scrollbar reaches last page");
    check(!document.handleMouse(80,440,1),"Help Back control closes page");
    document.show(DocumentScreen::About);
    check(!document.handleMouse(260,342,1),"About footer closes modal");
    remove("SOUND.CFG");
    mouse.hide();screen.restoreTextMode();
    if(fclose(checks))return 2;
    FILE *result=fopen("RESULT.TXT","w");if(!result)return 2;
    fputs(errors ? "Mouse tests failed\n" : "Mouse tests passed\n",result);
    return fclose(result) ? 2 : errors ? 1 : 0;
}

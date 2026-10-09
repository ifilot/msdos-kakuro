#include <dos.h>
#include <stdio.h>
#include "DOCVIEW.H"
#include "MENU.H"
#include "GAME.H"
unsigned _stklen=16384;
static int errors;
static FILE *checks;
static void check(int ok,const char *message)
{if(!ok){++errors;fprintf(checks,"FAIL: %s\n",message);}}
static int pixel(int x,int y)
{
    volatile unsigned char far *v=(volatile unsigned char far *)MK_FP(0xA000,0);
    int value=0;
    for(int p=0;p<4;++p){outport(0x3CE,(p<<8)|4);if(v[(unsigned int)y*80+x/8]&(0x80>>(x&7)))value|=1<<p;}
    return value;
}
static int dump(const char *filename)
{
    FILE *file=fopen(filename,"wb");if(!file)return 0;
    unsigned char row[640];
    for(int y=0;y<480;++y) {
        for(int x=0;x<640;++x)row[x]=pixel(x,y);
        if(fwrite(row,1,640,file)!=640){fclose(file);return 0;}
    }
    return !fclose(file);
}
int main()
{
    checks=fopen("CHECKS.TXT","w");if(!checks)return 2;
    VgaScreen screen;screen.initialize();
    MenuScreen menu(screen);menu.loadCatalog("071.PUZ");
    check(menu.handleKey(0x13B)==MenuScreen::Help && menu.handleKey('h')==MenuScreen::Help,"Help shortcuts");
    check(menu.handleKey(0x13C)==MenuScreen::About && menu.handleKey('a')==MenuScreen::About,"About shortcuts");
    int selection=menu.selectedIndex();
    DocumentScreen document(screen);
    document.show(DocumentScreen::Help);
    check(document.topLine()==0 && document.lineCount()>36,"Help opens first page");
    check(pixel(48,96)==2 && pixel(64,104)==0,"Journal reading panel");
    document.handleKey(Game::Up);check(!document.topLine(),"Scroll clamps at top");
    document.handleKey(Game::Down);check(document.topLine()==1,"Scroll one line");
    document.handleKey(0x151);check(document.topLine()==19,"Page scroll");
    document.handleKey(0x14F);int last=document.lineCount()-18;
    check(document.topLine()==last,"End selects last screen");
    document.handleKey(Game::Down);check(document.topLine()==last,"Scroll clamps at bottom");
    document.handleKey(0x149);check(document.topLine()==last-18,"Page up");
    document.handleKey(0x147);check(!document.topLine(),"Home resets scroll");
    check(!document.handleKey(27),"Escape returns to journal");
    menu.draw();
    int savedCorner=pixel(100,112);
    document.show(DocumentScreen::About);
    check(!document.topLine() && !document.lineCount(),"About is a compact non-scrolling modal");
    check(pixel(112,118)==10 && pixel(120,190)==12,"About frame and paper face");
    check(pixel(100,112)==savedCorner,"About preserves journal pixels outside modal");
    document.handleKey(Game::Down); document.handleKey(0x14F);
    check(!document.topLine(),"About ignores scrolling keys");
    check(!document.handleKey(13),"Enter returns to journal");
    check(menu.selectedIndex()==selection,"Document leaves selection intact");
    menu.draw();check(pixel(100+4*72+24,112+3*56)==6,"Returning journal restores selected card");
    document.show(DocumentScreen::Help);
    if(!dump("SCREEN.RAW"))return 2;
    menu.draw();
    document.show(DocumentScreen::About);
    if(!dump("ABOUT.RAW"))return 2;
    FILE *file;
    file=fopen("PALETTE.RAW","wb");if(!file)return 2;outportb(0x3C7,0);
    for(int i=0;i<768;++i)fputc(inportb(0x3C9),file);
    if(fclose(file)||fclose(checks))return 2;
    file=fopen("RESULT.TXT","w");if(!file)return 2;
    fputs(errors ? "Document tests failed\n" : "Document tests passed\n",file);
    return fclose(file) ? 2 : errors ? 1 : 0;
}

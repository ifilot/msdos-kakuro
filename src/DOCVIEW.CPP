#include <stdio.h>
#include "DOCVIEW.H"
#include "DOCDATA.H"
#include "PROFONT.H"
#include "GAME.H"
#include "BUILDINF.H"

enum { Paper = 0, Rose = 1, Brown = 2, Ink = 3 };
static void panel(VgaScreen &screen, int x, int y, int w, int h)
{
    screen.fillRect(x+2,y+3,w,h,Ink);
    screen.fillRect(x,y,w,h,Brown);
    screen.fillRect(x+1,y+1,w-2,h-2,Paper);
}
DocumentScreen::DocumentScreen(VgaScreen &display)
    : screen(display), current(Help), top(0), count(0) {}

void DocumentScreen::show(Document document)
{
    current=document; top=0;
    count=0;
    if(document==About) { showAbout(); return; }
    count=sizeof(documentHELP)/sizeof(documentHELP[0]);
    screen.setDrawingScale(1);
    static const unsigned char colors[4][3]={{57,53,36},{35,21,19},{14,9,8},{5,5,6}};
    for (int i=0;i<4;++i) screen.setPaletteColor(i,colors[i][0],colors[i][1],colors[i][2]);
    if (!screen.loadHighBitmap("MENU.VGA")) screen.clear(Paper);
    panel(screen,48,24,544,56); panel(screen,48,96,544,320); panel(screen,48,424,544,40);
    ProFont::drawCentered(screen,72,30,496,28,"HOW TO PLAY",Brown,2);
    screen.drawText(72,64,"RULES, CONTROLS AND PUZZLES",Rose);
    screen.drawText(72,440,"< BACK Esc",Brown);
    screen.drawText(192,440,"ARROWS / PGUP PGDN",Rose);
    screen.drawText(448,440,"^ UP",Brown); screen.drawText(528,440,"DOWN v",Brown);
    renderText();
}
void DocumentScreen::showAbout()
{
    screen.setDrawingScale(1);
    // Keep the journal in VGA memory; dim it with its palette for the modal.
    static const unsigned char theme[9][3]={
        {57,53,36},{35,21,19},{14,9,8},{5,5,6},{59,56,43},
        {44,31,24},{63,48,16},{24,14,9},{58,35,33}
    };
    for(int i=0;i<9;++i)
        screen.setPaletteColor(i,theme[i][0]/2,theme[i][1]/2,theme[i][2]/2);
    enum { Frame=10,Gold=11,Face=12,Accent=13,Shadow=14 };
    screen.setPaletteColor(Frame,14,9,8);
    screen.setPaletteColor(Gold,63,48,16);
    screen.setPaletteColor(Face,59,56,43);
    screen.setPaletteColor(Accent,35,21,19);
    screen.setPaletteColor(Shadow,4,4,4);
    screen.fillRect(116,122,416,244,Shadow);
    screen.fillRect(112,118,416,244,Frame);
    screen.fillRect(114,120,412,240,Gold);
    screen.fillRect(116,122,408,236,Face);
    ProFont::drawCentered(screen,128,136,384,32,"ABOUT KAKURO",Frame,2);
    screen.drawLine(136,180,504,180,Accent);
    static const char *labels[6]={"Version:","Author:","Compiler:","Built:","Commit:","GitHub:"};
    const char *values[6]={KAKURO_VERSION,KAKURO_AUTHOR,KAKURO_COMPILER,
                          KAKURO_BUILD_DATE,KAKURO_COMMIT,KAKURO_REPOSITORY};
    for(i=0;i<6;++i) {
        screen.drawText(140,196+i*24,labels[i],Accent);
        screen.drawText(232,196+i*24,values[i],Frame);
    }
    screen.drawLine(136,328,504,328,Accent);
    screen.drawText(248,342,"Enter / Esc: close",Frame);
}

void DocumentScreen::renderText()
{
    screen.setDrawingScale(1);
    screen.fillRect(72,108,480,292,Paper);
    const DocumentLine *lines=documentHELP;
    for (int row=0;row<VisibleLines && top+row<count;++row)
        screen.drawText(72,112+row*16,lines[top+row].text,lines[top+row].heading ? Rose : Brown);
    screen.fillRect(72,402,480,8,Paper);
    char status[40];
    int end=top+VisibleLines; if(end>count)end=count;
    sprintf(status,"LINES %d-%d OF %d",top+1,end,count);
    screen.drawText(72,402,status,Rose);
    screen.fillRect(572,112,12,288,Brown); screen.fillRect(573,113,10,286,Rose);
    int maximum=count>VisibleLines ? count-VisibleLines : 0;
    int height=maximum ? VisibleLines*288/count : 288;
    if(height<16)height=16;
    int offset=maximum ? top*(288-height)/maximum : 0;
    screen.fillRect(574,112+offset,8,height,Paper);
}
int DocumentScreen::handleKey(int key)
{
    if(key==27 || key==13) return 0;
    if(!count)return 1;
    int next=top, maximum=count>VisibleLines ? count-VisibleLines : 0;
    switch(key) {
        case Game::Up: --next; break;
        case Game::Down: ++next; break;
        case 0x149: next-=VisibleLines; break;
        case 0x151: case ' ': next+=VisibleLines; break;
        case 0x147: next=0; break;
        case 0x14F: next=maximum; break;
    }
    if(next<0)next=0; if(next>maximum)next=maximum;
    if(next!=top) {top=next;renderText();}
    return 1;
}

int DocumentScreen::handleMouse(int x,int y,int button)
{
    if(button==2)return handleKey(27);
    if(button!=1)return 1;
    if(current==About) {
        if(x>=112 && x<528 && y>=328 && y<362)return handleKey(13);
        return 1;
    }
    if(y>=424 && y<464) {
        if(x>=48 && x<184)return handleKey(27);
        if(x>=400 && x<512)return handleKey(Game::Up);
        if(x>=512 && x<592)return handleKey(Game::Down);
    }
    if(x>=572 && x<584 && y>=112 && y<400) {
        int maximum=count>VisibleLines ? count-VisibleLines : 0;
        top=(int)((long)(y-112)*maximum/287);
        renderText();
    }
    return 1;
}

HoverRect DocumentScreen::hoverAt(int x,int y) const
{
    if(current==About) {
        if(x>=112 && x<528 && y>=328 && y<362)return HoverRect(112,328,416,34,1);
        return HoverRect();
    }
    if(y>=424 && y<464) {
        if(x>=48 && x<184)return HoverRect(48,424,136,40);
        if(x>=400 && x<512 && top>0)return HoverRect(400,424,112,40);
        if(x>=512 && x<592 && top<count-VisibleLines)return HoverRect(512,424,80,40);
    }
    if(x>=572 && x<584 && y>=112 && y<400)return HoverRect(572,112,12,288);
    return HoverRect();
}

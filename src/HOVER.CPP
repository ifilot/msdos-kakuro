#include <stdlib.h>
#include <string.h>
#include "HOVER.H"

Hover::~Hover() { free(saved); }

unsigned char Hover::original(unsigned index) const
{
    return (saved[index>>1] >> ((index&1)*4)) & 15;
}

void Hover::drawRow(int x,int y,int length,const unsigned char *colors)
{
    for(int start=0;start<length;) {
        int end=start+1;
        while(end<length && colors[end]==colors[start])++end;
        screen.fillRect(x+start,y,end-start,1,colors[start]);
        start=end;
    }
}

void Hover::clear()
{
    if(!current.width)return;
    int scale=screen.drawingScaleFactor();screen.setDrawingScale(1);
    unsigned char row[VgaScreen::Width];
    unsigned index=0;
    for(int y=0;y<current.height;++y) {
        for(int x=0;x<current.width;++x)row[x]=original(index++);
        drawRow(current.x,current.y+y,current.width,row);
    }
    screen.setDrawingScale(scale);
    current=HoverRect();
}

void Hover::update(const HoverRect &target)
{
    HoverRect next;
    if(target.width>=4 && target.height>=4 && target.x>=0 && target.y>=0
        && target.width<=VgaScreen::Width-target.x
        && target.height<=VgaScreen::Height-target.y
        && (unsigned long)(target.width-2)*(target.height-2)<=SnapshotBytes*2UL)
        next=HoverRect(target.x+1,target.y+1,target.width-2,target.height-2,target.dialog);
    if(next.x==current.x && next.y==current.y && next.width==current.width
        && next.height==current.height && next.dialog==current.dialog)return;
    clear();
    if(!next.width)return;
    // Allocate once, only when a mouse first hovers a control. Failure leaves
    // the game usable without hover feedback; nothing has been painted yet.
    if(!saved)saved=(unsigned char *)malloc(SnapshotBytes);
    if(!saved)return;
    unsigned pixels=(unsigned long)next.width*next.height;
    memset(saved,0,(pixels+1U)/2U);
    unsigned char row[VgaScreen::Width];
    unsigned counts[16]={0};
    unsigned index=0;
    for(int y=0;y<next.height;++y) {
        screen.readPixels(next.x,next.y+y,row,next.width);
        for(int x=0;x<next.width;++x,++index) {
            ++counts[row[x]];
            saved[index>>1]|=row[x]<<((index&1)*4);
        }
    }
    // The dominant color is the face; recolor it dark and keep the glyphs
    // light. This also works for selected cards and disabled audio buttons.
    int face=0;
    for(int color=1;color<16;++color)if(counts[color]>counts[face])face=color;
    unsigned char background=next.dialog ? 14 : 3;
    unsigned char foreground=next.dialog ? 12 : 0;
    int scale=screen.drawingScaleFactor();screen.setDrawingScale(1);
    index=0;
    for(y=0;y<next.height;++y) {
        for(int x=0;x<next.width;++x,++index)
            row[x]=original(index)==face ? background : foreground;
        drawRow(next.x,next.y+y,next.width,row);
    }
    screen.setDrawingScale(scale);
    current=next;
}

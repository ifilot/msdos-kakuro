#include <dos.h>
#include "MOUSE.H"

int Mouse::initialize()
{
    hide();
    installed=visible=previous=0;
    refreshPosition();
    // Uninstalled vectors may be null or point to a bare IRET. Do not call them.
    const unsigned char far *handler=(const unsigned char far *)getvect(0x33);
    if(!handler || *handler==0xCF)return 0;
    union REGS r;
    r.x.ax=0;int86(0x33,&r,&r);
    if(r.x.ax!=0xFFFF)return 0;
    installed=1;
    r.x.ax=7;r.x.cx=0;r.x.dx=639;int86(0x33,&r,&r);
    r.x.ax=8;r.x.cx=0;r.x.dx=479;int86(0x33,&r,&r);
    r.x.ax=3;int86(0x33,&r,&r);previous=r.x.bx;
    return 1;
}
void Mouse::show()
{
    if(!installed || visible)return;
    union REGS r;r.x.ax=1;int86(0x33,&r,&r);visible=1;
}
void Mouse::hide()
{
    if(!installed || !visible)return;
    union REGS r;r.x.ax=2;int86(0x33,&r,&r);visible=0;
}
int Mouse::poll(int &x,int &y)
{
    if(!installed)return 0;
    union REGS r;r.x.ax=3;int86(0x33,&r,&r);
    x=r.x.cx;y=r.x.dx;
    int moved=x!=previousX || y!=previousY;
    previousX=x;previousY=y;
    unsigned pressed=r.x.bx & ~previous;
    previous=r.x.bx;
    return pressed&1 ? 1 : pressed&2 ? 2 : moved ? Motion : 0;
}

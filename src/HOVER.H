#ifndef KAKURO_HOVER_H
#define KAKURO_HOVER_H
#include "VGA.H"

struct HoverRect {
    int x,y,width,height;
    unsigned char dialog;
    HoverRect(int left=0,int top=0,int w=0,int h=0,unsigned char modal=0)
        : x(left),y(top),width(w),height(h),dialog(modal) {}
};

// Save the hovered interior as packed four-bit pixels. No image reloads or
// full-screen redraws. Clear before changing content, with the cursor hidden.
class Hover {
public:
    Hover(VgaScreen &display) : screen(display), saved(0) {}
    ~Hover();
    void update(const HoverRect &target);
    void clear();
private:
    enum { SnapshotBytes = 16384 };
    VgaScreen &screen;
    HoverRect current;
    unsigned char *saved;
    unsigned char original(unsigned index) const;
    void drawRow(int x,int y,int length,const unsigned char *colors);
    Hover(const Hover &);
    Hover &operator=(const Hover &);
};
#endif

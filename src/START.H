#ifndef KAKURO_START_H
#define KAKURO_START_H

#include "VGA.H"

class StartScreen {
public:
    StartScreen(VgaScreen &display) : screen(display) {}
    int show(const char *bitmap = "START.VGA", const char *palette = "START.PAL");
    // Enter starts (1), Escape exits (-1), all other keys wait (0).
    int handleKey(int key) const;
private:
    VgaScreen &screen;
};

#endif

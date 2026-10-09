#ifndef KAKURO_PROFONT_H
#define KAKURO_PROFONT_H

#include "VGA.H"

class ProFont {
public:
    // Center visible ASCII ink, preserving the 12-pixel advance and baseline.
    static void drawCentered(VgaScreen &screen, int x, int y, int width, int height,
                             const char *text, unsigned char color, int scale = 1);
};

#endif

#ifndef KAKURO_DIGITS_H
#define KAKURO_DIGITS_H

#include "VGA.H"

class DigitFonts {
public:
    enum Style { Clean, Round, Slab, Lcd, Bold, StyleCount };
    static const char *name(int style);
    static void drawDigit(VgaScreen &screen, int x, int y, int size,
                          unsigned char digit, int style, unsigned char color);
    static void drawSamples(VgaScreen &screen);
};

#endif

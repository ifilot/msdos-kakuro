#include <stdio.h>
#include "DIGITS.H"
#include "DIGITDAT.H"

const char *DigitFonts::name(int style)
{
    return style >= 0 && style < StyleCount ? digitNames[style] : "";
}

void DigitFonts::drawDigit(VgaScreen &screen, int x, int y, int size,
                           unsigned char digit, int style, unsigned char color)
{
    if (style < 0 || style >= StyleCount || digit > 9 || size < 2) return;
    const DigitSet &set = digitSets[style];
    screen.drawCenteredBitmap(x + 1, y + 1, size - 1, size - 1,
                              set.rows[digit], set.width, set.height, color);
}

void DigitFonts::drawSamples(VgaScreen &screen)
{
    screen.setPaletteColor(0, 3, 5, 8);
    screen.setPaletteColor(18, 28, 36, 43);
    screen.setPaletteColor(19, 57, 59, 58);
    screen.setPaletteColor(20, 34, 53, 52);
    screen.setPaletteColor(21, 3, 10, 13);
    screen.setPaletteColor(22, 61, 62, 59);
    screen.clear(0);
    screen.drawText(84, 8, "BITMAP DIGIT SAMPLES", 22);
    for (int style = 0; style < StyleCount; ++style) {
        int top = 38 + style * 36;
        char label[16];
        sprintf(label, "%d %s", style + 1, name(style));
        screen.drawText(8, top + 7, label, 22);
        for (int digit = 1; digit <= 9; ++digit) {
            int left = 88 + (digit - 1) * 22;
            screen.colorSquare(left + 1, top + 1, 21, digit & 1 ? 20 : 19);
            drawDigit(screen, left, top, 22, (unsigned char)digit, style, 21);
        }
        screen.drawGrid(88, top, 9, 1, 22, 18);
    }
    screen.drawText(40, 222, "Choose 1-5 in chat", 22);
    screen.drawText(40, 232, "Any key closes preview", 22);
}

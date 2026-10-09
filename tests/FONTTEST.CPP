#include <dos.h>
#include <stdio.h>
#include "DIGITS.H"

static unsigned char readPixel(int x, int y)
{
    outport(0x3CE, ((x & 3) << 8) | 0x04);
    return ((volatile unsigned char far *)MK_FP(0xA000, 0))[y * 80 + x / 4];
}

int main()
{
    VgaScreen screen;
    screen.initializeModeX();
    // Off-center font padding must not shift the visible glyph.
    static const unsigned char padded[5] = {0, 1, 1, 1, 0};
    screen.drawCenteredBitmap(0, 0, 21, 21, padded, 5, 5, 21);
    for (int y = 0; y < 21; ++y)
        for (int x = 0; x < 21; ++x) {
            unsigned char expected = x == 10 && y >= 9 && y <= 11 ? 21 : 0;
            if (readPixel(x, y) != expected) return 1;
        }
    DigitFonts::drawSamples(screen);
    // Each digit's ink must sit within half a pixel of the cell center.
    for (int style = 0; style < 5; ++style) {
        for (int digit = 1; digit <= 9; ++digit) {
            int left = 88 + (digit - 1) * 22;
            int top = 38 + style * 36;
            int minX = 22, maxX = -1, minY = 22, maxY = -1;
            for (int row = 1; row < 22; ++row)
                for (int col = 1; col < 22; ++col)
                    if (readPixel(left + col, top + row) == 21) {
                        if (col < minX) minX = col;
                        if (col > maxX) maxX = col;
                        if (row < minY) minY = row;
                        if (row > maxY) maxY = row;
                    }
            if (maxX < 0 || minX + maxX < 21 || minX + maxX > 22
                || minY + maxY < 21 || minY + maxY > 22) return 1;
        }
    }
    FILE *pixels = fopen("SCREEN.RAW", "wb");
    if (!pixels) return 2;
    unsigned char scanline[320];
    for (y = 0; y < 240; ++y) {
        for (int x = 0; x < 320; ++x) scanline[x] = readPixel(x, y);
        if (fwrite(scanline, 1, 320, pixels) != 320) return 2;
    }
    if (fclose(pixels)) return 2;
    FILE *palette = fopen("PALETTE.RAW", "wb");
    if (!palette) return 2;
    outportb(0x3C7, 0);
    for (int i = 0; i < 768; ++i)
        if (fputc(inportb(0x3C9), palette) == EOF) return 2;
    if (fclose(palette)) return 2;
    FILE *result = fopen("RESULT.TXT", "w");
    if (!result) return 2;
    fputs("Font tests passed\n", result);
    return fclose(result) ? 2 : 0;
}

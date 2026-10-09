#include <dos.h>
#include <stdio.h>
#include "VGA.H"

unsigned _stklen = 16384;
static FILE *checks;
static void stage(const char *label, int errors)
{ fprintf(checks, "%s: %d errors\n", label, errors); fflush(checks); }

static unsigned char readPixel(int x, int y)
{
    volatile unsigned char far *memory =
        (volatile unsigned char far *)MK_FP(0xA000, 0);
    outport(0x3CE, ((x & 3) << 8) | 0x04);
    return memory[y * 80 + x / 4];
}

static int checkPixel(int x, int y, unsigned char expected)
{
    unsigned char actual = readPixel(x, y);
    if (actual == expected)
        return 0;
    fprintf(checks, "Pixel (%d,%d): expected %u, got %u\n", x, y,
           (unsigned int)expected, (unsigned int)actual);
    return 1;
}

static int checkMono(VgaScreen &screen, int x, int y,
                     const unsigned short *rows, int width, int height,
                     int scale, int drawingScale)
{
    screen.clear(3);
    screen.setDrawingScale(drawingScale);
    screen.drawMonoRows(x, y, rows, width, height, 5, scale);
    screen.setDrawingScale(1);
    int factor = scale * drawingScale;
    x *= drawingScale; y *= drawingScale;
    int errors = 0;
    volatile unsigned char far *video =
        (volatile unsigned char far *)MK_FP(0xA000, 0);
    for (int sy = y - 2; sy < y + height * factor + 2; ++sy) {
        if (sy < 0 || sy >= 480) continue;
        for (int sx = x - 2; sx < x + width * factor + 2; ++sx) {
            if (sx < 0 || sx >= 640) continue;
            unsigned char expected = 3, actual = 0;
            if (sx >= x && sx < x + width * factor
                && sy >= y && sy < y + height * factor
                && (rows[(sy - y) / factor]
                    & (1U << (width - 1 - (sx - x) / factor)))) expected = 5;
            for (int plane = 0; plane < 4; ++plane) {
                outport(0x3CE, (plane << 8) | 4);
                if (video[(unsigned int)sy * 80U + sx / 8] & (0x80 >> (sx & 7)))
                    actual |= 1 << plane;
            }
            if (actual != expected) {
                if (errors < 16) fprintf(checks, "Mono (%d,%d): expected %u got %u\n",
                    sx, sy, (unsigned)expected, (unsigned)actual);
                ++errors;
            }
        }
    }
    fprintf(checks, "Mono x%d y%d scale%d: %d errors\n", x, y, factor, errors);
    fflush(checks);
    return errors;
}

int main()
{
    int errors = 0;
    checks = fopen("CHECKS.TXT", "w");
    if (!checks) return 2;
    {
        VgaScreen screen;
        screen.initializeModeX();
        int x, y;
        for (y = 0; y < 16; ++y)
            for (x = 0; x < 16; ++x)
                screen.fillRect(x * 8, y * 8, 8, 8,
                                (unsigned char)(y * 16 + x));

        FILE *pixels = fopen("SCREEN.RAW", "wb");
        if (!pixels) return 2;
        unsigned char scanline[320];
        for (y = 0; y < VgaScreen::LowHeight; ++y) {
            for (x = 0; x < VgaScreen::LowWidth; ++x) {
                unsigned char expected = x < 128 && y < 128
                    ? (unsigned char)((y / 8) * 16 + x / 8) : 0;
                scanline[x] = readPixel(x, y);
                if (scanline[x] != expected) ++errors;
            }
            if (fwrite(scanline, 1, sizeof(scanline), pixels) != sizeof(scanline))
                return 3;
        }
        if (fclose(pixels)) return 3;

        FILE *palette = fopen("PALETTE.RAW", "wb");
        if (!palette) return 2;
        outportb(0x3C7, 0);
        for (int i = 0; i < 768; ++i)
            if (fputc(inportb(0x3C9), palette) == EOF) return 3;
        if (fclose(palette)) return 3;

        stage("ModeX initial", errors);
        // Verify unaligned plane boundaries, clipping, and signed overflow.
        screen.fillRect(3, 130, 9, 2, 42);
        screen.fillRect(-3, 140, 5, 2, 43);
        screen.fillRect(310, -3, 20, 5, 44);
        screen.fillRect(319, 239, 32767, 32767, 45);
        screen.fillRect(-32768, 0, 32767, 1, 99);
        screen.fillRect(10, 150, 0, 10, 99);
        screen.putPixel(318, 239, 46);
        screen.putPixel(-1, 239, 99);
        screen.putPixel(320, 239, 99);
        screen.putPixel(318, 240, 99);
        for (x = 0; x < 16; ++x)
            errors += checkPixel(x, 130, x >= 3 && x < 12 ? 42 : 0);
        errors += checkPixel(3, 132, 0);
        errors += checkPixel(0, 140, 43);
        errors += checkPixel(1, 141, 43);
        errors += checkPixel(2, 140, 0);
        errors += checkPixel(309, 0, 0);
        errors += checkPixel(319, 1, 44);
        errors += checkPixel(319, 2, 0);
        errors += checkPixel(319, 239, 45);
        errors += checkPixel(318, 239, 46);
        errors += checkPixel(0, 0, 0);
        errors += checkPixel(10, 150, 0);

        stage("ModeX clipping", errors);
        screen.setPaletteColor(42, 1, 32, 255);
        outportb(0x3C7, 42);
        unsigned char red = inportb(0x3C9);
        unsigned char green = inportb(0x3C9);
        unsigned char blue = inportb(0x3C9);
        if (red != 1 || green != 32 || blue != 63) ++errors;

        screen.clear(17);
        for (y = 0; y < VgaScreen::LowHeight; ++y)
            for (x = 0; x < VgaScreen::LowWidth; ++x)
                if (readPixel(x, y) != 17) ++errors;

        screen.drawLine(-2, -2, 3, 3, 30);
        errors += checkPixel(0, 0, 30);
        errors += checkPixel(3, 3, 30);
        errors += checkPixel(4, 4, 17);
        screen.drawGrid(20, 20, 2, 2, 8, 31);
        errors += checkPixel(28, 24, 31);
        errors += checkPixel(24, 28, 31);
        errors += checkPixel(36, 36, 31);
        errors += checkPixel(24, 24, 17);
        screen.colorSquare(40, 40, 3, 32);
        errors += checkPixel(42, 42, 32);
        errors += checkPixel(43, 42, 17);
        screen.drawNumber(50, 50, 12, 33);
        errors += checkPixel(51, 50, 33);
        errors += checkPixel(50, 50, 17);
        errors += checkPixel(54, 50, 33);
        screen.drawChar(60, 60, 'A', 34, 2);
        int ink = 0;
        for (y = 60; y < 76; y += 2)
            for (x = 60; x < 76; x += 2) {
                unsigned char color = readPixel(x, y);
                if (color == 34) ++ink;
                errors += checkPixel(x + 1, y, color);
                errors += checkPixel(x, y + 1, color);
                errors += checkPixel(x + 1, y + 1, color);
            }
        if (!ink) ++errors;
        stage("ModeX drawing", errors);
        // Native high-resolution drawing and background planes.
        screen.initialize();
        static const unsigned short monoRows[4] = {0xA59, 0, 0xFFF, 0x801};
        // Unaligned 12-bit glyphs, transparent blank rows, clipping, and both scales.
        errors += checkMono(screen, 7, 20, monoRows, 12, 4, 1, 1);
        errors += checkMono(screen, -5, 30, monoRows, 12, 4, 1, 1);
        errors += checkMono(screen, 631, 477, monoRows, 12, 4, 2, 1);
        errors += checkMono(screen, 101, 80, monoRows, 12, 4, 2, 2);
        errors += checkMono(screen, -2, -1, monoRows, 12, 4, 4, 2);
        stage("Mono rows", errors);
        screen.clear(3);
        screen.fillRect(7, 1, 4, 1, 5);
        screen.putPixel(8, 1, 6);
        screen.fillRect(-2, -2, 3, 3, 9);
        screen.putPixel(639, 479, 12);
        volatile unsigned char far *hi = (volatile unsigned char far *)MK_FP(0xA000, 0);
        for (int plane = 0; plane < 4; ++plane) {
            outport(0x3CE, (plane << 8) | 4);
            unsigned char expected = (3 & (1 << plane)) ? 255 : 0;
            unsigned char mask = 1;
            expected = (expected & ~mask) | ((5 & (1 << plane)) ? mask : 0);
            unsigned char actual = hi[80];
            if (actual != expected) {
                fprintf(checks, "Rect plane%d offset80 expected%u actual%u\n", plane,
                    (unsigned)expected, (unsigned)actual); ++errors;
            }
            expected = (3 & (1 << plane)) ? 255 : 0;
            expected = (expected & ~0xE0) | ((5 & (1 << plane)) ? 0x60 : 0)
                       | ((6 & (1 << plane)) ? 0x80 : 0);
            actual = hi[81];
            if (actual != expected) {
                fprintf(checks, "Rect plane%d offset81 expected%u actual%u\n", plane,
                    (unsigned)expected, (unsigned)actual); ++errors;
            }
            expected = (3 & (1 << plane)) ? 255 : 0;
            expected = (expected & ~0x80) | ((9 & (1 << plane)) ? 0x80 : 0);
            actual = hi[0];
            if (actual != expected) {
                fprintf(checks, "Rect plane%d offset0 expected%u actual%u\n", plane,
                    (unsigned)expected, (unsigned)actual); ++errors;
            }
            expected = (3 & (1 << plane)) ? 255 : 0;
            expected = (expected & ~1) | ((12 & (1 << plane)) ? 1 : 0);
            actual = hi[38399U];
            if (actual != expected) {
                fprintf(checks, "Rect plane%d offset38399U expected%u actual%u\n", plane,
                    (unsigned)expected, (unsigned)actual); ++errors;
            }
        }
        stage("Native rectangle", errors);
        if (!screen.loadHighBitmap("BOARD.VGA")) ++errors;
        FILE *background = fopen("BOARD.VGA", "rb");
        if (!background) return 2;
        volatile unsigned char far *video = (volatile unsigned char far *)MK_FP(0xA000, 0);
        for (plane = 0; plane < 4; ++plane) {
            outport(0x3CE, (plane << 8) | 4);
            for (unsigned int offset = 0; offset < 38400U; ++offset) {
                int expected = fgetc(background);
                if (expected == EOF) return 2;
                if (video[offset] != expected) ++errors;
            }
        }
        if (fclose(background)) return 2;
        stage("Loaded bitmap", errors);
        if (screen.loadHighBitmap("MISSING.VGA")) ++errors;
        // Failed loads preserve the already rendered frame.
        outport(0x3CE, 0x0004);
        unsigned char saved = video[38399U];
        if (screen.loadHighBitmap("MISSING.VGA") || video[38399U] != saved) ++errors;
        background = fopen("SHORT.VGA", "wb");
        if (!background || fputc(0, background) == EOF || fclose(background)) return 2;
        if (screen.loadHighBitmap("SHORT.VGA")) ++errors;
    }
    union REGS registers;
    registers.h.ah = 0x0F;
    int86(0x10, &registers, &registers);
    if (registers.h.al != 3) ++errors;
    stage("Final", errors);
    if (fclose(checks)) return 3;
    FILE *result = fopen("RESULT.TXT", "w");
    if (!result) return 2;
    if (errors) fprintf(result, "VGA tests failed: %d errors\n", errors);
    else fprintf(result, "VGA tests passed\n");
    if (fclose(result)) return 3;
    return errors ? 1 : 0;
}

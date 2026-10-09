#include <dos.h>
#include <stdio.h>
#include <string.h>
#include "START.H"
#include "GAME.H"

unsigned _stklen = 16384;
static int errors = 0;
static FILE *checks;

static void check(int condition, const char *message)
{
    if (!condition) { ++errors; fprintf(checks, "FAIL: %s\n", message); }
}

static int videoMode()
{
    union REGS r;
    r.h.ah = 0x0F;
    int86(0x10, &r, &r);
    return r.h.al & 0x7F;
}

static void readPalette(unsigned char *rgb)
{
    outportb(0x3C7, 0);
    for (int i = 0; i < 768; ++i) rgb[i] = inportb(0x3C9);
}

int main()
{
    checks = fopen("CHECKS.TXT", "w");
    if (!checks) return 2;
    VgaScreen screen;
    StartScreen start(screen);
    unsigned char baseline[768], restored[768], palette[12];
    screen.initialize();
    readPalette(baseline);
    check(start.show(), "Load start screen");
    check(videoMode() == 0x12, "640x480 BIOS mode 12h");
    outportb(0x3D4, 0x12);
    check(inportb(0x3D5) == 0xDF, "480 visible scanlines");
    for (int color = 0; color < 16; ++color) {
        inportb(0x3DA);
        outportb(0x3C0, color | 0x20);
        check(inportb(0x3C1) == color, "Direct attribute-to-DAC mapping");
    }
    inportb(0x3DA); outportb(0x3C0, 0x20);
    FILE *file = fopen("START.PAL", "rb");
    if (!file || fread(palette, 1, 12, file) != 12) return 2;
    if (fclose(file)) return 2;
    readPalette(restored);
    check(!memcmp(palette, restored, 12), "Four custom DAC colors");
    file = fopen("PALETTE.RAW", "wb");
    if (!file || fwrite(restored, 1, 768, file) != 768 || fclose(file)) return 2;

    // Compare every displayed pixel with the original CX16 packed image.
    FILE *source = fopen("SOURCE.BIN", "rb");
    FILE *pixels = fopen("SCREEN.RAW", "wb");
    if (!source || !pixels) return 2;
    volatile unsigned char far *memory = (volatile unsigned char far *)MK_FP(0xA000, 0);
    unsigned char packed[160], scanline[640], planes[4];
    long mismatches = 0;
    for (int y = 0; y < 480; ++y) {
        if (fread(packed, 1, 160, source) != 160) return 2;
        for (int byte = 0; byte < 80; ++byte) {
            for (int plane = 0; plane < 4; ++plane) {
                outport(0x3CE, (plane << 8) | 4);
                planes[plane] = memory[(unsigned int)y * 80U + byte];
            }
            for (int bit = 0; bit < 8; ++bit) {
                int x = byte * 8 + bit;
                unsigned char value = 0;
                for (plane = 0; plane < 4; ++plane)
                    if (planes[plane] & (0x80 >> bit)) value |= 1 << plane;
                scanline[x] = value;
                int expected = (packed[x / 4] >> (6 - (x & 3) * 2)) & 3;
                if (value != expected) ++mismatches;
            }
        }
        if (fwrite(scanline, 1, 640, pixels) != 640) return 2;
    }
    if (fclose(source) || fclose(pixels)) return 2;
    check(!mismatches, "All 307200 pixels match the CX16 splash");
    check(!start.handleKey('1') && !start.handleKey(Game::Down), "Other keys wait");
    check(start.handleKey(13) == 1 && start.handleKey(27) == -1, "Enter and Escape");

    check(videoMode() == 0x12, "Start retains high-resolution mode");
    Game game(screen);
    check(game.loadPuzzle("PUZZLES\\001.PUZ"), "Load game after splash");
    game.draw();
    readPalette(restored);
    check(restored[4 * 3] == 55 && restored[4 * 3 + 1] == 53,
          "Game installs its own palette after splash");
    check(videoMode() == 0x12, "Gameplay retains BIOS mode 12h");
    unsigned char answerPixel = 0;
    for (int plane = 0; plane < 4; ++plane) {
        outport(0x3CE, (plane << 8) | 4);
        if (memory[132U * 80U + 264U / 8U] & 0x80) answerPixel |= 1 << plane;
    }
    check(answerPixel == 4, "High-resolution board draws correctly");
    check(start.show(), "Repeat high-resolution transition");
    screen.restoreTextMode();
    check(videoMode() == 3, "Exit from splash restores text mode");
    check(!start.show("MISSING.VGA") && videoMode() == 3, "Missing bitmap restores text");
    check(!start.show("START.VGA", "MISSING.PAL") && videoMode() == 3,
          "Missing palette restores text");
    file = fopen("SHORT.VGA", "wb");
    if (!file || fputc(0, file) == EOF || fclose(file)) return 2;
    check(!start.show("SHORT.VGA") && videoMode() == 3, "Truncated bitmap rejected");
    palette[0] = 64;
    file = fopen("BAD.PAL", "wb");
    if (!file || fwrite(palette, 1, 12, file) != 12 || fclose(file)) return 2;
    check(!start.show("START.VGA", "BAD.PAL") && videoMode() == 3,
          "Invalid DAC component rejected");
    if (fclose(checks)) return 2;
    file = fopen("RESULT.TXT", "w");
    if (!file) return 2;
    fprintf(file, errors ? "Start screen tests failed\n" : "Start screen tests passed\n");
    if (fclose(file)) return 2;
    return errors ? 1 : 0;
}

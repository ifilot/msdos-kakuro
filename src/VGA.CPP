#include <dos.h>
#include <stdio.h>
#include <mem.h>
#include <string.h>
#include "VGA.H"
#include "ASSETS.H"

VgaScreen::VgaScreen()
    : memory((volatile unsigned char far *)MK_FP(0xA000, 0)), font(0), fontHigh(0), active(0), drawingScale(1)
{
}

VgaScreen::~VgaScreen()
{
    restoreTextMode();
}

void VgaScreen::initializeModeX()
{
    union REGS registers;
    registers.x.ax = 0x0013;
    int86(0x10, &registers, &registers);

    loadFont();

    // Start from BIOS mode 13h, retaining its standard 256-entry palette.
    // Disable chain-four, select the 25 MHz clock, then set 480 scanlines
    // with double scanning for a 320x240 logical display at about 60 Hz.
    outport(0x3C4, 0x0604);
    outport(0x3C4, 0x0100);
    outportb(0x3C2, 0xE3);
    outport(0x3C4, 0x0300);
    outportb(0x3D4, 0x11);
    outportb(0x3D5, inportb(0x3D5) & 0x7F);
    static const unsigned int timing[] = {
        0x0D06, 0x3E07, 0x4109, 0xEA10, 0xAC11,
        0xDF12, 0x0014, 0xE715, 0x0616, 0xE317
    };
    for (unsigned int i = 0; i < sizeof(timing) / sizeof(timing[0]); ++i)
        outport(0x3D4, timing[i]);
    active = 1;
    drawingScale = 1;
    clear(0);
}

void VgaScreen::loadFont()
{
    // BIOS font query: BH=3 selects the ROM 8x8 glyphs, returned in ES:BP.
    struct REGPACK fontRegisters = {0};
    fontRegisters.r_ax = 0x1130;
    fontRegisters.r_bx = 0x0300;
    fontRegisters.r_flags = 0x0200;
    intr(0x10, &fontRegisters);
    font = (const unsigned char far *)MK_FP(fontRegisters.r_es, fontRegisters.r_bp);
    fontRegisters.r_ax = 0x1130;
    fontRegisters.r_bx = 0x0400;
    intr(0x10, &fontRegisters);
    fontHigh = (const unsigned char far *)MK_FP(fontRegisters.r_es, fontRegisters.r_bp);

}

void VgaScreen::initialize()
{
    initializeHighResolution();
    outportb(0x3C4, 1);
    outportb(0x3C5, inportb(0x3C5) & ~0x20);
}

void VgaScreen::initializeHighResolution()
{
    union REGS registers;
    registers.x.ax = 0x0012;
    int86(0x10, &registers, &registers);
    active = 2;
    drawingScale = 1;
    loadFont();
    // Hide loading; ensure color indices go directly to DAC entries 0..15.
    outportb(0x3C4, 1);
    outportb(0x3C5, inportb(0x3C5) | 0x20);
    for (int color = 0; color < 16; ++color) {
        inportb(0x3DA); // Reset the attribute controller address/data flip-flop.
        outportb(0x3C0, color);
        outportb(0x3C0, color);
    }
    inportb(0x3DA);
    outportb(0x3C0, 0x14);
    outportb(0x3C0, 0); // No high DAC index bits from Color Select.
    inportb(0x3DA);
    outportb(0x3C0, 0x20); // Enable attribute-controller display output.
    outportb(0x3C6, 0xFF);
}

int VgaScreen::loadPackedBitmap(const char *filename)
{
    AssetStream stream;
    if (!stream.open(filename)) return -1;
    outport(0x3CE, 0x0001);
    outport(0x3CE, 0x0003);
    outport(0x3CE, 0x0005);
    outport(0x3CE, 0xFF08);
    outport(0x3C4, 0x0C02);
    _fmemset((void far *)memory, 0, 38400U);
    unsigned char buffer[2048];
    int valid = 1;
    for (int plane = 0; plane < stream.planes() && valid; ++plane) {
        outport(0x3C4, (0x0100 << plane) | 2);
        unsigned int offset = 0;
        while (offset < 38400U) {
            unsigned int count = 38400U - offset;
            if (count > sizeof(buffer)) count = sizeof(buffer);
            if (!stream.read(buffer, count)) { valid = 0; break; }
            _fmemcpy((void far *)(memory + offset), buffer, count);
            offset += count;
        }
    }
    if (valid) {
        const unsigned char *rgb = stream.palette();
        for (int i = 0; i < 4; ++i)
            setPaletteColor(i, rgb[i * 3], rgb[i * 3 + 1], rgb[i * 3 + 2]);
    }
    stream.close();
    outport(0x3C4, 0x0F02);
    return valid;
}

int VgaScreen::loadPlanarBitmap(const char *bitmapFile, const char *paletteFile)
{
    if (active != 2) return 0;
    // An explicit alternate palette must retain loose-file validation semantics.
    int packed = paletteFile && !stricmp(paletteFile, "START.PAL")
        ? loadPackedBitmap(bitmapFile) : -1;
    if (packed >= 0) {
        if (packed) {
            outportb(0x3C4, 1);
            outportb(0x3C5, inportb(0x3C5) & ~0x20);
        }
        return packed;
    }
    FILE *bitmap = fopen(bitmapFile, "rb");
    if (!bitmap) return 0;
    FILE *palette = fopen(paletteFile, "rb");
    if (!palette) { fclose(bitmap); return 0; }
    unsigned char rgb[12] = {0};
    int valid = fread(rgb, 1, sizeof(rgb), palette) == sizeof(rgb);
    if (fgetc(palette) != EOF || ferror(palette)) valid = 0;
    if (fclose(palette)) valid = 0;
    for (int i = 0; i < 12; ++i) if (rgb[i] > 63) valid = 0;
    if (fseek(bitmap, 0L, SEEK_END) || ftell(bitmap) != 76800L
        || fseek(bitmap, 0L, SEEK_SET)) valid = 0;
    if (!valid) { fclose(bitmap); return 0; }

    // Write mode 0, CPU data unchanged, all eight bits enabled.
    outport(0x3CE, 0x0001);
    outport(0x3CE, 0x0003);
    outport(0x3CE, 0x0005);
    outport(0x3CE, 0xFF08);
    // Explicitly clear the unused high planes so only four colors are used.
    outport(0x3C4, 0x0C02);
    _fmemset((void far *)memory, 0, 38400U);
    unsigned char buffer[2048];
    for (int plane = 0; plane < 2 && valid; ++plane) {
        outport(0x3C4, (0x0100 << plane) | 2);
        unsigned int offset = 0;
        while (offset < 38400U) {
            unsigned int count = 38400U - offset;
            if (count > sizeof(buffer)) count = sizeof(buffer);
            if (fread(buffer, 1, count, bitmap) != count) { valid = 0; break; }
            _fmemcpy((void far *)(memory + offset), buffer, count);
            offset += count;
        }
    }
    if (fclose(bitmap)) valid = 0;
    if (!valid) return 0;
    for (i = 0; i < 4; ++i)
        setPaletteColor(i, rgb[i * 3], rgb[i * 3 + 1], rgb[i * 3 + 2]);
    outport(0x3C4, 0x0F02);
    outportb(0x3C4, 1);
    outportb(0x3C5, inportb(0x3C5) & ~0x20);
    return 1;
}

int VgaScreen::loadModeXBitmap(const char *filename)
{
    if (active != 1) return 0;
    FILE *file = fopen(filename, "rb");
    if (!file) return 0;
    if (fseek(file, 0L, SEEK_END) || ftell(file) != 76800L
        || fseek(file, 0L, SEEK_SET)) {
        fclose(file);
        return 0;
    }
    unsigned char scanline[80];
    int valid = 1;
    for (int plane = 0; plane < 4 && valid; ++plane) {
        outport(0x3C4, (0x0100 << plane) | 2);
        for (int row = 0; row < LowHeight; ++row) {
            if (fread(scanline, 1, 80, file) != 80) { valid = 0; break; }
            unsigned int base = (unsigned int)row * 80U;
            for (int col = 0; col < 80; ++col) memory[base + col] = scanline[col];
        }
    }
    if (fclose(file)) valid = 0;
    outport(0x3C4, 0x0F02);
    return valid;
}

int VgaScreen::loadHighBitmap(const char *filename)
{
    if (active != 2) return 0;
    int packed = loadPackedBitmap(filename);
    if (packed >= 0) return packed;
    FILE *file = fopen(filename, "rb");
    if (!file) return 0;
    if (fseek(file, 0L, SEEK_END) || ftell(file) != 153600L
        || fseek(file, 0L, SEEK_SET)) { fclose(file); return 0; }
    outport(0x3CE, 0x0001);
    outport(0x3CE, 0x0003);
    outport(0x3CE, 0x0005);
    outport(0x3CE, 0xFF08);
    unsigned char buffer[2048];
    int valid = 1;
    for (int plane = 0; plane < 4 && valid; ++plane) {
        outport(0x3C4, (0x0100 << plane) | 2);
        unsigned int offset = 0;
        while (offset < 38400U) {
            unsigned int count = 38400U - offset;
            if (count > sizeof(buffer)) count = sizeof(buffer);
            if (fread(buffer, 1, count, file) != count) { valid = 0; break; }
            _fmemcpy((void far *)(memory + offset), buffer, count);
            offset += count;
        }
    }
    if (fclose(file)) valid = 0;
    outport(0x3C4, 0x0F02);
    return valid;
}

void VgaScreen::restoreTextMode()
{
    if (active) {
        union REGS registers;
        registers.x.ax = 0x0003;
        int86(0x10, &registers, &registers);
        active = 0;
    }
}

void VgaScreen::clear(unsigned char color)
{
    fillRect(0, 0, drawingWidth(), drawingHeight(), color);
}

void VgaScreen::putPixel(int x, int y, unsigned char color)
{
    fillRect(x, y, 1, 1, color);
}

void VgaScreen::fillRect(int x, int y, int width, int height, unsigned char color)
{
    if (!active || width <= 0 || height <= 0) return;
    long right = (long)x + width, bottom = (long)y + height;
    int limitX = drawingWidth(), limitY = drawingHeight();
    if (right <= 0 || bottom <= 0 || x >= limitX || y >= limitY) return;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (right > limitX) right = limitX;
    if (bottom > limitY) bottom = limitY;
    if (active == 1) {
        for (int plane = 0; plane < 4; ++plane) {
            int first = x + ((plane - x) & 3);
            if (first >= right) continue;
            outport(0x3C4, (0x0100 << plane) | 2);
            int count = ((int)right - 1 - first) / 4 + 1;
            for (int row = y; row < bottom; ++row) {
                unsigned int base = (unsigned int)row * 80U + first / 4;
                for (int col = 0; col < count; ++col) memory[base + col] = color;
            }
        }
        return;
    }
    x *= drawingScale; y *= drawingScale;
    right *= drawingScale; bottom *= drawingScale;
    int firstByte = x >> 3, lastByte = ((int)right - 1) >> 3;
    unsigned char leftMask = 255 >> (x & 7);
    unsigned char rightMask = 255 << (7 - (((int)right - 1) & 7));
    // VGA set/reset supplies the color simultaneously to all four planes.
    // Partial bytes need one volatile read to load all four VGA latches.
    outport(0x3C4, 0x0F02);
    outport(0x3CE, ((color & 15) << 8) | 0);
    outport(0x3CE, 0x0F01);
    outport(0x3CE, 0x0003);
    outport(0x3CE, 0x0005);
    unsigned int base;
    if (firstByte == lastByte) {
        outport(0x3CE, ((leftMask & rightMask) << 8) | 8);
        base = (unsigned int)y * 80U + firstByte;
        for (int row = y; row < bottom; ++row, base += 80U) {
            unsigned char latch = memory[base];
            memory[base] = latch;
        }
        return;
    }
    int firstFull = firstByte, lastFull = lastByte;
    if (leftMask != 255) {
        outport(0x3CE, (leftMask << 8) | 8);
        base = (unsigned int)y * 80U + firstByte;
        for (int row = y; row < bottom; ++row, base += 80U) {
            unsigned char latch = memory[base];
            memory[base] = latch;
        }
        ++firstFull;
    }
    if (rightMask != 255) {
        outport(0x3CE, (rightMask << 8) | 8);
        base = (unsigned int)y * 80U + lastByte;
        for (int row = y; row < bottom; ++row, base += 80U) {
            unsigned char latch = memory[base];
            memory[base] = latch;
        }
        --lastFull;
    }
    if (firstFull <= lastFull) {
        outport(0x3CE, 0xFF08);
        unsigned int count = lastFull - firstFull + 1;
        base = (unsigned int)y * 80U + firstFull;
        // REP STOSB in the Borland runtime, rather than a C byte loop.
        if (count == 80)
            _fmemset((void far *)(memory + base), 0, count * ((int)bottom - y));
        else
            for (int row = y; row < bottom; ++row, base += 80U)
                _fmemset((void far *)(memory + base), 0, count);
    }
}

void VgaScreen::setPaletteColor(unsigned char index, unsigned char red,
                                unsigned char green, unsigned char blue)
{
    if (!active || (active == 2 && index >= 16))
        return;
    outportb(0x3C8, index);
    outportb(0x3C9, red > 63 ? 63 : red);
    outportb(0x3C9, green > 63 ? 63 : green);
    outportb(0x3C9, blue > 63 ? 63 : blue);
}

void VgaScreen::drawLine(int x1, int y1, int x2, int y2, unsigned char color)
{
    // Grid and border lines are overwhelmingly horizontal or vertical.
    // Preserve the overflow-safe general path for extreme coordinates.
    long span;
    if (y1 == y2) {
        int left = x1 < x2 ? x1 : x2;
        span = (long)(x1 < x2 ? x2 : x1) - left + 1;
        if (span <= 32767L) { fillRect(left, y1, (int)span, 1, color); return; }
    }
    if (x1 == x2) {
        int top = y1 < y2 ? y1 : y2;
        span = (long)(y1 < y2 ? y2 : y1) - top + 1;
        if (span <= 32767L) { fillRect(x1, top, 1, (int)span, color); return; }
    }
    long dx = (long)x2 - x1;
    long dy = (long)y2 - y1;
    int stepX = dx < 0 ? -1 : 1;
    int stepY = dy < 0 ? -1 : 1;
    if (dx < 0) dx = -dx;
    if (dy > 0) dy = -dy;
    long error = dx + dy;
    for (;;) {
        putPixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        long twiceError = 2 * error;
        if (twiceError >= dy) { error += dy; x1 += stepX; }
        if (twiceError <= dx) { error += dx; y1 += stepY; }
    }
}

void VgaScreen::drawGrid(int x, int y, int columns, int rows, int size,
                         unsigned char color)
{
    if (size <= 0 || columns <= 0 || rows <= 0
        || columns > drawingWidth() / size || rows > drawingHeight() / size
        || x < -drawingWidth() || x > drawingWidth() || y < -drawingHeight() || y > drawingHeight()) return;
    int width = columns * size, height = rows * size;
    for (int column = 0; column <= columns; ++column)
        drawLine(x + column * size, y, x + column * size, y + height, color);
    for (int row = 0; row <= rows; ++row)
        drawLine(x, y + row * size, x + width, y + row * size, color);
}

void VgaScreen::colorSquare(int x, int y, int size, unsigned char color)
{
    fillRect(x, y, size, size, color);
}

void VgaScreen::drawChar(int x, int y, unsigned char character,
                         unsigned char color, int scale)
{
    if (!active || !font || scale < 1 || scale > 4
        || x < -32 || x >= drawingWidth() || y < -32 || y >= drawingHeight()) return;
    const unsigned char far *glyph = character < 128
        ? font + (unsigned int)character * 8
        : fontHigh + (unsigned int)(character - 128) * 8;
    unsigned short rows[8];
    for (int row = 0; row < 8; ++row) rows[row] = glyph[row];
    drawMonoRows(x, y, rows, 8, 8, color, scale);
}

void VgaScreen::drawText(int x, int y, const char *text, unsigned char color,
                         int scale)
{
    if (!text || scale < 1 || scale > 4) return;
    int start = x;
    while (*text && y < drawingHeight()) {
        if (*text == '\n') { x = start; y += 8 * scale; }
        else {
            drawChar(x, y, (unsigned char)*text, color, scale);
            // Prevent integer overflow for very long strings.
            if (x < drawingWidth()) x += 8 * scale;
        }
        ++text;
    }
}

void VgaScreen::drawNumber(int x, int y, unsigned int number, unsigned char color)
{
    static const unsigned char digits[10][5] = {
        {7,5,5,5,7}, {2,6,2,2,7}, {7,1,7,4,7}, {7,1,7,1,7}, {5,5,7,1,1},
        {7,4,7,1,7}, {7,4,7,5,7}, {7,1,1,1,1}, {7,5,7,5,7}, {7,5,7,1,7}
    };
    if (x < -20 || x >= drawingWidth() || y < -5 || y >= drawingHeight()) return;
    unsigned char reversed[5];
    int count = 0;
    do { reversed[count++] = number % 10; number /= 10; } while (number);
    while (count) {
        const unsigned char *glyph = digits[reversed[--count]];
        drawBitmap(x, y, glyph, 3, 5, color);
        x += 4;
    }
}

void VgaScreen::drawBitmap(int x, int y, const unsigned char *rows, int width,
                           int height, unsigned char color)
{
    if (!rows || width < 1 || width > 8 || height < 1 || height > 32
        || x < -8 || x >= drawingWidth() || y < -32 || y >= drawingHeight()) return;
    unsigned short expanded[32];
    for (int row = 0; row < height; ++row) expanded[row] = rows[row];
    drawMonoRows(x, y, expanded, width, height, color);
}

void VgaScreen::drawMonoRows(int x, int y, const unsigned short *rows,
                            int width, int height, unsigned char color, int scale)
{
    if (!active || !rows || width < 1 || width > 16 || height < 1 || height > 32
        || scale < 1 || scale > 4) return;
    // Clip before multiplying coordinates, including callers outside the screen.
    if ((long)x + width * scale <= 0 || (long)y + height * scale <= 0
        || x >= drawingWidth() || y >= drawingHeight()) return;
    if (active == 1) {
        for (int row = 0; row < height; ++row)
            for (int col = 0; col < width; ++col)
                if (rows[row] & (1U << (width - 1 - col)))
                    fillRect(x + col * scale, y + row * scale, scale, scale, color);
        return;
    }
    x *= drawingScale; y *= drawingScale;
    scale *= drawingScale;
    outport(0x3C4, 0x0F02);
    outport(0x3CE, ((color & 15) << 8) | 0);
    outport(0x3CE, 0x0F01);
    outport(0x3CE, 0x0003);
    outport(0x3CE, 0x0005);
    for (int row = 0; row < height; ++row) {
        int top = y + row * scale;
        if (top >= Height) break;
        if (top + scale <= 0 || !rows[row]) continue;
        // Seventeen bytes cover 16 pixels at the maximum combined scale of 8.
        unsigned char masks[17] = {0};
        int left = x < 0 ? 0 : x;
        int right = x + width * scale;
        if (right > Width) right = Width;
        int firstByte = left >> 3, lastByte = (right - 1) >> 3;
        if (scale == 1 && x >= 0 && x + width <= Width) {
            // Native font rows need only shifts into two/three VGA bytes.
            unsigned short bits = rows[row] << (16 - width);
            int shift = x & 7;
            masks[0] = bits >> (8 + shift);
            masks[1] = bits >> shift;
            if (shift) masks[2] = bits << (8 - shift);
        } else {
            for (int col = 0; col < width; ++col)
                if (rows[row] & (1U << (width - 1 - col)))
                    for (int repeat = 0; repeat < scale; ++repeat) {
                        int pixel = x + col * scale + repeat;
                        if (pixel >= 0 && pixel < Width)
                            masks[(pixel >> 3) - firstByte] |= 0x80 >> (pixel & 7);
                    }
        }
        if (top < 0) top = 0;
        int bottom = y + (row + 1) * scale;
        if (bottom > Height) bottom = Height;
        for (int byte = firstByte; byte <= lastByte; ++byte) {
            unsigned char mask = masks[byte - firstByte];
            if (!mask) continue;
            outport(0x3CE, (mask << 8) | 8);
            unsigned int offset = (unsigned int)top * 80U + byte;
            for (int scan = top; scan < bottom; ++scan, offset += 80U) {
                unsigned char latch = memory[offset];
                memory[offset] = latch;
            }
        }
    }
}

void VgaScreen::drawCenteredBitmap(int x, int y, int boxWidth, int boxHeight,
                                   const unsigned char *rows, int width,
                                   int height, unsigned char color)
{
    if (!rows || width < 1 || width > 8 || height < 1 || height > 32
        || boxWidth < 1 || boxWidth > drawingWidth() || boxHeight < 1 || boxHeight > drawingHeight()
        || x < -drawingWidth() || x >= drawingWidth() || y < -drawingHeight() || y >= drawingHeight()) return;
    int left = width, right = -1, top = height, bottom = -1;
    for (int row = 0; row < height; ++row)
        for (int col = 0; col < width; ++col)
            if (rows[row] & (1 << (width - 1 - col))) {
                if (col < left) left = col;
                if (col > right) right = col;
                if (row < top) top = row;
                if (row > bottom) bottom = row;
            }
    if (right < 0) return;
    if (right - left + 1 > boxWidth || bottom - top + 1 > boxHeight) return;
    drawBitmap(x + (boxWidth - (right - left + 1)) / 2 - left,
               y + (boxHeight - (bottom - top + 1)) / 2 - top,
               rows, width, height, color);
}

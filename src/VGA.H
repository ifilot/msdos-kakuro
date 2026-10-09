#ifndef KAKURO_VGA_H
#define KAKURO_VGA_H

class VgaScreen {
public:
    enum { Width = 640, Height = 480, LowWidth = 320, LowHeight = 240 };

    VgaScreen();
    ~VgaScreen();
    // Default: native 640x480, 16-color VGA.
    void initialize();
    void initializeModeX(); // Legacy font/palette preview only.
    // Optional 2x drawing coordinates for the game UI; bitmaps stay native.
    void setDrawingScale(int scale) { drawingScale = active == 2 && scale == 2 ? 2 : 1; }
    int drawingScaleFactor() const { return drawingScale; }
    // Read native mode-12h pixel indices, independent of drawing scale.
    void readPixels(int x, int y, unsigned char *colors, int count);
    int drawingWidth() const { return (active == 1 ? LowWidth : Width) / drawingScale; }
    int drawingHeight() const { return (active == 1 ? LowHeight : Height) / drawingScale; }
    int loadHighBitmap(const char *filename);
    // BIOS mode 12h: 640x480, 16 planar colors. Bitmap uses colors 0..3.
    void initializeHighResolution();
    // Two 38,400-byte planes and four RGB DAC triples; streams small rows.
    int loadPlanarBitmap(const char *bitmapFile, const char *paletteFile);
    // Four 19,200-byte Mode X planes, streamed without a large RAM buffer.
    int loadModeXBitmap(const char *filename);
    void restoreTextMode();
    void clear(unsigned char color = 0);
    void putPixel(int x, int y, unsigned char color);
    void fillRect(int x, int y, int width, int height, unsigned char color);
    void drawLine(int x1, int y1, int x2, int y2, unsigned char color);
    // Grid dimensions count cells; lines include the outside border.
    void drawGrid(int x, int y, int columns, int rows, int cellSize,
                  unsigned char color);
    void colorSquare(int x, int y, int size, unsigned char color);
    // Transparent text using the VGA BIOS 8x8 font; scale is 1..4.
    void drawChar(int x, int y, unsigned char character, unsigned char color,
                  int scale = 1);
    void drawText(int x, int y, const char *text, unsigned char color,
                  int scale = 1);
    // Compact 3x5 digits for the two halves of a clue cell.
    void drawNumber(int x, int y, unsigned int number, unsigned char color);
    // One byte per row, with width (1..8) low bits describing the glyph.
    void drawBitmap(int x, int y, const unsigned char *rows, int width,
                    int height, unsigned char color);
    // Transparent low-bit monochrome rows, leftmost bit = 1 << (width-1).
    // Batches a glyph's VGA register setup; supports width 1..16 and scale 1..4.
    void drawMonoRows(int x, int y, const unsigned short *rows, int width,
                      int height, unsigned char color, int scale = 1);
    // Center the visible ink, rather than the font's nominal bounding box.
    void drawCenteredBitmap(int x, int y, int boxWidth, int boxHeight,
                            const unsigned char *rows, int width, int height,
                            unsigned char color);
    // VGA DAC components are six-bit values (0..63).
    void setPaletteColor(unsigned char index, unsigned char red,
                         unsigned char green, unsigned char blue);

private:
    volatile unsigned char far *memory;
    const unsigned char far *font;
    const unsigned char far *fontHigh;
    int active, drawingScale;
    void loadFont();
    int loadPackedBitmap(const char *filename);
    VgaScreen(const VgaScreen &);
    VgaScreen &operator=(const VgaScreen &);
};

#endif

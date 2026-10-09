#include "BLOSSOM.H"

void Blossom::draw(VgaScreen &screen, int x, int y, int filled)
{
    // Precomputed from the original five circular petals: identical pixels,
    // three masked glyph passes instead of per-pixel distance calculations.
    static const unsigned short outline[13] = {
        0x00E0,0x0110,0x0110,0x0E0E,0x1001,0x1001,0x1001,
        0x0C06,0x0208,0x0404,0x0404,0x0444,0x03B8
    };
    static const unsigned short inside[13] = {
        0x0000,0x00E0,0x00E0,0x01F0,0x0FFE,0x0FFE,0x0FFE,
        0x03F8,0x01F0,0x03F8,0x03F8,0x03B8,0x0000
    };
    static const unsigned short center[13] = {
        0x0000,0x0000,0x0000,0x0000,0x0000,0x0040,0x00A0,
        0x0040,0x0000,0x0000,0x0000,0x0000,0x0000
    };
    screen.drawMonoRows(x, y, outline, 13, 13, filled ? 1 : 7);
    if (filled) {
        screen.drawMonoRows(x, y, inside, 13, 13, 8);
        screen.drawMonoRows(x, y, center, 13, 13, 6);
    }
}

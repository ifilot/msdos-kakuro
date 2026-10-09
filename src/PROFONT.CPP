// ProFont: Copyright (c) 2014 Carl Osterwald, Stephen C. Gilardi, Andrew Welch.
// MIT license retained in assets/fonts/PROFONT-LICENSE.txt.
#include "PROFONT.H"
#include "PROFDAT.H"

void ProFont::drawCentered(VgaScreen &screen, int x, int y, int width, int height,
                           const char *text, unsigned char color, int scale)
{
    if (!text || width < 1 || height < 1 || scale < 1 || scale > 2) return;
    int left = 4096, right = -1, top = 22, bottom = -1, advance = 0;
    for (const char *cursor = text; *cursor && advance < 2048; ++cursor, advance += 12) {
        unsigned char code = (unsigned char)*cursor;
        if (code < 32 || code > 126) code = '?';
        const unsigned char *bounds = profontBounds[code - 32];
        if (bounds[0] > bounds[1]) continue; // Blank glyph, including space.
        if (advance + bounds[0] < left) left = advance + bounds[0];
        if (advance + bounds[1] > right) right = advance + bounds[1];
        if (bounds[2] < top) top = bounds[2];
        if (bounds[3] > bottom) bottom = bounds[3];
    }
    if (right < left || (right - left + 1) * scale > width
        || (bottom - top + 1) * scale > height) return;
    int originX = x + (width - (right - left + 1) * scale) / 2 - left * scale;
    int originY = y + (height - (bottom - top + 1) * scale) / 2 - top * scale;
    advance = 0;
    for (cursor = text; *cursor && advance < 2048; ++cursor, advance += 12) {
        unsigned char code = (unsigned char)*cursor;
        if (code < 32 || code > 126) code = '?';
        screen.drawMonoRows(originX + advance * scale, originY,
                            profontRows[code - 32], 12, 22, color, scale);
    }
}

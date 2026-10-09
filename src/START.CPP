#include "START.H"

int StartScreen::show(const char *bitmap, const char *palette)
{
    screen.initializeHighResolution();
    if (screen.loadPlanarBitmap(bitmap, palette)) return 1;
    screen.restoreTextMode();
    return 0;
}

int StartScreen::handleKey(int key) const
{
    if (key == 13) return 1;
    if (key == 27) return -1;
    return 0;
}
